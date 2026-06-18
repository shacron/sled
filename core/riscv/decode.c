// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2026 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>

#include <core/riscv/inst.h>
#include <core/riscv/rv.h>
#include <sled/error.h>
#include <sled/slac.h>
#include <sled/sym.h>
#include <sled/types.h>

#define FENCE_W (1u << 0)
#define FENCE_R (1u << 1)
#define FENCE_O (1u << 2)
#define FENCE_I (1u << 3)

// result formatting for prints
#define PR_B        (1u << 0) // print branch if it occured
#define PR_D        (1u << 1) // print D register
#define PR_DF32     (1u << 2) // print fp32 register D
#define PR_DF64     (1u << 3) // print fp64 register D
#define PR_ST       (1u << 4) // print store of r0 to addr at d0
#define PR_SZ1      (0u << 8) // size 1
#define PR_SZ2      (1u << 8) // size 2
#define PR_SZ4      (2u << 8) // size 4
#define PR_SZ8      (3u << 8) // size 8
#define PR_SZF32    (4u << 8) // size fp32
#define PR_SZF64    (5u << 8) // size fp64

#define PR_BL       (PR_B | PR_D)

#define PR_SIZE(f)  ((f >> 8) & 0x7)
#define PR_ST1      (PR_ST | PR_SZ1)
#define PR_ST2      (PR_ST | PR_SZ2)
#define PR_ST4      (PR_ST | PR_SZ4)
#define PR_ST8      (PR_ST | PR_SZ8)
#define PR_STF32    (PR_ST | PR_SZF32)
#define PR_STF64    (PR_ST | PR_SZF64)


#define SIGN_EXT_IMM12(inst) (((i4)inst.raw) >> 20)

#define RVC_TO_REG(r) ((r) + 8)

#define ABS(v) ((v) < 0 ? -(v) : (v))
#define SIGNSTR(v) ((v) < 0 ? "-" : "")

static inline i4 sign_extend32(i4 value, u1 valid_bits) {
    const u1 shift = (32 - valid_bits);
    return ((i4)((u4)value << shift) >> shift);
}

static const u2 rv_barrier_map[4] = {
    0, BARRIER_STORE, BARRIER_LOAD, BARRIER_LOAD | BARRIER_STORE
};

#if SLAC_TRACE


// these defines enable pretty printing for disassembly
// they make the instructions more conformant to the recommended aliases
#define RV_PRETTY_PRINT 1

#if RV_TRACE_EXPAND_C_OPS
// print C-extension ops the same as the full-length encoding
#define STRACE_EXPAND(si, ...) STRACE(si, __VA_ARGS__)
#define STRACE_C(si, ...)
#else
// print c.<op> short format
#define STRACE_EXPAND(...)
#define STRACE_C(si, ...) STRACE(si, __VA_ARGS__)
#endif

static const char priv_level_char[4] = { 'u', 's', 'h', 'm' };

static const char *rv_barrier_string[4] = {
    [0]                             = "",
    [BARRIER_STORE]                 = ".rl",
    [BARRIER_LOAD]                  = ".aq",
    [BARRIER_LOAD | BARRIER_STORE]  = ".aqrl"
};

static void rv_fence_op_name(u1 op, char *s) {
    if (op & FENCE_I) *s++ = 'i';
    if (op & FENCE_O) *s++ = 'o';
    if (op & FENCE_R) *s++ = 'r';
    if (op & FENCE_W) *s++ = 'w';
    *s = '\0';
}

#else

#define STRACE_EXPAND(...)
#define STRACE_C(si, ...)

#endif // SLAC_TRACE

static inline void slac_in(sl_slac_inst_t *si, u2 op, u1 arg, u4 format) {
    si->type = SLAC_TYPE(op);
    si->func = SLAC_FUNC(op);
    si->arg = arg;
#if SLAC_TRACE
    si->desc.print_format = format;
#endif
}

static void set_nop(sl_slac_inst_t *si) {
    // si->pred = SLAC_PRED_NEVER;
    si->type = SLAC_TYPE_SYS;
    si->func = SLAC_FUNC_NOP;
}

static int rv_slac_undef(rv_core_t *c, sl_slac_inst_t *si) {
    si->type = SLAC_TYPE_SYS;
    si->func = SLAC_FUNC_UNDEF;
    si->arg = SLAC_IN_ARG_NONE;
    STRACE(si, "undefined");
    return 0;
}

#if SLAC_TRACE
int rv_slac_print_pre(sl_core_t *c, sl_slac_inst_t *si, char *buf, int buflen) {
    int len = snprintf(buf, buflen, "%s", si->desc.s);

    int padding = 57 - len;
    len += snprintf(buf + len, buflen - len, "%*s", padding, ";");
    return len;
}

int rv_slac_print_post(sl_core_t *c, sl_slac_inst_t *si, char *buf, int buflen) {
    int len = 0;
    const u4 format = si->desc.print_format;
    u1 reg;

    if (format & PR_B) {
        if (c->branch_taken) {
            len += snprintf(buf, buflen, " pc=%#" PRIx64, c->pc);
            buf += len;
            buflen -= len;
        }
    }
    if (format & PR_D) {
        reg = si->d0;
        len += snprintf(buf, buflen, " %s=%#" PRIx64, rv_name_for_reg(reg), c->r[reg]);
        buf += len;
        buflen -= len;
    } else {
        if (format & PR_DF32) {
            reg = si->d0;
            len += snprintf(buf, buflen, " f%u=%g", reg, c->f[reg].f);
            buf += len;
            buflen -= len;
        } else if (format & PR_DF64) {
            reg = si->d0;
            len += snprintf(buf, buflen, " f%u=%g", reg, c->f[reg].d);
            buf += len;
            buflen -= len;
        }
    }
    if (format & PR_ST) {
        const u1 size = PR_SIZE(format);
        const u8 addr = c->r[si->r0] + si->simm;
        switch (size) {
        case PR_ST1 >> 8:  len += snprintf(buf, buflen, " [%#" PRIx64 "]=%#x", addr, (u1)c->r[si->d0]); break;
        case PR_ST2 >> 8:  len += snprintf(buf, buflen, " [%#" PRIx64 "]=%#x", addr, (u2)c->r[si->d0]); break;
        case PR_ST4 >> 8:  len += snprintf(buf, buflen, " [%#" PRIx64 "]=%#x", addr, (u4)c->r[si->d0]); break;
        case PR_ST8 >> 8:  len += snprintf(buf, buflen, " [%#" PRIx64 "]=%#" PRIx64, addr, c->r[si->d0]); break;
        case PR_STF32 >> 8: len += snprintf(buf, buflen, " [%#" PRIx64 "]=%#g", addr, c->f[si->d0].f); break;
        case PR_STF64 >> 8: len += snprintf(buf, buflen, " [%#" PRIx64 "]=%#g", addr, c->f[si->d0].d); break;
        }
        buf += len;
        buflen -= len;
    }

    return len;
}
#endif // SLAC_TRACE

static int rv_decode_u_type(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    si->d0 = inst.u.rd;
    si->uimm = (u8)(i4)(inst.raw & 0xfffff000);

#if SLAC_TRACE
    u8 print_uimm = si->uimm;
    if (c->core.mode == SL_CORE_MODE_4)
        print_uimm &= 0xffffffff;
#endif

    if (inst.u.opcode == OP_LUI) {
        slac_in(si, SLAC_OP_MOVI, SLAC_IN_ARG_DI, PR_D);
        STRACE(si, "lui x%u, %#" PRIx64, si->d0, print_uimm);
    } else { // OP_AUIPC
        // add pc + offset 
        // todo: optimize to MOV if address is known?
        slac_in(si, SLAC_OP_ADR4K, SLAC_IN_ARG_DI, PR_D);
        STRACE(si, "auipc x%u, %#" PRIx64, si->d0, print_uimm);
    }
    if (si->d0 == 0) set_nop(si);
    return 0;
}

static int rv_decode_alu_imm(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    u4 shift;
    u4 func7;
    const bool is_rv32 = c->core.mode == SL_CORE_MODE_4;
    if (is_rv32) {
        shift = inst.i.imm & 31;
        func7 = inst.i.imm >> 5;
    } else {
        shift = inst.i.imm & 63;
        func7 = (inst.i.imm >> 5) & (~1);
    }

    switch (inst.i.funct3) {
    case 0b000: // ADDI
        slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRI, PR_D);
        si->simm = ((i4)inst.raw) >> 20;  // sign extend immediate
#if RV_PRETTY_PRINT
        if (inst.i.rs1 == RV_ZERO)
            STRACE(si, "li x%u, %s0x%x", inst.i.rd, SIGNSTR((i4)si->simm), ABS((i4)si->simm));
        else if (si->simm == 0)
            STRACE(si, "mv x%u, x%u", inst.i.rd, inst.i.rs1);
        else
            STRACE(si, "addi x%u, x%u, %s0x%x", inst.i.rd, inst.i.rs1, SIGNSTR((i4)si->simm), ABS((i4)si->simm));
#else
        STRACE(si, "addi x%u, x%u, %d", inst.i.rd, inst.i.rs1, (i4)si->simm);
#endif
        break;

    case 0b001: // SLLI
        if (func7 != 0) return rv_slac_undef(c, si);
        slac_in(si, SLAC_OP_SHL, SLAC_IN_ARG_DRI, PR_D);
        si->uimm = shift;
        STRACE(si, "slli x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
        break;

    case 0b101:
        if (func7 == 0) {   // SRLI
            slac_in(si, SLAC_OP_SHR, SLAC_IN_ARG_DRI, PR_D);
            STRACE(si, "srli x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
        } else if (func7 == 0b0100000) {  //SRAI
            slac_in(si, SLAC_OP_SHRS, SLAC_IN_ARG_DRI, PR_D);
            STRACE(si, "srai x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
        } else {
            return rv_slac_undef(c, si);
        }
        si->uimm = shift;
        break;

    case 0b010: // SLTI
        slac_in(si, SLAC_OP_CSELS, SLAC_IN_ARG_DRI, PR_D);
        si->simm = ((i4)inst.raw) >> 20;  // sign extend immediate
        STRACE(si, "slti x%u, x%u, %u", inst.i.rd, inst.i.rs1, (u4)si->simm);
        break;

    case 0b011: // SLTIU
        slac_in(si, SLAC_OP_CSEL, SLAC_IN_ARG_DRI, PR_D);
        // docs: the immediate is first sign-extended to XLEN bits then treated as an unsigned number
        if (is_rv32) si->uimm = (u4)(((i4)inst.raw) >> 20);  // sign extend immediate
        else         si->uimm = (u8)(i8)(((i4)inst.raw) >> 20);  // sign extend immediate
        STRACE(si, "sltiu x%u, x%u, %" PRIu64, inst.i.rd, inst.i.rs1, si->uimm);
        break;

    case 0b100: // XORI
        slac_in(si, SLAC_OP_XOR, SLAC_IN_ARG_DRI, PR_D);
        si->simm = ((i4)inst.raw) >> 20;  // sign extend immediate
        STRACE(si, "xori x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (u4)si->simm);
        break;

    case 0b110: // ORI
        slac_in(si, SLAC_OP_OR, SLAC_IN_ARG_DRI, PR_D);
        si->simm = ((i4)inst.raw) >> 20;  // sign extend immediate
        STRACE(si, "ori x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (u4)si->simm);
        break;

    case 0b111: // ANDI
        slac_in(si, SLAC_OP_AND, SLAC_IN_ARG_DRI, PR_D);
        si->simm = ((i4)inst.raw) >> 20;  // sign extend immediate
        STRACE(si, "andi x%u, x%u, %#x", inst.i.rd, inst.i.rs1, (u4)si->simm);
        break;

    default:
        return rv_slac_undef(c, si);
    }

    si->d0 = inst.i.rd;
    si->r0 = inst.i.rs1;
    if (si->d0 == 0) set_nop(si);
    return 0;
}

// 32-bit instructions in 64-bit mode
static int rv64_decode_alu_imm4(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    if (c->core.mode != SL_CORE_MODE_8) return rv_slac_undef(c, si);

    const u4 shift = inst.i.imm & 63;
    switch (inst.i.funct3) {
    case 0b000: // ADDIW
        if (inst.i.imm == 0) {
            slac_in(si, SLAC_OP_MOVR, SLAC_IN_ARG_DR, PR_D);
            STRACE(si, "sext.w x%u, x%u", inst.i.rd, inst.i.rs1);
        } else {
            slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRI, PR_D);
            si->simm = SIGN_EXT_IMM12(inst);
            STRACE(si, "addiw x%u, x%u, %d", inst.i.rd, inst.i.rs1, (i4)si->simm);
        }
        break;

    case 0b001: // SLLIW
        if (shift > 31) return rv_slac_undef(c, si);
        slac_in(si, SLAC_OP_SHL, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "slliw x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
        si->uimm = shift;
        break;

    case 0b101: // SRLIW SRAIW
    {
        if (shift > 31) return rv_slac_undef(c, si);
        const u4 imm = inst.i.imm >> 5;
        si->uimm = shift;
        if (imm == 0) {  // SRLIW
            slac_in(si, SLAC_OP_SHR, SLAC_IN_ARG_DRI, PR_D);
            STRACE(si, "srliw x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
            break;
        } else if (imm == 0b0100000) {   // SRAIW
            slac_in(si, SLAC_OP_SHRS, SLAC_IN_ARG_DRI, PR_D);
            STRACE(si, "sraiw x%u, x%u, %u", inst.i.rd, inst.i.rs1, shift);
            break;
        }
        return rv_slac_undef(c, si);
    }

    default:
        return rv_slac_undef(c, si);
    }

    si->len = SLAC_IN_LEN_4;
    si->sx8 = 1;            // w instructions always extend their output to 8 bytes
    si->r0 = inst.i.rs1;
    si->d0 = inst.i.rd;
    if (inst.i.rd == RV_ZERO) set_nop(si);
    return 0;
}

static int rv_decode_alu(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    STRACE_DECL_OPSTR;
    si->type = SLAC_TYPE_ALU;
    si->arg = SLAC_IN_ARG_DRR;
    switch (inst.r.funct7) {
    case 0:
        switch (inst.r.funct3) {
        case 0b000: // ADD
            si->func = SLAC_FUNC_ADD;
            STRACE_OPSTR("add");
            break;
        case 0b001: // SLL
            si->func = SLAC_FUNC_SHL;
            STRACE_OPSTR("sll");
            break;
        case 0b010: // SLT
            si->func = SLAC_FUNC_CSELS;
            STRACE_OPSTR("slt");
            break;
        case 0b011:  // SLTU
            si->func = SLAC_FUNC_CSEL;
            STRACE_OPSTR("sltu");
            break;
        case 0b100:  // XOR
            si->func = SLAC_FUNC_XOR;
            STRACE_OPSTR("xor");
            break;
        case 0b101:  // SRL
            si->func = SLAC_FUNC_SHR;
            STRACE_OPSTR("srl");
            break;
        case 0b110:  // OR
            si->func = SLAC_FUNC_OR;
            STRACE_OPSTR("or");
            break;
        case 0b111:  // AND
            si->func = SLAC_FUNC_AND;
            STRACE_OPSTR("and");
            break;
        }
        break;

    case 0b0100000:
        switch (inst.r.funct3) {
        case 0b000: // SUB
            si->func = SLAC_FUNC_SUB;
            STRACE_OPSTR("sub");
            break;
        case 0b101: // SRA
            si->func = SLAC_FUNC_SHRS;
            STRACE_OPSTR("sra");
            break;
        default:  goto undef;
        }
        break;

    case 0b0000001: // M extensions
        if (!(c->core.arch_options & SL_RISCV_EXT_M)) goto undef;
        switch (inst.r.funct3) {
        case 0b000: // MUL
            si->func = SLAC_FUNC_MUL;
            STRACE_OPSTR("mul");
            break;
        case 0b001: // MULH
            si->func = SLAC_FUNC_MULHSS;
            STRACE_OPSTR("mulh");
            break;
        case 0b010: // MULHSU
            si->func = SLAC_FUNC_MULHSU;
            STRACE_OPSTR("mulhsu");
            break;
        case 0b011: // MULHU
            si->func = SLAC_FUNC_MULHUU;
            STRACE_OPSTR("mulhu");
            break;
        case 0b100: // DIV
            si->func = SLAC_FUNC_DIVS;
            STRACE_OPSTR("div");
            break;
        case 0b101: // DIVU
            si->func = SLAC_FUNC_DIV;
            STRACE_OPSTR("divu");
            break;
        case 0b110: // REM
            si->func = SLAC_FUNC_MODS;
            STRACE_OPSTR("rem");
            break;
        case 0b111: // REMU
            si->func = SLAC_FUNC_MOD;
            STRACE_OPSTR("remu");
            break;
        }
        break;

    default:
        goto undef;
    }

    si->r0 = inst.r.rs1;
    si->r1 = inst.r.rs2;
    si->d0 = inst.r.rd;
    if (si->d0 == 0) set_nop(si);

#if SLAC_TRACE
    si->desc.print_format = PR_D;
    STRACE(si, "%s x%u, x%u, x%u", opstr_, inst.r.rd, inst.r.rs1, inst.r.rs2);
#endif
    return 0;

undef:
    return rv_slac_undef(c, si);
}

static int rv64_decode_alu4(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    if (c->core.mode != SL_CORE_MODE_8) return rv_slac_undef(c, si);

    STRACE_DECL_OPSTR;
    si->type = SLAC_TYPE_ALU;
    si->arg = SLAC_IN_ARG_DRR;
#if SLAC_TRACE
    si->desc.print_format = PR_D;
#endif
    switch (inst.r.funct7) {
    case 0b0000000:
        switch (inst.r.funct3) {
        case 0b000: // ADDW
            si->func = SLAC_FUNC_ADD;
            STRACE_OPSTR("addw");
            break;
        case 0b001: // SLLW
            si->func = SLAC_FUNC_SHL;
            STRACE_OPSTR("sllw");
            break;
        case 0b101: // SRLW
            si->func = SLAC_FUNC_SHR;
            STRACE_OPSTR("srlw");
            break;
        default:
            goto undef;
        }
        break;

    case 0b0100000:
        switch (inst.r.funct3) {
        case 0b000: // SUBW
            si->func = SLAC_FUNC_SUB;
            STRACE_OPSTR("subw");
            break;
        case 0b101: // SRAW
            si->func = SLAC_FUNC_SHRS;
            STRACE_OPSTR("sraw");
            break;
        default:
            goto undef;
        }
        break;

    case 0b0000001: // RV64M extensions
        if (!(c->core.arch_options & SL_RISCV_EXT_M)) goto undef;
        switch (inst.r.funct3) {
        case 0b000: // MULW
            si->func = SLAC_FUNC_MUL;
            STRACE_OPSTR("mulw");
            break;
        case 0b100: // DIVW
            si->func = SLAC_FUNC_DIVS;
            STRACE_OPSTR("divw");
            break;
        case 0b101: // DIVUW
            si->func = SLAC_FUNC_DIV;
            STRACE_OPSTR("divuw");
            break;
        case 0b110: // REMW
            si->func = SLAC_FUNC_MODS;
            STRACE_OPSTR("remw");
            break;
        case 0b111: // REMUW
            si->func = SLAC_FUNC_MOD;
            STRACE_OPSTR("remuw");
            break;
        default:
            goto undef;
        }
        break;

    default:
        goto undef;
    }

    si->len = SLAC_IN_LEN_4;
    si->sx8 = 1;            // w instructions always extend their output to 8 bytes
    si->r0 = inst.r.rs1;
    si->r1 = inst.r.rs2;
    si->d0 = inst.r.rd;
    if (inst.r.rd == RV_ZERO) set_nop(si);
    STRACE(si, "%s x%u, x%u, x%u", opstr_, inst.r.rd, inst.r.rs1, inst.r.rs2);
    return 0;

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_jump(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    i4 imm = (inst.j.imm3 << 12) | (inst.j.imm2 << 11) | (inst.j.imm1 << 1);
    // or imm4 with sign extend
    imm |= ((i4)(inst.raw & 0x80000000)) >> (31 - 20);
    si->simm = imm;

#if SLAC_TRACE
    // silly to do this twice, slac branch should accept full target address in immediate
    const u8 trace_dest = imm + c->core.pc;

#if RV_PRETTY_PRINT
    sl_sym_entry_t *sym = sl_core_get_sym_for_addr(&c->core, trace_dest);
    char *symname = "";
    if (sym != NULL)
        symname = sym->name;
#endif
#endif

    if (inst.j.rd == RV_ZERO) {        // J
        slac_in(si, SLAC_OP_B, SLAC_IN_ARG_I, PR_B);
        STRACE(si, "j %#" PRIx64, trace_dest);
    } else {
        slac_in(si, SLAC_OP_BL, SLAC_IN_ARG_DI, PR_BL);
        si->r2 = 4; // pc offset to step
        si->d0 = inst.j.rd;
#if RV_PRETTY_PRINT
        if (inst.j.rd == RV_RA)
            STRACE(si, "jal %#" PRIx64 " <%s>", trace_dest, symname);
        else
            STRACE(si, "jal x%u, %#" PRIx64 " <%s>", inst.j.rd, trace_dest, symname);
#else
        STRACE(si, "jal x%u, %#" PRIx64, inst.j.rd, trace_dest);
#endif
    }
    return 0;
}

// JALR: I-type
static int rv_decode_jalr(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    if (inst.i.funct3 != 0)
        return rv_slac_undef(c, si);

    si->r0 = inst.i.rs1;
    si->simm = ((i4)inst.raw) >> 20;
    if (inst.i.rd == RV_ZERO) {
        slac_in(si, SLAC_OP_B, SLAC_IN_ARG_R1, PR_B);
        STRACE(si, "ret");
    } else {
        slac_in(si, SLAC_OP_BL, SLAC_IN_ARG_DRI, PR_BL);
        STRACE(si, "jalr %d(x%u)", (i4)si->simm, inst.i.rs1);
        si->r2 = 4; // pc offset to step
        si->d0 = inst.i.rd;
    }
    return 0;
}

static int rv_decode_branch(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    STRACE_DECL_OPSTR;

    switch (inst.b.funct3) {
    case 0b000: // BEQ
        slac_in(si, SLAC_OP_CBEQ, SLAC_IN_ARG_RRI, PR_B);
        STRACE_OPSTR("beq");
        break;

    case 0b001: // BNE
        slac_in(si, SLAC_OP_CBNE, SLAC_IN_ARG_RRI, PR_B);
        STRACE_OPSTR("bne");
        break;

    case 0b100: // BLT
        slac_in(si, SLAC_OP_CBLTS, SLAC_IN_ARG_RRI, PR_B);
        STRACE_OPSTR("blt");
        break;

    case 0b101: // BGE
        slac_in(si, SLAC_OP_CBGES, SLAC_IN_ARG_RRI, PR_B);
        STRACE_OPSTR("bge");
        break;

    case 0b110: // BLTU
        slac_in(si, SLAC_OP_CBLTU, SLAC_IN_ARG_RRI, PR_B);
        STRACE_OPSTR("bltu");
        break;

    case 0b111: // BGEU
        slac_in(si, SLAC_OP_CBGEU, SLAC_IN_ARG_RRI, PR_B);
        STRACE_OPSTR("bgeu");
        break;

    default:
        return rv_slac_undef(c, si);
    }

    si->r0 = inst.b.rs1;
    si->r1 = inst.b.rs2;
    i4 imm = (inst.b.imm3 << 11) | (inst.b.imm2 << 5) | (inst.b.imm1 << 1);
    // or imm4 with sign extend
    imm |= ((i4)(inst.raw & 0x80000000)) >> (31 - 12);
    si->simm = imm;
    STRACE(si, "%s x%u, x%u, %#" PRIx64, opstr_, inst.b.rs1, inst.b.rs2, c->core.pc + imm);
    return 0;
}

// I-type
static int rv_decode_load(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    STRACE_DECL_OPSTR;
    const i4 imm = ((i4)inst.raw) >> 20;
    si->simm = imm;
    si->r0 = inst.i.rs1;
    si->d0 = inst.i.rd;
    if (si->d0 == 0) si->d0 = SLAC_REG_DISCARD;

    switch (inst.i.funct3) {
    case 0b000: // LB
        slac_in(si, SLAC_OP_LD1S, SLAC_IN_ARG_DRI, PR_D);
        STRACE_OPSTR("lb");
        break;

    case 0b001: // LH
        slac_in(si, SLAC_OP_LD2S, SLAC_IN_ARG_DRI, PR_D);
        STRACE_OPSTR("lh");
        break;

    case 0b010: // LW
        slac_in(si, SLAC_OP_LD4S, SLAC_IN_ARG_DRI, PR_D);
        STRACE_OPSTR("lw");
        break;

    case 0b100: // LBU
        slac_in(si, SLAC_OP_LD1, SLAC_IN_ARG_DRI, PR_D);
        STRACE_OPSTR("lbu");
        break;

    case 0b101: // LHU
        slac_in(si, SLAC_OP_LD2, SLAC_IN_ARG_DRI, PR_D);
        STRACE_OPSTR("lhu");
        break;

    case 0b110: // LWU
        if (c->core.mode != SL_CORE_MODE_8)
            return rv_slac_undef(c, si);
        slac_in(si, SLAC_OP_LD4, SLAC_IN_ARG_DRI, PR_D);
        STRACE_OPSTR("lwu");
        break;

    case 0b011: // LD
        if (c->core.mode != SL_CORE_MODE_8)
            return rv_slac_undef(c, si);
        slac_in(si, SLAC_OP_LD8, SLAC_IN_ARG_DRI, PR_D);
        STRACE_OPSTR("ld");
        break;

    default:
        return rv_slac_undef(c, si);
    }

#if RV_PRETTY_PRINT
    STRACE(si, "%s x%u, %s0x%x(x%u)", opstr_, inst.i.rd, SIGNSTR(imm), ABS(imm), inst.i.rs1);
#else
    STRACE(si, "%s x%u, %d(x%u)", opstr_, inst.i.rd, imm, inst.i.rs1);
#endif
    return 0;
}

static int rv_decode_store(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    STRACE_DECL_OPSTR;
    si->simm = (((i4)inst.raw >> 20) & ~(0x1f)) | inst.s.imm1;
    si->r0 = inst.s.rs1;
    si->d0 = inst.s.rs2;
    switch (inst.s.funct3) {
    case 0b000: // SB
        slac_in(si, SLAC_OP_ST1, SLAC_IN_ARG_DRI, PR_ST1);
        STRACE_OPSTR("sb");
        break;
    case 0b001: // SH
        slac_in(si, SLAC_OP_ST2, SLAC_IN_ARG_DRI, PR_ST2);
        STRACE_OPSTR("sh");
        break;
    case 0b010: // SW
        slac_in(si, SLAC_OP_ST4, SLAC_IN_ARG_DRI, PR_ST4);
        STRACE_OPSTR("sw");
        break;
    case 0b011: // SD
        if (c->core.mode != SL_CORE_MODE_8) return rv_slac_undef(c, si); 
        slac_in(si, SLAC_OP_ST8, SLAC_IN_ARG_DRI, PR_ST8);
        STRACE_OPSTR("sd");
        break;
    default:
        return rv_slac_undef(c, si);
    }
#if RV_PRETTY_PRINT
    STRACE(si, "%s x%u, %s0x%x(x%u)", opstr_, inst.s.rs2, SIGNSTR((i4)si->simm), ABS((i4)si->simm), inst.s.rs1);
#else
    STRACE(si, "%s x%u, %d(x%u)", opstr_, inst.s.rs2, (i4)si->simm, inst.s.rs1);
#endif
    return 0;
}

static int decode_c_alu32(rv_core_t *c, sl_slac_inst_t *si, rv_cinst_t ci) {
    switch (ci.cba.funct2) {
    case 0b00:  // C.SRLI
        if (c->core.mode == SL_CORE_MODE_4) {
            if (ci.cba.imm1) return SL_ERR_UNDEF;
            si->uimm = ci.cba.imm0;
        } else {
            si->uimm = CBA_IMM(ci);
        }
        if (si->uimm == 0) return SL_ERR_UNDEF;
        si->d0 = RVC_TO_REG(ci.cba.rsd);
        si->r0 = si->d0;
        slac_in(si, SLAC_OP_SHR, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "c.srli x%u, %u", si->d0, (u4)si->uimm);
        return 0;

    case 0b01:  // C.SRAI
        if (c->core.mode == SL_CORE_MODE_4) {
            if (ci.cba.imm1) return SL_ERR_UNDEF;
            si->uimm = ci.cba.imm0;
        } else {
            si->uimm = CBA_IMM(ci);
        }
        if (si->uimm == 0) return SL_ERR_UNDEF;
        si->d0 = RVC_TO_REG(ci.cba.rsd);
        si->r0 = si->d0;
        slac_in(si, SLAC_OP_SHRS, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "c.srai x%u, %u", si->d0, (u4)si->uimm);
        return 0;

    case 0b10:  // C.ANDI
        si->simm = sign_extend32(CBA_IMM(ci), 6);
        si->d0 = RVC_TO_REG(ci.cba.rsd);
        si->r0 = si->d0;
        slac_in(si, SLAC_OP_AND, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "c.andi x%u, %#" PRIx64, si->d0, si->simm);
        return 0;

    default:
        break;
    }

    // case 0b11

    STRACE_DECL_OPSTR;
    si->r0 = RVC_TO_REG(ci.cs.rs1);
    si->r1 = RVC_TO_REG(ci.cs.rs2);
    si->d0 = si->r0;

    switch ((ci.cs.imm1 & 4) | ci.cs.imm0) {
    case 0b000: // C.SUB
        slac_in(si, SLAC_OP_SUB, SLAC_IN_ARG_DRR, PR_D);
        STRACE_OPSTR("c.sub");
        break;

    case 0b001: // C.XOR
        slac_in(si, SLAC_OP_XOR, SLAC_IN_ARG_DRR, PR_D);
        STRACE_OPSTR("c.xor");
        break;

    case 0b010: // C.OR
        slac_in(si, SLAC_OP_OR, SLAC_IN_ARG_DRR, PR_D);
        STRACE_OPSTR("c.or");
        break;

    case 0b011: // C.AND
        slac_in(si, SLAC_OP_AND, SLAC_IN_ARG_DRR, PR_D);
        STRACE_OPSTR("c.and");
        break;

    case 0b100: // C.SUBW
        if (c->core.mode != SL_CORE_MODE_8)
            return SL_ERR_UNDEF;
        slac_in(si, SLAC_OP_SUB, SLAC_IN_ARG_DRR, PR_D);
        STRACE_OPSTR("c.subw");
        si->len = SLAC_IN_LEN_4;
        si->sx8 = 1;
        break;

    case 0b101: // C.ADDW
        if (c->core.mode != SL_CORE_MODE_8)
            return SL_ERR_UNDEF;
        slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRR, PR_D);
        STRACE_OPSTR("c.addw");
        si->len = SLAC_IN_LEN_4;
        si->sx8 = 1;
        break;

    default:
        return SL_ERR_UNDEF;
    }

    if (si->d0 == 0)
        set_nop(si);
    STRACE(si, "%s x%u, x%u", opstr_, si->d0, si->r1);
    return 0;
}

static int rv_decode_c(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    if ((c->core.arch_options & SL_RISCV_EXT_C) == 0)
        goto undef;

    int err;
    rv_cinst_t ci;
    ci.raw = (u2)inst.raw;
    const u2 op = (ci.cj.opcode << 3) | ci.cj.funct3;
    switch (op) {
    case 0b00000: // C.ADDI4SPN
        if (ci.raw == 0) goto undef;
        si->uimm = (u4)CIW_IMM(ci);
        si->d0 = RVC_TO_REG(ci.ciw.rd);
        si->r0 = RV_SP;
        slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "c.addi4spn x%u, %u", si->d0, (u4)si->uimm);
        break;

    case 0b00001:
        // todo: implement me
        goto undef; // C.FLD

    case 0b00010: // C.LW
        si->uimm = CS_IMM_SCALED_4(ci);
        si->d0 = RVC_TO_REG(ci.cl.rd);
        si->r0 = RVC_TO_REG(ci.cl.rs);
        slac_in(si, SLAC_OP_LD4, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "c.lw x%u, %u(x%u)", si->d0, (u4)si->uimm, si->r0);
        break;

    case 0b00011:
        if (c->core.mode == SL_CORE_MODE_4) {
            // C.FLW
            if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
            const u4 imm = CI_IMM_SCALED_4(ci);
            si->uimm = imm;
            si->r0 = RVC_TO_REG(ci.cl.rs);
            si->d0 = RVC_TO_REG(ci.cl.rd);
            slac_in(si, SLAC_OP_FLD32, SLAC_IN_ARG_DRI, PR_DF32);
            STRACE_EXPAND(si, "flw f%u, %u(x%u)", si->d0, imm, si->r0);
            STRACE_C(si, "c.flw f%u, %u(x%u)", si->d0, imm, si->r0);
        } else {
            // C.LD
            const u4 imm = ((ci.cl.imm0 & 2) << 1) | ((ci.cl.imm1 ) << 3) | ((ci.cl.imm0 & 1) << 6);
            si->uimm = imm;
            si->r0 = RVC_TO_REG(ci.cl.rs);
            si->d0 = RVC_TO_REG(ci.cl.rd);
            slac_in(si, SLAC_OP_LD8, SLAC_IN_ARG_DRI, PR_D);
            STRACE_EXPAND(si, "ld x%u, %u(x%u)", si->d0, imm, si->r0);
            STRACE_C(si, "c.ld x%u, %u(x%u)", si->d0, imm, si->r0);
        }
        break;

    case 0b00100: goto undef;   // reserved

    case 0b00101:   // C.FSD
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        si->uimm = CS_IMM_SCALED_8(ci);
        si->d0 = RVC_TO_REG(ci.cs.rs2);
        si->r0 = RVC_TO_REG(ci.cs.rs1);
        slac_in(si, SLAC_OP_FST64, SLAC_IN_ARG_DRI, PR_STF64);
        STRACE(si, "c.fsd f%u, %u(x%u)" PRIx64, si->d0, (u4)si->uimm, si->r0);
        break;

    case 0b00110:   // C.SW
        si->uimm = CS_IMM_SCALED_4(ci);
        si->r0 = RVC_TO_REG(ci.cs.rs1);
        si->d0 = RVC_TO_REG(ci.cs.rs2);
        slac_in(si, SLAC_OP_ST4, SLAC_IN_ARG_DRI, PR_ST4);
        STRACE(si, "c.sw x%u, %u(x%u)", si->d0, (u4)si->uimm, si->r0);
        break;

    case 0b00111:
        if (c->core.mode == SL_CORE_MODE_4) {
            // C.FSW
            if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
            si->uimm = CS_IMM_SCALED_4(ci);
            si->d0 = RVC_TO_REG(ci.cs.rs2);
            si->r0 = RVC_TO_REG(ci.cs.rs1);
            slac_in(si, SLAC_OP_FST32, SLAC_IN_ARG_DRI, PR_STF32);
            STRACE(si, "c.fsw f%u, %u(x%u)" PRIx64, si->d0, (u4)si->uimm, si->r0);
        } else {
            // C.SD
            si->uimm = CS_IMM_SCALED_8(ci);
            si->d0 = RVC_TO_REG(ci.cs.rs2);
            si->r0 = RVC_TO_REG(ci.cs.rs1);
            slac_in(si, SLAC_OP_ST8, SLAC_IN_ARG_DRI, PR_ST8);
            STRACE(si, "c.sd x%u, %u(x%u)" PRIx64, si->d0, (u4)si->uimm, si->r0);
            break;
        }

    case 0b01000:
        if (inst.raw == 1) { // C.NOP
            slac_in(si, SLAC_OP_NOP, SLAC_IN_ARG_NONE, 0);
            // si->pred = SLAC_PRED_NEVER;
            STRACE(si, "c.nop");
            break;
        }
        // C.ADDI
        if (ci.ci.rsd == 0) goto undef;
        si->simm = sign_extend32(CI_IMM(ci), 6);
        si->r0 = ci.ci.rsd;
        si->d0 = ci.ci.rsd;
        slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "c.addi x%u, %d", ci.ci.rsd, (i4)si->simm);
        break;

    case 0b01001:
        if (c->core.mode == SL_CORE_MODE_4) {
            // C.JAL
            si->simm = sign_extend32(CJ_IMM(ci), 12);
            si->d0 = RV_RA;
            si->r2 = 2; // pc offset to step
            slac_in(si, SLAC_OP_BL, SLAC_IN_ARG_DI, PR_BL);
            STRACE(si, "c.jal %d", (i4)si->simm);   // todo: make this absolute address
        } else {
            // C.ADDIW
            if (ci.ci.rsd == 0) goto undef;
            si->simm = sign_extend32(CI_IMM(ci), 6);
            si->r0 = ci.ci.rsd;
            si->d0 = ci.ci.rsd;
            if (si->simm == 0) {
                slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRI, PR_D);
                STRACE(si, "c.sext.w x%u", ci.ci.rsd);
            } else {
                slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRI, PR_D);
                STRACE(si, "c.addiw x%u, %d", ci.ci.rsd, (i4)si->simm);
            }
            si->len = SLAC_IN_LEN_4;
            si->sx8 = 1;
        }
        break;

    case 0b01010:   // C.LI
        if (ci.ci.rsd == 0) goto undef;
        si->simm = sign_extend32(CI_IMM(ci), 6);
        si->d0 = ci.ci.rsd;
        slac_in(si, SLAC_OP_MOVI, SLAC_IN_ARG_DI, PR_D);
        STRACE(si, "c.li x%u, %d", ci.ci.rsd, (i4)si->simm);
        break;

    case 0b01011:
        if (ci.ci.rsd == 0) goto undef;
        if (ci.ci.rsd == RV_SP) {   // C.ADDI16SP
            si->simm = sign_extend32((CI_ADDI16SP_IMM(ci)), 10);
            si->r0 = RV_SP;
            si->d0 = RV_SP;
            slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRI, PR_D);
            STRACE(si, "c.addi16sp %d", (i4)si->simm);
        } else {    // C.LUI
            si->simm = sign_extend32((CI_IMM(ci) << 12), 18);
            if (si->simm == 0) goto undef;
            si->d0 = ci.ci.rsd;
            slac_in(si, SLAC_OP_MOVI, SLAC_IN_ARG_DI, PR_D);
            STRACE(si, "c.lui x%u, %#x", ci.ci.rsd, (u4)si->simm);
        }
        break;

    case 0b01100:
        err = decode_c_alu32(c, si, ci);
        if (err) {
            if (err == SL_ERR_UNDEF)
                goto undef;
            return err;
        }
        break;

    case 0b01101:   // C.J
        si->simm = sign_extend32(CJ_IMM(ci), 12);
        slac_in(si, SLAC_OP_B, SLAC_IN_ARG_I, PR_B);
        STRACE(si, "c.j %#" PRIx64, c->core.pc + si->simm);
        break;

    case 0b01110:   // C.BEQZ
        si->simm = sign_extend32(CB_IMM(ci), 9);
        si->r0 = RVC_TO_REG(ci.cb.rs);
        si->r1 = RV_ZERO;
        slac_in(si, SLAC_OP_CBEQ, SLAC_IN_ARG_RRI, PR_B);
        STRACE(si, "c.beqz x%u, %#" PRIx64, si->r0, si->simm + c->core.pc);
        break;

    case 0b01111:   // C.BNEZ
        si->simm = sign_extend32(CB_IMM(ci), 9);
        si->r0 = RVC_TO_REG(ci.cb.rs);
        si->r1 = RV_ZERO;
        slac_in(si, SLAC_OP_CBNE, SLAC_IN_ARG_RRI, PR_B);
        STRACE(si, "c.bnez x%u, %#" PRIx64, si->r0, si->simm + c->core.pc);
        break;

    case 0b10000:   // C.SLLI
        if (ci.ci.rsd == RV_ZERO) goto undef;
        if (c->core.mode == SL_CORE_MODE_4) {
            if (ci.ci.imm1) goto undef;
            si->uimm = ci.ci.imm0;
        } else {
            si->uimm = CI_IMM(ci);
        }
        if (si->uimm == 0) goto undef;
        si->r0 = ci.ci.rsd;
        si->d0 = ci.ci.rsd;
        slac_in(si, SLAC_OP_SHL, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "c.slli x%u, %u", ci.ci.rsd, (u4)si->uimm);
        break;

    case 0b10001:   // C.FLDSP
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        si->uimm = CI_IMM_SCALED_8(ci);
        si->d0 = ci.ci.rsd;
        si->r0 = RV_SP;
        slac_in(si, SLAC_OP_FLD64, SLAC_IN_ARG_DRI, PR_DF64);
        STRACE_C(si, "c.fldsp f%u, %u", si->d0, (u4)si->uimm);
        STRACE_EXPAND(si, "fld f%u, %u(sp)", si->d0, (u4)si->uimm);
        break;

    case 0b10010:   // C.LWSP
        if (ci.ci4.rd == RV_ZERO)
            goto undef;
        si->uimm = CI_IMM_SCALED_4(ci);
        si->d0 = ci.ci.rsd;
        si->r0 = RV_SP;
        slac_in(si, SLAC_OP_LD4, SLAC_IN_ARG_DRI, PR_D);
        STRACE(si, "c.lwsp x%u, %u", ci.ci.rsd, (u4)si->uimm);
        break;

    case 0b10011:
        if (c->core.mode == SL_CORE_MODE_4) {
            // C.FLWSP
            if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
            const u4 imm = CI_IMM_SCALED_4(ci);
            si->uimm = imm;
            si->d0 = ci.ci.rsd;
            si->r0 = RV_SP;
            slac_in(si, SLAC_OP_FLD32, SLAC_IN_ARG_DRI, PR_DF32);
            STRACE_EXPAND(si, "flw f%u, %u(sp)", si->d0, imm);
            STRACE_C(si, "c.flwsp f%u, %u", si->d0, imm);
        } else {
            if (ci.ci.rsd == RV_ZERO) goto undef;
            // C.LDSP
            si->uimm = CI_IMM_SCALED_8(ci);
            si->d0 = ci.ci.rsd;
            si->r0 = RV_SP;
            slac_in(si, SLAC_OP_LD8, SLAC_IN_ARG_DRI, PR_D);
            STRACE(si, "c.ldsp x%u, %u", ci.ci.rsd, (u4)si->uimm);
        }
        break;

    case 0b10100:   // C.JR C.MV C.EBREAK C.JALR C.ADD
        if (ci.cr.funct4 == 0) {
            if (ci.cr.rsd == 0)
                goto undef;
            if (ci.cr.rs2 == 0) {
                // C.JR
                si->d0 = ci.cr.rsd;
                si->r0 = ci.cr.rsd;
                slac_in(si, SLAC_OP_B, SLAC_IN_ARG_R1, PR_B);
                STRACE(si, "c.jr x%u", ci.ci.rsd);
            } else {
                // C.MV
                si->r0 = ci.cr.rs2;
                si->d0 = ci.cr.rsd;
                slac_in(si, SLAC_OP_MOVR, SLAC_IN_ARG_DR, PR_D);
                STRACE(si, "c.mv x%u, x%u", ci.ci.rsd, ci.cr.rs2);
                if ((si->d0 == RV_ZERO) || (si->d0 == si->r0))
                    set_nop(si);
            }
        } else {
            if (ci.cr.rs2 == 0) {
                if (ci.cr.rsd == 0) {
                    // C.EBREAK
                    slac_in(si, SLAC_OP_BREAK, 0, 0);
                    STRACE(si, "c.ebreak");
                } else {
                    // C.JALR
                    si->r0 = ci.cr.rsd;
                    si->d0 = ci.cr.rsd;
                    slac_in(si, SLAC_OP_BL, SLAC_IN_ARG_DR, PR_BL);
                    STRACE(si, "c.jalr x%u", ci.ci.rsd);
                }
            } else {
                if (ci.cr.rsd == 0)
                    goto undef;
                // C.ADD
                si->d0 = ci.cr.rsd;
                // note that this flips r0 and r1 for the convenience of the print format
                si->r0 = ci.cr.rs2;
                si->r1 = ci.cr.rsd;
                slac_in(si, SLAC_OP_ADD, SLAC_IN_ARG_DRR, PR_D);
                STRACE(si, "c.add x%u, x%u", ci.ci.rsd, ci.cr.rs2);
            }
        }
        break;

    case 0b10101:   // C.FSDSP
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        si->uimm = CSS_IMM_SCALED_8(ci);;
        si->d0 = ci.css.rs2;    // d0 is the data source register
        si->r0 = RV_SP;         // r0 is the address register
        slac_in(si, SLAC_OP_FST64, SLAC_IN_ARG_DRI, PR_STF64);
        STRACE_EXPAND(si, "fsd f%u, %u(sp)", si->d0, imm);
        STRACE_C(si, "c.fsdsp f%u, %u", si->d0, (u4)si->uimm);
        break;

    case 0b10110:   // C.SWSP
        si->uimm = CSS_IMM_SCALED_4(ci);
        si->d0 = ci.css.rs2;
        si->r0 = RV_SP;
        slac_in(si, SLAC_OP_ST4, SLAC_IN_ARG_DRI, PR_ST4);
        STRACE(si, "c.swsp x%u, %u", ci.css.rs2, (u4)si->uimm);
        break;

    case 0b10111:
        if (c->core.mode == SL_CORE_MODE_4) {
            // C.FSWSP
            if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
            const u4 imm = CSS_IMM_SCALED_4(ci);
            si->uimm = imm;
            si->d0 = ci.css.rs2;    // d0 is the data source register
            si->r0 = RV_SP;         // r0 is the address register
            slac_in(si, SLAC_OP_FST32, SLAC_IN_ARG_DRI, PR_STF32);
            STRACE_EXPAND(si, "fsw f%u, %u(sp)", si->d0, imm);
            STRACE_C(si, "c.fswsp f%u, %u", si->d0, imm);
            break;
        } else {
            // C.SDSP
            si->uimm = CSS_IMM_SCALED_8(ci);
            si->d0 = ci.css.rs2;
            si->r0 = RV_SP;
            slac_in(si, SLAC_OP_ST8, SLAC_IN_ARG_DRI, PR_ST8);
            STRACE(si, "c.sdsp x%u, %u", ci.css.rs2, (u4)si->uimm);
            break;
        }

    default:
        goto undef;
    }

    si->sh = 1;
    return 0;

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_fp_load(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    const i4 imm = ((i4)inst.raw) >> 20;
    si->d0 = inst.i.rd;
    si->r0 = inst.i.rs1;
    si->simm = imm;

    // imm[11:0] rs1 010 rd 0000111 FLW
    // imm[11:0] rs1 011 rd 0000111 FLD
    // imm[11:0] rs1 100 rd 0000111 FLQ
    // imm[11:0] rs1 001 rd 0000111 FLH
    switch (inst.r.funct3) {
    case 0b010:
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        slac_in(si, SLAC_OP_FLD32, SLAC_IN_ARG_DRI, PR_DF32);
        STRACE(si, "flw f%u, %d(x%u)", inst.i.rd, imm, inst.i.rs1);
        break;

    case 0b011:
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        slac_in(si, SLAC_OP_FLD64, SLAC_IN_ARG_DRI, PR_DF64);
        STRACE(si, "fld f%u, %d(x%u)", inst.i.rd, imm, inst.i.rs1);
        break;

    case 0b100:
    case 0b001:
    default:    goto undef;
    }

    return 0;

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_fp_store(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    const i4 imm = (((i4)inst.raw >> 20) & ~(0x1f)) | inst.s.imm1;
    si->d0 = inst.s.rs2;    // d0 is the data source register
    si->r0 = inst.i.rs1;    // r0 is the address register
    si->simm = imm;

    //imm[11:5] rs2 rs1 010 imm[4:0] 0100111 FSW
    //imm[11:5] rs2 rs1 011 imm[4:0] 0100111 FSD
    switch (inst.s.funct3) {
    case 0b010:
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        slac_in(si, SLAC_OP_FST32, SLAC_IN_ARG_DRI, PR_STF32);
        STRACE(si, "fsw f%u, %d(x%u)", si->d0, imm, si->r0);
        break;

    case 0b011:
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        slac_in(si, SLAC_OP_FST64, SLAC_IN_ARG_DRI, PR_STF64);
        STRACE(si, "fsd f%u, %d(x%u)", si->d0, imm, si->r0);
        break;

    default:    goto undef;
    }
    return 0;

undef:
    return rv_slac_undef(c, si);

}

static int rv_decode_fp32(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    si->d0 = inst.r.rd;
    si->r0 = inst.r.rs1;

    switch (inst.r.funct7 >> 2) {
    // 0000000 rs2     rs1  rm   rd  1010011  FADD.S
    case 0b00000:
        slac_in(si, SLAC_OP_FADD32, SLAC_IN_ARG_DRR, PR_DF32);
        si->r1 = inst.r.rs2;
        STRACE(si, "fadd.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
        break;

    // 0000100 rs2     rs1  rm   rd  1010011  FSUB.S
    case 0b00001:
        slac_in(si, SLAC_OP_FSUB32, SLAC_IN_ARG_DRR, PR_DF32);
        si->r1 = inst.r.rs2;
        STRACE(si, "fsub.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
        break;

    // 0001000 rs2     rs1  rm   rd  1010011  FMUL.S
    case 0b00010:
        slac_in(si, SLAC_OP_FMUL32, SLAC_IN_ARG_DRR, PR_DF32);
        si->r1 = inst.r.rs2;
        STRACE(si, "fmul.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
        break;

    // 0001100 rs2     rs1  rm   rd  1010011  FDIV.S
    case 0b00011:
        slac_in(si, SLAC_OP_FDIV32, SLAC_IN_ARG_DRR, PR_DF32);
        si->r1 = inst.r.rs2;
        STRACE(si, "fdiv.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
        break;

    // 01011 size 00000   rs1  rm   rd  1010011  FSQRT.S
    case 0b01011:
        slac_in(si, SLAC_OP_FSQRT32, SLAC_IN_ARG_DR, PR_DF32);
        STRACE(si, "fsqrt.s f%u, f%u", si->d0, si->r0);
        break;

    // 00100 size rs2     rs1  000  rd  1010011  FSGNJ.S
    // 00100 size rs2     rs1  001  rd  1010011  FSGNJN.S
    // 00100 size rs2     rs1  010  rd  1010011  FSGNJX.S
    case 0b00100:
        si->r1 = inst.r.rs2;
        switch (inst.r.funct3) {
        case 0b000:
            slac_in(si, SLAC_OP_FS32, SLAC_IN_ARG_DRR, PR_DF32);
            STRACE(si, "fsgnj.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        case 0b001:
            slac_in(si, SLAC_OP_FSN32, SLAC_IN_ARG_DRR, PR_DF32);
            STRACE(si, "fsgnjn.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        case 0b010:
            slac_in(si, SLAC_OP_FSX32, SLAC_IN_ARG_DRR, PR_DF32);
            STRACE(si, "fsgnjx.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        default:    goto undef;
        }
        break;

    // 00101 size rs2     rs1  000  rd  1010011  FMIN.S
    // 00101 size rs2     rs1  001  rd  1010011  FMAX.S
    case 0b00101:
        switch (inst.r.funct3) {
        case 0b000:
            slac_in(si, SLAC_OP_FMIN32, SLAC_IN_ARG_DRR, PR_DF32);
            si->r1 = inst.r.rs2;
            STRACE(si, "fmin.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        case 0b001:
            slac_in(si, SLAC_OP_FMAX32, SLAC_IN_ARG_DRR, PR_DF32);
            si->r1 = inst.r.rs2;
            STRACE(si, "fmax.s f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        default:    goto undef;
        }
        break;

    case 0b11000:
        // todo: set correct rounding mode
        switch (inst.r.funct3) {
        // 11000 size 00000   rs1  rm   rd  1010011  FCVT.W.S/D
        case 0b00000:
            slac_in(si, SLAC_OP_FCVT_S4_TO_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fcvt.w.s f%u, x%u", si->d0, si->r0);
            // todo: replace with fmov immediate if r0 = zero
            break;

        // 11000 size 00001   rs1  rm   rd  1010011  FCVT.WU.S
        case 0b00001:
            slac_in(si, SLAC_OP_FCVT_U4_TO_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fcvt.wu.s f%u, x%u", si->d0, si->r0);
            break;

        // 11000 size 00010   rs1  rm   rd  1010011  FCVT.L.S/D
        case 0b00010:
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_FCVT_S8_TO_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fcvt.l.s f%u, x%u", si->d0, si->r0);
            break;

        // 11000 size 00011   rs1  rm   rd  1010011  FCVT.LU.S/D
        case 0b11000:
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_FCVT_U8_TO_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fcvt.lu.s f%u, x%u", si->d0, si->r0);
            break;

        default:    goto undef;
        }
        break;

    // 10100 size rs2     rs1  010  rd  1010011  FEQ.S
    // 10100 size rs2     rs1  001  rd  1010011  FLT.S
    // 10100 size rs2     rs1  000  rd  1010011  FLE.S
    case 0b10100:
        switch (inst.r.funct3) {
        case 0b010:
            slac_in(si, SLAC_OP_FEQ32, SLAC_IN_ARG_DRR, PR_DF32);
            si->r1 = inst.r.rs2;
            STRACE(si, "feq.s x%u, f%u, f%u", si->d0, si->r0, si->r1);
            if (si->d0 == RV_ZERO)
                si->d0 = SLAC_REG_DISCARD;
            break;

        case 0b001:
            slac_in(si, SLAC_OP_FLT32, SLAC_IN_ARG_DRR, PR_D);
            si->r1 = inst.r.rs2;
            STRACE(si, "flt.s x%u, f%u, f%u", si->d0, si->r0, si->r1);
            if (si->d0 == RV_ZERO)
                si->d0 = SLAC_REG_DISCARD;
            break;

        case 0b000:
            slac_in(si, SLAC_OP_FLE32, SLAC_IN_ARG_DRR, PR_DF32);
            si->r1 = inst.r.rs2;
            STRACE(si, "fle.s x%u, f%u, f%u", si->d0, si->r0, si->r1);
            if (si->d0 == RV_ZERO)
                si->d0 = SLAC_REG_DISCARD;
            break;

        default:    goto undef;
        }
        break;

    case 0b11100:
        switch (inst.r.funct3) {
        case 0b000:
            // 11100 size 00000   rs1  000  rd  1010011  FMV.X.W
            // move to register from float
            slac_in(si, SLAC_OP_MOV_RF32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fmv.x.w x%u, f%u", si->d0, si->r0);
            if (si->d0 == RV_ZERO)
                set_nop(si);
            break;

        // 11100 size 00000   rs1  001  rd  1010011  FCLASS.S/D
        case 0b001: {
            if (si->d0 == RV_ZERO) {
                set_nop(si);
                break;
            }
            slac_in(si, SLAC_OP_FCLASS_F32, SLAC_IN_ARG_DR, PR_D);
            STRACE(si, "fclass.s x%u, f%u", si->d0, si->r0);
            break;
        }
        default:    goto undef;
        }
        break;

    case 0b11010:
        switch (inst.r.rs2) {
        // 11010 00 00000   rs1  rm   rd  1010011  FCVT.S.W
        // 11010 01 00000   rs1  rm   rd  1010011  FCVT.D.W
        case 0b00000:
            slac_in(si, SLAC_OP_FCVT_F32_TO_S4, SLAC_IN_ARG_DR, PR_D);
            STRACE(si, "fcvt.s.w f%u, x%u", si->d0, si->r0);
            break;

        // 11010 size 00001   rs1  rm   rd  1010011  FCVT.S.WU
        // 11010 size 00001   rs1  rm   rd  1010011  FCVT.D.WU
        case 0b00001:
            slac_in(si, SLAC_OP_FCVT_U4_TO_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fcvt.s.wu f%u, x%u", si->d0, si->r0);
            break;

        // 11010 size 00010   rs1  rm   rd  1010011  FCVT.S.L
        case 0b00010:
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_FCVT_S8_TO_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fcvt.s.l f%u, x%u", si->d0, si->r0);
            break;

        // 11010 size 00011   rs1  rm   rd  1010011  FCVT.S.LU
        case 0b00011:
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_FCVT_U8_TO_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fcvt.s.lu f%u, x%u", si->d0, si->r0);
            break;

        default:
            goto undef;
        }
        break;


    case 0b01000:
        // the encoding for this instruction is an idiotic hack
        // if fmt = 0 (normally implies fp32) and rs2 = 1 then this is FCVT.S.D
        // this instruction requires both F and D extensions
        // see FCVT.D.S for the inverse of this.
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        // 01000 00 00001 rs1 rm rd 1010011 FCVT.S.D
        if (inst.r.rs2 != 1) goto undef;
        slac_in(si, SLAC_OP_FCVT_F64_TO_F32, SLAC_IN_ARG_DRR, PR_DF32);
        STRACE(si, "fcvt.s.d f%u, f%u", si->d0, si->r0);
        break;

    case 0b11110:
        if (inst.r.funct3 != 000) goto undef;
        // 11110 size 00000   rs1  000  rd  1010011  FMV.W.X
        slac_in(si, SLAC_OP_MOV_FR32, SLAC_IN_ARG_DR, PR_DF32);
        STRACE(si, "fmv.w.x f%u, x%u", si->d0, si->r0);
        break;

    default:    goto undef;
    }

    return 0;

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_fp64(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    si->d0 = inst.r.rd;
    si->r0 = inst.r.rs1;

    switch (inst.r.funct7 >> 2) {
    // 0000000 rs2     rs1  rm   rd  1010011  FADD.S
    case 0b00000:
        slac_in(si, SLAC_OP_FADD64, SLAC_IN_ARG_DRR, PR_DF64);
        si->r1 = inst.r.rs2;
        STRACE(si, "fadd.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
        break;

    // 0000100 rs2     rs1  rm   rd  1010011  FSUB.S
    case 0b00001:
        slac_in(si, SLAC_OP_FSUB64, SLAC_IN_ARG_DRR, PR_DF64);
        si->r1 = inst.r.rs2;
        STRACE(si, "fsub.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
        break;

    // 0001000 rs2     rs1  rm   rd  1010011  FMUL.S
    case 0b00010:
        slac_in(si, SLAC_OP_FMUL64, SLAC_IN_ARG_DRR, PR_DF64);
        si->r1 = inst.r.rs2;
        STRACE(si, "fmul.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
        break;

    // 0001100 rs2     rs1  rm   rd  1010011  FDIV.S
    case 0b00011:
        slac_in(si, SLAC_OP_FDIV64, SLAC_IN_ARG_DRR, PR_DF64);
        si->r1 = inst.r.rs2;
        STRACE(si, "fdiv.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
        break;

    // 01011 size 00000   rs1  rm   rd  1010011  FSQRT.S
    case 0b01011:
        slac_in(si, SLAC_OP_FSQRT64, SLAC_IN_ARG_DR, PR_DF64);
        STRACE(si, "fsqrt.d f%u, f%u", si->d0, si->r0);
        break;

    // 00100 size rs2     rs1  000  rd  1010011  FSGNJ.S
    // 00100 size rs2     rs1  001  rd  1010011  FSGNJN.S
    // 00100 size rs2     rs1  010  rd  1010011  FSGNJX.S
    case 0b00100:
        si->r1 = inst.r.rs2;
        switch (inst.r.funct3) {
        case 0b000:
            slac_in(si, SLAC_OP_FS64, SLAC_IN_ARG_DRR, PR_DF64);
            STRACE(si, "fsgnj.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        case 0b001:
            slac_in(si, SLAC_OP_FSN64, SLAC_IN_ARG_DRR, PR_DF64);
            STRACE(si, "fsgnjn.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        case 0b010:
            slac_in(si, SLAC_OP_FSX64, SLAC_IN_ARG_DRR, PR_DF64);
            STRACE(si, "fsgnjx.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        default:    goto undef;
        }
        break;

    // 00101 size rs2     rs1  000  rd  1010011  FMIN.S
    // 00101 size rs2     rs1  001  rd  1010011  FMAX.S
    case 0b00101:
        switch (inst.r.funct3) {
        case 0b000:
            slac_in(si, SLAC_OP_FMIN64, SLAC_IN_ARG_DRR, PR_DF64);
            si->r1 = inst.r.rs2;
            STRACE(si, "fmin.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        case 0b001:
            slac_in(si, SLAC_OP_FMAX64, SLAC_IN_ARG_DRR, PR_DF64);
            si->r1 = inst.r.rs2;
            STRACE(si, "fmax.d f%u, f%u, f%u", si->d0, si->r0, si->r1);
            break;

        default:    goto undef;
        }
        break;

    case 0b11000:
        // todo: set correct rounding mode
        switch (inst.r.funct3) {
        // 11000 size 00000   rs1  rm   rd  1010011  FCVT.W.S/D
        case 0b00000:
            slac_in(si, SLAC_OP_FCVT_S4_TO_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fcvt.w.d f%u, x%u", si->d0, si->r0);
            // todo: replace with fmov immediate if r0 = zero
            break;

        // 11000 size 00001   rs1  rm   rd  1010011  FCVT.WU.S
        case 0b00001:
            slac_in(si, SLAC_OP_FCVT_U4_TO_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fcvt.wu.d f%u, x%u", si->d0, si->r0);
            break;

        // 11000 size 00010   rs1  rm   rd  1010011  FCVT.L.S/D
        case 0b00010:
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_FCVT_S8_TO_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fcvt.l.d f%u, x%u", si->d0, si->r0);
            break;

        // 11000 size 00011   rs1  rm   rd  1010011  FCVT.LU.S/D
        case 0b11000:
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_FCVT_U8_TO_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fcvt.lu.d f%u, x%u", si->d0, si->r0);
            break;

        default:    goto undef;
        }
        break;

    // 10100 size rs2     rs1  010  rd  1010011  FEQ.S
    // 10100 size rs2     rs1  001  rd  1010011  FLT.S
    // 10100 size rs2     rs1  000  rd  1010011  FLE.S
    case 0b10100:
        switch (inst.r.funct3) {
        case 0b010:
            slac_in(si, SLAC_OP_FEQ64, SLAC_IN_ARG_DRR, PR_DF64);
            si->r1 = inst.r.rs2;
            STRACE(si, "feq.d x%u, f%u, f%u", si->d0, si->r0, si->r1);
            if (si->d0 == RV_ZERO)
                si->d0 = SLAC_REG_DISCARD;
            break;

        case 0b001:
            slac_in(si, SLAC_OP_FLT64, SLAC_IN_ARG_DRR, PR_D);
            si->r1 = inst.r.rs2;
            STRACE(si, "flt.d x%u, f%u, f%u", si->d0, si->r0, si->r1);
            if (si->d0 == RV_ZERO)
                si->d0 = SLAC_REG_DISCARD;
            break;

        case 0b000:
            slac_in(si, SLAC_OP_FLE64, SLAC_IN_ARG_DRR, PR_DF64);
            si->r1 = inst.r.rs2;
            STRACE(si, "fle.d x%u, f%u, f%u", si->d0, si->r0, si->r1);
            if (si->d0 == RV_ZERO)
                si->d0 = SLAC_REG_DISCARD;
            break;

        default:    goto undef;
        }
        break;

    case 0b11100:
        switch (inst.r.funct3) {
        case 0b000:
            // 11100 size 00000   rs1  000  rd  1010011  FMV.X.D
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_MOV_RF64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fmv.x.d x%u, f%u", si->d0, si->r0);
            if (si->d0 == RV_ZERO)
                set_nop(si);
            break;

        // 11100 size 00000   rs1  001  rd  1010011  FCLASS.S/D
        case 0b001: {
            if (si->d0 == RV_ZERO) {
                set_nop(si);
                break;
            }
            slac_in(si, SLAC_OP_FCLASS_F64, SLAC_IN_ARG_DR, PR_D);
            STRACE(si, "fclass.d x%u, f%u", si->d0, si->r0);
            break;
        }
        default:    goto undef;
        }
        break;

    case 0b11010:
        switch (inst.r.rs2) {
        // 11010 00 00000   rs1  rm   rd  1010011  FCVT.S.W
        // 11010 01 00000   rs1  rm   rd  1010011  FCVT.D.W
        case 0b00000:
            slac_in(si, SLAC_OP_FCVT_F64_TO_S4, SLAC_IN_ARG_DR, PR_D);
            STRACE(si, "fcvt.d.w f%u, x%u", si->d0, si->r0);
            break;

        // 11010 size 00001   rs1  rm   rd  1010011  FCVT.S.WU
        // 11010 size 00001   rs1  rm   rd  1010011  FCVT.D.WU
        case 0b00001:
            slac_in(si, SLAC_OP_FCVT_U4_TO_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fcvt.d.wu f%u, x%u", si->d0, si->r0);
            break;

        // 11010 size 00010   rs1  rm   rd  1010011  FCVT.S.L
        case 0b00010:
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_FCVT_S8_TO_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fcvt.d.l f%u, x%u", si->d0, si->r0);
            break;

        // 11010 size 00011   rs1  rm   rd  1010011  FCVT.S.LU
        case 0b00011:
            if (c->core.mode != SL_CORE_MODE_8) goto undef;
            slac_in(si, SLAC_OP_FCVT_U8_TO_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fcvt.d.lu f%u, x%u", si->d0, si->r0);
            break;

        default:    goto undef;
        }
        break;

    case 0b01000:
        // the encoding for this instruction is an idiotic hack
        // if fmt = 1 (normally implies fp64) and rs2 = 0 then this is FCVT.D.S
        // this instruction requires both F and D extensions
        // see FCVT.S.D for the inverse of this.
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        // 01000 01 00000 rs1 rm rd 1010011 FCVT.D.S
        if (inst.r.rs2 != 0) goto undef;
        slac_in(si, SLAC_OP_FCVT_F32_TO_F64, SLAC_IN_ARG_DR, PR_DF64);
        STRACE(si, "fcvt.d.s f%u, f%u", si->d0, si->r0);
        break;

    case 0b11110:
        // 11110 size 00000   rs1  000  rd  1010011  FMV.D.X
        if (c->core.mode != SL_CORE_MODE_8) goto undef;
        if (inst.r.funct3 != 000) goto undef;
        slac_in(si, SLAC_OP_MOV_FR64, SLAC_IN_ARG_DR, PR_DF64);
        STRACE(si, "fmv.d.x f%u, x%u", si->d0, si->r0);
        break;

    default:    goto undef;
    }

    return 0;

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_fp(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    const u1 fmt = inst.r.funct7 & 3;

    switch(fmt) {
    case 0b00:  // 32-bit single-precision
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        return rv_decode_fp32(c, si, inst);

    case 0b01:
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        return rv_decode_fp64(c, si, inst);

    case 0b10: // 16-bit half-precision
    case 0b11: // 128-bit quad-precision
    default:
        goto undef;
    }

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_fp_mac(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    // fexcept_t flags;

    si->d0 = inst.r4.rd;
    si->r0 = inst.r4.rs1;
    si->r1 = inst.r4.rs2;
    si->r2 = inst.r4.funct5;

    if (inst.r4.fmt == 0b10) {
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        switch (inst.r4.opcode) {
        // rs3 00 rs2 rs1 rm rd 1000011 FMADD.S
        case 0b1000011:
            slac_in(si, SLAC_OP_FMADD_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fmadd.s f%u, f%u, f%u, f%u", si->d0, si->r0, si->r1, si->r2);
            break;

        // rs3 00 rs2 rs1 rm rd 1000111 FMSUB.S
        case 0b1000111:
            slac_in(si, SLAC_OP_FMSUB_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fmsub.s f%u, f%u, f%u, f%u", si->d0, si->r0, si->r1, si->r2);
            break;

        // rs3 00 rs2 rs1 rm rd 1001011 FNMSUB.S
        case 0b1001011:
            slac_in(si, SLAC_OP_FNMADD_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fnmsub.s f%u, f%u, f%u, f%u", si->d0, si->r0, si->r1, si->r2);
            break;

        // rs3 00 rs2 rs1 rm rd 1001111 FNMADD.S
        case 0b1001111:
            slac_in(si, SLAC_OP_FNMSUB_F32, SLAC_IN_ARG_DR, PR_DF32);
            STRACE(si, "fnmadd.s f%u, f%u, f%u, f%u", si->d0, si->r0, si->r1, si->r2);
            break;

        default:    goto undef;
        }
        return 0;
    } else if (inst.r4.fmt == 0b01) {
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        switch (inst.r4.opcode) {
        // rs3 00 rs2 rs1 rm rd 1000011 FMADD.D
        case 0b1000011:
            slac_in(si, SLAC_OP_FMADD_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fmadd.d f%u, f%u, f%u, f%u", si->d0, si->r0, si->r1, si->r2);
            break;

        // rs3 00 rs2 rs1 rm rd 1000111 FMSUB.D
        case 0b1000111:
            slac_in(si, SLAC_OP_FMSUB_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fmsub.d f%u, f%u, f%u, f%u", si->d0, si->r0, si->r1, si->r2);
            break;

        // rs3 00 rs2 rs1 rm rd 1001011 FNMSUB.D
        case 0b1001011:
            slac_in(si, SLAC_OP_FNMADD_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fnmsub.d f%u, f%u, f%u, f%u", si->d0, si->r0, si->r1, si->r2);
            break;

        // rs3 00 rs2 rs1 rm rd 1001111 FNMADD.D
        case 0b1001111:
            slac_in(si, SLAC_OP_FNMSUB_F64, SLAC_IN_ARG_DR, PR_DF64);
            STRACE(si, "fnmadd.d f%u, f%u, f%u, f%u", si->d0, si->r0, si->r1, si->r2);
            break;

        default:    goto undef;
        }
        return 0;
    }

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_atomic(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    if ((c->core.arch_options & SL_RISCV_EXT_A) == 0) goto undef;

    const u1 op = inst.r.funct7 >> 2;
    const u1 barrier = inst.r.funct7 & 3;   // 1: acquire, 0: release

    if (inst.r.funct3 == 0b010) {
        switch (op) {
        case 0b00010: { // LR.W
            if (inst.r.rs2 != 0) goto undef;
            slac_in(si, SLAC_OP_LX, SLAC_IN_ARG_DRI, PR_D);
            STRACE(si, "lr.w %s", rv_barrier_string[barrier]);
            si->len = SLAC_IN_LEN_4;
            si->uimm = rv_barrier_map[barrier];
            si->r0 = inst.r.rs1;
            if (inst.r.rd == RV_ZERO) si->d0 = SLAC_REG_DISCARD;
            else si->d0 = inst.r.rd;
            return 0;
        }

        default:
            return SL_ERR_SLAC_UNDECODED;
        }
    }

    return SL_ERR_SLAC_UNDECODED;

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_sync(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    if ((inst.i.rd != 0) || (inst.i.rs1 != 0)) goto undef;
    switch (inst.i.funct3) {
    case 0b000:
    { // FENCE
        const u4 succ = inst.i.imm & 0xf;
        const u4 pred = (inst.i.imm >> 4) & 0xf;
        u4 bar = 0;
        if (pred & (FENCE_W | FENCE_O)) bar |= BARRIER_STORE;
        if (succ & (FENCE_R | FENCE_I)) bar |= BARRIER_LOAD;
        if ((pred & (FENCE_I | FENCE_O)) || (succ & (FENCE_I | FENCE_O))) bar |= BARRIER_SYSTEM;
        slac_in(si, SLAC_OP_MBAR, SLAC_IN_ARG_I, 0);
#if SLAC_TRACE
        char p[5], s[5];
        rv_fence_op_name(pred, p);
        rv_fence_op_name(succ, s);
        STRACE(si, "fence %s, %s", p, s);
#endif
        si->uimm = bar;
        si->r0 = pred;
        si->r1 = succ;
        return 0;
    }

    case 0b001:
        if (inst.i.imm != 0) goto undef;
        slac_in(si, SLAC_OP_IBAR, SLAC_IN_ARG_NONE, 0);
        STRACE(si, "fence.i");
        return 0;

    default:
        goto undef;
    }

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_sys(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    if (inst.r.funct3 == 0b000) {
/*
        if (inst.r.rd != 0) goto undef;
        switch (inst.r.funct7) {
            case 0b0000000:
            if (inst.r.rs1 == 0) {
                if (inst.r.rs2 == 0) {  // ECALL
                    // RV_TRACE_PRINT(c, "ecall");
                    return sl_core_synchronous_exception(&c->core, EX_SYSCALL, inst.raw, 0);
                } else if (inst.r.rs2 == 1) {   // EBREAK
                    // RV_TRACE_PRINT(c, "ebreak");
                    return rv_exec_ebreak(c);
                }
            }
            goto undef;

        case 0b0011000: // MRET
            // RV_TRACE_PRINT(c, "mret");
            if (c->core.el != SL_CORE_EL_MONITOR) goto undef;
            err = rv_exception_return(c, RV_OP_MRET);
            if (!err) // RV_TRACE_RD(c, SL_CORE_REG_PC, c->core.pc);
            return err;

        case 0b0001000:
            if (inst.r.rs2 == 0b00010) {
                // RV_TRACE_PRINT(c, "sret");
                if (c->core.el < SL_CORE_EL_SUPERVISOR) goto undef;
                err = rv_exception_return(c, RV_OP_SRET);
                if (!err) // RV_TRACE_RD(c, SL_CORE_REG_PC, c->core.pc);
                return err;
            }
            if (inst.r.rs2 == 0b00101) { // WFI
                if (c->core.el == SL_CORE_EL_USER) goto undef;
                // RV_TRACE_PRINT(c, "wfi");
                return sl_engine_wait_for_interrupt(&c->core.engine);
            }
            goto undef;

        case 0b0001001: // SFENCE.VMA
        case 0b0001011: // SINVAL.VMA
        case 0b0001100: // SFENCE.W.INVAL SFENCE.INVAL.IR
            err = SL_ERR_UNIMPLEMENTED;
            break;

        default:
            goto undef;
        }
        return err;
*/
        return SL_ERR_SLAC_UNDECODED;
    }
    if (inst.r.funct3 == 0b100) {
/*
        switch (inst.r.funct7) {
        case 0b0110000: // HLV.B HLV.BU
        case 0b0110010: // HLV.H HLV.HU HLVX.HU
        case 0b0110100: // HLV.W HLVX.WU
        case 0b0110001: // HSV.B
        case 0b0110011: // HSV.H
        case 0b0110101: // HSV.W
            err = SL_ERR_UNIMPLEMENTED;
            break;

        default:
            goto undef;
        }
        return err;
*/
        return SL_ERR_SLAC_UNDECODED;
    }

    // CSR instruction

    // inst.i.rd == 0 : write-only instruction, no destination GPR
    // inst.i.funct3 : csr op
    // inst.i.imm : csr

    si->type = SLAC_TYPE_SYS;
    si->d0 = inst.i.rd;     // destination register. If 0 then no writeback
    si->r0 = inst.i.rs1;    // r0 = source register or 8-bit immediate value
    const u2 csr_addr = inst.i.imm;
    si->uimm = csr_addr; // uimm = the target csr
    si->arg = PR_D;
    STRACE_FORMAT(si, PR_D);    // todo, print csr value

    switch (inst.i.funct3) {
    case 1:
        if (inst.i.rd == 0) {
            si->func = SLAC_FUNC_CSRWR;
            STRACE_FORMAT(si, 0);
        } else
            si->func = SLAC_FUNC_CSRSWP;
        si->arg = SLAC_IN_ARG_DR;
        STRACE(si, "csrrw x%u, x%u, %s", inst.i.rd, inst.i.rs1, c->ext.name_for_sysreg(c, csr_addr));
        break;
    case 2:
        si->func = SLAC_FUNC_CSROR;
        si->arg = SLAC_IN_ARG_DR;
        STRACE(si, "csrrs x%u, x%u, %s", inst.i.rd, inst.i.rs1, c->ext.name_for_sysreg(c, csr_addr));
        break;
    case 3:
        si->func = SLAC_FUNC_CSRCLR;
        si->arg = SLAC_IN_ARG_DR;
        STRACE(si, "csrrc x%u, x%u, %s", inst.i.rd, inst.i.rs1, c->ext.name_for_sysreg(c, csr_addr));
        break;
    case 5:
        if (inst.i.rd == 0) {
            si->func = SLAC_FUNC_CSRWR;
            STRACE_FORMAT(si, 0);
        } else
            si->func = SLAC_FUNC_CSRSWP;
        si->arg = SLAC_IN_ARG_DI;
        STRACE(si, "csrrwi x%u, %#x, %s", inst.i.rd, inst.i.rs1, c->ext.name_for_sysreg(c, csr_addr));
        break;
    case 6:
        if (inst.i.rs1 == 0)
            si->func = SLAC_FUNC_CSRRD;
        else
            si->func = SLAC_FUNC_CSROR;
        si->arg = SLAC_IN_ARG_DI;
        STRACE(si, "csrrsi x%u, %#x, %s", inst.i.rd, inst.i.rs1, c->ext.name_for_sysreg(c, csr_addr));
        break;
    case 7:
        if (inst.i.rs1 == 0)
            si->func = SLAC_FUNC_CSRRD;
        else
            si->func = SLAC_FUNC_CSRCLR;
        si->arg = SLAC_IN_ARG_DI;
        STRACE(si, "csrrci x%u, %#x, %s", inst.i.rd, inst.i.rs1, c->ext.name_for_sysreg(c, csr_addr));
        break;
    default: goto undef;
    }
    return 0;

undef:
    return rv_slac_undef(c, si);
}

result4_t riscv_core_decode(sl_core_t *core, sl_slac_inst_t *si) {
    rv_core_t *c = (rv_core_t *)core;
    rv_inst_t inst;
    inst.raw = si->desc.machine_op;
    int err = 0;
    result4_t result = {};

    si->raw = 0;
    si->len = SLAC_IN_LEN_MODE;

    // 16 bit compressed instructions
    if ((inst.u.opcode & 3) != 3) {
        result.value = 2;
#if SLAC_TRACE
        si->desc.len = snprintf(si->desc.s, SLAC_BUF_LEN, "[%c] %10" PRIx64 "      %04x  ", priv_level_char[c->core.el], c->core.pc, (u2)si->desc.machine_op);
#endif
        result.err = rv_decode_c(c, si, inst);
        return result;
    }
    result.value = 4;
#if SLAC_TRACE
    si->desc.len = snprintf(si->desc.s, SLAC_BUF_LEN, "[%c] %10" PRIx64 "  %08x  ", priv_level_char[c->core.el], c->core.pc, si->desc.machine_op);
#endif

    switch (inst.u.opcode) {

    // U-type instructions
    case OP_LUI:                                                           // LUI
    case OP_AUIPC:      err = rv_decode_u_type(c, si, inst);        break; // AUIPC
    case OP_JAL:        err = rv_decode_jump(c, si, inst);          break; // JAL
    case OP_BRANCH:     err = rv_decode_branch(c, si, inst);        break; // BEQ BNE BLT BGE BLTU BGEU
    case OP_JALR:       err = rv_decode_jalr(c, si, inst);          break; // JALR
    case OP_LOAD:       err = rv_decode_load(c, si, inst);          break; // LB LH LW LBU LHU
    case OP_STORE:      err = rv_decode_store(c, si, inst);         break; // SB SH SW
    case OP_IMM:        err = rv_decode_alu_imm(c, si, inst);       break; // ADDI SLTI SLTIU XORI ORI ANDI SLLI SRLI SRAI
    case OP_MISC_MEM:   err = rv_decode_sync(c, si, inst);          break; // FENCE FENCE.I
    case OP_ALU:        err = rv_decode_alu(c, si, inst);           break; // ADD SUB SLL SLT SLTU XOR SRL SRA OR AND
    case OP_IMM32:      err = rv64_decode_alu_imm4(c, si, inst);    break;
    case OP_ALU32:      err = rv64_decode_alu4(c, si, inst);        break;
    case OP_AMO:        err = rv_decode_atomic(c, si, inst);        break;
    case OP_SYSTEM:     err = rv_decode_sys(c, si, inst);           break; // ECALL EBREAK CSRRW CSRRS CSRRC CSRRWI CSRRSI CSRRCI

    case OP_FP:         err = rv_decode_fp(c, si, inst);            break;
    case OP_FP_LOAD:    err = rv_decode_fp_load(c, si, inst);       break;
    case OP_FP_STORE:   err = rv_decode_fp_store(c, si, inst);      break;

    case OP_FMADD_S:
    case OP_FMSUB_S:
    case OP_FNMSUB_S:
    case OP_FNMADD_S:   err = rv_decode_fp_mac(c, si, inst);        break;

    default:
        // err = rv_dec_unknown(dec, inst);
        err = SL_ERR_SLAC_UNDECODED;
        break;
    }
    result.err = err;
    return result;
}



