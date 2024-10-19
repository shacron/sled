// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>

#include <core/riscv/inst.h>
#include <core/riscv/rv.h>
#include <sled/error.h>
#include <sled/slac.h>
#include <sled/types.h>

#define FENCE_W (1u << 0)
#define FENCE_R (1u << 1)
#define FENCE_O (1u << 2)
#define FENCE_I (1u << 3)

enum {
    RV_PRINT_NONE = 0,
    RV_PRINT_TYPE_U,
    RV_PRINT_TYPE_DRR,
    RV_PRINT_TYPE_DRRS,
    RV_PRINT_TYPE_ALUI_S,
    RV_PRINT_TYPE_ALUI_X,
    RV_PRINT_TYPE_J,
    RV_PRINT_TYPE_B,
    RV_PRINT_TYPE_L,
    RV_PRINT_TYPE_S,
    RV_PRINT_TYPE_RET,
    RV_PRINT_TYPE_SINGLE,

    RV_PRINT_TYPE_MBAR,
};

static void rv_fence_op_name(u1 op, char *s) {
    if (op & FENCE_I) *s++ = 'i';
    if (op & FENCE_O) *s++ = 'o';
    if (op & FENCE_R) *s++ = 'r';
    if (op & FENCE_W) *s++ = 'w';
    *s = '\0';
}

static inline void set_opcode(sl_slac_inst_t *si, u1 type, u2 op, u1 arg, const char *str) {
    si->raw = 0;
    si->pred = SLAC_PRED_ALWAYS;
    si->type = type;
    si->arg = arg;
    si->op = op;
    si->len = SLAC_IN_LEN_MODE;
    si->desc.str = str;
}

static void set_nop(sl_slac_inst_t *si) {
    si->raw = 0;
    si->pred = SLAC_PRED_NEVER;
    si->type = SLAC_IN_TYPE_SYS;
    si->op = SLAC_IN_OP_NOP;
}

static int rv_slac_undef(rv_core_t *c, sl_slac_inst_t *si) {
    set_opcode(si, SLAC_IN_TYPE_SYS, SLAC_IN_OP_UNDEF, SLAC_IN_ARG_NONE, "undefined");
    return 0;
}

int rv_slac_print_pre(sl_core_t *c, sl_slac_inst_t *si, char *buf, int buflen) {
    static const char pl_char[4] = { 'u', 's', 'h', 'm' };
    u8 val;

    int len = snprintf(buf, buflen, "[%c] %10" PRIx64 "  ", pl_char[c->el], c->pc);
    len += snprintf(buf + len, buflen - len, "%08x  ", si->desc.machine_op);

    switch (si->desc.print_type) {
    case RV_PRINT_TYPE_U:
        len += snprintf(buf + len, buflen - len, "%s x%u, %#" PRIx64, si->desc.str, si->d0, si->uimm);
        break;

    case RV_PRINT_TYPE_DRR:
    case RV_PRINT_TYPE_DRRS:
        len += snprintf(buf + len, buflen - len, "%s x%u, x%u, x%u", si->desc.str, si->d0, si->r0, si->r1);
        break;

    case RV_PRINT_TYPE_ALUI_S:
        len += snprintf(buf + len, buflen - len, "%s x%u, x%u, %" PRIi64, si->desc.str, si->d0, si->r0, si->simm);
        break;

    case RV_PRINT_TYPE_ALUI_X:
        len += snprintf(buf + len, buflen - len, "%s x%u, x%u, %#" PRIx64, si->desc.str, si->d0, si->r0, si->uimm);
        break;

    case RV_PRINT_TYPE_J:
        val = c->pc + si->simm;
        if (si->op == SLAC_IN_OP_BL)
            len += snprintf(buf + len, buflen - len, "%s x%u, %#" PRIx64, si->desc.str, si->d0, val);
        else
            len += snprintf(buf + len, buflen - len, "%s %#" PRIx64, si->desc.str, val);
        break;

    case RV_PRINT_TYPE_RET:
    case RV_PRINT_TYPE_SINGLE:
        len += snprintf(buf + len, buflen - len, "%s", si->desc.str);
        break;

    case RV_PRINT_TYPE_B:
        val = c->pc + si->simm;
        len += snprintf(buf + len, buflen - len, "%s x%u, x%u, %#" PRIx64, si->desc.str, si->r0, si->r1, val);
        break;

    case RV_PRINT_TYPE_L:
    case RV_PRINT_TYPE_S: {
        u4 dst = si->d0;
        if (dst == SLAC_REG_DISCARD) dst = 0;
        len += snprintf(buf + len, buflen - len, "%s x%u, %" PRIi64 "(x%u)", si->desc.str, dst, si->simm, si->r0);
        break;
    }

    case RV_PRINT_TYPE_MBAR: {
        char p[5], s[5];
        rv_fence_op_name(si->r0, p);
        rv_fence_op_name(si->r1, s);
        len += snprintf(buf + len, buflen - len, "%s %s, %s", si->desc.str, p, s);
        break;
    }

    default:
        return len + snprintf(buf + len, buflen - len, "%s TODO", si->desc.str);
    }
    int padding = 57 - len;
    len += snprintf(buf + len, buflen - len, "%*s", padding, ";");
    return len;
}

int rv_slac_print_post(sl_core_t *c, sl_slac_inst_t *si, char *buf, int buflen) {
    int len = 0;
    const u1 d0 = si->d0;
    switch (si->desc.print_type) {
    case RV_PRINT_TYPE_U:
    case RV_PRINT_TYPE_DRR:
    case RV_PRINT_TYPE_DRRS:
    case RV_PRINT_TYPE_ALUI_S:
    case RV_PRINT_TYPE_ALUI_X:
    case RV_PRINT_TYPE_L:
        len += snprintf(buf, buflen, " %s=%#" PRIx64, rv_name_for_reg(d0), c->r[d0]);
        break;

    case RV_PRINT_TYPE_S:
        len += snprintf(buf, buflen, " [%#" PRIx64 "] = %#" PRIx64, c->r[d0] + si->simm, c->r[si->r0]);
        break;

    case RV_PRINT_TYPE_J:
    case RV_PRINT_TYPE_RET:
        if (si->op == SLAC_IN_OP_BL)
            len += snprintf(buf, buflen, " pc=%#" PRIx64 ", %s=%#" PRIx64, c->pc, rv_name_for_reg(d0), c->r[d0]);
        else
            len += snprintf(buf, buflen, " pc=%#" PRIx64, c->pc);
        break;

    case RV_PRINT_TYPE_B:
        if (c->branch_taken)
            len += snprintf(buf, buflen, " pc=%#" PRIx64, c->pc);
        break;

    default:
       return 0;
    }
    return len;
}

static int rv_decode_u_type(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    si->uimm = (u8)(i4)(inst.raw & 0xfffff000);
    if (inst.u.opcode == OP_LUI) {
        set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_MOV, SLAC_IN_ARG_DI, "lui");
    } else { // OP_AUIPC
        // add pc + offset 
        // todo: optimize to MOV if address is known?
        set_opcode(si, SLAC_IN_TYPE_SYS, SLAC_IN_OP_ADR4K, SLAC_IN_ARG_DI, "auipc");
    }
    si->d0 = inst.u.rd;
    if (inst.u.rd == 0) set_nop(si);
    si->desc.print_type = RV_PRINT_TYPE_U;
    return 0;
}

static int rv_decode_alu_imm(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    u4 shift;
    u4 func7;
    const bool is_rv32 = c->core.mode == SL_CORE_MODE_32;
    if (is_rv32) {
        shift = inst.i.imm & 31;
        func7 = inst.i.imm >> 5;
    } else {
        shift = inst.i.imm & 63;
        func7 = (inst.i.imm >> 5) & (~1);
    }

    si->desc.print_type = RV_PRINT_TYPE_ALUI_X;

    switch (inst.i.funct3) {
    case 0b000: // ADDI
        set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_ADD, SLAC_IN_ARG_DRI, "addi");
        si->desc.print_type = RV_PRINT_TYPE_ALUI_S;
        break;

    case 0b001: // SLLI
        if (func7 != 0) return rv_slac_undef(c, si);
        set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_SHL, SLAC_IN_ARG_DRI, "slli");
        si->uimm = shift;
        si->desc.print_type = RV_PRINT_TYPE_ALUI_S;
        goto immediate_set;

    case 0b101:
        if (func7 == 0) {   // SRLI
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_SHRU, SLAC_IN_ARG_DRI, "srli");
        } else if (func7 == 0b0100000) {  //SRAI
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_SHRS, SLAC_IN_ARG_DRI, "srai");
        } else {
            return rv_slac_undef(c, si);
        }
        si->uimm = shift;
        si->desc.print_type = RV_PRINT_TYPE_ALUI_S;
        goto immediate_set;

    case 0b010: // SLTI
        set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_CSELS, SLAC_IN_ARG_DRI, "slti");
        si->desc.print_type = RV_PRINT_TYPE_ALUI_S;
        break;

    case 0b011: // SLTIU
        set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_CSELU, SLAC_IN_ARG_DRI, "sltiu");
        // docs: the immediate is first sign-extended to XLEN bits then treated as an unsigned number
        if (is_rv32) si->uimm = (u4)(((i4)inst.raw) >> 20);  // sign extend immediate
        else         si->uimm = (u8)(i8)(((i4)inst.raw) >> 20);  // sign extend immediate
        si->desc.print_type = RV_PRINT_TYPE_ALUI_S;
        goto immediate_set;

    case 0b100: // XORI
        set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_XOR, SLAC_IN_ARG_DRI, "xori");
        break;

    case 0b110: // ORI
        set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_OR, SLAC_IN_ARG_DRI, "ori");
        break;

    case 0b111: // ANDI
        set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_AND, SLAC_IN_ARG_DRI, "andi");
        break;

    default:
        return rv_slac_undef(c, si);
    }

    si->simm = ((i4)inst.raw) >> 20;  // sign extend immediate
immediate_set:
    si->d0 = inst.i.rd;
    si->r0 = inst.i.rs1;
    if (inst.i.rd == 0) set_nop(si);
    return 0;
}

static int rv_decode_alu(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    // RV_TRACE_DECL_OPSTR;
    // const uxlen_t u1 = c->core.r[inst.r.rs1];
    // const uxlen_t u2 = c->core.r[inst.r.rs2];
    // uxlen_t result;
    // ux2len_t result_ul;
    // sx2len_t result_sl;

    si->desc.print_type = RV_PRINT_TYPE_DRR;

    switch (inst.r.funct7) {
    case 0:
        switch (inst.r.funct3) {
        case 0b000: // ADD
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_ADD, SLAC_IN_ARG_DRR, "add");
            si->desc.print_type = RV_PRINT_TYPE_DRRS;
            break;

        case 0b001: // SLL
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_SHL, SLAC_IN_ARG_DRR, "sll");
            si->desc.print_type = RV_PRINT_TYPE_DRRS;
            break;

        case 0b010: // SLT
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_CSELS, SLAC_IN_ARG_DRR, "slt");
            si->desc.print_type = RV_PRINT_TYPE_DRRS;
            break;

        case 0b011: // SLTU
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_CSELU, SLAC_IN_ARG_DRR, "sltu");
            break;

        case 0b100: // XOR
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_XOR, SLAC_IN_ARG_DRR, "xor");
            break;

        case 0b101: // SRL
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_SHRU, SLAC_IN_ARG_DRR, "srl");
            break;

        case 0b110: // OR
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_OR, SLAC_IN_ARG_DRR, "or");
            break;

        case 0b111: // AND
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_AND, SLAC_IN_ARG_DRR, "and");
            break;
        }
        break;

    case 0b0100000:
        switch (inst.r.funct3) {
        case 0b000: // SUB
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_SUB, SLAC_IN_ARG_DRR, "sub");
            break;

        case 0b101: // SRA
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_SHRS, SLAC_IN_ARG_DRR, "sra");
            break;

        default:
            goto undef;
        }
        break;

    case 0b0000001: // M extensions
        if (!(c->core.arch_options & SL_RISCV_EXT_M)) goto undef;
        switch (inst.r.funct3) {
        case 0b000: // MUL
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_MUL, SLAC_IN_ARG_DRR, "mul");
            break;

        case 0b001: // MULH
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_MULHSS, SLAC_IN_ARG_DRR, "mulh");
            break;

        case 0b010: // MULHSU
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_MULHSU, SLAC_IN_ARG_DRR, "mulhsu");
            break;

        case 0b011: // MULHU
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_MULHUU, SLAC_IN_ARG_DRR, "mulhu");
            break;

        case 0b100: // DIV
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_DIVS, SLAC_IN_ARG_DRR, "div");
            break;

        case 0b101: // DIVU
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_DIVU, SLAC_IN_ARG_DRR, "divu");
            break;

        case 0b110: // REM
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_MODS, SLAC_IN_ARG_DRR, "rem");
            break;

        case 0b111: // REMU
            set_opcode(si, SLAC_IN_TYPE_ALU, SLAC_IN_OP_MODU, SLAC_IN_ARG_DRR, "remu");
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
    return 0;

undef:
    return rv_slac_undef(c, si);
}

static int rv_decode_jump(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    i4 imm = (inst.j.imm3 << 12) | (inst.j.imm2 << 11) | (inst.j.imm1 << 1);
    // or imm4 with sign extend
    imm |= ((i4)(inst.raw & 0x80000000)) >> (31 - 20);
    si->simm = imm;

    if (inst.j.rd == RV_ZERO) {        // J
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_B, SLAC_IN_ARG_I, "j");
    } else {
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_BL, SLAC_IN_ARG_DI, "jal");
        si->d0 = inst.j.rd;
    }
    si->desc.print_type = RV_PRINT_TYPE_J;
    return 0;
}

// JALR: I-type
static int rv_decode_jalr(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    if (inst.i.funct3 != 0)
        return rv_slac_undef(c, si);

    i4 imm = ((i4)inst.raw) >> 20;
    if (inst.i.rd == RV_ZERO) {
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_B, SLAC_IN_ARG_R1, "ret");
        si->desc.print_type = RV_PRINT_TYPE_RET;
    } else {
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_BL, SLAC_IN_ARG_DRI, "jalr");
        si->d0 = inst.i.rd;
        si->desc.print_type = RV_PRINT_TYPE_J;
    }
    si->r0 = inst.i.rs1;
    si->simm = imm;
    return 0;
}

static int rv_decode_branch(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    switch (inst.b.funct3) {
    case 0b000: // BEQ
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_CBEQ, SLAC_IN_ARG_RRI, "beq");
        break;

    case 0b001: // BNE
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_CBNE, SLAC_IN_ARG_RRI, "bne");
        break;

    case 0b100: // BLT
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_CBLTS, SLAC_IN_ARG_RRI, "blt");
        break;

    case 0b101: // BGE
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_CBGES, SLAC_IN_ARG_RRI, "bge");
        break;

    case 0b110: // BLTU
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_CBLTU, SLAC_IN_ARG_RRI, "bltu");
        break;

    case 0b111: // BGEU
        set_opcode(si, SLAC_IN_TYPE_BR, SLAC_IN_OP_CBGEU, SLAC_IN_ARG_RRI, "bgeu");
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
    si->desc.print_type = RV_PRINT_TYPE_B;
    return 0;
}

// I-type
static int rv_decode_load(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    const i4 imm = ((i4)inst.raw) >> 20;
    si->simm = imm;
    si->r0 = inst.i.rs1;
    si->d0 = inst.i.rd;
    if (si->d0 == 0) si->d0 = SLAC_REG_DISCARD;

    switch (inst.i.funct3) {
    case 0b000: // LB
        set_opcode(si, SLAC_IN_TYPE_LD, SLAC_IN_OP_LDBS, SLAC_IN_ARG_DRI, "lb");
        break;

    case 0b001: // LH
        set_opcode(si, SLAC_IN_TYPE_LD, SLAC_IN_OP_LDHS, SLAC_IN_ARG_DRI, "lh");
        break;

    case 0b010: // LW
        set_opcode(si, SLAC_IN_TYPE_LD, SLAC_IN_OP_LDWS, SLAC_IN_ARG_DRI, "lw");
        break;

    case 0b100: // LBU
        set_opcode(si, SLAC_IN_TYPE_LD, SLAC_IN_OP_LDB, SLAC_IN_ARG_DRI, "lbu");
        break;

    case 0b101: // LHU
        set_opcode(si, SLAC_IN_TYPE_LD, SLAC_IN_OP_LDH, SLAC_IN_ARG_DRI, "lhu");
        break;

    case 0b110: // LWU
        if (c->core.mode != SL_CORE_MODE_64)
            return rv_slac_undef(c, si);
        set_opcode(si, SLAC_IN_TYPE_LD, SLAC_IN_OP_LDW, SLAC_IN_ARG_DRI, "lwu");
        break;

    case 0b011: // LD
        if (c->core.mode != SL_CORE_MODE_64)
            return rv_slac_undef(c, si);
        set_opcode(si, SLAC_IN_TYPE_LD, SLAC_IN_OP_LDX, SLAC_IN_ARG_DRI, "ld");
        break;

    default:
        return rv_slac_undef(c, si);
    }
    si->desc.print_type = RV_PRINT_TYPE_L;
    return 0;
}

static int rv_decode_store(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
    const i4 imm = (((i4)inst.raw >> 20) & ~(0x1f)) | inst.s.imm1;
    si->simm = imm;
    si->d0 = inst.s.rs1;
    si->r0 = inst.s.rs2;
    switch (inst.s.funct3) {
    case 0b000: // SB
        set_opcode(si, SLAC_IN_TYPE_ST, SLAC_IN_OP_STB, SLAC_IN_ARG_DRI, "sb");
        break;

    case 0b001: // SH
        set_opcode(si, SLAC_IN_TYPE_ST, SLAC_IN_OP_STH, SLAC_IN_ARG_DRI, "sh");
        break;

    case 0b010: // SW
        set_opcode(si, SLAC_IN_TYPE_ST, SLAC_IN_OP_STW, SLAC_IN_ARG_DRI, "sw");
        break;

    case 0b011: // SD
        if (c->core.mode != SL_CORE_MODE_64)
            return rv_slac_undef(c, si);
        set_opcode(si, SLAC_IN_TYPE_ST, SLAC_IN_OP_STX, SLAC_IN_ARG_DRI, "sd");
        break;

    default:
        return rv_slac_undef(c, si);
    }
    si->desc.print_type = RV_PRINT_TYPE_S;
    return 0;
}

int rv_decode_sync(rv_core_t *c, sl_slac_inst_t *si, rv_inst_t inst) {
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
        set_opcode(si, SLAC_IN_TYPE_SYS, SLAC_IN_OP_MBAR, SLAC_IN_ARG_I, "fence");
        si->uimm = bar;
        si->r0 = pred;
        si->r1 = succ;
        si->desc.print_type = RV_PRINT_TYPE_MBAR;
        return 0;
    }

    case 0b001:
        if (inst.i.imm != 0) goto undef;
        set_opcode(si, SLAC_IN_TYPE_SYS, SLAC_IN_OP_MBAR, SLAC_IN_ARG_NONE, "fence.i");
        si->desc.print_type = RV_PRINT_TYPE_SINGLE;
        return 0;

    default:
        goto undef;
    }

undef:
    return rv_slac_undef(c, si);
}

int riscv_core_decode(sl_core_t *core, sl_slac_inst_t *si) {
    rv_core_t *c = (rv_core_t *)core;
    rv_inst_t inst;
    inst.raw = si->desc.machine_op;
    int err = 0;

    // 16 bit compressed instructions
    if ((inst.u.opcode & 3) != 3) {
        if (!(c->core.arch_options & SL_RISCV_EXT_C)) return rv_slac_undef(c, si);
        c->core.prev_len = 2;
        return SL_ERR_SLAC_UNDECODED;   // todo
        // c->c_inst = 1;
        // return XLEN_PREFIX(dispatch16)(c, inst);
    }
    c->core.prev_len = 4;

    switch (inst.u.opcode) {

    // U-type instructions
    case OP_LUI:  // LUI
    case OP_AUIPC:  // AUIPC
        err = rv_decode_u_type(c, si, inst);
        break;

    // J-type instructions
    case OP_JAL:  // JAL
        err = rv_decode_jump(c, si, inst);
        break;

    // B-type
    case OP_BRANCH:  // BEQ BNE BLT BGE BLTU BGEU
        err = rv_decode_branch(c, si, inst);
        break;

    // I-type
    case OP_JALR:  // JALR
        err = rv_decode_jalr(c, si, inst);
        break;

    // I-type
    case OP_LOAD:  // LB LH LW LBU LHU
        err = rv_decode_load(c, si, inst);
        break;

    // S-type
    case OP_STORE:  // SB SH SW
        err = rv_decode_store(c, si, inst);
        break;

    // I-type
    case OP_IMM:  // ADDI SLTI SLTIU XORI ORI ANDI SLLI SRLI SRAI
        err = rv_decode_alu_imm(c, si, inst);
        break;

    // Other
    case OP_MISC_MEM:  // FENCE FENCE.I
        err = rv_decode_sync(c, si, inst);
        break;

    // R-type
    case OP_ALU:  // ADD SUB SLL SLT SLTU XOR SRL SRA OR AND
        err = rv_decode_alu(c, si, inst);
        break;

#if 0
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

    case OP_FMADD_S:
    case OP_FMSUB_S:
    case OP_FNMSUB_S:
    case OP_FNMADD_S:
        err = rv_exec_fp_mac(c, inst);
        break;

    case OP_SYSTEM:  // ECALL EBREAK CSRRW CSRRS CSRRC CSRRWI CSRRSI CSRRCI
        err = rv_exec_system(c, inst);
        break;

    case OP_AMO:
        err = rv_exec_atomic(c, inst);
        break;
#endif

    default:
        // err = rv_dec_unknown(dec, inst);
        err = SL_ERR_SLAC_UNDECODED;
        break;
    }
    return err;
}



