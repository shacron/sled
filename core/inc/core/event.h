// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/lock.h>
#include <core/types.h>
#include <sled/event.h>

typedef struct sl_event_queue sl_event_queue_t;

struct sl_event_queue {
    lock_t lock;
    cond_t available;
    sl_list_t list;
};

int ev_queue_init(sl_event_queue_t *q);

void ev_queue_add(sl_event_queue_t *q, sl_event_t *ev);
sl_event_t * ev_queue_remove(sl_event_queue_t *q, bool wait);
sl_list_node_t * ev_queue_remove_all(sl_event_queue_t *q, bool wait);

void ev_queue_wait(sl_event_queue_t *q);

// queue_has_entries does not take the lock, it suitable for polling where
// eventual consistency is sufficient.
bool ev_queue_maybe_has_entries(sl_event_queue_t *q);

void ev_queue_shutdown(sl_event_queue_t *q);
