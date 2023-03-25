// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/irq.h>
#include <core/lock.h>
#include <core/mapper.h>
#include <core/types.h>
#include <sled/device.h>
#include <sled/io.h>
#include <sled/list.h>

#define DEV_FLAG_EMBEDDED   (1u << 0)

struct sl_dev {
    sl_list_node_t node;

    uint16_t type;
    uint16_t flags;
    uint32_t id;
    uint64_t base;
    const char *name;

    lock_t lock;
    sl_irq_ep_t irq_ep;
    sl_mapper_t mapper;
    void *context;          // context of owner object
    sl_dev_ops_t ops;
    sl_event_queue_t *q;
};

// device API
static inline void dev_lock(sl_dev_t *d) { lock_lock(&d->lock); }
static inline void dev_unlock(sl_dev_t *d) { lock_unlock(&d->lock); }

// internal device calls
void device_init(sl_dev_t *d, uint32_t type, const char *name, const sl_dev_ops_t *ops);
void device_shutdown(sl_dev_t *d);

int device_mapper_io(void *ctx, sl_io_op_t *op);
