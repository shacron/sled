// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <assert.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/bus.h>
#include <core/common.h>
#include <core/core.h>
#include <core/sem.h>
#include <core/sym.h>
#include <sled/arch.h>
#include <sled/error.h>

static void config_set_internal(core_t *c, sl_core_params_t *p);

// Called in device context
// Send a message to dispatch loop to handle interrupt change
static int core_accept_irq(sl_irq_ep_t *ep, uint32_t num, bool high) {
    core_ev_t *ev = malloc(sizeof(*ev));
    if (ev == NULL) return SL_ERR_MEM;

    ev->type = CORE_EV_IRQ;
    ev->option = 0;
    ev->arg[0] = num;
    ev->arg[1] = high;

    core_t *c = containerof(ep, core_t, irq_ep);
    queue_add(&c->event_q, &ev->node);
    return 0;
}

void core_set_wfi(core_t *c, bool enable) {
    if (enable) c->state |= (1u << SL_CORE_STATE_WFI);
    else c->state &= ~(1u << SL_CORE_STATE_WFI);
}

// Called in dispatch loop context
int core_handle_irq_event(core_t *c, core_ev_t *ev) {
    sl_irq_ep_t *ep = &c->irq_ep;
    uint32_t num = ev->arg[0];
    bool high = ev->arg[1];
    int err = sl_irq_endpoint_assert(ep, num, high);
    return err;
}

void core_event_send(core_t *c, core_ev_t *ev) {
    queue_add(&c->event_q, &ev->node);
}

int sl_core_async_command(core_t *c, uint32_t cmd, bool wait) {
    int err = 0;
    sl_sem_t sem;

    core_ev_t *ev = calloc(1, sizeof(*ev));
    if (ev == NULL) return SL_ERR_MEM;

    ev->type = CORE_EV_RUNMODE;
    ev->option = cmd;
    ev->flags = 0;
    if (wait) {
        ev->flags |= CORE_EV_FLAG_SIGNAL;
        if (sl_sem_init(&sem, 0)) goto out_err;
        ev->arg[0] = (uintptr_t)&sem;
    }
    core_event_send(c, ev);
    if (wait) {
        sl_sem_wait(&sem);
        sl_sem_destroy(&sem);
    }
    return 0;

out_err:
    free(ev);
    return err;
}

uint8_t sl_core_get_arch(core_t *c) {
    return c->arch;
}

int core_wait_for_interrupt(core_t *c) {
    sl_irq_ep_t *ep = &c->irq_ep;
    if (ep->asserted) return 0;
    core_set_wfi(c, true);
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
    queue_init(&c->event_q);
    sl_irq_endpoint_set_enabled(&c->irq_ep, SL_IRQ_VEC_ALL);
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
    // delete all pending events...

    queue_shutdown(&c->event_q);
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

static int core_handle_runmode_event(core_t *c, core_ev_t *ev) {
    int err = 0;
    switch(ev->option) {
    case SL_CORE_CMD_RUN:   core_set_wfi(c, false); break;
    case SL_CORE_CMD_HALT:  core_set_wfi(c, true);  break;
    case SL_CORE_CMD_EXIT:  err = SL_ERR_EXITED;    break;
    default:
        printf("unknown core cmd option %u\n", ev->option);
        err = SL_ERR_ARG;
        break;
    }
    if (ev->flags & CORE_EV_FLAG_SIGNAL) {
        sl_sem_t *sem = (sl_sem_t *)ev->arg[0];
        sl_sem_post(sem);
    }
    return err;
}

static int core_handle_events(core_t *c, bool wait) {
    int err = 0;
    list_node_t *ev_list = queue_remove_all(&c->event_q, wait);

    while (err == 0) {
        core_ev_t *ev = (core_ev_t *)ev_list;
        if (ev == NULL) break;
        ev_list = ev->node.next;

        // do something with it.
        // printf("got event %u\n", ev->type);

        switch (ev->type) {
        case CORE_EV_IRQ:       err = core_handle_irq_event(c, ev);     break;
        case CORE_EV_RUNMODE:   err = core_handle_runmode_event(c, ev); break;
        default:
            printf("unknown event type %u\n", ev->type);
            err = SL_ERR_STATE;
            break;
        }
        free(ev);
    }
    return err;
}

static int core_handle_interrupts(core_t *c) {
    sl_irq_ep_t *ep = &c->irq_ep;
    if (ep->asserted == 0) return 0;
    core_set_wfi(c, false);
    return c->ops.interrupt(c);
}

static int core_service_event_queue(core_t *c) {
    int err = 0;

    if (likely(!CORE_IS_WFI(c->state))) {
        if (queue_maybe_has_entries(&c->event_q))
            if ((err = core_handle_events(c, false))) goto out_err;
        if (CORE_INT_ENABLED(c->state))
            if ((err = core_handle_interrupts(c))) goto out_err;
        return 0;
    }

    // if (unlikely(!CORE_INT_ENABLED(c->state))) {
    //     // interrupts are disabled but WFI was called. This is technically legal, but results
    //     // in a core deadlock. Users probably want to be informed about that.
    //     printf("Guest state error: core is in WFI but interrupts are disabled.\n");
    //     return SL_ERR_STATE;
    // }

    do {
        if ((err = core_handle_events(c, true))) goto out_err;
        if ((err = core_handle_interrupts(c))) goto out_err;
    } while (CORE_IS_WFI(c->state));
    return 0;

out_err:
    return err;
}

int sl_core_step(core_t *c, uint32_t num) {
    int err = 0;
    for (uint32_t i = 0; i < num; i++) {
        if ((err = core_service_event_queue(c))) break;
        if ((err = c->ops.step(c))) break;
    }
    return err;
}

int sl_core_dispatch_loop(core_t *c, bool run) {
    core_set_wfi(c, !run);

    int err = 0;
    for ( ; ; ) {
        if ((err = sl_core_step(c, 0x80000000))) break;
    }
    return err;
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
