// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/irq.h>
#include <core/lock.h>
#include <core/mapper.h>
#include <core/obj.h>
#include <core/types.h>
#include <sled/device.h>
#include <sled/io.h>
#include <sled/list.h>

#define DEV_FLAG_EMBEDDED   (1u << 0)

struct sl_dev {
    sl_obj_t obj_;
    sl_list_node_t node;

    uint16_t type;
    uint64_t base;

    lock_t lock;
    sl_irq_ep_t irq_ep;
    sl_map_ep_t map_ep;     // incoming io from external mapper
    void *context;          // context of owner object
    sl_dev_ops_t ops;
    sl_event_queue_t *q;
    sl_mapper_t *mapper;    // held for owner object, todo: remove
};

// device API
static inline void dev_lock(sl_dev_t *d) { lock_lock(&d->lock); }
static inline void dev_unlock(sl_dev_t *d) { lock_unlock(&d->lock); }

// internal device calls
void device_embedded_init(sl_dev_t *d, uint32_t type, const char *name, const sl_dev_ops_t *ops);
void device_embedded_shutdown(sl_dev_t *d);
