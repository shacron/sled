// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>

#include <core/core.h>
#include <core/lock.h>
#include <core/obj.h>
#include <core/sem.h>
#include <core/worker.h>
#include <sled/error.h>

#define SL_WORKER_STATE_ENGINE_RUNNABLE   (1u << 0)

static void queue_add(sl_worker_t *w, sl_event_t *ev) {
    lock_lock(&w->lock);
    sl_list_add_last(&w->ev_list, &ev->node);
    cond_signal_all(&w->has_event);
    lock_unlock(&w->lock);
}

static int handle_events(sl_worker_t *w, bool wait) {
    lock_lock(&w->lock);
    if (wait) {
        while (sl_list_peek_first(&w->ev_list) == NULL) {
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
        const u4 id = ev->epid;
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

int sl_worker_add_engine(sl_worker_t *w, sl_engine_t *e, u4 *id_out) {
    int err = sl_worker_add_event_endpoint(w, &e->event_ep, id_out);
    if (err) return err;

    sl_obj_retain(e);
    if (w->engine) sl_obj_release(w->engine);
    w->engine = e;
    // engine does not retain worker to avoid a retain loop
    e->worker = w;
    return 0;
}

int sl_worker_add_event_endpoint(sl_worker_t *w, sl_event_ep_t *ep, u4 *id_out) {
    for (u4 i = 0; i < SL_WORKER_MAX_EPS; i++) {
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

    u4 flags = ev->flags;
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
        void *n = atomic_load_explicit((_Atomic(sl_list_node_t *)*)&w->ev_list.first, memory_order_relaxed);
        if (n != NULL) {
            if ((err = handle_events(w, false))) return err;
        }
    }

    while ((w->state & SL_WORKER_STATE_ENGINE_RUNNABLE) == 0) {
        if ((err = handle_events(w, true))) return err;
    }

    return w->engine->ops.step(w->engine);
}

int sl_worker_step(sl_worker_t *w, u8 num) {
    for (u8 i = 0; i < num; i++) {
        int err = single_step(w);
        if (err) return err;
    }
    return 0;
}

int sl_worker_run(sl_worker_t *w) {
    assert(!w->thread_running);
    for ( ; ; ) {
        int err = single_step(w);
        if (err) return err;
    }
}

static void * workloop_thread(void *arg) {
    sl_worker_t *w = arg;
    w->thread_status = sl_worker_run(w);
    return NULL;
}

int sl_worker_thread_run(sl_worker_t *w) {
    assert(!w->thread_running);
    w->thread_running = true;
    int err = pthread_create(&w->thread, NULL, workloop_thread, w);
    if (err) {
        w->thread_running = false;
        return SL_ERR_STATE;
    }
    return 0;
}

int sl_worker_thread_join(sl_worker_t *w) {
    assert(w->thread_running);
    int err = pthread_join(w->thread, NULL);
    switch (err) {
    case 0:
        w->thread_running = false;
        return 0;

    case EINVAL:
    case ESRCH:
    case EDEADLK:
        return SL_ERR_STATE;

    default:    return SL_ERR;
    }
}

void worker_obj_shutdown(void *o) {
    sl_worker_t *w = o;
    assert(!w->thread_running);
    if (w->engine) sl_obj_release(w->engine);
    lock_destroy(&w->lock);
    cond_destroy(&w->has_event);
}

int worker_obj_init(void *o, const char *name) {
    sl_worker_t *w = o;
    w->name = name;
    w->thread_running = false;
    lock_init(&w->lock);
    cond_init(&w->has_event);
    return 0;
}
