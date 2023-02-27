// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/common.h>
#include <core/core.h>
#include <core/sym.h>
#include <sled/arch.h>
#include <sled/error.h>

static void config_set_internal(core_t *c, sl_core_params_t *p);

static int core_accept_irq(irq_endpoint_t *ep, uint32_t num, bool high) {
    core_t *c = containerof(ep, core_t, irq_ep);

    lock_lock(&c->lock);
    bool was_clear = (ep->asserted == 0);
    int err = irq_endpoint_assert(ep, num, high);
    if (!err && was_clear && (ep->asserted > 0)) {
        c->pending_irq = 1;
        cond_signal_all(&c->cond_int_asserted);
    }
    lock_unlock(&c->lock);

    return err;
}

uint8_t sl_core_get_arch(core_t *c) {
    return c->arch;
}

int core_wait_for_interrupt(core_t *c) {
    irq_endpoint_t *ep = &c->irq_ep;
    lock_lock(&c->lock);
    if (ep->asserted == 0) cond_wait(&c->cond_int_asserted, &c->lock);
    lock_unlock(&c->lock);
    return 0;
}

void core_config_get(core_t *c, sl_core_params_t *p) {
    p->arch = c->arch;
    p->subarch = c->subarch;
    p->id = c->id;
    p->options = c->options;
    p->arch_options = c->arch_options;
}

int core_config_set(core_t *c, sl_core_params_t *p) {
    if (c->arch != p->arch) return SL_ERR_ARG;
    config_set_internal(c, p);
    return 0;
}

static void config_set_internal(core_t *c, sl_core_params_t *p) {
    c->arch = p->arch;
    c->subarch = p->subarch;
    c->id = p->id;
    c->options = p->options;
    c->arch_options = p->arch_options;
}

int core_init(core_t *c, sl_core_params_t *p, bus_t *b) {
    config_set_internal(c, p);
    c->bus = b;
    c->irq_ep.assert = core_accept_irq;
    lock_init(&c->lock);
    cond_init(&c->cond_int_asserted);
    irq_endpoint_set_enabled(&c->irq_ep, IRQ_VEC_ALL);
    return 0;
}

int core_shutdown(core_t *c) {
#if WITH_SYMBOLS
    sym_list_t *n = NULL;
    for (sym_list_t *s = c->symbols; s != NULL; s = n) {
        n = s->next;
        sym_free(s);
    }
#endif
    lock_destroy(&c->lock);
    return 0;
}

const core_ops_t * core_get_ops(core_t *c) {
    return &c->ops;
}

uint64_t sl_core_get_cycles(core_t *c) {
    return c->ticks;
}

void core_interrupt_set(core_t *c, bool enable) {
    uint32_t bit = (1u << SL_CORE_STATE_INTERRUPTS_EN);
    if (enable) c->state |= bit;
    else c->state &= ~bit;
}

int core_endian_set(core_t *c, bool big) {
    c->ops.set_state(c, SL_CORE_OPT_ENDIAN_BIG, big);
    return 0;
}

void core_instruction_barrier(core_t *c) {
    atomic_thread_fence(memory_order_acquire);
}

void core_memory_barrier(core_t *c, uint32_t type) {
    // currently ignoring system and sync
    type &= (BARRIER_LOAD | BARRIER_STORE);
    switch (type) {
    case 0: return;
    case BARRIER_LOAD:
        atomic_thread_fence(memory_order_acquire);
        break;
    case BARRIER_STORE:
        atomic_thread_fence(memory_order_release);
        break;
    case (BARRIER_LOAD | BARRIER_STORE):
        atomic_thread_fence(memory_order_acq_rel);
        break;
    }
}

int sl_core_mem_read(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    bus_t *b = c->bus;
    if (b == NULL) return SL_ERR_IO_NODEV;
    return bus_read(b, addr, size, count, buf);
}

int sl_core_mem_write(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    bus_t *b = c->bus;
    if (b == NULL) return SL_ERR_IO_NODEV;
    return bus_write(b, addr, size, count, buf);
}

void sl_core_set_reg(core_t *c, uint32_t reg, uint64_t value) {
    c->ops.set_reg(c, reg, value);
}

uint64_t sl_core_get_reg(core_t *c, uint32_t reg) {
    return c->ops.get_reg(c, reg);
}

int sl_core_step(core_t *c, uint32_t num) {
    return c->ops.step(c, num);
}

int sl_core_run(core_t *c) {
    return c->ops.run(c);
}

int sl_core_set_state(core_t *c, uint32_t state, bool enabled) {
    return c->ops.set_state(c, state, enabled);
}

uint32_t sl_core_get_reg_count(core_t *c, int type) {
    return sl_arch_get_reg_count(c->arch, c->subarch, type);
}

#if WITH_SYMBOLS
void core_add_symbols(core_t *c, sym_list_t *list) {
    list->next = c->symbols;
    c->symbols = list;
}

sym_entry_t *core_get_sym_for_addr(core_t *c, uint64_t addr) {
    sym_entry_t *nearest = NULL;
    uint64_t distance = ~0;
    for (sym_list_t *list = c->symbols; list != NULL; list = list->next) {
        for (uint64_t i = 0; i < list->num; i++) {
            if (list->ent[i].addr > addr) continue;
            if (list->ent[i].addr == addr) return &list->ent[i];
            uint64_t d = addr - list->ent[i].addr;
            if (d > distance) continue;
            distance = d;
            nearest = &list->ent[i];
        }
    }
    return nearest;
}
#endif
