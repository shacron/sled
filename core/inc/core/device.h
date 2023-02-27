// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#include <core/irq.h>
#include <core/lock.h>
#include <sled/device.h>

typedef struct sl_dev sl_dev_t;

struct sl_dev {
    uint32_t type;
    uint32_t id;
    uint64_t base;
    uint32_t length;
    const char *name;

    lock_t lock;
    irq_endpoint_t irq_ep;

    int (*read)(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    int (*write)(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    void (*destroy)(sl_dev_t *dev);
};

// device API
int device_create(uint32_t type, const char *name, sl_dev_t **dev_out);

static inline void dev_lock(sl_dev_t *d) { lock_lock(&d->lock); }
static inline void dev_unlock(sl_dev_t *d) { lock_unlock(&d->lock); }

// internal device calls
void dev_init(sl_dev_t *d, uint32_t type);
void dev_shutdown(sl_dev_t *d);
