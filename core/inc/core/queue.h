// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/lock.h>
#include <sled/list.h>

struct queue {
    lock_t lock;
    cond_t available;
    sl_list_t list;
};

int queue_init(queue_t *q);

void queue_add(queue_t *q, sl_list_node_t *n);
sl_list_node_t * queue_remove(queue_t *q, bool wait);
sl_list_node_t * queue_remove_all(queue_t *q, bool wait);

void queue_wait(queue_t *q);

// queue_has_entries does not take the lock, it suitable for polling where
// eventual consistency is sufficient.
bool queue_maybe_has_entries(queue_t *q);

void queue_shutdown(queue_t *q);
