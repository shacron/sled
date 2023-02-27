// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <assert.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <core/common.h>
#include <core/riscv.h>
#include <core/riscv/csr.h>
#include <core/riscv/rv.h>
#include <sled/arch.h>
#include <sled/error.h>
#include <sled/riscv/csr.h>

static int rv_load_pc(rv_core_t *c, uint32_t *inst) {
    return sl_core_mem_read(&c->core, c->pc, 4, 1, inst);
}

static void riscv_set_reg(core_t *c, uint32_t reg, uint64_t value) {
    rv_core_t *rc = (rv_core_t *)c;

    if (reg == 0) return;  // always zero
    if (reg < 32) {
        rc->r[reg] = value;
        return;
    }

    rv_sr_pl_t *sr = NULL;
    if (reg >= SL_RV_CORE_REG_BASE) sr = rv_get_pl_csrs(rc, RV_PL_MACHINE);

    switch (reg) {
    case SL_CORE_REG_PC:   rc->pc = value;         break;
    case SL_CORE_REG_SP:   rc->r[RV_SP] = value;   break;
    case SL_CORE_REG_LR:   rc->r[RV_RA] = value;   break;
    case SL_RV_CORE_REG(RV_CSR_MTVEC):     sr->tvec = value;       break;
    case SL_RV_CORE_REG(RV_CSR_MSCRATCH):  sr->scratch = value;    break;
    case SL_RV_CORE_REG(RV_CSR_MEPC):      sr->epc = value;        break;
    case SL_RV_CORE_REG(RV_CSR_MCAUSE):    sr->cause = value;      break;
    case SL_RV_CORE_REG(RV_CSR_MTVAL):     sr->tval = value;       break;
    case SL_RV_CORE_REG(RV_CSR_MIP):       sr->ip = value;         break;
    default:
        assert(false);
        break;
    }
}

static uint64_t riscv_get_reg(core_t *c, uint32_t reg) {
    rv_core_t *rc = (rv_core_t *)c;

    if (reg == 0) return 0;  // always zero
    if (reg < 32)
        return rc->r[reg];

    switch (reg) {
    case SL_CORE_REG_PC: return rc->pc;
    case SL_CORE_REG_SP: return rc->r[RV_SP];
    case SL_CORE_REG_LR: return rc->r[RV_RA];
    case SL_CORE_REG_ARG0: return rc->r[RV_A0];
    case SL_CORE_REG_ARG1: return rc->r[RV_A1];
    default:
        assert(false);
        return 0xbaddbaddbaddbadd;
    }
}

uint32_t rv_get_pending_irq(rv_core_t *c) {
    return atomic_load_explicit((_Atomic uint32_t*)&c->core.pending_irq, memory_order_relaxed);
}

static int riscv_core_set_state(core_t *c, uint32_t state, bool enabled) {
    rv_core_t *rc = (rv_core_t *)c;

    const uint32_t bit = (1u << state);
    switch (state) {

    case SL_CORE_STATE_64BIT:
        if (enabled) {
            rc->mode = RV_MODE_RV64;
            c->state |= bit;
        } else {
            rc->mode = RV_MODE_RV32;
            c->state &= ~bit;
        }
        return 0;

    case SL_CORE_STATE_INTERRUPTS_EN:
        core_interrupt_set(c, enabled);
        return 0;

    case SL_CORE_STATE_ENDIAN_BIG:
        break;

    default:
        return -1;
    }

    if (enabled) c->state |= bit;
    else c->state &= ~bit;
    return 0;
}

static void riscv_core_next_pc(rv_core_t *c) {
    if (c->c_inst) {
        c->pc += 2;
        c->c_inst = 0;
    } else {
        c->pc += 4;
    }
}

// rv_handle_pending_irq copies asynchronous pending IRQs to the synchronous
// irq_asserted flag. irq_asserted is only touched in the dispatch loop.
static void rv_handle_pending_irq(rv_core_t *c) {
    irq_endpoint_t *ep = &c->core.irq_ep;
    lock_lock(&c->core.lock);
    c->irq_asserted = ep->asserted;
    c->core.pending_irq = 0;
    lock_unlock(&c->core.lock);
}

// Synchronous irq handler - invokes an exception before the next instruction is dispatched.
static int rv_interrupt_taken(rv_core_t *c) {
    uint32_t asserted = c->irq_asserted;
    if (asserted == 0) return 0;

    static const uint8_t irq_pri[] = {
        RV_INT_EXTERNAL_M, RV_INT_TIMER_M, RV_INT_SW_M, RV_INT_EXTERNAL_S, RV_INT_TIMER_S, RV_INT_SW_S
    };

    int err = 0;
    for (int i = 0; i < 6; i++) {
        const uint8_t bit = irq_pri[i];
        const uint32_t num = (1u << bit);
        if (asserted & num) {
            if ((err = rv_exception_enter(c, bit | RV_CAUSE64_INT, 0))) return err;
            c->irq_asserted &= ~(num);
            return 0;
        }
    }
    return SL_ERR_STATE;
}

static int riscv_core_step(core_t *c, uint32_t num) {
    rv_core_t *rc = (rv_core_t *)c;
    int err = 0;
    for (uint32_t i = 0; i < num; i++) {
        if (rv_get_pending_irq(rc)) rv_handle_pending_irq(rc);
        if (CORE_INT_ENABLED(c->state)) {
            if ((err = rv_interrupt_taken(rc))) break;
        }
        uint32_t inst;
        if ((err = rv_load_pc(rc, &inst)))
            return rv_synchronous_exception(rc, EX_ABORT_INST, rc->pc, err);
        rc->jump_taken = 0;
        if ((err = rv_dispatch(rc, inst))) break;
        rc->core.ticks++;
        if(!rc->jump_taken) riscv_core_next_pc(rc);
    }
    return err;
}

static int riscv_core_run(core_t *c) {
    int err = 0;
    do {
        err = riscv_core_step(c, 1000000);
    } while (!err);
    return err;
}

static int riscv_core_destroy(core_t *c) {
    core_shutdown(c);
    rv_core_t *rc = (rv_core_t *)c;
    if (rc->ext.destroy != NULL) rc->ext.destroy(rc->ext_private);
    free(c);
    return 0;
}

int riscv_core_create(sl_core_params_t *p, bus_t *bus, core_t **core_out) {
    int err;
    rv_core_t *rc = calloc(1, sizeof(*rc));
    if (rc == NULL) return SL_ERR_MEM;

    if ((err = core_init(&rc->core, p, bus))) {
        free(rc);
        return err;
    }

    rc->mode = RV_MODE_RV32;
    rc->pl = RV_PL_MACHINE;

    rc->core.options |= (SL_CORE_OPT_ENDIAN_BIG | SL_CORE_OPT_ENDIAN_LITTLE);

    rc->core.ops.set_reg = riscv_set_reg;
    rc->core.ops.get_reg = riscv_get_reg;
    rc->core.ops.step = riscv_core_step;
    rc->core.ops.run = riscv_core_run;
    rc->core.ops.set_state = riscv_core_set_state;
    rc->core.ops.destroy = riscv_core_destroy;

    rc->mimpid = 'sled';
    rc->mhartid = p->id;
    rc->ext.name_for_sysreg = rv_name_for_sysreg;

    *core_out = &rc->core;
    return 0;
}
