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
static int engine_irq_transition_async(sl_irq_ep_t *ep, uint32_t num, bool high) {
    sl_event_t *ev = calloc(1, sizeof(*ev));
    if (ev == NULL) return SL_ERR_MEM;

    sl_engine_t *e = containerof(ep, sl_engine_t, irq_ep);

    ev->epid = e->epid;
    ev->flags |= SL_EV_FLAG_FREE;
    ev->type = CORE_EV_IRQ;
    ev->option = 0;
    ev->arg[0] = num;
    ev->arg[1] = high;

    sl_worker_event_enqueue_async(e->worker, ev);
    return 0;
}

void sl_engine_retain(sl_engine_t *e) { sl_obj_retain(&e->obj_); }
void sl_engine_release(sl_engine_t *e) { sl_obj_release(&e->obj_); }

static void engine_set_wfi(sl_engine_t *e, bool enable) {
    if (enable) e->state |= (1u << SL_CORE_STATE_WFI);
    else e->state &= ~(1u << SL_CORE_STATE_WFI);
    sl_worker_set_engine_runnable(e->worker, !enable);
}

static int engine_handle_irq_event(sl_engine_t *e, sl_event_t *ev) {
    sl_irq_ep_t *ep = &e->irq_ep;
    uint32_t num = ev->arg[0];
    bool high = ev->arg[1];
    int err = sl_irq_endpoint_assert(ep, num, high);
    return err;
}

static void * engine_workloop_thread(void *arg) {
    sl_engine_t *e = arg;
    e->thread_status = sl_engine_run(e);
    return NULL;
}

static void engine_shutdown(void *o) {
    sl_engine_t *e = o;
    sl_worker_release(e->worker);
}

int sl_engine_thread_run(sl_engine_t *e) {
    int err = pthread_create(&e->thread, NULL, engine_workloop_thread, e);
    if (err) return SL_ERR_STATE;
    return 0;
}

void sl_engine_set_context(sl_engine_t *e, void *ctx) { e->context = ctx; }
void * sl_engine_get_context(sl_engine_t *e) { return e->context; }

int sl_engine_async_command(sl_engine_t *e, uint32_t cmd, bool wait) {
    sl_event_t *ev = calloc(1, sizeof(*ev));
    if (ev == NULL) return SL_ERR_MEM;
    ev->epid = e->epid;
    ev->type = CORE_EV_RUNMODE;
    ev->option = cmd;
    ev->flags = SL_EV_FLAG_FREE;
    if (wait) ev->flags |= SL_EV_FLAG_SIGNAL;
    sl_worker_event_enqueue_async(e->worker, ev);
    return 0;
}

int sl_engine_wait_for_interrupt(sl_engine_t *e) {
    sl_irq_ep_t *ep = &e->irq_ep;
    if (ep->asserted) return 0;
    engine_set_wfi(e, true);
    return 0;
}

void sl_engine_shutdown(sl_engine_t *e) {
    sl_worker_release(e->worker);
}

const sl_engine_ops_t * engine_get_ops(sl_engine_t *e) {
    return &e->ops;
}

void sl_engine_interrupt_set(sl_engine_t *e, bool enable) {
    uint32_t bit = (1u << SL_CORE_STATE_INTERRUPTS_EN);
    if (enable) e->state |= bit;
    else e->state &= ~bit;
}

static int engine_handle_runmode_event(sl_engine_t *e, sl_event_t *ev) {
    int err = 0;
    switch(ev->option) {
    case SL_CORE_CMD_RUN:   engine_set_wfi(e, false); break;
    case SL_CORE_CMD_HALT:  engine_set_wfi(e, true);  break;
    case SL_CORE_CMD_EXIT:  err = SL_ERR_EXITED;    break;
    default:
        printf("unknown engine cmd option %u\n", ev->option);
        err = SL_ERR_ARG;
        break;
    }
    if (ev->flags & SL_EV_FLAG_SIGNAL) {
        sl_sem_t *sem = (sl_sem_t *)ev->signal;
        sl_sem_post(sem);
    }
    return err;
}

static int engine_event_handle(sl_event_ep_t *ep, sl_event_t *ev) {
    sl_engine_t *e = containerof(ep, sl_engine_t, event_ep);
    int err = 0;

    switch (ev->type) {
    case CORE_EV_IRQ:
        if ((err = engine_handle_irq_event(e, ev))) break;
        if (e->state & (1u << SL_CORE_STATE_INTERRUPTS_EN)) err = sl_engine_handle_interrupts(e);
        break;

    case CORE_EV_RUNMODE:   err = engine_handle_runmode_event(e, ev); break;
    default:
        ev->err = SL_ERR_ARG;
        printf("unknown event type %u\n", ev->type);
        break;
    }
    return err;
}

int sl_engine_handle_interrupts(sl_engine_t *e) {
    sl_irq_ep_t *ep = &e->irq_ep;
    if (ep->asserted == 0) return 0;
    engine_set_wfi(e, false);
    return e->ops.interrupt(e);
}

int sl_engine_step(sl_engine_t *e, uint64_t num) {
    return sl_worker_step(e->worker, num);
}

int sl_engine_run(sl_engine_t *e) {
    return sl_worker_run(e->worker);
}

int sl_engine_init(sl_engine_t *e, sl_bus_t *b) {
    int err = sl_worker_create("eng_worker", &e->worker);
    if (err) return err;
    sl_worker_add_engine(e->worker, e, &e->epid);
    e->bus = b;
    e->irq_ep.assert = engine_irq_transition_async;
    e->event_ep.handle = engine_event_handle;
    return 0;
}

static const sl_obj_vtable_t engine_vtab = {
    .type = SL_OBJ_TYPE_ENGINE,
    .shutdown = engine_shutdown,
};

int sl_engine_allocate(const char *name, const sl_engine_ops_t *ops, sl_engine_t **e_out) {
    sl_engine_t *e = sl_obj_allocate(sizeof(*e), name, &engine_vtab);
    if (e == NULL) return SL_ERR_MEM;

    int err = sl_engine_init(e, NULL);
    if (err) {
        free(e);
        return err;
    }

    *e_out = e;
    return 0;
}