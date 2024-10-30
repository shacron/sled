// SPDX-License-Identifier: MIT License
// Copyright (c) 2024 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>

#include <core/core.h>
#include <core/ex.h>
#include <sled/error.h>
#include <sled/slac.h>

#if SLAC_RLEN == 1
    u1 typedef urlen_t;
    i1 typedef srlen_t;
    u2 typedef ur2len_t;
    i2 typedef sr2len_t;
    result8_t typedef resultrlen_t;
    #define RLEN_PREFIX(name) slac1_ ## name
    #define PRIRLENx PRIx8
#elif SLAC_RLEN == 2
    u2 typedef urlen_t;
    i2 typedef srlen_t;
    u4 typedef ur2len_t;
    i4 typedef sr2len_t;
    result16_t typedef resultrlen_t;
    #define RLEN_PREFIX(name) slac2_ ## name
    #define PRIRLENx PRIx16
#elif SLAC_RLEN == 4
    u4 typedef urlen_t;
    i4 typedef srlen_t;
    u8 typedef ur2len_t;
    i8 typedef sr2len_t;
    result32_t typedef resultrlen_t;
    #define RLEN_PREFIX(name) slac4_ ## name
    #define PRIRLENx PRIx32
#else // SLAC_RLEN == 8
    u8 typedef urlen_t;
    i8 typedef srlen_t;
    __uint128_t typedef ur2len_t;
    __int128_t  typedef sr2len_t;
    result64_t  typedef resultrlen_t;
    #define RLEN_PREFIX(name) slac8_ ## name
    #define PRIRLENx PRIx64
#endif

 #define SLAC_RLEN_BITS (SLAC_RLEN * 8)


// #define SLAC_TRACE 1

/*
opcode execution:
predicate
load arguments
call dispatch function based on type (jump table)
write back
*/

static inline u8 sign_extend8(srlen_t x) { return (u8)(i8)x; }

#if SLAC_RLEN == 4
#define SX8_EXTEND(a) (si->sx8 ? sign_extend8(a) : (a))
#else
#define SX8_EXTEND(a) (a)
#endif

__attribute__((no_sanitize("signed-integer-overflow")))
static inline sr2len_t mul_ssl(srlen_t a, srlen_t b) { return (sr2len_t)a * b; }

static int RLEN_PREFIX(exec_alu_di)(sl_core_t *c, sl_slac_inst_t *si) {
    urlen_t result;
    switch(si->op) {
    case SLAC_IN_OP_MOV:    result = si->uimm;          break;
    default:
        return SL_ERR_SLAC_INVALID;
    }
    c->r[si->d0] = SX8_EXTEND(result);
    return 0;
}

static int RLEN_PREFIX(exec_alu_dr)(sl_core_t *c, sl_slac_inst_t *si) {
    urlen_t result;
    const urlen_t val = c->r[si->r0];
    switch(si->op) {
    case SLAC_IN_OP_MOV:    result = val;       break;
    default:
        return SL_ERR_SLAC_INVALID;
    }
    c->r[si->d0] = SX8_EXTEND(result);
    return 0;
}

static int RLEN_PREFIX(exec_alu_dri)(sl_core_t *c, sl_slac_inst_t *si) {
    urlen_t result;
    const urlen_t val = c->r[si->r0];
    const urlen_t uimm = si->uimm;
    switch(si->op) {
    case SLAC_IN_OP_ADD:    result = val + uimm;    break;
    case SLAC_IN_OP_SUB:    result = val - uimm;    break;
    case SLAC_IN_OP_RSUB:   result = uimm - val;    break;
    case SLAC_IN_OP_AND:    result = val & uimm;    break;
    case SLAC_IN_OP_OR:     result = val | uimm;    break;
    case SLAC_IN_OP_XOR:    result = val ^ uimm;    break;
    case SLAC_IN_OP_NOT:    result = ~uimm;         break;
    case SLAC_IN_OP_SHL:    result = val << uimm;   break;
    case SLAC_IN_OP_SHRU:   result = val >> uimm;   break;
    case SLAC_IN_OP_SHRS:   result = ((srlen_t)val) >> uimm;    break;
    case SLAC_IN_OP_CSELS:  result = ((srlen_t)val < (srlen_t)uimm) ? 1 : 0; break;
    case SLAC_IN_OP_CSELU:  result = (val < uimm) ? 1 : 0;      break;
    default:
        return SL_ERR_SLAC_INVALID;
    }

    c->r[si->d0] = SX8_EXTEND(result);
    return 0;
}

static int RLEN_PREFIX(exec_alu_drr)(sl_core_t *c, sl_slac_inst_t *si) {
    urlen_t result;
    const urlen_t r0 = c->r[si->r0];
    const urlen_t r1 = c->r[si->r1];
    const u1 shift = r1 & (SLAC_RLEN_BITS - 1);
    switch(si->op) {
    case SLAC_IN_OP_ADD:    result = r0 + r1;       break;
    case SLAC_IN_OP_SUB:    result = r0 - r1;       break;
    case SLAC_IN_OP_RSUB:   result = r1 - r0;       break;
    case SLAC_IN_OP_AND:    result = r0 & r1;       break;
    case SLAC_IN_OP_OR:     result = r0 | r1;       break;
    case SLAC_IN_OP_XOR:    result = r0 ^ r1;       break;
    case SLAC_IN_OP_SHL:    result = r0 << shift;   break;
    case SLAC_IN_OP_SHRU:   result = r0 >> shift;   break;
    case SLAC_IN_OP_SHRS:   result = ((srlen_t)r0) >> shift;                break;
    case SLAC_IN_OP_CSELS:  result = ((srlen_t)r0 < (srlen_t)r1) ? 1 : 0;   break;
    case SLAC_IN_OP_CSELU:  result = (r0 < r1) ? 1 : 0;                     break;
    case SLAC_IN_OP_MUL:    result = r0 * r1;                               break;
    case SLAC_IN_OP_MULHSS: result = mul_ssl(r0, r1) >> SLAC_RLEN_BITS;     break;
    case SLAC_IN_OP_MULHSU: result = ((sr2len_t)r0 * r1) >> SLAC_RLEN_BITS; break;
    case SLAC_IN_OP_MULHUU: result = (urlen_t)(((ur2len_t)r0 * r1) >> SLAC_RLEN_BITS);  break;
    case SLAC_IN_OP_DIVS:
        if (r1 == 0) result = ~((urlen_t)0);
        else         result = (srlen_t)r0 / (srlen_t)r1;
        break;
    case SLAC_IN_OP_DIVU:
        if (r1 == 0) result = ~((urlen_t)0);
        else         result = r0 / r1;
        break;
    case SLAC_IN_OP_MODS:
        if (r1 == 0) result = r0;
        else         result = (srlen_t)r0 % (srlen_t)r1;
        break;
    case SLAC_IN_OP_MODU:
        if (r1 == 0) result = r0;
        else         result = r0 % r1;
        break;

    default:
        return SL_ERR_SLAC_INVALID;
    }
    c->r[si->d0] = SX8_EXTEND(result);
    return 0;
}

static int RLEN_PREFIX(exec_alu)(sl_core_t *c, sl_slac_inst_t *si) {
    // printf("alu %s\n", si->desc.str);
    switch (si->arg) {
    case SLAC_IN_ARG_DI:    return RLEN_PREFIX(exec_alu_di)(c, si);
    case SLAC_IN_ARG_DR:    return RLEN_PREFIX(exec_alu_dr)(c, si);
    case SLAC_IN_ARG_DRI:   return RLEN_PREFIX(exec_alu_dri)(c, si);
    case SLAC_IN_ARG_DRR:   return RLEN_PREFIX(exec_alu_drr)(c, si);
    }
    return SL_ERR_SLAC_INVALID;
}

static int RLEN_PREFIX(exec_load)(sl_core_t *c, sl_slac_inst_t *si) {
    int err;
    u8 x;
    u4 w;
    u2 h;
    u1 b;

    urlen_t target = c->r[si->r0] + si->simm;

    switch (si->op) {
    case SLAC_IN_OP_LDB:
        err = sl_core_mem_read(c, target, 1, 1, &b);
        x = b;
        break;

    case SLAC_IN_OP_LDBS:
        err = sl_core_mem_read(c, target, 1, 1, &b);
        x = (urlen_t)(i1)b;
        break;

    case SLAC_IN_OP_LDH:
        err = sl_core_mem_read(c, target, 2, 1, &h);
        x = h;
        break;

    case SLAC_IN_OP_LDHS:
        err = sl_core_mem_read(c, target, 2, 1, &h);
        x = (urlen_t)(i2)h;
        break;

    case SLAC_IN_OP_LDW:
        err = sl_core_mem_read(c, target, 4, 1, &w);
        x = w;
        break;

    case SLAC_IN_OP_LDWS:
        err = sl_core_mem_read(c, target, 4, 1, &w);
        x = (urlen_t)(i4)w;
        break;

    case SLAC_IN_OP_LDX:
        err = sl_core_mem_read(c, target, 8, 1, &x);
        break;

    default:
        return SL_ERR_SLAC_INVALID;
    }

    if (err) return sl_core_synchronous_exception(c, EX_ABORT_LOAD, target, err);
    if (si->d0 != SLAC_REG_DISCARD)
        c->r[si->d0] = x;
    return 0;
}

static int RLEN_PREFIX(exec_store)(sl_core_t *c, sl_slac_inst_t *si) {
    urlen_t val = c->r[si->r0];
    const urlen_t dest = c->r[si->d0] + si->simm;
    u4 w;
    u2 h;
    u1 b;
    int err = 0;

    switch (si->op) {
    case SLAC_IN_OP_STB:
        b = (u1)val;
        err = sl_core_mem_write(c, dest, 1, 1, &b);
        break;

    case SLAC_IN_OP_STH:
        h = (u2)val;
        err = sl_core_mem_write(c, dest, 2, 1, &h);
        break;

    case SLAC_IN_OP_STW:
        w = (u4)val;
        err = sl_core_mem_write(c, dest, 4, 1, &w);
        break;

    case SLAC_IN_OP_STX:
        err = sl_core_mem_write(c, dest, 8, 1, &val);
        break;
    }
    if (err)
        return sl_core_synchronous_exception(c, EX_ABORT_STORE, dest, err);
    return 0;
}

static int RLEN_PREFIX(exec_sys)(sl_core_t *c, sl_slac_inst_t *si) {
    urlen_t result;

    switch (si->op) {
    case SLAC_IN_OP_ADR4K:
        result = c->pc + si->simm;
        break;

    case SLAC_IN_OP_MBAR:
        sl_core_memory_barrier(c, si->uimm);
        return 0;

    case SLAC_IN_OP_IBAR:
        sl_core_instruction_barrier(c);
        return 0;

    case SLAC_IN_OP_NOP:
        return 0;

    case SLAC_IN_OP_UNDEF:  return SL_ERR_UNDEF;
    default:
        return SL_ERR_SLAC_INVALID;
    }
    c->r[si->d0] = result;
    return 0;
}

static int RLEN_PREFIX(exec_br)(sl_core_t *c, sl_slac_inst_t *si) {
    urlen_t result;
    bool cond = false;

    switch (si->op) {
    case SLAC_IN_OP_BL:
        // todo: fix pc value based on instruction size
        c->r[si->d0] = (urlen_t)(c->pc + 4);        // slac:bli/bl*
    case SLAC_IN_OP_B:

/*
arm:
    b - unconditional, pc rel
    b.cond - conditional, pc rel
    bl - uncond, pc rel, link
    blr - uncond, reg, link
    br - uncond, reg

// slac
    // any can use predicate
    b   uncond, pc rel
    br  uncond, reg + imm
    bl  uncond, pc rel, link
    blr uncond, reg + imm, link
    cbxx - compare and branch (rr), pc rel
*/
        if (si->arg & SLAC_IN_ARG_R1)
            result = c->r[si->r0]; // slac:br/blr
        else
            result = c->pc;              // slac:bi/bli
        result += si->simm;
        c->pc = result;
        c->branch_taken = 1;
        return 0;

    case SLAC_IN_OP_CBEQ:   cond = ((urlen_t)c->r[si->r0] == (urlen_t)c->r[si->r1]);    break;
    case SLAC_IN_OP_CBNE:   cond = ((urlen_t)c->r[si->r0] != (urlen_t)c->r[si->r1]);    break;
    case SLAC_IN_OP_CBLTU:  cond = ((urlen_t)c->r[si->r0] <  (urlen_t)c->r[si->r1]);    break;
    case SLAC_IN_OP_CBLTS:  cond = ((srlen_t)c->r[si->r0] <  (srlen_t)c->r[si->r1]);    break;
    case SLAC_IN_OP_CBGEU:  cond = ((urlen_t)c->r[si->r0] >= (urlen_t)c->r[si->r1]);    break;
    case SLAC_IN_OP_CBGES:  cond = ((srlen_t)c->r[si->r0] >= (srlen_t)c->r[si->r1]);    break;

    default:
        return SL_ERR_UNIMPLEMENTED;
    }

    if (cond) {
        c->pc = (urlen_t)(c->pc + si->simm);
        c->branch_taken = 1;
    }
    return 0;
}

// todo: move this
int rv_slac_print_pre(sl_core_t *c, sl_slac_inst_t *si, char *buf, int buflen);
int rv_slac_print_post(sl_core_t *c, sl_slac_inst_t *si, char *buf, int buflen);

int RLEN_PREFIX(dispatch)(sl_core_t *c, sl_slac_inst_t *si) {
#if SLAC_TRACE
    #define BUFLEN 256
    char buf[BUFLEN];
    int len = rv_slac_print_pre(c, si, buf, BUFLEN);
#endif

    int err = 0;

#if 0
    if (__builtin_expect(si->pred, SLAC_PRED_ALWAYS))
        goto type_dispatch;

    switch (si->pred) {
    case SLAC_PRED_NEVER:   goto out;

    case SLAC_PRED_EQ:
    case SLAC_PREG_NE:
    case SLAC_PRED_HS:
    case SLAC_PRED_LO:
    case SLAC_PRED_POS:
    case SLAC_PRED_NEG:
    case SLAC_PRED_NOF:
    case SLAC_PRED_OF:
    case SLAC_PRED_HI:
    case SLAC_PRED_LS:
    case SLAC_PRED_GE:
    case SLAC_PRED_LT:
    case SLAC_PRED_GT:
    case SLAC_PRED_LE:
        return SL_ERR_UNIMPLEMENTED;
    }

type_dispatch:
#endif
    switch (si->type) {
    case SLAC_IN_TYPE_ALU:  err = RLEN_PREFIX(exec_alu)(c, si);     break;
    case SLAC_IN_TYPE_LD:   err = RLEN_PREFIX(exec_load)(c, si);    break;
    case SLAC_IN_TYPE_ST:   err = RLEN_PREFIX(exec_store)(c, si);   break;
    case SLAC_IN_TYPE_SYS:  err = RLEN_PREFIX(exec_sys)(c, si);     break;
    case SLAC_IN_TYPE_BR:   err = RLEN_PREFIX(exec_br)(c, si);      break;

    case SLAC_IN_TYPE_FP:
    case SLAC_IN_TYPE_VEC:
    case SLAC_IN_TYPE_SIMD:
    case SLAC_IN_TYPE_ATOMIC:
    default:
        return SL_ERR_SLAC_INVALID;
    }

// out:
#if SLAC_TRACE
    rv_slac_print_post(c, si, buf + len, BUFLEN - len);
    puts(buf);
#endif
    return err;
}
