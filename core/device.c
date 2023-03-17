// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/common.h>
#include <core/device.h>
#include <sled/error.h>

int intc_create(const char *name, sl_dev_t **dev_out);
int rtc_create(const char *name, sl_dev_t **dev_out);
int uart_create(const char *name, sl_dev_t **dev_out);

static atomic_uint_least32_t device_id = 0;

// default irq handler wrapper
static int device_accept_irq(sl_irq_ep_t *ep, uint32_t num, bool high) {
    if (num > 31) return SL_ERR_ARG;
    sl_dev_t *d = containerof(ep, sl_dev_t, irq_ep);
    dev_lock(d);
    int err = sl_irq_endpoint_assert(ep, num, high);
    dev_unlock(d);
    return err;
}

int device_create(uint32_t type, const char *name, sl_dev_t **dev_out) {
    int err;
    sl_dev_t *d;

    switch (type) {
    case SL_DEV_UART: err = uart_create(name, &d); break;
    case SL_DEV_INTC: err = intc_create(name, &d); break;
    case SL_DEV_RTC:  err = rtc_create(name, &d);  break;
    default:
        return SL_ERR_ARG;
    }
    if (err) return err;
    *dev_out = d;
    return 0;
}

static int dev_dummy_read(void *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    return SL_ERR_IO_NORD;
}

static int dev_dummy_write(void *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    return SL_ERR_IO_NOWR;
}

// Ugh, redirect for now until io port is everywhere
static int dev_port_io(sl_io_port_t *p, sl_io_op_t *op, void *cookie) {
    sl_dev_t *d = containerof(p, sl_dev_t, port);
    if (op->direction == IO_DIR_IN) return d->ops.read(d->context, op->addr, op->size, op->count, op->buf);
    return d->ops.write(d->context, op->addr, op->size, op->count, op->buf);
}

void sl_device_set_context(sl_dev_t *d, void *ctx) { d->context = ctx; }
void * sl_device_get_context(sl_dev_t *d) { return d->context; }
void sl_device_lock(sl_dev_t *d) { lock_lock(&d->lock); }
void sl_device_unlock(sl_dev_t *d) { lock_unlock(&d->lock); }
sl_irq_ep_t * sl_device_get_irq_ep(sl_dev_t *d) { return &d->irq_ep; }

void sl_device_destroy(sl_dev_t *dev) {
    if (dev == NULL) return;
    if (dev->ops.destroy != NULL)
        dev->ops.destroy(dev->context);
    lock_destroy(&dev->lock);
    free(dev);
}

int sl_device_create(uint32_t type, const char *name, const sl_dev_ops_t *ops, sl_dev_t **dev_out) {
    sl_dev_t *d = calloc(1, sizeof(*d));
    if (d == NULL) return SL_ERR_MEM;

    *dev_out = d;
    d->type = type;
    d->name = name;
    d->id = atomic_fetch_add_explicit(&device_id, 1, memory_order_relaxed);
    d->irq_ep.assert = device_accept_irq;
    memcpy(&d->ops, ops, sizeof(*ops));
    if (d->ops.read == NULL) d->ops.read = dev_dummy_read;
    if (d->ops.write == NULL) d->ops.write = dev_dummy_write;
    d->port.io = dev_port_io;
    lock_init(&d->lock);
    return 0;
}
