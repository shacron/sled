// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stdatomic.h>
#include <stdio.h>

#include <core/common.h>
#include <core/device.h>
#include <sled/error.h>

int intc_create(sl_dev_t **dev_out);
int rtc_create(sl_dev_t **dev_out);
int uart_create(sl_dev_t **dev_out);

static atomic_uint_least32_t device_id = 0;

// default irq handler wrapper
static int device_accept_irq(irq_endpoint_t *ep, uint32_t num, bool high) {
    if (num > 31) return SL_ERR_ARG;
    sl_dev_t *d = containerof(ep, sl_dev_t, irq_ep);
    dev_lock(d);
    int err = irq_endpoint_assert(ep, num, high);
    dev_unlock(d);
    return err;
}

int device_create(uint32_t type, const char *name, sl_dev_t **dev_out) {
    int err;
    sl_dev_t *d;

    switch (type) {
    case SL_DEV_UART: err = uart_create(&d); break;
    case SL_DEV_INTC: err = intc_create(&d); break;
    case SL_DEV_RTC:  err = rtc_create(&d);  break;
    default:
        return SL_ERR_ARG;
    }
    if (err) return err;
    d->name = name;
    *dev_out = d;
    return 0;
}

static int dev_dummy_read(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    return SL_ERR_IO_NORD;
}

static int dev_dummy_write(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    return SL_ERR_IO_NOWR;
}

void dev_init(sl_dev_t *d, uint32_t type) {
    d->type = type;
    d->id = atomic_fetch_add_explicit(&device_id, 1, memory_order_relaxed);
    d->irq_ep.assert = device_accept_irq;
    d->read = dev_dummy_read;
    d->write = dev_dummy_write;
    lock_init(&d->lock);
}

void dev_shutdown(sl_dev_t *d) {
    lock_destroy(&d->lock);
}
