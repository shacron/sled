// SPDX-License-Identifier: MIT License
// Copyright (c) 202-2023 Shac Ron and The Sled Project

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include <core/riscv/csr.h>
#include <core/riscv/inst.h>
#include <core/riscv/rv.h>
#include <sled/error.h>

static void rv_dump_core_state(rv_core_t *c) {
    printf("pc=%"PRIx64", sp=%"PRIx64", ra=%"PRIx64", ticks=%"PRIu64"\n", c->pc, c->r[RV_SP], c->r[RV_RA], c->core.ticks);
    for (u32 i = 0; i < 32; i += 4) {
        if (i < 10) printf(" ");
        printf("x%u: %16"PRIx64"  %16"PRIx64"  %16"PRIx64"  %16"PRIx64"\n", i, c->r[i], c->r[i+1], c->r[i+2], c->r[i+3]);
    }
}

rv_sr_pl_t* rv_get_pl_csrs(rv_core_t *c, u8 pl) {
    assert(c->pl != 0);
    return &c->sr_pl[pl - 1];
}

int rv_exception_enter(rv_core_t *c, u64 cause, u64 addr) {
    // todo: check medeleg / mideleg for priv level dispatching

    rv_sr_pl_t *r = rv_get_pl_csrs(c, RV_PL_MACHINE);
    r->cause = cause;
    r->epc = c->pc;
    r->tval = addr;

    // update status register
    csr_status_t s;
    s.raw = c->status;
    s.m_mpie = s.m_mie;             // previous interrupt state
    s.m_mie = 0;                    // disable interrupts
    s.m_mpp = c->pl;                // previous priv level
    c->status = s.raw;

    c->pl = RV_PL_MACHINE;          // todo: fix me

    u64 tvec = r->tvec;
    if (cause & RV_CAUSE64_INT) {
        const u64 ci = cause & ~RV_CAUSE64_INT;
        // m->ip = 1ull << ci;
        if (tvec & 1) tvec += (ci << 2);
    }
    c->pc = tvec;
    c->jump_taken = 1;
    core_interrupt_set(&c->core, false);
    return 0;
}

int rv_exception_return(rv_core_t *c, u8 op) {
    bool int_enabled = true;
    u8 dest_pl;
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

    rv_sr_pl_t *r = rv_get_pl_csrs(c, c->pl);
    c->pc = r->epc;
    c->pl = dest_pl;
    c->jump_taken = 1;

    core_interrupt_set(&c->core, int_enabled);
    return 0;
}

int rv_synchronous_exception(rv_core_t *c, core_ex_t ex, u64 value, u32 status) {
    rv_inst_t inst;

    // todo: check if nested exception

    switch (ex) {
    case EX_SYSCALL:
        if (c->core.options & SL_CORE_OPT_TRAP_SYSCALL) return SL_ERR_SYSCALL;
        return rv_exception_enter(c, RV_EX_CALL_FROM_U + c->pl, value);

    case EX_UNDEFINDED:
        if (c->core.options & SL_CORE_OPT_TRAP_UNDEF) {
            inst.raw = (u32)value;
            printf("UNDEFINED instruction %08x (op=%x) at pc=%" PRIx64 "\n", inst.raw, inst.b.opcode, c->pc);
            rv_dump_core_state(c);
            return SL_ERR_UNDEF;
        } else {
            return rv_exception_enter(c, RV_EX_INST_ILLEGAL, value);
        }

    case EX_ABORT_LOAD:
        if (c->core.options & SL_CORE_OPT_TRAP_ABORT) {
            printf("LOAD FAULT (rd) at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            rv_dump_core_state(c);
            return status;
        } else {
            u32 fault = RV_EX_LOAD_FAULT;
            if (status == SL_ERR_IO_ALIGN) fault = RV_EX_LOAD_ALIGN;
            return rv_exception_enter(c, fault, value);
        }

    case EX_ABORT_STORE:
        if (c->core.options & SL_CORE_OPT_TRAP_ABORT) {
            printf("STORE FAULT at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            rv_dump_core_state(c);
            return status;
        } else {
            u32 fault = RV_EX_STORE_FAULT;
            if (status == SL_ERR_IO_ALIGN) fault = RV_EX_STORE_ALIGN;
            return rv_exception_enter(c, fault, value);
        }

    case EX_ABORT_INST:
        if (c->core.options & SL_CORE_OPT_TRAP_PREFETCH_ABORT) {
            printf("PREFETCH FAULT at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            rv_dump_core_state(c);
            return status;
        } else {
            u32 fault = RV_EX_INST_FAULT;
            if (status == SL_ERR_IO_ALIGN) fault = RV_EX_INST_ALIGN;
            return rv_exception_enter(c, fault, value);
        }

    default:
        printf("\nUNHANDLED EXCEPTION type %u\n", ex);
        return SL_ERR_UNIMPLEMENTED;
    }
}
