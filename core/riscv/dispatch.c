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

int rv_fp32_exec_fp(rv_core_t *c, rv_inst_t inst);
int rv_fp64_exec_fp(rv_core_t *c, rv_inst_t inst);

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
        sl_core_memory_barrier(&c->core, bar);
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
        sl_core_instruction_barrier(&c->core);
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
    c->core.monitor_status = MONITOR_UNARMED;
    if ((err = sl_core_mem_atomic(&c->core, addr, 4, op, operand, 0, &result, ord, memory_order_relaxed))) return err;
    if (rd != RV_ZERO) {
        c->core.r[rd] = (u4)result;
        RV_TRACE_RD(c, rd, c->core.r[rd]);
    }
    return 0;
}

static int rv_atomic_alu64(rv_core_t *c, u8 addr, u1 op, u4 operand, u1 rd, u1 ord) {
    u8 result;
    int err;
    c->core.monitor_status = MONITOR_UNARMED;
    if ((err = sl_core_mem_atomic(&c->core, addr, 8, op, operand, 0, &result, ord, memory_order_relaxed))) return err;
    if (rd != RV_ZERO) {
        c->core.r[rd] = result;
        RV_TRACE_RD(c, rd, c->core.r[rd]);
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
    const u8 addr = c->core.r[inst.r.rs1];
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
            c->core.monitor_addr = addr;
            c->core.monitor_status = MONITOR_UNARMED;
            if (barrier & 1) atomic_thread_fence(memory_order_release);
            u4 w;
            if ((err = sl_core_mem_read(&c->core, addr, 4, 1, &w))) break;
            if (barrier & 2) atomic_thread_fence(memory_order_acquire);
            c->core.monitor_value = w;
            c->core.monitor_status = MONITOR_ARMED32;
            if (rd != RV_ZERO) {
                c->core.r[rd] = w;
                RV_TRACE_RD(c, rd, c->core.r[rd]);
            }
            break;
        }

        case 0b00011: // SC.W
            RV_TRACE_PRINT(c, "sc.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            if (c->core.monitor_status != MONITOR_ARMED32) {
                c->core.monitor_status = MONITOR_UNARMED;
                return 0;
            }
            if (c->core.monitor_addr != addr) {
                c->core.monitor_status = MONITOR_UNARMED;
                return 0;
            }
            // new_val, old_val
            // todo: clarify if barrier is invoked on failure and ord_failure needs to be set
            if ((err = sl_core_mem_atomic(&c->core, addr, 4, IO_OP_ATOMIC_CAS, c->core.r[inst.r.rs2], c->core.monitor_value, &result, ord, ord))) return err;
            if (rd != RV_ZERO) {
                c->core.r[rd] = (u4)result;
                RV_TRACE_RD(c, rd, c->core.r[rd]);
            }
            c->core.monitor_status = MONITOR_UNARMED;
            break;

        case 0b00001: // AMOSWAP.W
            RV_TRACE_PRINT(c, "amoswap.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SWAP, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b00000: // AMOADD.W
            RV_TRACE_PRINT(c, "amoadd.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_ADD, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b00100: // AMOXOR.W
            RV_TRACE_PRINT(c, "amoxor.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_XOR, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b01100: // AMOAND.W
            RV_TRACE_PRINT(c, "amoand.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_AND, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b01000: // AMOOR.W
            RV_TRACE_PRINT(c, "amoor.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_OR, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b10000: // AMOMIN.W
            RV_TRACE_PRINT(c, "amomin.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SMIN, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b10100: // AMOMAX.W
            RV_TRACE_PRINT(c, "amomax.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SMAX, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b11000: // AMOMINU.W
            RV_TRACE_PRINT(c, "amominu.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_UMIN, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b11100: // AMOMAXU.W
            RV_TRACE_PRINT(c, "amomaxu.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_UMAX, c->core.r[inst.r.rs2], rd, ord);
            break;

        default: goto undef;
        }
        return 0;
    }

    if (inst.r.funct3 == 0b011) {
        if (c->core.mode != SL_CORE_MODE_64) goto undef;

        switch (op) {
        case 0b00010: { // LR.D
            if (inst.r.rs2 != 0) goto undef;
            RV_TRACE_PRINT(c, "lr.d%s x%u, (x%u)", bstr, rd, inst.r.rs1);
            c->core.monitor_addr = addr;
            c->core.monitor_status = MONITOR_UNARMED;
            if (barrier & 1) atomic_thread_fence(memory_order_release);
            u8 d;
            if ((err = sl_core_mem_read(&c->core, addr, 8, 1, &d))) break;
            if (barrier & 2) atomic_thread_fence(memory_order_acquire);
            c->core.monitor_value = d;
            c->core.monitor_status = MONITOR_ARMED64;
            if (rd != RV_ZERO) {
                c->core.r[rd] = d;
                RV_TRACE_RD(c, rd, c->core.r[rd]);
            }
            break;
        }

        case 0b00011: // SC.D
            RV_TRACE_PRINT(c, "sc.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            if (c->core.monitor_status != MONITOR_ARMED64) {
                c->core.monitor_status = MONITOR_UNARMED;
                return 0;
            }
            if (c->core.monitor_addr != addr) {
                c->core.monitor_status = MONITOR_UNARMED;
                return 0;
            }
            if ((err = sl_core_mem_atomic(&c->core, addr, 8, IO_OP_ATOMIC_CAS, c->core.r[inst.r.rs2], c->core.monitor_value, &result, ord, ord))) return err;
            if (rd != RV_ZERO) {
                c->core.r[rd] = result;
                RV_TRACE_RD(c, rd, c->core.r[rd]);
            }
            c->core.monitor_status = MONITOR_UNARMED;
            break;

        case 0b00001: // AMOSWAP.D
            RV_TRACE_PRINT(c, "amoswap.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SWAP, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b00000: // AMOADD.D
            RV_TRACE_PRINT(c, "amoadd.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_ADD, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b00100: // AMOXOR.D
            RV_TRACE_PRINT(c, "amoxor.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_XOR, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b01100: // AMOAND.D
            RV_TRACE_PRINT(c, "amoand.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_AND, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b01000: // AMOOR.D
            RV_TRACE_PRINT(c, "amoor.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_OR, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b10000: // AMOMIN.D
            RV_TRACE_PRINT(c, "amomin.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SMIN, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b10100: // AMOMAX.D
            RV_TRACE_PRINT(c, "amomax.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SMAX, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b11000: // AMOMINU.D
            RV_TRACE_PRINT(c, "amominu.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_UMIN, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b11100: // AMOMAXU.D
            RV_TRACE_PRINT(c, "amomaxu.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_UMAX, c->core.r[inst.r.rs2], rd, ord);
            break;

        default: goto undef;
        }
        return 0;
    }
undef:
    return rv_undef(c, inst);
}

int rv_exec_fp(rv_core_t *c, rv_inst_t inst) {
    const u1 fmt = inst.r.funct7 & 3;

    switch(fmt) {
    case 0b00:  // 32-bit single-precision
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        return rv_fp32_exec_fp(c, inst);

    case 0b01:
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        return rv_fp64_exec_fp(c, inst);

    case 0b10: // 16-bit half-precision
    case 0b11: // 128-bit quad-precision
    default:
        goto undef;
    }

undef:
    return rv_undef(c, inst);
}

int rv_exec_fp_mac(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    fexcept_t flags;

    if (inst.r4.fmt == 0b10) {
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;

        const float rs1 = c->core.f[inst.r4.rs1].f;
        const float rs2 = c->core.f[inst.r4.rs2].f;
        const float rs3 = c->core.f[inst.r4.funct5].f;
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
        c->core.fexc |= flags;
        c->core.f[inst.r4.rd].f = result;
        RV_TRACE_RDF(c, inst.r4.rd, result);
        RV_TRACE_PRINT(c, "%s f%u, f%u, f%u, f%u", opstr, inst.r4.rd, inst.r4.rs1, inst.r4.rs2, inst.r4.funct5);
        return 0;
    } else if (inst.r4.fmt == 0b01) {
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;

        const double rs1 = c->core.f[inst.r4.rs1].d;
        const double rs2 = c->core.f[inst.r4.rs2].d;
        const double rs3 = c->core.f[inst.r4.funct5].d;
        double result;

        feclearexcept(FE_ALL_EXCEPT);

        switch (inst.r4.opcode) {
        // rs3 00 rs2 rs1 rm rd 1000011 FMADD.D
        case 0b1000011:
            RV_TRACE_OPSTR("fmadd.d");
            result = (rs1 * rs2) + rs3;
            break;

        // rs3 00 rs2 rs1 rm rd 1000111 FMSUB.D
        case 0b1000111:
            RV_TRACE_OPSTR("fmsub.d");
            result = (rs1 * rs2) - rs3;
            break;

        // rs3 00 rs2 rs1 rm rd 1001011 FNMSUB.D
        case 0b1001011:
            RV_TRACE_OPSTR("fnmsub.d");
            result = -(rs1 * rs2) + rs3; // yes, fnmsub adds and fnmadd subtracts
            break;

        // rs3 00 rs2 rs1 rm rd 1001111 FNMADD.D
        case 0b1001111:
            RV_TRACE_OPSTR("fnmadd.d");
            result = -(rs1 * rs2) - rs3;
            break;

        default:    goto undef;
        }

        fegetexceptflag(&flags, FE_ALL_EXCEPT);
        c->core.fexc |= flags;
        c->core.f[inst.r4.rd].d = result;
        RV_TRACE_RDD(c, inst.r4.rd, result);
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
    u8 addr = c->core.r[inst.i.rs1] + imm;
    if (c->core.mode == SL_CORE_MODE_32) addr &= 0xffffffff;
    sl_fp_reg_t val = {};

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
    c->core.f[inst.i.rd].u8 = val.u8;
    RV_TRACE_PRINT(c, "%s f%u, %d(x%u)", opstr, inst.i.rd, imm, inst.i.rs1);
    return 0;

undef:
    return rv_undef(c, inst);
}

int rv_exec_fp_store(rv_core_t *c, rv_inst_t inst) {
    RV_TRACE_DECL_OPSTR;
    const i4 imm = (((i4)inst.raw >> 20) & ~(0x1f)) | inst.s.imm1;
    u8 addr = c->core.r[inst.s.rs1] + imm;
    if (c->core.mode == SL_CORE_MODE_32) addr &= 0xffffffff;
    sl_fp_reg_t val;
    int err = 0;

    //imm[11:5] rs2 rs1 010 imm[4:0] 0100111 FSW
    //imm[11:5] rs2 rs1 011 imm[4:0] 0100111 FSD
    switch (inst.s.funct3) {
    case 0b010:
        RV_TRACE_OPSTR("fsw");
        if ((c->core.arch_options & SL_RISCV_EXT_F) == 0) goto undef;
        val.u4 = c->core.f[inst.s.rs2].u4;
        err = sl_core_mem_write(&c->core, addr, 4, 1, &val.u4);
        RV_TRACE_STORE_F(c, addr, inst.s.rs2, val.f);
        break;

    case 0b011:
        RV_TRACE_OPSTR("fsd");
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        val.u8 = c->core.f[inst.s.rs2].u8;
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
            if (c->core.el != SL_CORE_EL_MONITOR) goto undef;
            err = rv_exception_return(c, RV_OP_MRET);
            if (!err) RV_TRACE_RD(c, SL_CORE_REG_PC, c->pc);
            return err;

        case 0b0001000:
            if (inst.r.rs2 == 0b00010) {
                RV_TRACE_PRINT(c, "sret");
                if (c->core.el < SL_CORE_EL_SUPERVISOR) goto undef;
                err = rv_exception_return(c, RV_OP_SRET);
                if (!err) RV_TRACE_RD(c, SL_CORE_REG_PC, c->pc);
                return err;
            }
            if (inst.r.rs2 == 0b00101) { // WFI
                if (c->core.el == SL_CORE_EL_USER) goto undef;
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
    else value = c->core.r[inst.i.rs1];

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
        if (c->core.mode == SL_CORE_MODE_32) result.value = (u4)result.value;
        c->core.r[inst.i.rd] = result.value;
    }

#ifdef RV_TRACE
    RV_TRACE_RD(c, inst.i.rd, c->core.r[inst.i.rd]);
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
    tr.sp = c->core.r[RV_SP];
    tr.opcode = instruction;
    tr.options = 0;
    tr.pl = c->core.el;
    tr.rd = RV_ZERO;
    tr.cur = 0;
    tr.opstr[0] = 0;
    c->core.trace = &tr;
#endif

    if (c->core.mode == SL_CORE_MODE_32) err = rv32_dispatch(c, inst);
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
            sl_sym_entry_t *e = sl_core_get_sym_for_addr(&c->core, c->pc);
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

