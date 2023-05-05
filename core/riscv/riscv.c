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

static int rv_load_pc(rv_core_t *c, u32 *inst) {
    return sl_core_mem_read(&c->core, c->pc, 4, 1, inst);
}

static void riscv_set_reg(core_t *c, u32 reg, u64 value) {
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

static u64 riscv_get_reg(core_t *c, u32 reg) {
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

static int riscv_core_set_state(core_t *c, u32 state, bool enabled) {
    rv_core_t *rc = (rv_core_t *)c;

    const u32 bit = (1u << state);
    switch (state) {

    case SL_CORE_STATE_64BIT:
        if (enabled) {
            rc->mode = RV_MODE_RV64;
            c->engine.state |= bit;
        } else {
            rc->mode = RV_MODE_RV32;
            c->engine.state &= ~bit;
        }
        break;

    case SL_CORE_STATE_INTERRUPTS_EN:
        sl_engine_interrupt_set(&c->engine, enabled);
        break;

    case SL_CORE_STATE_ENDIAN_BIG:
        if (enabled) c->engine.state |= bit;
        else c->engine.state &= ~bit;
        break;

    default:
        return SL_ERR_ARG;
    }

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

// Synchronous irq handler - invokes an exception before the next instruction is dispatched.
static int riscv_interrupt(sl_engine_t *e) {
    sl_irq_ep_t *ep = &e->irq_ep;
    rv_core_t *rc = (rv_core_t *)e;

    static const u8 irq_pri[] = {
        RV_INT_EXTERNAL_M, RV_INT_TIMER_M, RV_INT_SW_M, RV_INT_EXTERNAL_S, RV_INT_TIMER_S, RV_INT_SW_S
    };

    for (int i = 0; i < 6; i++) {
        const u8 bit = irq_pri[i];
        const u32 num = (1u << bit);
        if (ep->asserted & num)
            return rv_exception_enter(rc, bit | RV_CAUSE64_INT, 0);
    }
    return SL_ERR_STATE;
}

static int riscv_core_step(sl_engine_t *e) {
    int err = 0;
    rv_core_t *rc = (rv_core_t *)e;
    u32 inst;
    if ((err = rv_load_pc(rc, &inst)))
        return rv_synchronous_exception(rc, EX_ABORT_INST, rc->pc, err);
    rc->jump_taken = 0;
    if ((err = rv_dispatch(rc, inst))) return err;
    rc->core.ticks++;
    if(!rc->jump_taken) riscv_core_next_pc(rc);
    return err;
}

static void rv_core_shutdown(void *o) {
    rv_core_t *rc = o;
    core_shutdown(&rc->core);
    if (rc->ext.destroy != NULL) rc->ext.destroy(rc->ext_private);
}

static const sl_obj_vtable_t rvcore_vtab = {
    .type = SL_OBJ_TYPE_CORE,
    .shutdown = rv_core_shutdown,
};

int riscv_core_create(sl_core_params_t *p, sl_bus_t *bus, core_t **core_out) {
    int err;
    rv_core_t *rc = sl_obj_allocate(sizeof(*rc), p->name, &rvcore_vtab);
    if (rc == NULL) return SL_ERR_MEM;

    if ((err = core_init(&rc->core, p, bus))) {
        free(rc);
        return err;
    }

    rc->mode = RV_MODE_RV32;
    rc->pl = RV_PL_MACHINE;

    rc->core.options |= (SL_CORE_OPT_ENDIAN_BIG | SL_CORE_OPT_ENDIAN_LITTLE);

    rc->core.engine.ops.step = riscv_core_step;
    rc->core.engine.ops.interrupt = riscv_interrupt;
    rc->core.ops.set_reg = riscv_set_reg;
    rc->core.ops.get_reg = riscv_get_reg;
    rc->core.ops.set_state = riscv_core_set_state;

    rc->mimpid = 'sled';
    rc->mhartid = p->id;
    rc->ext.name_for_sysreg = rv_name_for_sysreg;

    *core_out = &rc->core;
    return 0;
}
