// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/common.h>
#include <core/device.h>
#include <core/event.h>
#include <sled/error.h>

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

static int dev_dummy_read(void *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    return SL_ERR_IO_NORD;
}

static int dev_dummy_write(void *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    return SL_ERR_IO_NOWR;
}

int device_mapper_io(void *ctx, sl_io_op_t *op) {
    sl_dev_t *d = ctx;
    if (op->direction == IO_DIR_IN) return d->ops.read(d->context, op->addr, op->size, op->count, op->buf);
    return d->ops.write(d->context, op->addr, op->size, op->count, op->buf);
}

int sl_device_io(void *ctx, sl_io_op_t *op) {
    sl_dev_t *d = ctx;
    return d->ops.io(ctx, op);
}

void sl_device_set_context(sl_dev_t *d, void *ctx) { d->context = ctx; }
void * sl_device_get_context(sl_dev_t *d) { return d->context; }
void sl_device_lock(sl_dev_t *d) { lock_lock(&d->lock); }
void sl_device_unlock(sl_dev_t *d) { lock_unlock(&d->lock); }
sl_irq_ep_t * sl_device_get_irq_ep(sl_dev_t *d) { return &d->irq_ep; }
sl_mapper_t * sl_device_get_mapper(sl_dev_t *d) { return &d->mapper; }


void sl_device_set_event_queue(sl_dev_t *d, sl_event_queue_t *eq) { d->q = eq; }
void sl_device_set_mapper_mode(sl_dev_t *d, int mode) { mapper_set_mode(&d->mapper, mode); }

int sl_device_send_event_async(sl_dev_t *d, sl_event_t *ev) {
    if (d->q == NULL) return SL_ERR_UNSUPPORTED;
    return sl_event_send_async(d->q, ev);
}

static int device_handle_event(sl_event_t *ev) {
    if (ev->type != SL_MAP_EV_TYPE_UPDATE) return SL_ERR_ARG;
    sl_dev_t *d = ev->cookie;
    return mapper_update(&d->mapper, ev);
}

int sl_device_update_mapper_async(sl_dev_t *d, uint32_t ops, uint32_t count, sl_mapper_entry_t *ent_list) {
    if (d->q == NULL) return SL_ERR_UNSUPPORTED;

    sl_event_t *ev = calloc(1, sizeof(*ev));
    if (ev == NULL) return SL_ERR_MEM;

    ev->flags = SL_EV_FLAG_FREE | SL_EV_FLAG_CALLBACK;
    ev->type = SL_MAP_EV_TYPE_UPDATE;
    ev->arg[0] = ops;
    ev->arg[1] = count;
    ev->arg[2] = (uintptr_t)ent_list;
    ev->callback = device_handle_event;
    ev->cookie = d;
    int err = sl_event_send_async(d->q, ev);
    if (err) free(ev);
    return err;
}

void device_shutdown(sl_dev_t *d) {
    mapper_shutdown(&d->mapper);
    lock_destroy(&d->lock);
}

void sl_device_destroy(sl_dev_t *d) {
    if (d == NULL) return;
    uint16_t flags = d->flags;
    if (d->ops.destroy != NULL)
        d->ops.destroy(d->context);
    if (flags & DEV_FLAG_EMBEDDED) return;
    device_shutdown(d);
    free(d);
}

void device_init(sl_dev_t *d, uint32_t type, const char *name, const sl_dev_ops_t *ops) {
    d->type = type;
    d->flags = DEV_FLAG_EMBEDDED;
    d->name = name;
    d->id = atomic_fetch_add_explicit(&device_id, 1, memory_order_relaxed);
    d->irq_ep.assert = device_accept_irq;
    lock_init(&d->lock);
    mapper_init(&d->mapper);
    memcpy(&d->ops, ops, sizeof(*ops));
    if (d->ops.read == NULL) d->ops.read = dev_dummy_read;
    if (d->ops.write == NULL) d->ops.write = dev_dummy_write;
}

int sl_device_create(uint32_t type, const char *name, const sl_dev_ops_t *ops, sl_dev_t **dev_out) {
    sl_dev_t *d = calloc(1, sizeof(*d));
    if (d == NULL) return SL_ERR_MEM;
    *dev_out = d;
    device_init(d, type, name, ops);
    d->flags &= ~DEV_FLAG_EMBEDDED;
    return 0;
}
