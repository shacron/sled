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
#include <sled/event.h>
#include <sled/io.h>
#include <sled/worker.h>

// Called in device context
// Send a message to dispatch loop to handle interrupt change
static int core_irq_transition_async(sl_irq_ep_t *ep, uint32_t num, bool high) {
    sl_event_t *ev = calloc(1, sizeof(*ev));
    if (ev == NULL) return SL_ERR_MEM;

    ev->flags |= SL_EV_FLAG_FREE;
    ev->type = CORE_EV_IRQ;
    ev->option = 0;
    ev->arg[0] = num;
    ev->arg[1] = high;

    core_t *c = containerof(ep, core_t, irq_ep);
    sl_worker_event_enqueue_async(c->worker, ev);
    return 0;
}

void sl_core_retain(core_t *c) { sl_obj_retain(&c->obj_); }
void sl_core_release(core_t *c) { sl_obj_release(&c->obj_); }

void core_set_wfi(core_t *c, bool enable) {
    if (enable) c->state |= (1u << SL_CORE_STATE_WFI);
    else c->state &= ~(1u << SL_CORE_STATE_WFI);
    sl_worker_set_engine_runnable(c->worker, !enable);
}

static int core_handle_irq_event(core_t *c, sl_event_t *ev) {
    sl_irq_ep_t *ep = &c->irq_ep;
    uint32_t num = ev->arg[0];
    bool high = ev->arg[1];
    int err = sl_irq_endpoint_assert(ep, num, high);
    return err;
}

int sl_core_async_command(core_t *c, uint32_t cmd, bool wait) {
    sl_event_t *ev = calloc(1, sizeof(*ev));
    if (ev == NULL) return SL_ERR_MEM;
    ev->epid = c->epid;
    ev->type = CORE_EV_RUNMODE;
    ev->option = cmd;
    ev->flags = SL_EV_FLAG_FREE;
    if (wait) ev->flags |= SL_EV_FLAG_SIGNAL;
    sl_worker_event_enqueue_async(c->worker, ev);
    return 0;
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

static void config_set_internal(core_t *c, sl_core_params_t *p) {
    c->arch = p->arch;
    c->subarch = p->subarch;
    c->id = p->id;
    c->options = p->options;
    c->arch_options = p->arch_options;
}

int core_config_set(core_t *c, sl_core_params_t *p) {
    if (c->arch != p->arch) return SL_ERR_ARG;
    config_set_internal(c, p);
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

    sl_worker_release(c->worker);
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
    sl_io_op_t op;
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.direction = IO_DIR_IN;
    op.align = 1;
    op.buf = buf;
    op.agent = c;
    return sl_mapper_io(c->mapper, &op);
}

int sl_core_mem_write(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    sl_io_op_t op;
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.direction = IO_DIR_OUT;
    op.align = 1;
    op.buf = buf;
    op.agent = c;
    return sl_mapper_io(c->mapper, &op);
}

void sl_core_set_reg(core_t *c, uint32_t reg, uint64_t value) {
    c->ops.set_reg(c, reg, value);
}

uint64_t sl_core_get_reg(core_t *c, uint32_t reg) {
    return c->ops.get_reg(c, reg);
}

int sl_core_set_mapper(core_t *c, sl_dev_t *d) {
    sl_mapper_t *m = sl_device_get_mapper(d);
    m->next = c->mapper;
    c->mapper = m;

    uint32_t id;
    int err = sl_worker_add_event_endpoint(c->worker, &d->event_ep, &id);
    if (err) return err;
    sl_device_set_worker(d, c->worker, id);
    return id;
}

static int core_handle_runmode_event(core_t *c, sl_event_t *ev) {
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
    if (ev->flags & SL_EV_FLAG_SIGNAL) {
        sl_sem_t *sem = (sl_sem_t *)ev->signal;
        sl_sem_post(sem);
    }
    return err;
}

static int core_event_handle(sl_event_ep_t *ep, sl_event_t *ev) {
    core_t *c = containerof(ep, core_t, event_ep);
    int err = 0;

    switch (ev->type) {
    case CORE_EV_IRQ:       err = core_handle_irq_event(c, ev);     break;
    case CORE_EV_RUNMODE:   err = core_handle_runmode_event(c, ev); break;
    default:
        ev->err = SL_ERR_ARG;
        printf("unknown event type %u\n", ev->type);
        break;
    }
    return err;
}

int core_handle_interrupts(core_t *c) {
    sl_irq_ep_t *ep = &c->irq_ep;
    if (ep->asserted == 0) return 0;
    core_set_wfi(c, false);
    return c->ops.interrupt(c);
}

int sl_core_step(core_t *c, uint64_t num) {
    return sl_worker_step(c->worker, num);
}

int sl_core_run(core_t *c) {
    return sl_worker_run(c->worker);
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

int core_init(core_t *c, sl_core_params_t *p, sl_bus_t *b) {
    int err = sl_worker_create("core worker", &c->worker);
    if (err) return err;
    sl_worker_add_engine(c->worker, c, &c->epid);
    config_set_internal(c, p);
    c->bus = b;
    c->mapper = bus_get_mapper(b);
    c->irq_ep.assert = core_irq_transition_async;
    c->event_ep.handle = core_event_handle;
    sl_irq_endpoint_set_enabled(&c->irq_ep, SL_IRQ_VEC_ALL);
    return 0;
}
