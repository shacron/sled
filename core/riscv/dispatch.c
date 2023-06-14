// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include <core/riscv.h>
#include <core/riscv/csr.h>
#include <core/riscv/dispatch.h>
#include <core/riscv/inst.h>
#include <core/riscv/rv.h>
#include <core/riscv/trace.h>
#include <core/sym.h>
#include <sled/arch.h>
#include <sled/error.h>
#include <sled/io.h>

#define FENCE_W (1u << 0)
#define FENCE_R (1u << 1)
#define FENCE_O (1u << 2)
#define FENCE_I (1u << 3)

#define MONITOR_UNARMED 0
#define MONITOR_ARMED32 1
#define MONITOR_ARMED64 2

#if RV_TRACE
static void rv_fence_op_name(u1 op, char *s) {
    if (op & FENCE_I) *s++ = 'i';
    if (op & FENCE_O) *s++ = 'o';
    if (op & FENCE_R) *s++ = 'r';
    if (op & FENCE_W) *s++ = 'w';
    *s = '\0';
}
#endif

int rv_exec_mem(rv_core_t *c, rv_inst_t inst) {
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
        core_memory_barrier(&c->core, bar);
#if RV_TRACE
        char p[5], s[5];
        rv_fence_op_name(pred, p);
        rv_fence_op_name(succ, s);
        RV_TRACE_PRINT(c, "fence %s, %s", p, s);
#endif
        return 0;
    }

    case 0b001:
        if (inst.i.imm != 0) goto undef;
        core_instruction_barrier(&c->core);
        RV_TRACE_PRINT(c, "fence.i");
        return 0;

    default:
        goto undef;
    }

undef:
    return rv_undef(c, inst);
}

static int rv_atomic_alu32(rv_core_t *c, u8 addr, u1 op, u4 operand, u1 rd, u1 ord) {
    u8 result;
    int err;
    c->monitor_status = MONITOR_UNARMED;
    if ((err = sl_core_mem_atomic(&c->core, addr, 4, op, operand, 0, &result, ord, memory_order_relaxed))) return err;
    if (rd != RV_ZERO) {
        c->r[rd] = (u4)result;
        RV_TRACE_RD(c, rd, c->r[rd]);
    }
    return 0;
}

static int rv_atomic_alu64(rv_core_t *c, u8 addr, u1 op, u4 operand, u1 rd, u1 ord) {
    u8 result;
    int err;
    c->monitor_status = MONITOR_UNARMED;
    if ((err = sl_core_mem_atomic(&c->core, addr, 8, op, operand, 0, &result, ord, memory_order_relaxed))) return err;
    if (rd != RV_ZERO) {
        c->r[rd] = result;
        RV_TRACE_RD(c, rd, c->r[rd]);
    }
    return 0;
}

int rv_exec_atomic(rv_core_t *c, rv_inst_t inst) {
    if ((c->core.arch_options & SL_RISCV_EXT_A) == 0) goto undef;

    const memory_order ord_index[] = {
        memory_order_relaxed, memory_order_release, memory_order_acquire, memory_order_acq_rel
    };

    int err = 0;
    const u1 op = inst.r.funct7 >> 2;
    const u1 barrier = inst.r.funct7 & 3;   // 1: acquire, 0: release
    const u1 ord = ord_index[barrier];
    const u1 rd = inst.r.rd;
    const u8 addr = c->r[inst.r.rs1];
    u8 result;

#if RV_TRACE
    const char *bstr_index[] = { "", ".rl", ".aq", ".aqrl" };
    const char *bstr = bstr_index[barrier];
#endif

    if (inst.r.funct3 == 0b010) {
        if (addr & 3) return rv_synchronous_exception(c, EX_ABORT_LOAD, addr, SL_ERR_IO_ALIGN);

        switch (op) {
        case 0b00010: { // LR.W
            if (inst.r.rs2 != 0) goto undef;
            // we are faking a monitor by retaining the loaded value and then compare-exchanging with the stored value
            RV_TRACE_PRINT(c, "lr.w%s x%u, (x%u)", bstr, rd, inst.r.rs1);
            c->monitor_addr = addr;
            c->monitor_status = MONITOR_UNARMED;
            if (barrier & 1) atomic_thread_fence(memory_order_release);
            u4 w;
            if ((err = sl_core_mem_read(&c->core, addr, 4, 1, &w))) break;
            if (barrier & 2) atomic_thread_fence(memory_order_acquire);
            c->monitor_value = w;
            c->monitor_status = MONITOR_ARMED32;
            if (rd != RV_ZERO) {
                c->r[rd] = w;
                RV_TRACE_RD(c, rd, c->r[rd]);
            }
            break;
        }

        case 0b00011: // SC.W
            RV_TRACE_PRINT(c, "sc.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            if (c->monitor_status != MONITOR_ARMED32) {
                c->monitor_status = MONITOR_UNARMED;
                return 0;
            }
            if (c->monitor_addr != addr) {
                c->monitor_status = MONITOR_UNARMED;
                return 0;
            }
            // new_val, old_val
            // todo: clarify if barrier is invoked on failure and ord_failure needs to be set
            if ((err = sl_core_mem_atomic(&c->core, addr, 4, IO_OP_ATOMIC_CAS, c->r[inst.r.rs2], c->monitor_value, &result, ord, ord))) return err;
            if (rd != RV_ZERO) {
                c->r[rd] = (u4)result;
                RV_TRACE_RD(c, rd, c->r[rd]);
            }
            c->monitor_status = MONITOR_UNARMED;
            break;

        case 0b00001: // AMOSWAP.W
            RV_TRACE_PRINT(c, "amoswap.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SWAP, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b00000: // AMOADD.W
            RV_TRACE_PRINT(c, "amoadd.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_ADD, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b00100: // AMOXOR.W
            RV_TRACE_PRINT(c, "amoxor.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_XOR, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b01100: // AMOAND.W
            RV_TRACE_PRINT(c, "amoand.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_AND, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b01000: // AMOOR.W
            RV_TRACE_PRINT(c, "amoor.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_OR, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b10000: // AMOMIN.W
            RV_TRACE_PRINT(c, "amomin.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SMIN, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b10100: // AMOMAX.W
            RV_TRACE_PRINT(c, "amomax.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SMAX, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b11000: // AMOMINU.W
            RV_TRACE_PRINT(c, "amominu.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_UMIN, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b11100: // AMOMAXU.W
            RV_TRACE_PRINT(c, "amomaxu.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_UMAX, c->r[inst.r.rs2], rd, ord);
            break;

        default: goto undef;
        }
        return 0;
    }

    if (inst.r.funct3 == 0b011) {
        if (c->mode != RV_MODE_RV64) goto undef;

        switch (op) {
        case 0b00010: { // LR.D
            if (inst.r.rs2 != 0) goto undef;
            RV_TRACE_PRINT(c, "lr.d%s x%u, (x%u)", bstr, rd, inst.r.rs1);
            c->monitor_addr = addr;
            c->monitor_status = MONITOR_UNARMED;
            if (barrier & 1) atomic_thread_fence(memory_order_release);
            u8 d;
            if ((err = sl_core_mem_read(&c->core, addr, 8, 1, &d))) break;
            if (barrier & 2) atomic_thread_fence(memory_order_acquire);
            c->monitor_value = d;
            c->monitor_status = MONITOR_ARMED64;
            if (rd != RV_ZERO) {
                c->r[rd] = d;
                RV_TRACE_RD(c, rd, c->r[rd]);
            }
            break;
        }

        case 0b00011: // SC.D
            RV_TRACE_PRINT(c, "sc.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            if (c->monitor_status != MONITOR_ARMED64) {
                c->monitor_status = MONITOR_UNARMED;
                return 0;
            }
            if (c->monitor_addr != addr) {
                c->monitor_status = MONITOR_UNARMED;
                return 0;
            }
            if ((err = sl_core_mem_atomic(&c->core, addr, 8, IO_OP_ATOMIC_CAS, c->r[inst.r.rs2], c->monitor_value, &result, ord, ord))) return err;
            if (rd != RV_ZERO) {
                c->r[rd] = result;
                RV_TRACE_RD(c, rd, c->r[rd]);
            }
            c->monitor_status = MONITOR_UNARMED;
            break;

        case 0b00001: // AMOSWAP.D
            RV_TRACE_PRINT(c, "amoswap.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SWAP, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b00000: // AMOADD.D
            RV_TRACE_PRINT(c, "amoadd.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_ADD, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b00100: // AMOXOR.D
            RV_TRACE_PRINT(c, "amoxor.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_XOR, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b01100: // AMOAND.D
            RV_TRACE_PRINT(c, "amoand.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_AND, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b01000: // AMOOR.D
            RV_TRACE_PRINT(c, "amoor.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_OR, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b10000: // AMOMIN.D
            RV_TRACE_PRINT(c, "amomin.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SMIN, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b10100: // AMOMAX.D
            RV_TRACE_PRINT(c, "amomax.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SMAX, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b11000: // AMOMINU.D
            RV_TRACE_PRINT(c, "amominu.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_UMIN, c->r[inst.r.rs2], rd, ord);
            break;

        case 0b11100: // AMOMAXU.D
            RV_TRACE_PRINT(c, "amomaxu.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_UMAX, c->r[inst.r.rs2], rd, ord);
            break;

        default: goto undef;
        }
        return 0;
    }
undef:
    return rv_undef(c, inst);
}

int rv_exec_fp(rv_core_t *c, rv_inst_t inst) {
    rv_fp_reg_t result = {};
    fexcept_t flags;
    const u1 fmt = inst.r.funct7 & 3;
    const u1 rd = inst.r.rd;
    const u1 set_result = (1 << 0);
    const u1 set_flags = (1 << 1);
    const u1 set_result_and_flags = set_result | set_flags;

    u1 set_opts = set_result_and_flags;

    feclearexcept(FE_ALL_EXCEPT);

    if (fmt == 0) { // 32-bit single-precision
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;

        u8 uval;
        switch (inst.r.funct7 >> 2) {
        // 0000000 rs2     rs1  rm   rd  1010011  FADD.S
        case 0b00000:
            result.f = c->f[inst.r.rs1].f + c->f[inst.r.rs2].f;
            RV_TRACE_PRINT(c, "fadd.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        // 0000100 rs2     rs1  rm   rd  1010011  FSUB.S
        case 0b00001:
            result.f = c->f[inst.r.rs1].f - c->f[inst.r.rs2].f;
            RV_TRACE_PRINT(c, "fsub.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        // 0001000 rs2     rs1  rm   rd  1010011  FMUL.S
        case 0b00010:
            result.f = c->f[inst.r.rs1].f * c->f[inst.r.rs2].f;
            RV_TRACE_PRINT(c, "fmul.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        // 0001100 rs2     rs1  rm   rd  1010011  FDIV.S
        case 0b00011:
            result.f = c->f[inst.r.rs1].f / c->f[inst.r.rs2].f;
            RV_TRACE_PRINT(c, "fdiv.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        // 01011 size 00000   rs1  rm   rd  1010011  FSQRT.S
        case 0b01011:
            result.f = sqrtf(c->f[inst.r.rs1].f);
            RV_TRACE_PRINT(c, "fsqrt.s f%u, f%u", rd, inst.r.rs1);
            break;

        // 00100 size rs2     rs1  000  rd  1010011  FSGNJ.S
        // 00100 size rs2     rs1  001  rd  1010011  FSGNJN.S
        // 00100 size rs2     rs1  010  rd  1010011  FSGNJX.S
        case 0b00100:
            set_opts = set_result;
            result.u4 = c->f[inst.r.rs1].u4 & 0x7fffffff;
            switch (inst.r.funct3) {
            case 0b000:
                result.u4 |= (c->f[inst.r.rs2].u4 & 0x80000000);
                RV_TRACE_PRINT(c, "fsgnj.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
                break;

            case 0b001:
                result.u4 |= ((~c->f[inst.r.rs2].u4) & 0x80000000);
                RV_TRACE_PRINT(c, "fsgnjn.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
                break;

            case 0b010:
                result.u4 |= ((c->f[inst.r.rs1].u4 ^ c->f[inst.r.rs2].u4) & 0x80000000);
                RV_TRACE_PRINT(c, "fsgnjx.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
                break;

            default:    goto undef;
            }
            break;

        // 00101 size rs2     rs1  000  rd  1010011  FMIN.S
        // 00101 size rs2     rs1  001  rd  1010011  FMAX.S
        case 0b00101:
            switch (inst.r.funct3) {
            case 0b000:
                result.f = fminf(c->f[inst.r.rs1].f, c->f[inst.r.rs2].f);
                RV_TRACE_PRINT(c, "fmin.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
                break;

            case 0b001:
                result.f = fmaxf(c->f[inst.r.rs1].f, c->f[inst.r.rs2].f);
                RV_TRACE_PRINT(c, "fmaxf.s f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
                break;

            default:    goto undef;
            }
            break;

        case 0b11000:
            set_opts = set_flags;
            // todo: set correct rounding mode
            switch (inst.r.funct3) {
            // 11000 size 00000   rs1  rm   rd  1010011  FCVT.W.S
            case 0b00000:
                uval = (u4)(i4)c->f[inst.r.rs1].f;
                RV_TRACE_PRINT(c, "fcvt.w.s x%u, f%u", rd, inst.r.rs1);
                break;

            // 11000 size 00001   rs1  rm   rd  1010011  FCVT.WU.S
            case 0b00001:
                uval = (u4)c->f[inst.r.rs1].f;
                RV_TRACE_PRINT(c, "fcvt.wu.s x%u, f%u", rd, inst.r.rs1);
                break;

            // 11000 size 00010   rs1  rm   rd  1010011  FCVT.L.S
            case 0b00010:
                if (c->mode != RV_MODE_RV64) goto undef;
                uval = (u8)(i8)c->f[inst.r.rs1].f;
                RV_TRACE_PRINT(c, "fcvt.l.s x%u, f%u", rd, inst.r.rs1);
                break;

            // 11000 size 00011   rs1  rm   rd  1010011  FCVT.LU.S
            case 0b11000:
                if (c->mode != RV_MODE_RV64) goto undef;
                uval = (u8)c->f[inst.r.rs1].f;
                RV_TRACE_PRINT(c, "fcvt.lu.s x%u, f%u", rd, inst.r.rs1);
                break;

            default:    goto undef;
            }
            if (rd != RV_ZERO) {
                c->r[rd] = uval;
                RV_TRACE_RD(c, rd, c->r[rd]);
            }
            break;

        // 10100 size rs2     rs1  010  rd  1010011  FEQ.S
        // 10100 size rs2     rs1  001  rd  1010011  FLT.S
        // 10100 size rs2     rs1  000  rd  1010011  FLE.S
        case 0b10100:
            switch (inst.r.funct3) {
            case 0b010:
                uval = c->f[inst.r.rs1].f == c->f[inst.r.rs2].f;
                RV_TRACE_PRINT(c, "feq.s x%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
                break;

            case 0b001:
                uval = isless(c->f[inst.r.rs1].f, c->f[inst.r.rs2].f);
                RV_TRACE_PRINT(c, "flt.s x%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
                break;

            case 0b000:
                uval = islessequal(c->f[inst.r.rs1].f, c->f[inst.r.rs2].f);
                RV_TRACE_PRINT(c, "fle.s x%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
                break;

            default:    goto undef;
            }
            if (rd != RV_ZERO) {
                c->r[rd] = uval ? 1 : 0;
                RV_TRACE_RD(c, rd, c->r[rd]);
            }
            set_opts = set_flags;
            break;

        // 11100 size 00000   rs1  000  rd  1010011  FMV.X.W
        // 11100 size 00000   rs1  001  rd  1010011  FCLASS.S
        case 0b11100:
            set_opts = 0;
            switch (inst.r.funct3) {
            case 0b000: {
                RV_TRACE_PRINT(c, "fmv.x.w x%u, f%u", rd, inst.r.rs1);
                if (rd == RV_ZERO) break;
                if (c->mode == RV_MODE_RV32) c->r[rd] = c->f[inst.r.rs1].u4;
                else                         c->r[rd] = (i8)(i4)c->f[inst.r.rs1].u4;
                RV_TRACE_RD(c, rd, c->r[rd]);
                break;
            }
            case 0b001: {
                RV_TRACE_PRINT(c, "fclass.s x%u, f%u", rd, inst.r.rs1);
                if (rd == RV_ZERO) break;
                u1 type = 0;
                int cl = fpclassify(c->f[inst.r.rs1].f);
                switch (cl) {
                case FP_INFINITE:   type = 0;   break;
                case FP_NORMAL:     type = 1;   break;
                case FP_SUBNORMAL:  type = 2;   break;
                case FP_ZERO:       type = 3;   break;
                case FP_NAN:        type = (c->f[inst.r.rs1].f == NAN) ? 9 : 8;   break;
                }
                if ((type < 8) && (signbit(c->f[inst.r.rs1].f) == 0)) type = 7 - type;
                c->r[rd] = (1u << type);
                RV_TRACE_RD(c, rd, c->r[rd]);
                break;
            }
            default:    goto undef;
            }
            break;

        case 0b11010:
            switch (inst.r.rs2) {
            // 11010 size 00000   rs1  rm   rd  1010011  FCVT.S.W
            case 0b00000:
                result.f = (float)(i4)c->r[inst.r.rs1];
                RV_TRACE_PRINT(c, "fcvt.s.w f%u, x%u", rd, inst.r.rs1);
                break;

            // 11010 size 00001   rs1  rm   rd  1010011  FCVT.S.WU
            case 0b00001:
                result.f = (float)(u4)c->r[inst.r.rs1];
                RV_TRACE_PRINT(c, "fcvt.s.wu f%u, x%u", rd, inst.r.rs1);
                break;

            // 11010 size 00010   rs1  rm   rd  1010011  FCVT.S.L
            case 0b00010:
                if (c->mode != RV_MODE_RV64) goto undef;
                result.f = (float)(i8)c->r[inst.r.rs1];
                RV_TRACE_PRINT(c, "fcvt.s.l f%u, x%u", rd, inst.r.rs1);
                break;

            // 11010 size 00011   rs1  rm   rd  1010011  FCVT.S.LU
            case 0b00011:
                if (c->mode != RV_MODE_RV64) goto undef;
                result.f = (float)c->r[inst.r.rs1];
                RV_TRACE_PRINT(c, "fcvt.s.lu f%u, x%u", rd, inst.r.rs1);
                break;

            default:    goto undef;
            }
            break;

        // 11110 size 00000   rs1  000  rd  1010011  FMV.W.X
        case 0b11110:
            if (inst.r.funct3 != 000) goto undef;
            result.u4 = (u4)c->r[inst.r.rs1];
            RV_TRACE_PRINT(c, "fmv.w.x f%u, x%u", rd, inst.r.rs1);
            set_opts = set_result;
            break;

        default: goto undef;
        }
    } else {
        // todo: implement
        goto undef;
        // if (fmt == 1) { // 64-bit double-precision
        //     if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        //     goto undef;
        // }
        // if (fmt == 2) // 16-bit half-precision
        // if (fmt == 3) // 128-bit quad-precision
    }

    if (set_opts & set_flags) {
        fegetexceptflag(&flags, FE_ALL_EXCEPT);
        c->fcsr |= flags;
    }
    if (set_opts & set_result) {
        c->f[rd].u8 = result.u8;
        RV_TRACE_RDF(c, rd, c->f[rd].f);
    }
    return 0;

undef:
    return rv_undef(c, inst);
}

int rv_exec_fp_mac(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    fexcept_t flags;

    if (inst.r4.fmt == 2) {
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;

        const float rs1 = c->f[inst.r4.rs1].f;
        const float rs2 = c->f[inst.r4.rs2].f;
        const float rs3 = c->f[inst.r4.funct5].f;
        float result;

        feclearexcept(FE_ALL_EXCEPT);

        switch (inst.r4.opcode) {
        // rs3 00 rs2 rs1 rm rd 1000011 FMADD.S
        case 0b1000011:
            RV_TRACE_OPSTR("fmadd.s");
            result = (rs1 * rs2) + rs3;
            break;

        // rs3 00 rs2 rs1 rm rd 1000111 FMSUB.S
        case 0b1000111:
            RV_TRACE_OPSTR("fmsub.s");
            result = (rs1 * rs2) - rs3;
            break;

        // rs3 00 rs2 rs1 rm rd 1001011 FNMSUB.S
        case 0b1001011:
            RV_TRACE_OPSTR("fnmsub.s");
            result = -(rs1 * rs2) + rs3; // yes, fnmsub adds and fnmadd subtracts
            break;

        // rs3 00 rs2 rs1 rm rd 1001111 FNMADD.S
        case 0b1001111:
            RV_TRACE_OPSTR("fnmadd.s");
            result = -(rs1 * rs2) - rs3;
            break;

        default:    goto undef;
        }

        fegetexceptflag(&flags, FE_ALL_EXCEPT);
        c->fcsr |= flags;
        c->f[inst.r4.rd].f = result;
        RV_TRACE_RDF(c, inst.r4.rd, result);
        RV_TRACE_PRINT(c, "%s f%u, f%u, f%u, f%u", opstr, inst.r4.rd, inst.r4.rs1, inst.r4.rs2, inst.r4.funct5);
        return 0;
    }

undef:
    return rv_undef(c, inst);
}

int rv_exec_fp_load(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    int err;

    const i4 imm = ((i4)inst.raw) >> 20;
    u8 addr = c->r[inst.i.rs1] + imm;
    if (c->mode == RV_MODE_RV32) addr &= 0xffffffff;
    rv_fp_reg_t val = {};

    // imm[11:0] rs1 010 rd 0000111 FLW
    // imm[11:0] rs1 011 rd 0000111 FLD
    // imm[11:0] rs1 100 rd 0000111 FLQ
    // imm[11:0] rs1 001 rd 0000111 FLH
    switch (inst.r.funct3) {
    case 0b010:
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        RV_TRACE_OPSTR("flw");
        err = sl_core_mem_read(&c->core, addr, 4, 1, &val.u4);
        RV_TRACE_RDF(c, inst.i.rd, val.f);
        break;

    case 0b011:
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        RV_TRACE_OPSTR("fld");
        err = sl_core_mem_read(&c->core, addr, 8, 1, &val.u8);
        RV_TRACE_RDD(c, inst.i.rd, val.d);
        break;

    case 0b100:
    case 0b001:
    default:    goto undef;
    }

    if (err) {
        RV_TRACE_PRINT(c, "%s f%u, %d(x%u)            ; [%" PRIx64 "] = %s",
            opstr, inst.i.rd, imm, inst.i.rs1, addr, st_err(err));
        return rv_synchronous_exception(c, EX_ABORT_LOAD, addr, err);
    }
    c->f[inst.i.rd].u8 = val.u8;
    RV_TRACE_PRINT(c, "%s f%u, %d(x%u)", opstr, inst.i.rd, imm, inst.i.rs1);
    return 0;

undef:
    return rv_undef(c, inst);
}

int rv_exec_fp_store(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const i4 imm = (((i4)inst.raw >> 20) & ~(0x1f)) | inst.s.imm1;
    u8 addr = c->r[inst.s.rs1] + imm;
    if (c->mode == RV_MODE_RV32) addr &= 0xffffffff;
    rv_fp_reg_t val;
    int err = 0;

    //imm[11:5] rs2 rs1 010 imm[4:0] 0100111 FSW
    //imm[11:5] rs2 rs1 011 imm[4:0] 0100111 FSD
    switch (inst.s.funct3) {
    case 0b010:
        RV_TRACE_OPSTR("fsw");
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        val.u4 = c->f[inst.s.rs2].u4;
        err = sl_core_mem_write(&c->core, addr, 4, 1, &val.u4);
        RV_TRACE_STORE_F(c, addr, inst.s.rs2, val.f);
        break;

    case 0b011:
        RV_TRACE_OPSTR("fsd");
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        val.u8 = c->f[inst.s.rs2].u8;
        err = sl_core_mem_write(&c->core, addr, 8, 1, &val.u8);
        RV_TRACE_STORE_D(c, addr, inst.s.rs2, val.d);
        break;

    default:    goto undef;
    }

    if (err) {
        RV_TRACE_PRINT(c, "%s f%u, %d(x%u)            ; [%" PRIx64 "] = %s",
            opstr, inst.s.rs2, imm, inst.i.rs1, addr, st_err(err));
        return rv_synchronous_exception(c, EX_ABORT_STORE, addr, err);
    }
    RV_TRACE_PRINT(c, "%s f%u, %d(x%u)", opstr, inst.s.rs2, imm, inst.s.rs1);
    return 0;

undef:
    return rv_undef(c, inst);
}

int rv_exec_ebreak(rv_core_t *c) {
    if (c->core.options & SL_CORE_OPT_TRAP_BREAKPOINT)
        return SL_ERR_BREAKPOINT;
    // todo: debugger exception
    return SL_ERR_UNIMPLEMENTED;
}

// Format is the same as I type
int rv_exec_system(rv_core_t *c, rv_inst_t inst) {
    int err = 0;
    if (inst.r.funct3 == 0b000) {
        if (inst.r.rd != 0) goto undef;
        switch (inst.r.funct7) {
        case 0b0000000:
            if (inst.r.rs1 == 0) {
                if (inst.r.rs2 == 0) {  // ECALL
                    RV_TRACE_PRINT(c, "ecall");
                    return rv_synchronous_exception(c, EX_SYSCALL, inst.raw, 0);
                } else if (inst.r.rs2 == 1) {   // EBREAK
                    RV_TRACE_PRINT(c, "ebreak");
                    return rv_exec_ebreak(c);
                }
            }
            goto undef;

        case 0b0011000: // MRET
            RV_TRACE_PRINT(c, "mret");
            if (c->pl != RV_PL_MACHINE) goto undef;
            err = rv_exception_return(c, RV_OP_MRET);
            if (!err) RV_TRACE_RD(c, SL_CORE_REG_PC, c->pc);
            return err;

        case 0b0001000:
            if (inst.r.rs2 == 0b00010) {
                RV_TRACE_PRINT(c, "sret");
                if (c->pl < RV_PL_SUPERVISOR) goto undef;
                err = rv_exception_return(c, RV_OP_SRET);
                if (!err) RV_TRACE_RD(c, SL_CORE_REG_PC, c->pc);
                return err;
            }
            if (inst.r.rs2 == 0b00101) { // WFI
                if (c->pl == RV_PL_USER) goto undef;
                RV_TRACE_PRINT(c, "wfi");
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
    }
    if (inst.r.funct3 == 0b100) {
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
    }

    // CSR instruction

    const u4 csr_op = inst.i.funct3;
    RV_TRACE_DECL_OPSTR;

    switch (csr_op) {
    case 1: RV_TRACE_OPSTR("csrrw"); break;
    case 2: RV_TRACE_OPSTR("csrrs"); break;
    case 3: RV_TRACE_OPSTR("csrrc"); break;
    case 5: RV_TRACE_OPSTR("csrrwi"); break;
    case 6: RV_TRACE_OPSTR("csrrsi"); break;
    case 7: RV_TRACE_OPSTR("csrrci"); break;
    default: goto undef;
    }

    u8 value;
    int op = (csr_op & 3);
    const bool csr_imm = csr_op & 0b100;
    if (csr_imm) value = inst.i.rs1; // treat rs1 as immediate value
    else value = c->r[inst.i.rs1];

    if (op == RV_CSR_OP_SWAP) {
        if (inst.i.rd == 0) op = RV_CSR_OP_WRITE;
    } else {
        if (value == 0) op = RV_CSR_OP_READ;
    }

    const u4 csr_addr = inst.i.imm;
#ifdef RV_TRACE
    const char *name = c->ext.name_for_sysreg(c, csr_addr);
    char namebuf[16];
    if (name == NULL) {
        snprintf(namebuf, 16, "%#x", csr_addr);
        name = namebuf;
    }
    if (csr_imm) RV_TRACE_PRINT(c, "%s x%u, %u, %s", opstr, inst.i.rd, inst.i.rs1, name);
    else RV_TRACE_PRINT(c, "%s x%u, x%u, %s", opstr, inst.i.rd, inst.i.rs1, name);
#endif

    result64_t result = rv_csr_op(c, op, csr_addr, value);
    if (result.err == SL_ERR_UNDEF) return rv_undef(c, inst);
    if (result.err == SL_ERR_UNIMPLEMENTED) {
        printf("unimplemented CSR access %#x\n", csr_addr);
        assert(false);
        return SL_ERR_UNIMPLEMENTED;
    }
    if (result.err != 0) {
        printf("unexpected CSR access error %x: %s\n", csr_addr, st_err(result.err));
        assert(false);
        return result.err;
    }

    if (inst.i.rd != RV_ZERO) {
        if (c->mode == RV_MODE_RV32) result.value = (u4)result.value;
        c->r[inst.i.rd] = result.value;
    }

#ifdef RV_TRACE
    RV_TRACE_RD(c, inst.i.rd, c->r[inst.i.rd]);
    if (op != RV_CSR_OP_READ) {
        c->core.trace->options |= ITRACE_OPT_SYSREG;
        c->core.trace->addr = csr_addr;
        c->core.trace->aux_value = value;
    }
#endif // RV_TRACE


    return 0;

undef:
    return rv_undef(c, inst);
}

int rv_dispatch(rv_core_t *c, u4 instruction) {
    rv_inst_t inst;
    inst.raw = instruction;
    int err;

#if RV_TRACE
    itrace_t tr;
    tr.pc = c->pc;
    tr.sp = c->r[RV_SP];
    tr.opcode = instruction;
    tr.options = 0;
    tr.pl = c->pl;
    tr.rd = RV_ZERO;
    tr.cur = 0;
    tr.opstr[0] = 0;
    c->core.trace = &tr;
#endif

    if (c->mode == RV_MODE_RV32) err = rv32_dispatch(c, inst);
    else                         err = rv64_dispatch(c, inst);

#if RV_TRACE
    {
        #define BUFLEN 256
        char buf[BUFLEN];
        static const char pl_char[4] = { 'u', 's', 'h', 'm' };

        int len = snprintf(buf, BUFLEN, "[%c] %10" PRIx64 "  ", pl_char[tr.pl], tr.pc);
        if (tr.options & ITRACE_OPT_INST16)
            len += snprintf(buf + len, BUFLEN - len, "%04x      ", tr.opcode);
        else
            len += snprintf(buf + len, BUFLEN - len, "%08x  ", tr.opcode);

        len += snprintf(buf + len, BUFLEN - len, "%-30s", tr.opstr);
        if ((tr.options & ~ITRACE_OPT_INST16) == 0) {
            if (tr.rd != RV_ZERO)
                len += snprintf(buf + len, BUFLEN - len, "; %s=%#" PRIx64, rv_name_for_reg(tr.rd), tr.rd_value);
            goto trace_done;
        }
        if (tr.options & ITRACE_OPT_INST_STORE) {
            if (tr.options & ITRACE_OPT_FLOAT)
                len += snprintf(buf + len, BUFLEN - len, "; [%#" PRIx64 "] = %g", tr.addr, tr.f_value);
            else if (tr.options & ITRACE_OPT_DOUBLE)
                len += snprintf(buf + len, BUFLEN - len, "; [%#" PRIx64 "] = %g", tr.addr, tr.d_value);
            else
                len += snprintf(buf + len, BUFLEN - len, "; [%#" PRIx64 "] = %#" PRIx64, tr.addr, tr.rd_value);
            goto trace_done;
        }
        if (tr.options & ITRACE_OPT_SYSREG) {
            if (tr.rd == RV_ZERO)
                len += snprintf(buf + len, BUFLEN - len, ";");
            else
                len += snprintf(buf + len, BUFLEN - len, "; %s=%#" PRIx64, rv_name_for_reg(tr.rd), tr.rd_value);
            const char *n = c->ext.name_for_sysreg(c, tr.addr);
            if (n == NULL)
                len += snprintf(buf + len, BUFLEN - len, " csr(%#x) = %#" PRIx64, (u4)tr.addr, tr.aux_value);
            else
                len += snprintf(buf + len, BUFLEN - len, " %s = %#" PRIx64, n, tr.aux_value);
            goto trace_done;
        }
        if (tr.options & ITRACE_OPT_FLOAT) {
            len += snprintf(buf + len, BUFLEN - len, "; f%u=%g", tr.rd, tr.f_value);
            goto trace_done;
        }

trace_done:
        puts(buf);
#if WITH_SYMBOLS
        if (c->jump_taken) {
            sym_entry_t *e = core_get_sym_for_addr(&c->core, c->pc);
            if (e != NULL) {
                u8 dist = c->pc - e->addr;
                printf("<%s+%#"PRIx64">:\n", e->name, dist);
            }
        }
#endif
    }
#endif

    return err;
}

