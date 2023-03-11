// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>

#include <core/list.h>
#include <core/lock.h>

typedef struct queue queue_t;

struct queue {
    lock_t lock;
    cond_t available;
    list_t list;
};

int queue_init(queue_t *q);

void queue_add(queue_t *q, list_node_t *n);
list_node_t * queue_remove(queue_t *q, bool wait);
list_node_t * queue_remove_all(queue_t *q, bool wait);

void queue_wait(queue_t *q);

// queue_has_entries does not take the lock, it suitable for polling where
// eventual consistency is sufficient.
bool queue_maybe_has_entries(queue_t *q);

void queue_shutdown(queue_t *q);
