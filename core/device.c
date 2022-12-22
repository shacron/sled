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

#include <stdatomic.h>
#include <stdio.h>

#include <core/common.h>
#include <core/device.h>
#include <sled/error.h>

int intc_create(device_t **dev_out);
int rtc_create(device_t **dev_out);
int uart_create(device_t **dev_out);

static atomic_uint_least32_t device_id = 0;

// default irq handler wrapper
static int device_accept_irq(irq_endpoint_t *ep, uint32_t num, bool high) {
    if (num > 31) return SL_ERR_ARG;
    device_t *d = containerof(ep, device_t, irq_ep);
    dev_lock(d);
    int err = irq_endpoint_assert(ep, num, high);
    dev_unlock(d);
    return err;
}

int device_create(uint32_t type, const char *name, device_t **dev_out) {
    int err;
    device_t *d;

    switch (type) {
    case DEVICE_UART: err = uart_create(&d); break;
    case DEVICE_INTC: err = intc_create(&d); break;
    case DEVICE_RTC:  err = rtc_create(&d);  break;
    default:
        return SL_ERR_ARG;
    }
    if (err) return err;
    d->name = name;
    *dev_out = d;
    return 0;
}

static int dev_dummy_read(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    return SL_ERR_IO_NORD;
}

static int dev_dummy_write(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    return SL_ERR_IO_NOWR;
}

void dev_init(device_t *d, uint32_t type) {
    d->type = type;
    d->id = atomic_fetch_add_explicit(&device_id, 1, memory_order_relaxed);
    d->irq_ep.assert = device_accept_irq;
    d->read = dev_dummy_read;
    d->write = dev_dummy_write;
    lock_init(&d->lock);
}

void dev_shutdown(device_t *d) {
    lock_destroy(&d->lock);
}
