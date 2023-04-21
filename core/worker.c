// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <stdatomic.h>

#include <core/core.h>
#include <core/lock.h>
#include <core/obj.h>
#include <core/sem.h>
#include <sled/error.h>
#include <sled/worker.h>

#define SL_WORKER_STATE_ENGINE_RUNNABLE   (1u << 0)

#define SL_WORKER_MAX_EPS   64

struct sl_worker {
    sl_obj_t obj_;

    lock_t lock;
    cond_t has_event;
    sl_list_t ev_list;

    uint32_t state;
    sl_engine_t *engine;

    sl_event_ep_t *endpoint[SL_WORKER_MAX_EPS];
};

void sl_worker_retain(sl_worker_t *w) { sl_obj_retain(&w->obj_); }
void sl_worker_release(sl_worker_t *w) { sl_obj_release(&w->obj_); }

static void queue_add(sl_worker_t *w, sl_event_t *ev) {
    lock_lock(&w->lock);
    sl_list_add_tail(&w->ev_list, &ev->node);
    cond_signal_all(&w->has_event);
    lock_unlock(&w->lock);
}

static int handle_events(sl_worker_t *w, bool wait) {
    lock_lock(&w->lock);
    if (wait) {
        while (sl_list_peek_head(&w->ev_list) == NULL) {
            cond_wait(&w->has_event, &w->lock);
        }

    }
    sl_list_node_t *ev_list = sl_list_remove_all(&w->ev_list);
    lock_unlock(&w->lock);

    int err = 0;
    while (err == 0) {
        sl_event_t *ev = (sl_event_t *)ev_list;
        if (ev == NULL) break;
        ev_list = ev->node.next;
        const uint32_t id = ev->epid;
        if (id == SL_EV_EP_CALLBACK) {
            err = ev->callback(ev);
        } else {
            if ((id > SL_WORKER_MAX_EPS) || (w->endpoint[id] == NULL)) {
                ev->err = SL_ERR_ARG;
            } else {
                err = w->endpoint[id]->handle(w->endpoint[id], ev);
            }
        }
        if (ev->flags & SL_EV_FLAG_SIGNAL) sl_sem_post((sl_sem_t *)ev->signal);
        if (ev->flags & SL_EV_FLAG_FREE) free(ev);
    }
    return err;
}

void sl_worker_set_engine_runnable(sl_worker_t *w, bool runnable) {
    if (runnable) w->state |= SL_WORKER_STATE_ENGINE_RUNNABLE;
    else w->state &= ~SL_WORKER_STATE_ENGINE_RUNNABLE;
}

int sl_worker_add_engine(sl_worker_t *w, sl_engine_t *e, uint32_t *id_out) {
    int err = sl_worker_add_event_endpoint(w, &e->event_ep, id_out);
    if (err) return err;

    if (w->engine) sl_engine_release(w->engine);
    w->engine = e;
    sl_engine_retain(e);
    return 0;
}

int sl_worker_add_event_endpoint(sl_worker_t *w, sl_event_ep_t *ep, uint32_t *id_out) {
    for (uint32_t i = 0; i < SL_WORKER_MAX_EPS; i++) {
        if (w->endpoint[i] == NULL) {
            w->endpoint[i] = ep;
            *id_out = i;
            return 0;
        }
    }
    return SL_ERR_FULL;
}

int sl_worker_event_enqueue_async(sl_worker_t *w, sl_event_t *ev) {
    int err = 0;
    sl_sem_t sem;

    uint32_t flags = ev->flags;
    if (flags & SL_EV_FLAG_SIGNAL) {
        if ((err = sl_sem_init(&sem, 0))) return err;
        ev->signal = (uintptr_t)&sem;
    }

    queue_add(w, ev);

    if (flags & SL_EV_FLAG_SIGNAL) {
        sl_sem_wait(&sem);
        sl_sem_destroy(&sem);
    }
    return 0;
}

static int single_step(sl_worker_t *w) {
    int err = 0;

    if (w->state & SL_WORKER_STATE_ENGINE_RUNNABLE) {
        void *n = atomic_load_explicit((_Atomic(sl_list_node_t *)*)&w->ev_list.head, memory_order_relaxed);
        if (n != NULL) {
            if ((err = handle_events(w, false))) return err;
        }
    }

    while ((w->state & SL_WORKER_STATE_ENGINE_RUNNABLE) == 0) {
        if ((err = handle_events(w, true))) return err;
    }

    return w->engine->ops.step(w->engine);
}

int sl_worker_step(sl_worker_t *w, uint64_t num) {
    for (uint64_t i = 0; i < num; i++) {
        int err = single_step(w);
        if (err) return err;
    }
    return 0;
}

int sl_worker_run(sl_worker_t *w) {
    for ( ; ; ) {
        int err = single_step(w);
        if (err) return err;
    }
}

static void worker_shutdown(void *o) {
    sl_worker_t *w = o;
    if (w->engine) sl_engine_release(w->engine);
    lock_destroy(&w->lock);
    cond_destroy(&w->has_event);
}

static const sl_obj_vtable_t worker_vtab = {
    .type = SL_OBJ_TYPE_WORKER,
    .shutdown = worker_shutdown,
};

int sl_worker_create(const char *name, sl_worker_t **w_out) {
    sl_worker_t *w = sl_obj_allocate(sizeof(*w), name, &worker_vtab);
    *w_out = w;
    lock_init(&w->lock);
    cond_init(&w->has_event);
    return 0;
}