// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#pragma once

#include <stdint.h>

#include <core/irq.h>
#include <core/lock.h>
#include <sled/device.h>

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
