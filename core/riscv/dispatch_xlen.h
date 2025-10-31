// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <core/riscv.h>
#include <core/riscv/dispatch.h>
#include <core/riscv/trace.h>
#include <sled/arch.h>
#include <sled/error.h>

#include "xlen.h"

#define SIGN_EXT_IMM12(inst) (((i4)inst.raw) >> 20)

int rv_exec_fp(rv_core_t *c, rv_inst_t inst);
int rv_exec_fp_load(rv_core_t *c, rv_inst_t inst);
int rv_exec_fp_store(rv_core_t *c, rv_inst_t inst);
int rv_exec_fp_mac(rv_core_t *c, rv_inst_t inst);

static inline i4 sign_extend32(i4 value, u1 valid_bits) {
    const u1 shift = (32 - valid_bits);
    return ((i4)((u4)value << shift) >> shift);
}

__attribute__((no_sanitize("signed-integer-overflow")))
static inline sxlen_t mulx_ss(sxlen_t a, sxlen_t b) { return a * b; }

static int XLEN_PREFIX(exec_u_type)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const uxlen_t offset = (sxlen_t)(i4)(inst.raw & 0xfffff000);
    uxlen_t result;

    if (inst.u.opcode == OP_AUIPC) {
        RV_TRACE_OPSTR("aiupc");
        result = (uxlen_t)(c->core.pc + offset);    // AUIPC
    } else {
        RV_TRACE_OPSTR("lui");
        result = offset;                    // LUI
    }

    RV_TRACE_PRINT(c, "%s x%u, %#" PRIXLENx, opstr, inst.u.rd, offset);
    if (inst.u.rd != 0) {
        c->core.r[inst.u.rd] = result;
        RV_TRACE_RD(c, inst.u.rd, c->core.r[inst.u.rd]);
    }
    return 0;
}

static int XLEN_PREFIX(exec_jump)(rv_core_t *c, rv_inst_t inst) {
    i4 imm = (inst.j.imm3 << 12) | (inst.j.imm2 << 11) | (inst.j.imm1 << 1);
    // or imm4 with sign extend
    imm |= ((i4)(inst.raw & 0x80000000)) >> (31 - 20);

#if RV_TRACE
    uxlen_t trace_dest = RV_BR_TARGET(imm + c->core.pc, imm);
#endif
    if (inst.j.rd == RV_ZERO) {        // J
        RV_TRACE_PRINT(c, "j %#" PRIXLENx, trace_dest);
    } else {
        c->core.r[inst.j.rd] = (uxlen_t)(c->core.pc + 4);    // JAL
        RV_TRACE_PRINT(c, "jal x%u, %#" PRIXLENx, inst.j.rd, trace_dest);
    }
    c->core.pc = imm + c->core.pc;
    c->core.branch_taken = true;
    RV_TRACE_RD(c, SL_CORE_REG_PC, c->core.pc);
    return 0;
}

static int XLEN_PREFIX(exec_branch)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    bool cond = false;
    const uxlen_t u1 = (uxlen_t)c->core.r[inst.b.rs1];
    const uxlen_t u2 = (uxlen_t)c->core.r[inst.b.rs2];

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

    i4 imm = (inst.b.imm3 << 11) | (inst.b.imm2 << 5) | (inst.b.imm1 << 1);
    // or imm4 with sign extend
    imm |= ((i4)(inst.raw & 0x80000000)) >> (31 - 12);
    RV_TRACE_PRINT(c, "%s x%u, x%u, %#" PRIXLENx, opstr, inst.b.rs1, inst.b.rs2, (uxlen_t)RV_BR_TARGET(c->core.pc + imm, imm));
    if (cond) {
        c->core.pc = (uxlen_t)(c->core.pc + imm);
        c->core.branch_taken = true;
        RV_TRACE_RD(c, SL_CORE_REG_PC, c->core.pc);
    }
    return 0;
}

// JALR: I-type
static int XLEN_PREFIX(exec_jalr)(rv_core_t *c, rv_inst_t inst) {
    if (inst.i.funct3 != 0)
        return rv_undef(c, inst);

    const i4 imm = ((i4)inst.raw) >> 20;
    uxlen_t dest = c->core.r[inst.i.rs1] + imm;
    dest &= ~((uxlen_t)1);

    if (inst.i.rd == RV_ZERO) {
        RV_TRACE_PRINT(c, "ret");
    } else {
        c->core.r[inst.i.rd] = (uxlen_t)(c->core.pc + 4);
        RV_TRACE_PRINT(c, "jalr %d(x%u)", imm, inst.i.rs1);
    }
    c->core.pc = dest;
    c->core.branch_taken = true;
    RV_TRACE_RD(c, SL_CORE_REG_PC, c->core.pc);
    return 0;
}

// I-type
static int XLEN_PREFIX(exec_load)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    int err;
    uxlen_t x;
    u4 w;
    u2 h;
    u1 b;

    const i4 imm = ((i4)inst.raw) >> 20;
    const uxlen_t dest = c->core.r[inst.i.rs1] + imm;

    switch (inst.i.funct3) {
    case 0b000: // LB
        RV_TRACE_OPSTR("lb");
        err = sl_core_mem_read_single(&c->core, dest, 1, &b);
        x = (i1)b;
        break;

    case 0b001: // LH
        RV_TRACE_OPSTR("lh");
        err = sl_core_mem_read_single(&c->core, dest, 2, &h);
        x = (i2)h;
        break;

    case 0b010: // LW
        RV_TRACE_OPSTR("lw");
        err = sl_core_mem_read_single(&c->core, dest, 4, &w);
        x = (i4)w;
        break;

    case 0b100: // LBU
        RV_TRACE_OPSTR("lbu");
        err = sl_core_mem_read_single(&c->core, dest, 1, &b);
        x = b;
        break;

    case 0b101: // LHU
        RV_TRACE_OPSTR("lhu");
        err = sl_core_mem_read_single(&c->core, dest, 2, &h);
        x = h;
        break;

#if USING_RV64
    case 0b110: // LWU
        RV_TRACE_OPSTR("lwu");
        err = sl_core_mem_read_single(&c->core, dest, 4, &w);
        x = w;
        break;

    case 0b011: // LD
        RV_TRACE_OPSTR("ld");
        err = sl_core_mem_read_single(&c->core, dest, 8, &x);
        break;
#endif

    default:
        return rv_undef(c, inst);
    }

    if (err) {
        RV_TRACE_PRINT(c, "%s x%u, %d(x%u)            ; [%" PRIXLENx "] = %s", opstr, inst.i.rd, imm, inst.i.rs1, dest, st_err(err));
        return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, dest, err);
    }
    RV_TRACE_PRINT(c, "%s x%u, %d(x%u)", opstr, inst.i.rd, imm, inst.i.rs1);
    RV_TRACE_RD(c, inst.i.rd, x);
    if (inst.i.rd != RV_ZERO) c->core.r[inst.i.rd] = x;
    return 0;
}

static int XLEN_PREFIX(exec_store)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const i4 imm = (((i4)inst.raw >> 20) & ~(0x1f)) | inst.s.imm1;
    const uxlen_t dest = c->core.r[inst.s.rs1] + imm;
    uxlen_t val = c->core.r[inst.s.rs2];
    u4 w;
    u2 h;
    u1 b;
    int err = 0;

    switch (inst.s.funct3) {
    case 0b000: // SB
        b = (u1)val;
        err = sl_core_mem_write_single(&c->core, dest, 1, &b);
        val = b;
        RV_TRACE_OPSTR("sb");
        break;

    case 0b001: // SH
        h = (u2)val;
        err = sl_core_mem_write_single(&c->core, dest, 2, &h);
        val = h;
        RV_TRACE_OPSTR("sh");
        break;

    case 0b010: // SW
        w = (u4)val;
        err = sl_core_mem_write_single(&c->core, dest, 4, &w);
        val = w;
        RV_TRACE_OPSTR("sw");
        break;

#if USING_RV64
    case 0b011: // SD
        err = sl_core_mem_write_single(&c->core, dest, 8, &val);
        RV_TRACE_OPSTR("sd");
        break;
#endif

    default:
        return rv_undef(c, inst);
    }

    if (err) {
        RV_TRACE_PRINT(c, "%s x%u, %d(x%u)            ; [%" PRIXLENx "] = %s", opstr, inst.s.rs2, imm, inst.i.rs1, dest, st_err(err));
        return sl_core_synchronous_exception(&c->core, EX_ABORT_STORE, dest, err);
    }
    RV_TRACE_STORE(c, dest, inst.s.rs2, val);
    RV_TRACE_PRINT(c, "%s x%u, %d(x%u)", opstr, inst.s.rs2, imm, inst.s.rs1);
    return 0;
}

static int XLEN_PREFIX(exec_alu_imm)(rv_core_t *c, rv_inst_t inst) {
    const uxlen_t u1 = c->core.r[inst.i.rs1];
    const i4 imm = ((i4)inst.raw) >> 20;  // sign extend immediate
    const u4 shift = inst.i.imm & (XLEN - 1);
#if USING_RV32
    const u4 func7 = inst.i.imm >> 5;
#else
    const u4 func7 = (inst.i.imm >> 5) & (~1);
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
        RV_TRACE_PRINT(c, "slti x%u, x%u, %u", inst.i.rd, inst.i.rs1, (u4)imm);
        break;

    case 0b011: // SLTIU
        result = (u1 < (u4)imm) ? 1 : 0;
        RV_TRACE_PRINT(c, "sltiu x%u, x%u, %u", inst.i.rd, inst.i.rs1, (u4)imm);
        break;

    case 0b100: // XORI
        result = u1 ^ (uxlen_t)imm;
        RV_TRACE_PRINT(c, "xori x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (u4)imm);
        break;

    case 0b110: // ORI
        result = u1 | (uxlen_t)imm;
        RV_TRACE_PRINT(c, "ori x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (u4)imm);
        break;

    case 0b111: // ANDI
        result = u1 & (uxlen_t)imm;
        RV_TRACE_PRINT(c, "andi x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (u4)imm);
        break;
    }
    if (inst.i.rd != RV_ZERO) {
        c->core.r[inst.i.rd] = result;
        RV_TRACE_RD(c, inst.i.rd, c->core.r[inst.i.rd]);
    }
    return 0;
}

static int XLEN_PREFIX(exec_alu)(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const uxlen_t u1 = c->core.r[inst.r.rs1];
    const uxlen_t u2 = c->core.r[inst.r.rs2];
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
        if (!(c->core.arch_options & SL_RISCV_EXT_M)) goto undef;
        switch (inst.r.funct3) {
        case 0b000: // MUL
            result = u1 * u2;
            RV_TRACE_OPSTR("mul");
            break;

        case 0b001: // MULH
            result_sl = mulx_ss((sxlen_t)u1, (sxlen_t)u2);
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
        c->core.r[inst.r.rd] = result;
        RV_TRACE_RD(c, inst.i.rd, c->core.r[inst.i.rd]);
    }
    RV_TRACE_PRINT(c, "%s x%u, x%u, x%u", opstr, inst.r.rd, inst.r.rs1, inst.r.rs2);
    return 0;

undef:
    return rv_undef(c, inst);
}

#if USING_RV64
// 32-bit instructions in 64-bit mode
int rv64_exec_alu_imm32(rv_core_t *c, rv_inst_t inst) {
    const i4 s1 = (u4)c->core.r[inst.i.rs1];
    const u4 shift = inst.i.imm & 63;
    i4 result;

    switch (inst.i.funct3) {
    case 0b000: // ADDIW
    {
        const i4 imm = SIGN_EXT_IMM12(inst);
        result = (i8)(i4)(c->core.r[inst.i.rs1] + imm);
        if (imm == 0) RV_TRACE_PRINT(c, "sext.w x%u, x%u", inst.i.rd, inst.i.rs1);
        else          RV_TRACE_PRINT(c, "addiw x%u, x%u, %d", inst.i.rd, inst.i.rs1, imm);
        break;
    }

    case 0b001: // SLLIW
        if (shift > 31) return rv_undef(c, inst);
        result = (u4)s1 << shift;
        RV_TRACE_PRINT(c, "slliw x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
        break;

    case 0b101: // SRLIW SRAIW
    {
        const u4 imm = inst.i.imm >> 5;
        if (shift > 31) return rv_undef(c, inst);
        if (imm == 0) {  // SRLIW
            result = (u4)s1 >> shift;
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
        c->core.r[inst.i.rd] = (i8)result;
        RV_TRACE_RD(c, inst.i.rd, c->core.r[inst.i.rd]);
    }
    return 0;
}

static int rv64_exec_alu32(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const u4 u1 = c->core.r[inst.r.rs1];
    const u4 u2 = c->core.r[inst.r.rs2];
    const u4 shift = u2 & 0x1f;
    u8 result;

    switch (inst.r.funct7) {
    case 0b0000000:
        switch (inst.r.funct3) {
        case 0b000: // ADDW
            result = (i4)(u1 + u2);
            RV_TRACE_OPSTR("addw");
            break;

        case 0b001: // SLLW
            result = (u4)(u1 << shift);
            RV_TRACE_OPSTR("sllw");
            break;

        case 0b101: // SRLW
            result = (u4)(u1 >> shift);
            RV_TRACE_OPSTR("srlw");
            break;

        default:
            goto undef;
        }
        break;

    case 0b0100000:
        switch (inst.r.funct3) {
        case 0b000: // SUBW
            result = (i4)(u1 - u2);
            RV_TRACE_OPSTR("subw");
            break;

        case 0b101: // SRAW
            result = (u4)(((i4)u1) >> shift);
            RV_TRACE_OPSTR("sraw");
            break;

        default:
            goto undef;
        }
        break;

    case 0b0000001: // RV64M extensions
        if (!(c->core.arch_options & SL_RISCV_EXT_M)) goto undef;
        switch (inst.r.funct3) {
        case 0b000: // MULW
            result = (i4)(u1 * u2);
            RV_TRACE_OPSTR("mulw");
            break;

        case 0b100: // DIVW
            if (u2 == 0) result = ~((u8)0);
            else         result = (i4)((i4)u1 / (i4)u2);
            RV_TRACE_OPSTR("divw");
            break;

        case 0b101: // DIVUW
            if (u2 == 0) result = ~((u8)0);
            else         result = (i4)(u1 / u2);
            RV_TRACE_OPSTR("divuw");
            break;


        case 0b110: // REMW
            if (u2 == 0) result = (i4)u1;
            else         result = (i4)((i4)u1 % (i4)u2);
            RV_TRACE_OPSTR("remw");
            break;

        case 0b111: // REMUW
            if (u2 == 0) result = (i4)u1;
            else         result = (i4)(u1 % u2);
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
        c->core.r[inst.r.rd] = result;
        RV_TRACE_RD(c, inst.i.rd, c->core.r[inst.i.rd]);
    }
    RV_TRACE_PRINT(c, "%s x%u, x%u, x%u", opstr, inst.r.rd, inst.r.rs1, inst.r.rs2);
    return 0;

undef:
    return rv_undef(c, inst);
}
#endif // USING_RV64

#define RVC_TO_REG(r) ((r) + 8)

static int XLEN_PREFIX(dispatch_alu32)(rv_core_t *c, rv_cinst_t ci) {
    switch (ci.cba.funct2) {
    case 0b00:  // C.SRLI
    {
#if USING_RV32
        if (ci.cba.imm1) return SL_ERR_UNDEF;
        const u4 imm = ci.cba.imm0;
#else
        const u4 imm = CBA_IMM(ci);
#endif
        if (imm == 0) return SL_ERR_UNDEF;
        const u4 rd = RVC_TO_REG(ci.cba.rsd);
        const uxlen_t result = (uxlen_t)(c->core.r[rd] >> imm);
        c->core.r[rd] = result;
        RV_TRACE_RD(c, rd, result);
        RV_TRACE_PRINT(c, "c.srli x%u, %u", rd, imm);
        return 0;
    }

    case 0b01:  // C.SRAI
    {
#if USING_RV32
        if (ci.cba.imm1) return SL_ERR_UNDEF;
        const u4 imm = ci.cba.imm0;
#else
        const u4 imm = CBA_IMM(ci);
#endif
        if (imm == 0) return SL_ERR_UNDEF;
        const u4 rd = RVC_TO_REG(ci.cba.rsd);
        const uxlen_t result = ((sxlen_t)c->core.r[rd]) >> imm;
        c->core.r[rd] = result;
        RV_TRACE_RD(c, rd, result);
        RV_TRACE_PRINT(c, "c.srai x%u, %u", rd, imm);
        return 0;
    }

    case 0b10:  // C.ANDI
    {
        const sxlen_t imm = sign_extend32(CBA_IMM(ci), 6);
        const u4 rd = RVC_TO_REG(ci.cba.rsd);
        const uxlen_t result = (uxlen_t)(c->core.r[rd] & (uxlen_t)imm);
        c->core.r[rd] = result;
        RV_TRACE_RD(c, rd, result);
        RV_TRACE_PRINT(c, "c.andi x%u, %#" PRIXLENx, rd, imm);
        return 0;
    }

    default:
        break;
    }

    // case 0b11

    const u4 rs2 = RVC_TO_REG(ci.cs.rs2);
    const u4 rsd = RVC_TO_REG(ci.cs.rs1);
    uxlen_t result;
    switch ((ci.cs.imm1 & 4) | ci.cs.imm0) {
    case 0b000: // C.SUB
        result = (uxlen_t)(c->core.r[rsd] - c->core.r[rs2]);
        RV_TRACE_PRINT(c, "c.sub x%u, x%u", rsd, rs2);
        break;

    case 0b001: // C.XOR
        result = (uxlen_t)(c->core.r[rsd] ^ c->core.r[rs2]);
        RV_TRACE_PRINT(c, "c.xor x%u, x%u", rsd, rs2);
        break;

    case 0b010: // C.OR
        result = (uxlen_t)(c->core.r[rsd] | c->core.r[rs2]);
        RV_TRACE_PRINT(c, "c.or x%u, x%u", rsd, rs2);
        break;

    case 0b011: // C.AND
        result = (uxlen_t)(c->core.r[rsd] & c->core.r[rs2]);
        RV_TRACE_PRINT(c, "c.or x%u, x%u", rsd, rs2);
        break;

#if USING_RV64
    case 0b100: // C.SUBW
        result = (i4)(c->core.r[rsd] - c->core.r[rs2]);
        RV_TRACE_PRINT(c, "c.subw x%u, x%u", rsd, rs2);
        break;

    case 0b101: // C.ADDW
        result = (i4)(c->core.r[rsd] + c->core.r[rs2]);
        RV_TRACE_PRINT(c, "c.addw x%u, x%u", rsd, rs2);
        break;

#endif
    default: return SL_ERR_UNDEF;
    }
    c->core.r[rsd] = result;
    RV_TRACE_RD(c, rsd, result);
    return 0;
}

static int XLEN_PREFIX(dispatch16)(rv_core_t *c, rv_inst_t inst) {
    int err = 0;
    rv_cinst_t ci;
    ci.raw = (u2)inst.raw;
#if RV_TRACE
    c->core.trace->opcode = ci.raw;
    RV_TRACE_OPT(c, ITRACE_OPT_INST16);
#endif
    const u2 op = (ci.cj.opcode << 3) | ci.cj.funct3;
    switch (op) {
    case 0b00000:   // C.ADDI4SPN
    {
        if (ci.raw == 0) goto undef;
        const u4 imm = CIW_IMM(ci);
        const u4 rd = RVC_TO_REG(ci.ciw.rd);
        c->core.r[rd] = (uxlen_t)(c->core.r[RV_SP] + imm);
        RV_TRACE_RD(c, rd, c->core.r[rd]);
        RV_TRACE_PRINT(c, "c.addi4spn x%u, %u", rd, imm);
        break;
   }

    case 0b00001: goto undef;   // C.FLD

    case 0b00010:   // C.LW
    {
        const u4 imm = ((ci.cl.imm0 & 2) << 1) | ((ci.cl.imm1 ) << 3) | ((ci.cl.imm0 & 1) << 6);
        const u4 rs = RVC_TO_REG(ci.cl.rs);
        const u4 rd = RVC_TO_REG(ci.cl.rd);
        const u8 dest = c->core.r[rs] + imm;
        u4 val;
        err = sl_core_mem_read_single(&c->core, dest, 4, &val);
        RV_TRACE_RD(c, rd, val);
        RV_TRACE_PRINT(c, "c.lw x%u, %u(x%u)", rd, imm, rs);
        if (err)
            return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, dest, err);
        c->core.r[rd] = (i4)val; // sign extend
        break;
    }

    case 0b00011:
#if USING_RV32
        // C.FLW
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        else {
            const u4 imm = CI_IMM_SCALED_4(ci);
            const u4 rs = RVC_TO_REG(ci.cl.rs);
            const u4 rd = RVC_TO_REG(ci.cl.rd);
            const u4 addr = c->core.r[rs] + imm;
            float val;
            err = sl_core_mem_read_single(&c->core, addr, 4, &val);
            RV_TRACE_RDF(c, rd, val);
            RV_TRACE_PRINT(c, "c.flw f%u, %u(x%u)", rd, imm, rs);
            if (err) return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, addr, err);
            c->core.f[rd].f = val;
            break;
        }
#else
    {   // C.LD
        const u4 imm = ((ci.cl.imm0 & 2) << 1) | ((ci.cl.imm1 ) << 3) | ((ci.cl.imm0 & 1) << 6);
        const u4 rs = RVC_TO_REG(ci.cl.rs);
        const u4 rd = RVC_TO_REG(ci.cl.rd);
        const u8 addr = c->core.r[rs] + imm;
        u8 val;
        err = sl_core_mem_read_single(&c->core, addr, 8, &val);
        RV_TRACE_RD(c, rd, val);
        RV_TRACE_PRINT(c, "c.ld x%u, %u(x%u)", rd, imm, rs);
        if (err)
            return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, addr, err);
        c->core.r[rd] = val;
        break;
    }
#endif

    case 0b00100: goto undef;   // reserved
    case 0b00101: goto undef;   // C.FSD

    case 0b00110:   // C.SW
    {
        const u4 imm = CS_IMM_SCALED_4(ci);
        const u4 rs = RVC_TO_REG(ci.cs.rs1);
        const u4 rd = RVC_TO_REG(ci.cs.rs2);
        const uxlen_t dest = c->core.r[rs] + imm;
        u4 val = (u4)c->core.r[rd];
        err = sl_core_mem_write_single(&c->core, dest, 4, &val);
        RV_TRACE_STORE(c, dest, rd, val);
        RV_TRACE_PRINT(c, "c.sw x%u, %u(x%u)", rd, imm, rs);
        if (err) return sl_core_synchronous_exception(&c->core, EX_ABORT_STORE, dest, err);
        break;
    }

    case 0b00111:
#if USING_RV32
        goto undef; // C.FSW
#else
    {   // C.SD
        const u4 imm = CS_IMM_SCALED_8(ci);
        const u4 rs = RVC_TO_REG(ci.cs.rs1);
        const u4 rd = RVC_TO_REG(ci.cs.rs2);
        const u8 dest = c->core.r[rs] + imm;
        u8 val = c->core.r[rd];
        err = sl_core_mem_write_single(&c->core, dest, 8, &val);
        RV_TRACE_STORE(c, dest, rd, val);
        RV_TRACE_PRINT(c, "c.sd x%u, %u(x%u)" PRIx64, rd, imm, rs);
        if (err) return sl_core_synchronous_exception(&c->core, EX_ABORT_STORE, dest, err);
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
        const i4 imm = sign_extend32(CI_IMM(ci), 6);
        const uxlen_t result = (uxlen_t)(c->core.r[ci.ci.rsd] + imm);
        c->core.r[ci.ci.rsd] = result;
        RV_TRACE_RD(c, ci.ci.rsd, result);
        RV_TRACE_PRINT(c, "c.addi x%u, %d", ci.ci.rsd, imm);
        break;
    }

    case 0b01001:
    {
#if USING_RV32
        // C.JAL
        const i4 imm = sign_extend32(CJ_IMM(ci), 12);
        const uxlen_t result = c->core.pc + imm;
        c->core.r[RV_RA] = c->core.pc + 2;
        RV_TRACE_RD(c, SL_CORE_REG_PC, RV_BR_TARGET(result, imm));
        RV_TRACE_PRINT(c, "c.jal %d", imm);
        c->core.pc = result;
        c->core.branch_taken = true;
#else
        // C.ADDIW
        if (ci.ci.rsd == 0) goto undef;
        const i4 imm = sign_extend32(CI_IMM(ci), 6);
        const i8 result = (i4)(c->core.r[ci.ci.rsd] + imm);
        c->core.r[ci.ci.rsd] = result;
        RV_TRACE_RD(c, ci.ci.rsd, result);
        // if imm=0, this is sext.w rd
        RV_TRACE_PRINT(c, "c.addiw x%u, %d", ci.ci.rsd, imm);
#endif
        break;
    }

    case 0b01010:   // C.LI
    {
        if (ci.ci.rsd == 0) goto undef;
        const i4 val = sign_extend32(CI_IMM(ci), 6);
        c->core.r[ci.ci.rsd] = (sxlen_t)val;
        RV_TRACE_RD(c, ci.ci.rsd, c->core.r[ci.ci.rsd]);
        RV_TRACE_PRINT(c, "c.li x%u, %d", ci.ci.rsd, val);
        break;
    }

    case 0b01011:
        if (ci.ci.rsd == 0) goto undef;
        if (ci.ci.rsd == RV_SP) {   // C.ADDI16SP
            const i4 imm = sign_extend32((CI_ADDI16SP_IMM(ci)), 10);
            const uxlen_t sp = c->core.r[RV_SP] + imm;
            c->core.r[RV_SP] = sp;
            RV_TRACE_RD(c, RV_SP, sp);
            RV_TRACE_PRINT(c, "c.addi2sp %d", imm);
        } else {    // C.LUI
            const sxlen_t imm = sign_extend32((CI_IMM(ci) << 12), 18);
            if (imm == 0) goto undef;
            c->core.r[ci.ci.rsd] = imm;
            RV_TRACE_RD(c, ci.ci.rsd, c->core.r[ci.ci.rsd]);
            RV_TRACE_PRINT(c, "c.lui x%u, %#" PRIXLENx, ci.ci.rsd, imm);
        }
        break;

    case 0b01100:
        err = XLEN_PREFIX(dispatch_alu32)(c, ci);
        if (err != 0) {
            if (err == SL_ERR_UNDEF) goto undef;
            return err;
        }
        break;

    case 0b01101:   // C.J
    {
        const i4 imm = sign_extend32(CJ_IMM(ci), 12);
        uxlen_t result = c->core.pc + imm;
        RV_TRACE_RD(c, SL_CORE_REG_PC, result);
        RV_TRACE_PRINT(c, "c.j %#" PRIXLENx, RV_BR_TARGET(result, imm));
        c->core.pc = result;
        c->core.branch_taken = true;
        break;
    }

    case 0b01110:   // C.BEQZ
    {
        const i4 imm = sign_extend32(CB_IMM(ci), 9);
        const u4 rs = RVC_TO_REG(ci.cb.rs);
        const uxlen_t result = (uxlen_t)(c->core.pc + imm);
        if (c->core.r[rs] == 0) {
            c->core.pc = result;
            c->core.branch_taken = true;
            RV_TRACE_RD(c, SL_CORE_REG_PC, result);
        }
        RV_TRACE_PRINT(c, "c.beqz x%u, %#" PRIXLENx, rs, RV_BR_TARGET(result, imm));
        break;
    }

    case 0b01111:   // C.BNEZ
    {
        const i4 imm = sign_extend32(CB_IMM(ci), 9);
        const u4 rs = RVC_TO_REG(ci.cb.rs);
        const uxlen_t result = (uxlen_t)(c->core.pc + imm);
        if (c->core.r[rs] != 0) {
            c->core.pc = result;
            c->core.branch_taken = true;
            RV_TRACE_RD(c, SL_CORE_REG_PC, result);
        }
        RV_TRACE_PRINT(c, "c.bnez x%u, %#" PRIXLENx, rs, RV_BR_TARGET(result, imm));
        break;
    }

    case 0b10000:   // C.SLLI
        if (ci.ci.rsd == RV_ZERO) goto undef;
        else {
#if USING_RV32
            if (ci.ci.imm1) goto undef;
            const i4 imm = ci.ci.imm0;
#else
            const i4 imm = CI_IMM(ci);
#endif
            if (imm == 0) goto undef;
            const uxlen_t result = (uxlen_t)(c->core.r[ci.ci.rsd] << imm);
            c->core.r[ci.ci.rsd] = result;
            RV_TRACE_RD(c, ci.ci.rsd, result);
            RV_TRACE_PRINT(c, "c.slli x%u, %u", ci.ci.rsd, imm);
            break;
        }

    case 0b10001:   // C.FLDSP
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        else {
            const u4 imm = CI_IMM_SCALED_8(ci);
            const uxlen_t addr = c->core.r[RV_SP] + imm;
            double val;
            err = sl_core_mem_read_single(&c->core, addr, 8, &val);
            RV_TRACE_RDD(c, ci.ci.rsd, val);
            RV_TRACE_PRINT(c, "c.fldsp f%u, %u", ci.ci.rsd, imm);
            if (err)
                return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, addr, err);
            c->core.f[ci.ci.rsd].d = val;
            break;
        }

    case 0b10010:   // C.LWSP
        if (ci.ci4.rd == RV_ZERO) goto undef;
        else {
            const u4 imm = CI_IMM_SCALED_4(ci);
            const uxlen_t addr = c->core.r[RV_SP] + imm;
            u4 val;
            err = sl_core_mem_read_single(&c->core, addr, 4, &val);
            RV_TRACE_RD(c, ci.ci.rsd, val);
            RV_TRACE_PRINT(c, "c.lwsp x%u, %u", ci.ci.rsd, imm);
            if (err)
                return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, addr, err);
            c->core.r[ci.ci.rsd] = val;
            break;
        }

    case 0b10011:
#if USING_RV32
        // C.FLWSP
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        else {
            const u4 imm = CI_IMM_SCALED_4(ci);
            const u4 addr = c->core.r[RV_SP] + imm;
            float val;
            err = sl_core_mem_read_single(&c->core, addr, 4, &val);
            RV_TRACE_RDF(c, ci.ci.rsd, val);
            RV_TRACE_PRINT(c, "c.flwsp f%u, %u", ci.ci.rsd, imm);
            if (err) return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, addr, err);
            c->core.f[ci.ci.rsd].f = val;
            break;
        }
#else
        if (ci.ci.rsd == RV_ZERO) goto undef;
        else {
            // C.LDSP
            const u4 imm = CI_IMM_SCALED_8(ci);
            const u8 addr = c->core.r[RV_SP] + imm;
            u8 val;
            err = sl_core_mem_read_single(&c->core, addr, 8, &val);
            RV_TRACE_RD(c, ci.ci.rsd, val);
            RV_TRACE_PRINT(c, "c.ldsp x%u, %u", ci.ci.rsd, imm);
            if (err)
                return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, addr, err);
            c->core.r[ci.ci.rsd] = val;
            break;
        }
#endif

    case 0b10100:   // C.JR C.MV C.EBREAK C.JALR C.ADD
        if (ci.cr.funct4 == 0) {
            if (ci.cr.rsd == 0) goto undef;
            if (ci.cr.rs2 == 0) {
                // C.JR
                const uxlen_t addr = c->core.r[ci.cr.rsd];
                c->core.pc = addr;
                c->core.branch_taken = true;
                RV_TRACE_RD(c, SL_CORE_REG_PC, addr);
                RV_TRACE_PRINT(c, "c.jr x%u", ci.ci.rsd);
            } else {
                // C.MV
                const uxlen_t val = c->core.r[ci.cr.rs2];
                c->core.r[ci.cr.rsd] = val;
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
                    const uxlen_t addr = c->core.r[ci.cr.rsd];
                    c->core.r[ci.cr.rsd] = c->core.pc + 2;
                    c->core.pc = addr;
                    c->core.branch_taken = true;
                    RV_TRACE_RD(c, SL_CORE_REG_PC, addr);
                    RV_TRACE_PRINT(c, "c.jalr x%u", ci.ci.rsd);
                }
            } else {
                if (ci.cr.rsd == 0) goto undef;
                // C.ADD
                const uxlen_t val = c->core.r[ci.cr.rs2] + c->core.r[ci.cr.rsd];
                c->core.r[ci.cr.rsd] = val;
                RV_TRACE_RD(c, ci.cr.rsd, val);
                RV_TRACE_PRINT(c, "c.add x%u, x%u", ci.ci.rsd, ci.cr.rs2);
            }
        }
        break;

    case 0b10101:   // C.FSDSP
    {
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        const u4 imm = CSS_IMM_SCALED_8(ci);
        const u8 addr = c->core.r[RV_SP] + imm;
        u8 val = c->core.f[ci.css.rs2].u8;
        RV_TRACE_STORE_D(c, addr, ci.css.rs2, c->core.f[ci.css.rs2].d);
        err = sl_core_mem_write_single(&c->core, addr, 8, &val);
        RV_TRACE_PRINT(c, "c.fsdsp x%u, %u", ci.css.rs2, imm);
        if (err) return sl_core_synchronous_exception(&c->core, EX_ABORT_STORE, addr, err);
        break;
    }

    case 0b10110:   // C.SWSP
    {
        const u4 imm = CSS_IMM_SCALED_4(ci);
        const uxlen_t addr = c->core.r[RV_SP] + imm;
        u4 val = c->core.r[ci.css.rs2];
        err = sl_core_mem_write_single(&c->core, addr, 4, &val);
        RV_TRACE_STORE(c, addr, ci.css.rs2, val);
        RV_TRACE_PRINT(c, "c.swsp x%u, %u", ci.css.rs2, imm);
        if (err) return sl_core_synchronous_exception(&c->core, EX_ABORT_STORE, addr, err);
        break;
    }

    case 0b10111:
#if USING_RV32
    {   // C.FSWSP
        const u4 imm = CSS_IMM_SCALED_4(ci);
        const u4 addr = c->core.r[RV_SP] + imm;
        u4 val = c->core.f[ci.css.rs2].u4;
        RV_TRACE_STORE_F(c, addr, ci.css.rs2, c->core.f[ci.css.rs2].f);
        err = sl_core_mem_write_single(&c->core, addr, 4, &val);
        RV_TRACE_PRINT(c, "c.fswsp f%u, %u", ci.css.rs2, imm);
        if (err) return sl_core_synchronous_exception(&c->core, EX_ABORT_STORE, addr, err);
        break;
    }
#else
    {   // C.SDSP
        const u4 imm = CSS_IMM_SCALED_8(ci);
        const u8 addr = c->core.r[RV_SP] + imm;
        u8 val = c->core.r[ci.css.rs2];
        err = sl_core_mem_write_single(&c->core, addr, 8, &val);
        RV_TRACE_STORE(c, addr, ci.css.rs2, val);
        RV_TRACE_PRINT(c, "c.sdsp x%u, %u", ci.css.rs2, imm);
        if (err) return sl_core_synchronous_exception(&c->core, EX_ABORT_STORE, addr, err);
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
        if (!(c->core.arch_options & SL_RISCV_EXT_C)) return rv_undef(c, inst);
        c->core.prev_len = 2;
        return XLEN_PREFIX(dispatch16)(c, inst);
    }
    c->core.prev_len = 4;

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

    case OP_FP:
        err = rv_exec_fp(c, inst);
        break;

    case OP_FP_LOAD:
        err = rv_exec_fp_load(c, inst);
        break;

    case OP_FP_STORE:
        err = rv_exec_fp_store(c, inst);
        break;

    case OP_FMADD_S:
    case OP_FMSUB_S:
    case OP_FNMSUB_S:
    case OP_FNMADD_S:
        err = rv_exec_fp_mac(c, inst);
        break;

    // Other
    case OP_MISC_MEM:  // FENCE FENCE.I
        err = rv_exec_mem(c, inst);
        break;

    case OP_SYSTEM:  // ECALL EBREAK CSRRW CSRRS CSRRC CSRRWI CSRRSI CSRRCI
        err = rv_exec_system(c, inst);
        break;

    case OP_AMO:
        err = rv_exec_atomic(c, inst);
        break;

    default:
        err = rv_undef(c, inst);
        break;
    }
    return err;
}
