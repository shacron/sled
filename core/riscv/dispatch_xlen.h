// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#include <core/riscv.h>
#include <core/riscv/dispatch.h>
#include <core/riscv/trace.h>
#include <sled/error.h>

#include "xlen.h"

#define SIGN_EXT_IMM12(inst) (((int32_t)inst.raw) >> 20)

static inline int32_t sign_extend32(int32_t value, uint8_t valid_bits) {
    const uint8_t shift = (32 - valid_bits);
    return ((int32_t)((uint32_t)value << shift) >> shift);
}

static int XLEN_PREFIX(exec_u_type)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const uxlen_t offset = (sxlen_t)(int32_t)(inst.raw & 0xfffff000);
    uxlen_t result;

    if (inst.u.opcode == OP_AUIPC) {
        RV_TRACE_OPSTR("aiupc");
        result = (uxlen_t)(c->pc + offset);    // AUIPC
    } else {
        RV_TRACE_OPSTR("lui");
        result = offset;                    // LUI
    }

    RV_TRACE_PRINT(c, "%s x%u, %#" PRIXLENx, opstr, inst.u.rd, offset);
    if (inst.u.rd != 0) {
        c->r[inst.u.rd] = result;
        RV_TRACE_RD(c, inst.u.rd, c->r[inst.u.rd]);
    }
    return 0;
}

static int XLEN_PREFIX(exec_jump)(rv_core_t *c, rv_inst_t inst) {
    int32_t imm = (inst.j.imm3 << 12) | (inst.j.imm2 << 11) | (inst.j.imm1 << 1);
    // or imm4 with sign extend
    imm |= ((int32_t)(inst.raw & 0x80000000)) >> (31 - 20);

    uxlen_t dest = imm + c->pc;        // J
    if (inst.j.rd == RV_ZERO) {
        RV_TRACE_PRINT(c, "j %#0" PRIXLENx, dest);
    } else {
        c->r[inst.j.rd] = (uxlen_t)(c->pc + 4);    // JAL
        RV_TRACE_PRINT(c, "jal x%u, %#0" PRIXLENx, inst.j.rd, dest);
    }
    c->pc = dest;
    c->jump_taken = 1;
    RV_TRACE_RD(c, CORE_REG_PC, c->pc);
    return 0;
}

static int XLEN_PREFIX(exec_branch)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    bool cond = false;
    const uxlen_t u1 = (uxlen_t)c->r[inst.b.rs1];
    const uxlen_t u2 = (uxlen_t)c->r[inst.b.rs2];

    switch (inst.b.funct3) {
    case 0b000: // BEQ
        RV_TRACE_OPSTR("beq");
        cond = (u1 == u2);
        break;

    case 0b001: // BNE
        RV_TRACE_OPSTR("bne");
        cond = (u1 != u2);
        break;

    case 0b100: // BLT
        RV_TRACE_OPSTR("blt");
        cond = ((sxlen_t)u1 < (sxlen_t)u2);
        break;

    case 0b101: // BGE
        RV_TRACE_OPSTR("bge");
        cond = ((sxlen_t)u1 >= (sxlen_t)u2);
        break;

    case 0b110: // BLTU
        RV_TRACE_OPSTR("bltu");
        cond = (u1 < u2);
        break;

    case 0b111: // BGEU
        RV_TRACE_OPSTR("bgeu");
        cond = (u1 >= u2);
        break;

    default:
        return rv_undef(c, inst);
    }

    int32_t imm = (inst.b.imm3 << 11) | (inst.b.imm2 << 5) | (inst.b.imm1 << 1);
    // or imm4 with sign extend
    imm |= ((int32_t)(inst.raw & 0x80000000)) >> (31 - 12);
    RV_TRACE_PRINT(c, "%s x%u, x%u, %#" PRIXLENx, opstr, inst.b.rs1, inst.b.rs2, (uxlen_t)(c->pc+imm));
    if (cond) {
        c->pc = (uxlen_t)(c->pc + imm);
        c->jump_taken = 1;
        RV_TRACE_RD(c, CORE_REG_PC, c->pc);
    }
    return 0;
}

// JALR: I-type
static int XLEN_PREFIX(exec_jalr)(rv_core_t *c, rv_inst_t inst) {
    if (inst.i.funct3 != 0)
        return rv_undef(c, inst);

    const int32_t imm = ((int32_t)inst.raw) >> 20;
    uxlen_t dest = c->r[inst.i.rs1] + imm;
    dest &= ~((uxlen_t)1);

    if (inst.i.rd == RV_ZERO) {
        RV_TRACE_PRINT(c, "ret");
    } else {
        c->r[inst.i.rd] = (uxlen_t)(c->pc + 4);
        RV_TRACE_PRINT(c, "jalr %d(x%u)", imm, inst.i.rs1);
    }
    c->pc = dest;
    c->jump_taken = 1;
    RV_TRACE_RD(c, CORE_REG_PC, c->pc);
    return 0;
}

// I-type
static int XLEN_PREFIX(exec_load)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    int err;

    uxlen_t x;
    uint32_t w;
    uint16_t h;
    uint8_t b;

    const int32_t imm = ((int32_t)inst.raw) >> 20;
    const uxlen_t dest = c->r[inst.i.rs1] + imm;

    switch (inst.i.funct3) {
    case 0b000: // LB
        RV_TRACE_OPSTR("lb");
        err = core_mem_read(&c->core, dest, 1, 1, &b);
        x = (int8_t)b;
        break;

    case 0b001: // LH
        RV_TRACE_OPSTR("lh");
        err = core_mem_read(&c->core, dest, 2, 1, &h);
        x = (int16_t)h;
        break;

    case 0b010: // LW
        RV_TRACE_OPSTR("lw");
        err = core_mem_read(&c->core, dest, 4, 1, &w);
        x = (int32_t)w;
        break;

    case 0b100: // LBU
        RV_TRACE_OPSTR("lbu");
        err = core_mem_read(&c->core, dest, 1, 1, &b);
        x = b;
        break;

    case 0b101: // LHU
        RV_TRACE_OPSTR("lhu");
        err = core_mem_read(&c->core, dest, 2, 1, &h);
        x = h;
        break;

#if USING_RV64
    case 0b110: // LWU
        RV_TRACE_OPSTR("lwu");
        err = core_mem_read(&c->core, dest, 4, 1, &w);
        x = w;
        break;

    case 0b011: // LD
        RV_TRACE_OPSTR("ld");
        err = core_mem_read(&c->core, dest, 8, 1, &x);
        break;
#endif

    default:
        return rv_undef(c, inst);
    }

    if (err) {
        RV_TRACE_PRINT(c, "%s x%u, %d(x%u)            ; [%" PRIXLENx "] = %s", opstr, inst.i.rd, imm, inst.i.rs1, dest, st_err(err));
        return rv_synchronous_exception(c, EX_ABORT_LOAD, dest, err);
    }
    RV_TRACE_PRINT(c, "%s x%u, %d(x%u)", opstr, inst.i.rd, imm, inst.i.rs1);
    RV_TRACE_RD(c, inst.i.rd, x);
    if (inst.i.rd != RV_ZERO) c->r[inst.i.rd] = x;
    return 0;
}

static int XLEN_PREFIX(exec_store)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const int32_t imm = (((int32_t)inst.raw >> 20) & ~(0x1f)) | inst.s.imm1;
    const uxlen_t dest = c->r[inst.s.rs1] + imm;
    uxlen_t val = c->r[inst.s.rs2];
    uint32_t w;
    uint16_t h;
    uint8_t b;
    int err = 0;

    switch (inst.s.funct3) {
    case 0b000: // SB
        b = (uint8_t)val;
        err = core_mem_write(&c->core, dest, 1, 1, &b);
        val = b;
        RV_TRACE_OPSTR("sb");
        break;

    case 0b001: // SH
        h = (uint16_t)val;
        err = core_mem_write(&c->core, dest, 2, 1, &h);
        val = h;
        RV_TRACE_OPSTR("sh");
        break;

    case 0b010: // SW
        w = (uint32_t)val;
        err = core_mem_write(&c->core, dest, 4, 1, &w);
        val = w;
        RV_TRACE_OPSTR("sw");
        break;

#if USING_RV64
    case 0b011: // SD
        err = core_mem_write(&c->core, dest, 8, 1, &val);
        RV_TRACE_OPSTR("sd");
        break;
#endif

    default:
        return rv_undef(c, inst);
    }

    if (err) {
        RV_TRACE_PRINT(c, "%s x%u, %d(x%u)            ; [%" PRIXLENx "] = %s", opstr, inst.s.rs2, imm, inst.i.rs1, dest, st_err(err));
        return rv_synchronous_exception(c, EX_ABORT_STORE, dest, err);
    }
    RV_TRACE_STORE(c, dest, inst.s.rs2, val);
    RV_TRACE_PRINT(c, "%s x%u, %d(x%u)", opstr, inst.s.rs2, imm, inst.s.rs1);
    return 0;
}

static int XLEN_PREFIX(exec_alu_imm)(rv_core_t *c, rv_inst_t inst) {
    const uxlen_t u1 = c->r[inst.i.rs1];
    const int32_t imm = ((int32_t)inst.raw) >> 20;  // sign extend immediate
    const uint32_t shift = inst.i.imm & (XLEN - 1);
#if USING_RV32
    const uint32_t func7 = inst.i.imm >> 5;
#else
    const uint32_t func7 = (inst.i.imm >> 5) & (~1);
#endif
    uxlen_t result;

    switch (inst.i.funct3) {
    case 0b000: // ADDI
        result = u1 + imm;
        RV_TRACE_PRINT(c, "addi x%u, x%u, %d", inst.i.rd, inst.i.rs1, imm);
        break;

    case 0b001:
        if (func7 == 0) {   // SLLI
            result = u1 << shift;
            RV_TRACE_PRINT(c, "slli x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
            break;
        }
        return rv_undef(c, inst);

    case 0b101:
        if (func7 == 0) {   // SRLI
            result = u1 >> shift;
            RV_TRACE_PRINT(c, "srli x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
            break;
        } else if (func7 == 0b0100000) {  //SRAI
            result = (sxlen_t)u1 >> shift;
            RV_TRACE_PRINT(c, "srai x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
            break;
        }
        return rv_undef(c, inst);

    case 0b010: // SLTI
        result = ((sxlen_t)u1 < imm) ? 1 : 0;
        RV_TRACE_PRINT(c, "slti x%u, x%u, %u", inst.i.rd, inst.i.rs1, (uint32_t)imm);
        break;

    case 0b011: // SLTIU
        result = (u1 < (uint32_t)imm) ? 1 : 0;
        RV_TRACE_PRINT(c, "sltiu x%u, x%u, %u", inst.i.rd, inst.i.rs1, (uint32_t)imm);
        break;

    case 0b100: // XORI
        result = u1 ^ (uxlen_t)imm;
        RV_TRACE_PRINT(c, "xori x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (uint32_t)imm);
        break;

    case 0b110: // ORI
        result = u1 | (uxlen_t)imm;
        RV_TRACE_PRINT(c, "ori x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (uint32_t)imm);
        break;

    case 0b111: // ANDI
        result = u1 & (uxlen_t)imm;
        RV_TRACE_PRINT(c, "andi x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (uint32_t)imm);
        break;
    }
    if (inst.i.rd != RV_ZERO) {
        c->r[inst.i.rd] = result;
        RV_TRACE_RD(c, inst.i.rd, c->r[inst.i.rd]);
    }
    return 0;
}

static int XLEN_PREFIX(exec_alu)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const uxlen_t u1 = c->r[inst.r.rs1];
    const uxlen_t u2 = c->r[inst.r.rs2];
    uxlen_t result;
    ux2len_t result_ul;
    sx2len_t result_sl;

    switch (inst.r.funct7) {
    case 0:
        switch (inst.r.funct3) {
        case 0b000: // ADD
            result = u1 + u2;
            RV_TRACE_OPSTR("add");
            break;

        case 0b001: // SLL
            result = u1 << (u2 & (XLEN - 1));
            RV_TRACE_OPSTR("sll");
            break;

        case 0b010: // SLT
            result = (sxlen_t)u1 < (sxlen_t)u2 ? 1 : 0;
            RV_TRACE_OPSTR("slt");
            break;

        case 0b011: // SLTU
            result = u1 < u2 ? 1 : 0;
            RV_TRACE_OPSTR("sltu");
            break;

        case 0b100: // XOR
            result = u1 ^ u2;
            RV_TRACE_OPSTR("xor");
            break;

        case 0b101: // SRL
            result = u1 >> (u2 & (XLEN - 1));
            RV_TRACE_OPSTR("srl");
            break;

        case 0b110: // OR
            result = u1 | u2;
            RV_TRACE_OPSTR("or");
            break;

        case 0b111: // AND
            result = u1 & u2;
            RV_TRACE_OPSTR("and");
            break;
        }
        break;

    case 0b0100000:
        switch (inst.r.funct3) {
        case 0b000: // SUB
            result = u1 - u2;
            RV_TRACE_OPSTR("sub");
            break;

        case 0b101: // SRA
            result = ((sxlen_t)u1) >> (u2 & (XLEN - 1));
            RV_TRACE_OPSTR("sra");
            break;

        default:
            goto undef;
        }
        break;

    case 0b0000001: // M extensions
        if (!(c->core.arch_options & RISCV_EXT_M)) goto undef;
        switch (inst.r.funct3) {
        case 0b000: // MUL
            result = u1 * u2;
            RV_TRACE_OPSTR("mul");
            break;

        case 0b001: // MULH
            result_sl = (sxlen_t)u1 * (sxlen_t)u2;
            result = (uxlen_t)(result_sl >> XLEN);
            RV_TRACE_OPSTR("mulh");
            break;

        case 0b010: // MULHSU
            result_sl = (sxlen_t)u1 * u2;
            result = (uxlen_t)(result_sl >> XLEN);
            RV_TRACE_OPSTR("mulhsu");
            break;

        case 0b011: // MULHU
            result_ul = (ux2len_t)u1 * u2;
            result = (uxlen_t)(result_ul >> XLEN);
            RV_TRACE_OPSTR("mulhu");
            break;

        case 0b100: // DIV
            if (u2 == 0) result = ~((uxlen_t)0);
            else         result = (sxlen_t)u1 / (sxlen_t)u2;
            RV_TRACE_OPSTR("div");
            break;

        case 0b101: // DIVU
            if (u2 == 0) result = ~((uxlen_t)0);
            else         result = u1 / u2;
            RV_TRACE_OPSTR("divu");
            break;

        case 0b110: // REM
            if (u2 == 0) result = u1;
            else         result = (sxlen_t)u1 % (sxlen_t)u2;
            RV_TRACE_OPSTR("rem");
            break;

        case 0b111: // REMU
            if (u2 == 0) result = u1;
            else         result = u1 % u2;
            RV_TRACE_OPSTR("remu");
            break;
        }
        break;

    default:
        goto undef;
    }

    if (inst.r.rd != RV_ZERO) {
        c->r[inst.r.rd] = result;
        RV_TRACE_RD(c, inst.i.rd, c->r[inst.i.rd]);
    }
    RV_TRACE_PRINT(c, "%s x%u, x%u, x%u", opstr, inst.r.rd, inst.r.rs1, inst.r.rs2);
    return 0;

undef:
    return rv_undef(c, inst);
}

#if USING_RV64
// 32-bit instructions in 64-bit mode
int rv64_exec_alu_imm32(rv_core_t *c, rv_inst_t inst) {
    const int32_t s1 = (uint32_t)c->r[inst.i.rs1];
    const uint32_t shift = inst.i.imm & 63;
    int32_t result;

    switch (inst.i.funct3) {
    case 0b000: // ADDIW
    {
        const int32_t imm = SIGN_EXT_IMM12(inst);
        result = (int64_t)(int32_t)(c->r[inst.i.rs1] + imm);
        if (imm == 0) RV_TRACE_PRINT(c, "sext.w x%u, x%u", inst.i.rd, inst.i.rs1);
        else          RV_TRACE_PRINT(c, "addiw x%u, x%u, %d", inst.i.rd, inst.i.rs1, imm);
        break;
    }

    case 0b001: // SLLIW
        if (shift > 31) return rv_undef(c, inst);
        result = s1 << shift;
        RV_TRACE_PRINT(c, "slliw x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
        break;

    case 0b101: // SRLIW SRAIW
    {
        const uint32_t imm = inst.i.imm >> 5;
        if (shift > 31) return rv_undef(c, inst);
        if (imm == 0) {  // SRLIW
            result = (uint32_t)s1 >> shift;
            RV_TRACE_PRINT(c, "srliw x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
            break;
        } else if (imm == 0b0100000) {   // SRAIW
            result = s1 >> shift;
            RV_TRACE_PRINT(c, "sraiw x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
            break;
        }
        return rv_undef(c, inst);
    }

    default:
        return rv_undef(c, inst);
    }

    if (inst.i.rd != RV_ZERO) {
        c->r[inst.i.rd] = (int64_t)result;
        RV_TRACE_RD(c, inst.i.rd, c->r[inst.i.rd]);
    }
    return 0;
}

static int rv64_exec_alu32(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const uint32_t u1 = c->r[inst.r.rs1];
    const uint32_t u2 = c->r[inst.r.rs2];
    const uint32_t shift = u2 & 0x1f;
    uint64_t result;

    switch (inst.r.funct7) {
    case 0b0000000:
        switch (inst.r.funct3) {
        case 0b000: // ADDW
            result = (int32_t)(u1 + u2);
            RV_TRACE_OPSTR("addw");
            break;

        case 0b001: // SLLW
            result = (uint32_t)(u1 << shift);
            RV_TRACE_OPSTR("sllw");
            break;

        case 0b101: // SRLW
            result = (uint32_t)(u1 >> shift);
            RV_TRACE_OPSTR("srlw");
            break;

        default:
            goto undef;
        }
        break;

    case 0b0100000:
        switch (inst.r.funct3) {
        case 0b000: // SUBW
            result = (int32_t)(u1 - u2);
            RV_TRACE_OPSTR("subw");
            break;

        case 0b101: // SRAW
            result = (uint32_t)(((int32_t)u1) >> shift);
            RV_TRACE_OPSTR("sraw");
            break;

        default:
            goto undef;
        }
        break;

    case 0b0000001: // RV64M extensions
        if (!(c->core.arch_options & RISCV_EXT_M)) goto undef;
        switch (inst.r.funct3) {
        case 0b000: // MULW
            result = (int32_t)(u1 * u2);
            RV_TRACE_OPSTR("mulw");
            break;

        case 0b100: // DIVW
            if (u2 == 0) result = ~((uint64_t)0);
            else         result = (int32_t)((int32_t)u1 / (int32_t)u2);
            RV_TRACE_OPSTR("divw");
            break;

        case 0b101: // DIVUW
            if (u2 == 0) result = ~((uint64_t)0);
            else         result = (int32_t)(u1 / u2);
            RV_TRACE_OPSTR("divuw");
            break;


        case 0b110: // REMW
            if (u2 == 0) result = (int32_t)u1;
            else         result = (int32_t)((int32_t)u1 % (int32_t)u2);
            RV_TRACE_OPSTR("remw");
            break;

        case 0b111: // REMUW
            if (u2 == 0) result = (int32_t)u1;
            else         result = (int32_t)(u1 % u2);
            RV_TRACE_OPSTR("remuw");
            break;

        default:
            goto undef;
        }
        break;

    default:
        goto undef;
    }

    if (inst.r.rd != RV_ZERO) {
        c->r[inst.r.rd] = result;
        RV_TRACE_RD(c, inst.i.rd, c->r[inst.i.rd]);
    }
    RV_TRACE_PRINT(c, "%s x%u, x%u, x%u", opstr, inst.r.rd, inst.r.rs1, inst.r.rs2);
    return 0;

undef:
    return rv_undef(c, inst);
}
#endif // USING_RV64

#define RVC_TO_REG(r) ((r) + 8)

static int XLEN_PREFIX(dispatch_alu16)(rv_core_t *c, rv_cinst_t ci) {
    switch (ci.cba.funct2) {
    case 0b00:  // C.SRLI
    {
#if USING_RV32
        if (ci.cba.imm1) return SL_ERR_UNDEF;
        const uint32_t imm = ci.cba.imm0;
#else
        const uint32_t imm = CBAIMM(ci);
#endif
        if (imm == 0) return SL_ERR_UNDEF;
        const uint32_t rd = RVC_TO_REG(ci.cba.rsd);
        const uxlen_t result = (uxlen_t)(c->r[rd] >> imm);
        c->r[rd] = result;
        RV_TRACE_RD(c, rd, result);
        RV_TRACE_PRINT(c, "c.srli x%u, %u", rd, imm);
        return 0;
    }

    case 0b01:  // C.SRAI
    {
#if USING_RV32
        if (ci.cba.imm1) return SL_ERR_UNDEF;
        const uint32_t imm = ci.cba.imm0;
#else
        const uint32_t imm = CBAIMM(ci);
#endif
        if (imm == 0) return SL_ERR_UNDEF;
        const uint32_t rd = RVC_TO_REG(ci.cba.rsd);
        const uxlen_t result = ((sxlen_t)c->r[rd]) >> imm;
        c->r[rd] = result;
        RV_TRACE_RD(c, rd, result);
        RV_TRACE_PRINT(c, "c.srai x%u, %u", rd, imm);
        return 0;
    }

    case 0b10:  // C.ANDI
    {
        const sxlen_t imm = sign_extend32(CBAIMM(ci), 6);
        const uint32_t rd = RVC_TO_REG(ci.cba.rsd);
        const uxlen_t result = (uxlen_t)(c->r[rd] & (uxlen_t)imm);
        c->r[rd] = result;
        RV_TRACE_RD(c, rd, result);
        RV_TRACE_PRINT(c, "c.andi x%u, %#" PRIXLENx, rd, imm);
        return 0;
    }

    default:
        break;
    }

    // case 0b11

    const uint32_t rs2 = RVC_TO_REG(ci.cs.rs2);
    const uint32_t rsd = RVC_TO_REG(ci.cs.rs1);
    uxlen_t result;
    switch ((ci.cs.imm1 & 4) | ci.cs.imm0) {
    case 0b000: // C.SUB
        result = (uxlen_t)(c->r[rsd] - c->r[rs2]);
        RV_TRACE_PRINT(c, "c.sub x%u, x%u", rsd, rs2);
        break;

    case 0b001: // C.XOR
        result = (uxlen_t)(c->r[rsd] ^ c->r[rs2]);
        RV_TRACE_PRINT(c, "c.xor x%u, x%u", rsd, rs2);
        break;

    case 0b010: // C.OR
        result = (uxlen_t)(c->r[rsd] | c->r[rs2]);
        RV_TRACE_PRINT(c, "c.or x%u, x%u", rsd, rs2);
        break;

    case 0b011: // C.AND
        result = (uxlen_t)(c->r[rsd] & c->r[rs2]);
        RV_TRACE_PRINT(c, "c.or x%u, x%u", rsd, rs2);
        break;

#if USING_RV64
    case 0b100: // C.SUBW
        result = (int32_t)(c->r[rsd] - c->r[rs2]);
        RV_TRACE_PRINT(c, "c.subw x%u, x%u", rsd, rs2);
        break;

    case 0b101: // C.ADDW
        result = (int32_t)(c->r[rsd] + c->r[rs2]);
        RV_TRACE_PRINT(c, "c.addw x%u, x%u", rsd, rs2);
        break;

#endif
    default: return SL_ERR_UNDEF;
    }
    c->r[rsd] = result;
    RV_TRACE_RD(c, rsd, result);
    return 0;
}

static int XLEN_PREFIX(dispatch16)(rv_core_t *c, rv_inst_t inst) {
    int err = 0;
    rv_cinst_t ci;
    ci.raw = (uint16_t)inst.raw;
#if RV_TRACE
    c->core.trace->opcode = ci.raw;
    RV_TRACE_OPT(c, ITRACE_OPT_INST16);
#endif
    const uint16_t op = (ci.cj.opcode << 3) | ci.cj.funct3;
    switch (op) {
    case 0b00000:   // C.ADDI4SPN
    {
        if (ci.raw == 0) goto undef;
        const uint32_t imm = CIWIMM(ci);
        const uint32_t rd = RVC_TO_REG(ci.ciw.rd);
        c->r[rd] = (uxlen_t)(c->r[RV_SP] + imm);
        RV_TRACE_RD(c, rd, c->r[rd]);
        RV_TRACE_PRINT(c, "c.addi4spn x%u, %u", rd, imm);
        break;
   }

    case 0b00001: goto undef;   // C.FLD

    case 0b00010:   // C.LW
    {
        const uint32_t imm = ((ci.cl.imm0 & 2) << 1) | ((ci.cl.imm1 ) << 3) | ((ci.cl.imm0 & 1) << 6);
        const uint32_t rs = RVC_TO_REG(ci.cl.rs);
        const uint32_t rd = RVC_TO_REG(ci.cl.rd);
        const uint64_t dest = c->r[rs] + imm;

        uint32_t val;
        err = core_mem_read(&c->core, dest, 4, 1, &val);
        RV_TRACE_RD(c, rd, c->r[rd]);
        RV_TRACE_PRINT(c, "c.lw x%u, %u(x%u)", rd, imm, rs);
        if (err) return rv_synchronous_exception(c, EX_ABORT_LOAD, dest, err);
        c->r[rd] = (int32_t)val; // sign extend
        break;
    }

    case 0b00011:
#if USING_RV32
        goto undef; // C.FLW
#else
    {   // C.LD
        const uint32_t imm = ((ci.cl.imm0 & 2) << 1) | ((ci.cl.imm1 ) << 3) | ((ci.cl.imm0 & 1) << 6);
        const uint32_t rs = RVC_TO_REG(ci.cl.rs);
        const uint32_t rd = RVC_TO_REG(ci.cl.rd);
        const uint64_t dest = c->r[rs] + imm;

        uint64_t val;
        err = core_mem_read(&c->core, dest, 8, 1, &val);
        RV_TRACE_RD(c, rd, val);
        RV_TRACE_PRINT(c, "c.ld x%u, %u(x%u)", rd, imm, rs);
        if (err) return rv_synchronous_exception(c, EX_ABORT_LOAD, dest, err);
        c->r[rd] = val;
        break;
    }
#endif

    case 0b00100: goto undef;   // reserved
    case 0b00101: goto undef;   // C.FSD

    case 0b00110:   // C.SW
    {
        const uint32_t imm = CSIMM_SCALED_W(ci);
        const uint32_t rs = RVC_TO_REG(ci.cs.rs1);
        const uint32_t rd = RVC_TO_REG(ci.cs.rs2);
        const uxlen_t dest = c->r[rs] + imm;
        uint32_t val = (uint32_t)c->r[rd];
        err = core_mem_write(&c->core, dest, 4, 1, &val);
        RV_TRACE_STORE(c, dest, rd, val);
        RV_TRACE_PRINT(c, "c.sw x%u, %u(x%u)", rd, imm, rs);
        if (err) return rv_synchronous_exception(c, EX_ABORT_STORE, dest, err);
        break;
    }

    case 0b00111:
#if USING_RV32
        goto undef; // C.FSW
#else
    {   // C.SD
        const uint32_t imm = CSIMM_SCALED_D(ci);
        const uint32_t rs = RVC_TO_REG(ci.cs.rs1);
        const uint32_t rd = RVC_TO_REG(ci.cs.rs2);
        const uint64_t dest = c->r[rs] + imm;
        uint64_t val = c->r[rd];
        err = core_mem_write(&c->core, dest, 8, 1, &val);
        RV_TRACE_STORE(c, dest, rd, val);
        RV_TRACE_PRINT(c, "c.sd x%u, %u(x%u)" PRIx64, rd, imm, rs);
        if (err) return rv_synchronous_exception(c, EX_ABORT_STORE, dest, err);
        break;
    }
#endif

    case 0b01000:
    {
        if (inst.raw == 1) { // C.NOP
            RV_TRACE_PRINT(c, "c.nop");
            break;
        }
        // C.ADDI
        if (ci.ci.rsd == 0) goto undef;
        const int32_t imm = sign_extend32(CIIMM(ci), 6);
        const uxlen_t result = (uxlen_t)(c->r[ci.ci.rsd] + imm);
        c->r[ci.ci.rsd] = result;
        RV_TRACE_RD(c, ci.ci.rsd, result);
        RV_TRACE_PRINT(c, "c.addi x%u, %d", ci.ci.rsd, imm);
        break;
    }

    case 0b01001:
    {
#if USING_RV32
        // C.JAL
        const int32_t imm = sign_extend32(CJIMM(ci), 12);
        const uxlen_t result = c->pc + imm;
        c->r[RV_RA] = c->pc + 2;
        c->pc = result;
        c->jump_taken = 1;
        RV_TRACE_RD(c, CORE_REG_PC, result);
        RV_TRACE_PRINT(c, "c.jal %d", imm);
#else
        // C.ADDIW
        if (ci.ci.rsd == 0) goto undef;
        const int32_t imm = sign_extend32(CIIMM(ci), 6);
        const int64_t result = (int32_t)(c->r[ci.ci.rsd] + imm);
        c->r[ci.ci.rsd] = result;
        RV_TRACE_RD(c, ci.ci.rsd, result);
        // if imm=0, this is sext.w rd
        RV_TRACE_PRINT(c, "c.addiw x%u, %d", ci.ci.rsd, imm);
#endif
        break;
    }

    case 0b01010:   // C.LI
    {
        if (ci.ci.rsd == 0) goto undef;
        const int32_t val = sign_extend32(CIIMM(ci), 6);
        c->r[ci.ci.rsd] = (sxlen_t)val;
        RV_TRACE_RD(c, ci.ci.rsd, c->r[ci.ci.rsd]);
        RV_TRACE_PRINT(c, "c.li x%u, %d", ci.ci.rsd, val);
        break;
    }

    case 0b01011:
        if (ci.ci.rsd == 0) goto undef;
        if (ci.ci.rsd == RV_SP) {   // C.ADDI16SP
            const int32_t imm = sign_extend32((CIMM_ADDI16SP(ci)), 10);
            const uxlen_t sp = c->r[RV_SP] + imm;
            c->r[RV_SP] = sp;
            RV_TRACE_RD(c, RV_SP, sp);
            RV_TRACE_PRINT(c, "c.addi16sp %d", imm);
        } else {    // C.LUI
            const sxlen_t imm = sign_extend32((CIIMM(ci) << 12), 18);
            if (imm == 0) goto undef;
            c->r[ci.ci.rsd] = imm;
            RV_TRACE_RD(c, ci.ci.rsd, c->r[ci.ci.rsd]);
            RV_TRACE_PRINT(c, "c.lui x%u, %#" PRIXLENx, ci.ci.rsd, imm);
        }
        break;

    case 0b01100:
        err = XLEN_PREFIX(dispatch_alu16)(c, ci);
        if (err != 0) {
            if (err == SL_ERR_UNDEF) goto undef;
            return err;
        }
        break;

    case 0b01101:   // C.J
    {
        const int32_t imm = sign_extend32(CJIMM(ci), 12);
        uxlen_t result = c->pc + imm;
        c->pc = result;
        c->jump_taken = 1;
        RV_TRACE_RD(c, CORE_REG_PC, result);
        RV_TRACE_PRINT(c, "c.j %d", imm);
        break;
    }

    case 0b01110:   // C.BEQZ
    {
        const int32_t imm = sign_extend32(CBIMM(ci), 8);
        const uint32_t rs = RVC_TO_REG(ci.cb.rs);
        if (c->r[rs] == 0) {
            const uxlen_t result = c->pc + imm;
            c->pc = result;
            c->jump_taken = 1;
            RV_TRACE_RD(c, CORE_REG_PC, result);
        }
        RV_TRACE_PRINT(c, "c.beqz x%u, %d", rs, imm);
        break;
    }

    case 0b01111:   // C.BNEZ
    {
        const int32_t imm = sign_extend32(CBIMM(ci), 8);
        const uint32_t rs = RVC_TO_REG(ci.cb.rs);
        if (c->r[rs] != 0) {
            const uxlen_t result = (uxlen_t)(c->pc + imm);
            c->pc = result;
            c->jump_taken = 1;
            RV_TRACE_RD(c, CORE_REG_PC, result);
        }
        RV_TRACE_PRINT(c, "c.bnez x%u, %d", rs, imm);
        break;
    }

    case 0b10000:   // C.SLLI
        if (ci.ci.rsd == RV_ZERO) goto undef;
        else {
#if USING_RV32
            if (ci.ci.imm1) goto undef;
            const int32_t imm = ci.ci.imm0;
#else
            const int32_t imm = CIIMM(ci);
#endif
            if (imm == 0) goto undef;
            const uxlen_t result = (uxlen_t)(c->r[ci.ci.rsd] << imm);
            c->r[ci.ci.rsd] = result;
            RV_TRACE_RD(c, ci.ci.rsd, result);
            RV_TRACE_PRINT(c, "c.slli x%u, %u", ci.ci.rsd, imm);
            break;
        }

    case 0b10001: goto undef;   // C.FLDSP

    case 0b10010:   // C.LWSP
        if (ci.cilwsp.rd == RV_ZERO) goto undef;
        else {
            const uint32_t imm = CILWSPIMM(ci);
            const uxlen_t addr = c->r[RV_SP] + imm;

            uint32_t val;
            err = core_mem_read(&c->core, addr, 4, 1, &val);
            if (err) return rv_synchronous_exception(c, EX_ABORT_LOAD, addr, err);
            c->r[ci.ci.rsd] = val;
            RV_TRACE_RD(c, ci.ci.rsd, val);
            RV_TRACE_PRINT(c, "c.lwsp x%u, %u", ci.ci.rsd, imm);
            break;
        }

    case 0b10011:
#if USING_RV32
        goto undef; // C.FLWSP
#else
        if (ci.ci.rsd == RV_ZERO) goto undef;
        else {
            // C.LDSP
            const uint32_t imm = CLIDSPIMM(ci);
            const uint64_t addr = c->r[RV_SP] + imm;
            uint64_t val;
            err = core_mem_read(&c->core, addr, 8, 1, &val);
            if (err) return rv_synchronous_exception(c, EX_ABORT_LOAD, addr, err);
            c->r[ci.ci.rsd] = val;
            RV_TRACE_RD(c, ci.ci.rsd, val);
            RV_TRACE_PRINT(c, "c.ldsp x%u, %u", ci.ci.rsd, imm);
            break;
        }
#endif

    case 0b10100:   // C.JR C.MV C.EBREAK C.JALR C.ADD
        if (ci.cr.funct4 == 0) {
            if (ci.cr.rsd == 0) goto undef;
            if (ci.cr.rs2 == 0) {
                // C.JR
                const uxlen_t addr = c->r[ci.cr.rsd];
                c->pc = addr;
                c->jump_taken = 1;
                RV_TRACE_RD(c, CORE_REG_PC, addr);
                RV_TRACE_PRINT(c, "c.jr x%u", ci.ci.rsd);
            } else {
                // C.MV
                const uxlen_t val = c->r[ci.cr.rs2];
                c->r[ci.cr.rsd] = val;
                RV_TRACE_RD(c, ci.cr.rsd, val);
                RV_TRACE_PRINT(c, "c.mv x%u, x%u", ci.ci.rsd, ci.cr.rs2);
            }
        } else {
            if (ci.cr.rs2 == 0) {
                if (ci.cr.rsd == 0) {
                    // C.EBREAK
                    RV_TRACE_PRINT(c, "c.ebreak");
                    rv_exec_ebreak(c);
                } else {
                    // C.JALR
                    const uxlen_t addr = c->r[ci.cr.rsd];
                    c->r[ci.cr.rsd] = c->pc + 2;
                    c->pc = addr;
                    c->jump_taken = 1;
                    RV_TRACE_RD(c, CORE_REG_PC, addr);
                    RV_TRACE_PRINT(c, "c.jalr x%u", ci.ci.rsd);
                }
            } else {
                if (ci.cr.rsd == 0) goto undef;
                // C.ADD
                const uxlen_t val = c->r[ci.cr.rs2] + c->r[ci.cr.rsd];
                c->r[ci.cr.rsd] = val;
                RV_TRACE_RD(c, ci.cr.rsd, val);
                RV_TRACE_PRINT(c, "c.add x%u, x%u", ci.ci.rsd, ci.cr.rs2);
            }
        }
        break;

    case 0b10101: goto undef;   // C.FSDSP

    case 0b10110:   // C.SWSP
    {
        const uint32_t imm = CSSIMM_SCALED_W(ci);
        const uxlen_t addr = c->r[RV_SP] + imm;
        uint32_t val = c->r[ci.css.rs2];
        err = core_mem_write(&c->core, addr, 4, 1, &val);
        if (err) return rv_synchronous_exception(c, EX_ABORT_STORE, addr, err);
        RV_TRACE_STORE(c, addr, ci.css.rs2, val);
        RV_TRACE_PRINT(c, "c.swsp x%u, %u", ci.css.rs2, imm);
        break;
    }

    case 0b10111:
#if USING_RV32
        goto undef; // C.FSWSP
#else
    {   // C.SDSP
        const uint32_t imm = CSSIMM_SCALED_D(ci);
        const uint64_t addr = c->r[RV_SP] + imm;
        uint64_t val = c->r[ci.css.rs2];
        err = core_mem_write(&c->core, addr, 8, 1, &val);
        if (err) return rv_synchronous_exception(c, EX_ABORT_STORE, addr, err);
        RV_TRACE_STORE(c, addr, ci.css.rs2, val);
        RV_TRACE_PRINT(c, "c.sdsp x%u, %u", ci.css.rs2, imm);
        break;
    }
#endif

    default: goto undef;
    }

    return 0;

undef:
    return rv_undef(c, inst);
}

int XLEN_PREFIX(dispatch)(rv_core_t *c, rv_inst_t inst) {
    int err = 0;

    // 16 bit compressed instructions
    if ((inst.u.opcode & 3) != 3) {
        if (!(c->core.arch_options & RISCV_EXT_C)) return rv_undef(c, inst);
        c->c_inst = 1;
        return XLEN_PREFIX(dispatch16)(c, inst);
    }
    c->c_inst = 0;

    switch (inst.u.opcode) {
    // U-type instructions
    case OP_LUI:  // LUI
    case OP_AUIPC:  // AUIPC
        err = XLEN_PREFIX(exec_u_type)(c, inst);
        break;

    // J-type instructions
    case OP_JAL:  // JAL
        err = XLEN_PREFIX(exec_jump)(c, inst);
        break;

    // B-type
    case OP_BRANCH:  // BEQ BNE BLT BGE BLTU BGEU
        err = XLEN_PREFIX(exec_branch)(c, inst);
        break;

    // I-type
    case OP_JALR:  // JALR
        err = XLEN_PREFIX(exec_jalr)(c, inst);
        break;

    // I-type
    case OP_LOAD:  // LB LH LW LBU LHU
        err = XLEN_PREFIX(exec_load)(c, inst);
        break;

    // S-type
    case OP_STORE:  // SB SH SW
        err = XLEN_PREFIX(exec_store)(c, inst);
        break;

    // I-type
    case OP_IMM:  // ADDI SLTI SLTIU XORI ORI ANDI SLLI SRLI SRAI
        err = XLEN_PREFIX(exec_alu_imm)(c, inst);
        break;

    // R-type
    case OP_ALU:  // ADD SUB SLL SLT SLTU XOR SRL SRA OR AND
        err = XLEN_PREFIX(exec_alu)(c, inst);
        break;

#if USING_RV64
    case OP_IMM32:
        err = rv64_exec_alu_imm32(c, inst);
        break;

    case OP_ALU32:
        err = rv64_exec_alu32(c, inst);
        break;
#endif

    // Other
    case OP_MISC_MEM:  // FENCE FENCE.I
        err = rv_exec_mem(c, inst);
        break;

    case OP_SYSTEM:  // ECALL EBREAK CSRRW CSRRS CSRRC CSRRWI CSRRSI CSRRCI
        err = rv_exec_system(c, inst);
        break;

    default:
        err = rv_undef(c, inst);
        break;
    }
    return err;
}
