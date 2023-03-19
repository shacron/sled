// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <stdatomic.h>

#include <core/event.h>
#include <core/sem.h>
#include <sled/error.h>

int ev_queue_init(sl_event_queue_t *q) {
    lock_init(&q->lock);
    cond_init(&q->available);
    return 0;
}

void ev_queue_add(sl_event_queue_t *q, sl_event_t *ev) {
    lock_lock(&q->lock);
    sl_list_add_tail(&q->list, &ev->node);
    cond_signal_all(&q->available);
    lock_unlock(&q->lock);
}

sl_event_t * ev_queue_remove(sl_event_queue_t *q, bool wait) {
    sl_list_node_t *n = NULL;
    lock_lock(&q->lock);
    if (wait) {
        while (sl_list_peek_head(&q->list) == NULL)
            cond_wait(&q->available, &q->lock);
    }
    n = sl_list_remove_head(&q->list);
    lock_unlock(&q->lock);
    return (sl_event_t *)n;
}

sl_list_node_t * ev_queue_remove_all(sl_event_queue_t *q, bool wait) {
    sl_list_node_t *n = NULL;
    lock_lock(&q->lock);
    if (wait) {
        while (sl_list_peek_head(&q->list) == NULL)
            cond_wait(&q->available, &q->lock);
    }
    n = sl_list_remove_all(&q->list);
    lock_unlock(&q->lock);
    return n;
}

void ev_queue_wait(sl_event_queue_t *q) {
    lock_lock(&q->lock);
    while (sl_list_peek_head(&q->list) != NULL)
        cond_wait(&q->available, &q->lock);
    lock_unlock(&q->lock);
}

// This function is inherently racy. Its view of pending_event may not
// reflect changes another thread has recently made. This is okay.
// The queue lock must be taken to access entires, which will synchronize
// the thread's view of the data being touched.
bool ev_queue_maybe_has_entries(sl_event_queue_t *q) {
    sl_list_node_t *n = atomic_load_explicit((_Atomic(sl_list_node_t *)*)&q->list.head, memory_order_relaxed);
    return n != NULL;
}

void ev_queue_shutdown(sl_event_queue_t *q) {
    cond_destroy(&q->available);
    lock_destroy(&q->lock);
}

int sl_event_send_async(sl_event_queue_t *q, sl_event_t *ev) {
    int err = 0;
    sl_sem_t sem;

    if (ev->flags & SL_EV_FLAG_WAIT) {
        if ((err = sl_sem_init(&sem, 0))) return err;
        ev->signal = (uintptr_t)&sem;
    }

    ev_queue_add(q, ev);
    if (ev->flags & SL_EV_FLAG_WAIT) {
        sl_sem_wait(&sem);
        sl_sem_destroy(&sem);
    }
    return 0;
}
