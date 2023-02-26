// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#include <core/irq.h>
#include <core/lock.h>
#include <sled/device.h>

typedef struct device device_t;

struct device {
    uint32_t type;
    uint32_t id;
    uint64_t base;
    uint32_t length;
    const char *name;

    lock_t lock;
    irq_endpoint_t irq_ep;

    int (*read)(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    int (*write)(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    void (*destroy)(device_t *dev);
};

// device API
int device_create(uint32_t type, const char *name, device_t **dev_out);

static inline void dev_lock(device_t *d) { lock_lock(&d->lock); }
static inline void dev_unlock(device_t *d) { lock_unlock(&d->lock); }

// internal device calls
void dev_init(device_t *d, uint32_t type);
void dev_shutdown(device_t *d);
