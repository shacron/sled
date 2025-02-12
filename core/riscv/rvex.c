// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include <core/riscv/csr.h>
#include <core/riscv/inst.h>
#include <core/riscv/rv.h>
#include <sled/error.h>

rv_sr_pl_t* rv_get_pl_csrs(rv_core_t *c, u1 pl) {
    assert(c->core.el != 0);
    return &c->sr_pl[pl - 1];
}

int riscv_core_exception_enter(sl_core_t *core, u8 cause, u8 addr) {
    rv_core_t *c = (rv_core_t *)core;

    // todo: check medeleg / mideleg for priv level dispatching

    u4 rv_fault = 0;
    switch ((u4)cause) {
    case EX_SYSCALL:
        rv_fault = RV_EX_CALL_FROM_U + c->core.el;
        break;
    case EX_UNDEFINDED:         rv_fault = RV_EX_INST_ILLEGAL;  break;
    case EX_ABORT_LOAD:         rv_fault = RV_EX_LOAD_FAULT;    break;
    case EX_ABORT_LOAD_ALIGN:   rv_fault = RV_EX_LOAD_ALIGN;    break;
    case EX_ABORT_STORE:        rv_fault = RV_EX_STORE_FAULT;   break;
    case EX_ABORT_STORE_ALIGN:  rv_fault = RV_EX_STORE_ALIGN;   break;
    case EX_ABORT_INST:         rv_fault = RV_EX_INST_FAULT;    break;
    case EX_ABORT_INST_ALIGN:   rv_fault = RV_EX_INST_ALIGN;    break;
    default:
        assert(false);
        return SL_ERR_UNIMPLEMENTED;
    }
    cause = (cause & RV_CAUSE64_INT) | rv_fault;

    rv_sr_pl_t *r = rv_get_pl_csrs(c, SL_CORE_EL_MONITOR);
    r->cause = cause;
    r->epc = c->core.pc;
    r->tval = addr;

    // update status register
    csr_status_t s;
    s.raw = c->status;
    s.m_mpie = s.m_mie;             // previous interrupt state
    s.m_mie = 0;                    // disable interrupts
    s.m_mpp = c->core.el;                // previous priv level
    c->status = s.raw;

    c->core.el = SL_CORE_EL_MONITOR;          // todo: fix me

    u8 tvec = r->tvec;
    if (cause & RV_CAUSE64_INT) {
        const u8 ci = cause & ~RV_CAUSE64_INT;
        // m->ip = 1ull << ci;
        if (tvec & 1) tvec += (ci << 2);
    }
    c->core.pc = tvec;
    c->core.branch_taken = true;
    sl_core_interrupt_set(&c->core, false);
    return 0;
}

int rv_exception_return(rv_core_t *c, u1 op) {
    bool int_enabled = true;
    u1 dest_pl;
    csr_status_t s;
    s.raw = c->status;

    if (op == RV_OP_MRET) {
        // mret
        dest_pl = s.m_mpp;
        s.m_mie = s.m_mpie;
        s.m_mpie = 1;
        s.m_mpp = 0;
        int_enabled = s.m_mie;
    } else if (op == RV_OP_SRET) {
        if (s.m_tsr) return SL_ERR_UNDEF;   // trap sret
        dest_pl = s.spp;
        s.sie = s.spie;
        s.spp = 0;
        int_enabled = s.sie;
    } else {
        return SL_ERR_UNIMPLEMENTED;
    }
    c->status = s.raw;

    rv_sr_pl_t *r = rv_get_pl_csrs(c, c->core.el);
    c->core.pc = r->epc;
    c->core.el = dest_pl;
    c->core.branch_taken = true;

    sl_core_interrupt_set(&c->core, int_enabled);
    return 0;
}
