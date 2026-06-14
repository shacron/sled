// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2026 Shac Ron and The Sled Project

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
#include <core/sym.h>
#include <sled/arch.h>
#include <sled/error.h>
#include <sled/io.h>

static int rv_atomic_alu32(rv_core_t *c, u8 addr, u1 op, u4 operand, u1 rd, u1 ord) {
    u8 result;
    int err;
    c->core.monitor_status = MONITOR_UNARMED;
    if ((err = sl_core_mem_atomic(&c->core, addr, 4, op, operand, 0, &result, ord, memory_order_relaxed))) return err;
    if (rd != RV_ZERO) {
        c->core.r[rd] = (u4)result;
        // RV_TRACE_RD(c, rd, c->core.r[rd]);
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
        // RV_TRACE_RD(c, rd, c->core.r[rd]);
    }
    return 0;
}

static int rv_exec_atomic(rv_core_t *c, rv_inst_t inst) {
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

    if (inst.r.funct3 == 0b010) {
        if (addr & 3) return sl_core_synchronous_exception(&c->core, EX_ABORT_LOAD, addr, SL_ERR_IO_ALIGN);

        switch (op) {
        case 0b00011: // SC.W
            // RV_TRACE_PRINT(c, "sc.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            if (c->core.monitor_status != MONITOR_ARMED4) {
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
                // RV_TRACE_RD(c, rd, c->core.r[rd]);
            }
            c->core.monitor_status = MONITOR_UNARMED;
            break;

        case 0b00001: // AMOSWAP.W
            // RV_TRACE_PRINT(c, "amoswap.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SWAP, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b00000: // AMOADD.W
            // RV_TRACE_PRINT(c, "amoadd.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_ADD, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b00100: // AMOXOR.W
            // RV_TRACE_PRINT(c, "amoxor.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_XOR, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b01100: // AMOAND.W
            // RV_TRACE_PRINT(c, "amoand.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_AND, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b01000: // AMOOR.W
            // RV_TRACE_PRINT(c, "amoor.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_OR, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b10000: // AMOMIN.W
            // RV_TRACE_PRINT(c, "amomin.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SMIN, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b10100: // AMOMAX.W
            // RV_TRACE_PRINT(c, "amomax.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_SMAX, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b11000: // AMOMINU.W
            // RV_TRACE_PRINT(c, "amominu.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_UMIN, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b11100: // AMOMAXU.W
            // RV_TRACE_PRINT(c, "amomaxu.w%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu32(c, addr, IO_OP_ATOMIC_UMAX, c->core.r[inst.r.rs2], rd, ord);
            break;

        default: goto undef;
        }
        return 0;
    }

    if (inst.r.funct3 == 0b011) {
        if (c->core.mode != SL_CORE_MODE_8) goto undef;

        switch (op) {
        case 0b00010: { // LR.D
            if (inst.r.rs2 != 0) goto undef;
            // RV_TRACE_PRINT(c, "lr.d%s x%u, (x%u)", bstr, rd, inst.r.rs1);
            c->core.monitor_addr = addr;
            c->core.monitor_status = MONITOR_UNARMED;
            if (barrier & 1) atomic_thread_fence(memory_order_release);
            u8 d;
            if ((err = sl_core_mem_read(&c->core, addr, 8, 1, &d))) break;
            if (barrier & 2) atomic_thread_fence(memory_order_acquire);
            c->core.monitor_value = d;
            c->core.monitor_status = MONITOR_ARMED8;
            if (rd != RV_ZERO) {
                c->core.r[rd] = d;
                // RV_TRACE_RD(c, rd, c->core.r[rd]);
            }
            break;
        }

        case 0b00011: // SC.D
            // RV_TRACE_PRINT(c, "sc.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            if (c->core.monitor_status != MONITOR_ARMED8) {
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
                // RV_TRACE_RD(c, rd, c->core.r[rd]);
            }
            c->core.monitor_status = MONITOR_UNARMED;
            break;

        case 0b00001: // AMOSWAP.D
            // RV_TRACE_PRINT(c, "amoswap.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SWAP, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b00000: // AMOADD.D
            // RV_TRACE_PRINT(c, "amoadd.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_ADD, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b00100: // AMOXOR.D
            // RV_TRACE_PRINT(c, "amoxor.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_XOR, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b01100: // AMOAND.D
            // RV_TRACE_PRINT(c, "amoand.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_AND, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b01000: // AMOOR.D
            // RV_TRACE_PRINT(c, "amoor.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_OR, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b10000: // AMOMIN.D
            // RV_TRACE_PRINT(c, "amomin.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SMIN, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b10100: // AMOMAX.D
            // RV_TRACE_PRINT(c, "amomax.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_SMAX, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b11000: // AMOMINU.D
            // RV_TRACE_PRINT(c, "amominu.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_UMIN, c->core.r[inst.r.rs2], rd, ord);
            break;

        case 0b11100: // AMOMAXU.D
            // RV_TRACE_PRINT(c, "amomaxu.d%s x%u, x%u, (x%u)", bstr, rd, inst.r.rs2, inst.r.rs1);
            err = rv_atomic_alu64(c, addr, IO_OP_ATOMIC_UMAX, c->core.r[inst.r.rs2], rd, ord);
            break;

        default: goto undef;
        }
        return 0;
    }
undef:
    return rv_undef(c, inst);
}

static int rv_exec_ebreak(rv_core_t *c) {
    if (c->core.options & SL_CORE_OPT_TRAP_BREAKPOINT)
        return SL_ERR_BREAKPOINT;
    // todo: debugger exception
    return SL_ERR_UNIMPLEMENTED;
}

// Format is the same as I type
static int rv_exec_system(rv_core_t *c, rv_inst_t inst) {
    int err = 0;
    if (inst.r.funct3 == 0b000) {
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

    // CSR ops here: implemented in SLAC decoder

undef:
    return rv_undef(c, inst);
}

int rv_dispatch(sl_core_t *cc, u4 instruction) {
    rv_core_t *c = (rv_core_t *)cc;
    rv_inst_t inst;
    inst.raw = instruction;
    int err;

    // 16 bit compressed instructions unexpected here
    // they should all have been captured by the slac decoder
    if ((inst.u.opcode & 3) != 3)
        return rv_undef(c, inst);
    c->core.prev_len = 4;

    switch (inst.u.opcode) {
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

