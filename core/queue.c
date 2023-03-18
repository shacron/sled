// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <stdatomic.h>

#include <core/queue.h>

int queue_init(queue_t *q) {
    lock_init(&q->lock);
    cond_init(&q->available);
    return 0;
}

void queue_add(queue_t *q, sl_list_node_t *n) {
    lock_lock(&q->lock);
    sl_list_add_tail(&q->list, n);
    cond_signal_all(&q->available);
    lock_unlock(&q->lock);
}

sl_list_node_t * queue_remove(queue_t *q, bool wait) {
    sl_list_node_t *n = NULL;
    lock_lock(&q->lock);
    if (wait) {
        while (sl_list_peek_head(&q->list) == NULL)
            cond_wait(&q->available, &q->lock);
    }
    n = sl_list_remove_head(&q->list);
    lock_unlock(&q->lock);
    return n;
}

sl_list_node_t * queue_remove_all(queue_t *q, bool wait) {
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

void queue_wait(queue_t *q) {
    lock_lock(&q->lock);
    while (sl_list_peek_head(&q->list) != NULL)
        cond_wait(&q->available, &q->lock);
    lock_unlock(&q->lock);
}

// This function is inherently racy. Its view of pending_event may not
// reflect changes another thread has recently made. This is okay.
// The queue lock must be taken to access entires, which will synchronize
// the thread's view of the data being touched.
bool queue_maybe_has_entries(queue_t *q) {
    sl_list_node_t *n = atomic_load_explicit((_Atomic(sl_list_node_t *)*)&q->list.head, memory_order_relaxed);
    return n != NULL;
}

void queue_shutdown(queue_t *q) {
    cond_destroy(&q->available);
    lock_destroy(&q->lock);
}
