// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/irq.h>
#include <core/lock.h>
#include <core/types.h>
#include <sled/device.h>
#include <sled/io.h>

struct sl_dev {
    uint32_t type;
    uint32_t id;
    uint64_t base;
    const char *name;

    sl_io_port_t port;

    lock_t lock;
    sl_irq_ep_t irq_ep;

    void *context;
    sl_dev_ops_t ops;
};

// device API
int device_create(uint32_t type, const char *name, sl_dev_t **dev_out);

static inline void dev_lock(sl_dev_t *d) { lock_lock(&d->lock); }
static inline void dev_unlock(sl_dev_t *d) { lock_unlock(&d->lock); }

// internal device calls
void dev_init(sl_dev_t *d, uint32_t type);
void dev_shutdown(sl_dev_t *d);
