// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#include <assert.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <sled/arch.h>
#include <sl/common.h>
#include <sled/error.h>
#include <sl/riscv.h>
#include <sl/riscv/csr.h>
#include <sl/riscv/rv.h>

static int rv_load_pc(rv_core_t *c, uint32_t *inst) {
    return core_mem_read(&c->core, c->pc, 4, 1, inst);
}

static void riscv_set_reg(core_t *c, uint32_t reg, uint64_t value) {
    rv_core_t *rc = (rv_core_t *)c;

    if (reg == 0) return;  // always zero
    if (reg < 32) {
        rc->r[reg] = value;
        return;
    }

    switch (reg) {
    case CORE_REG_PC:
        rc->pc = value;
        break;

    case CORE_REG_SP:
        rc->r[RV_SP] = value;
        break;

    case CORE_REG_LR:
        rc->r[RV_RA] = value;
        break;

    case RV_REG_MSCRATCH:
        rv_get_mode_registers(rc, RV_PRIV_LEVEL_MACHINE)->scratch = value;
        break;

    case RV_REG_MEPC:
        rv_get_mode_registers(rc, RV_PRIV_LEVEL_MACHINE)->epc = value;
        break;

    case RV_REG_MCAUSE:
        rv_get_mode_registers(rc, RV_PRIV_LEVEL_MACHINE)->cause = value;
        break;

    case RV_REG_MTVAL:
        rv_get_mode_registers(rc, RV_PRIV_LEVEL_MACHINE)->tval = value;
        break;

    case RV_REG_MIP:
        rv_get_mode_registers(rc, RV_PRIV_LEVEL_MACHINE)->ip = value;
        break;

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
    case CORE_REG_PC: return rc->pc;
    case CORE_REG_SP: return rc->r[RV_SP];
    case CORE_REG_LR: return rc->r[RV_RA];
    case CORE_REG_ARG0: return rc->r[RV_A0];
    case CORE_REG_ARG1: return rc->r[RV_A1];
    default:
        assert(false);
        return 0xbaddbaddbaddbadd;
    }
}

static int riscv_irq_handler(irq_handler_t *h, uint32_t irq, bool high) {
    rv_core_t *rc = containerof(h, rv_core_t, core.irq_handler);
    if (irq > 31) return SL_ERR_ARG;
    uint32_t irq_bit = 1u << irq;
    for ( ; ; ) {
        uint32_t pending = atomic_load_explicit((_Atomic uint32_t*)&rc->pending_irq, memory_order_relaxed);
        uint32_t updated;
        if (high) updated = pending | irq_bit;
        else      updated = pending & ~irq_bit;
        if (atomic_compare_exchange_strong_explicit((_Atomic uint32_t*)&rc->pending_irq, &pending, updated,
            memory_order_relaxed, memory_order_relaxed))
            break;
    }
    return 0;
}

uint32_t rv_get_pending_irq(rv_core_t *c) {
    return atomic_load_explicit((_Atomic uint32_t*)&c->pending_irq, memory_order_relaxed);
}

static int riscv_core_set_state(core_t *c, uint32_t state, bool enabled) {
    rv_core_t *rc = (rv_core_t *)c;

    const uint32_t bit = (1u << state);
    switch (state) {

    case CORE_STATE_64BIT:
        if (enabled) {
            rc->mode = RV_MODE_RV64;
            c->state |= bit;
        } else {
            rc->mode = RV_MODE_RV32;
            c->state &= ~bit;
        }
        return 0;

    case CORE_STATE_INTERRUPTS_EN:
        core_interrupt_set(c, enabled);
        return 0;

    case CORE_STATE_ENDIAN_BIG:
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

static int riscv_core_step(core_t *c, uint32_t num) {
    rv_core_t *rc = (rv_core_t *)c;
    int err = 0;
    for (uint32_t i = 0; i < num; i++) {
        if (c->state & (1u << CORE_STATE_INTERRUPTS_EN)) {
            uint32_t pending = rv_get_pending_irq(rc);
            if (pending) {
                // todo: handle interrupt cause better
                if ((err = rv_exception_enter(rc, RV_INT_MACHINE_EXTERNAL, true))) break;
            }
        }

        uint32_t inst;
        if ((err = rv_load_pc(rc, &inst))) break;
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

int riscv_core_create(core_params_t *p, bus_t *bus, core_t **core_out) {
    int err;
    rv_core_t *rc = calloc(1, sizeof(*rc));
    if (rc == NULL) return SL_ERR_MEM;

    if ((err = core_init(&rc->core, p, bus))) {
        free(rc);
        return err;
    }

    rc->mode = RV_MODE_RV32;
    rc->priv_level = RV_PRIV_LEVEL_MACHINE;

    rc->core.options |= (CORE_OPT_ENDIAN_BIG | CORE_OPT_ENDIAN_LITTLE);

    rc->core.ops.set_reg = riscv_set_reg;
    rc->core.ops.get_reg = riscv_get_reg;
    rc->core.ops.step = riscv_core_step;
    rc->core.ops.run = riscv_core_run;
    rc->core.ops.set_state = riscv_core_set_state;
    rc->core.ops.destroy = riscv_core_destroy;
    rc->core.irq_handler.irq = riscv_irq_handler;

    rc->mimpid = 'sled';
    rc->mhartid = p->id;
    rc->ext.name_for_sysreg = rv_name_for_sysreg;

    *core_out = &rc->core;
    return 0;
}
