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

// default irq handler wrapper
static int device_accept_irq(sl_irq_ep_t *ep, u32 num, bool high) {
    if (num > 31) return SL_ERR_ARG;
    sl_dev_t *d = containerof(ep, sl_dev_t, irq_ep);
    dev_lock(d);
    int err = sl_irq_endpoint_assert(ep, num, high);
    dev_unlock(d);
    return err;
}

static int dev_dummy_read(void *d, u64 addr, u32 size, u32 count, void *buf) {
    return SL_ERR_IO_NORD;
}

static int dev_dummy_write(void *d, u64 addr, u32 size, u32 count, void *buf) {
    return SL_ERR_IO_NOWR;
}

static void dev_dummy_release(void *d) {}

int device_mapper_ep_io(sl_map_ep_t *ep, sl_io_op_t *op) {
    sl_dev_t *d = containerof(ep, sl_dev_t, map_ep);
    if (op->direction == IO_DIR_IN) return d->ops.read(d->context, op->addr, op->size, op->count, op->buf);
    return d->ops.write(d->context, op->addr, op->size, op->count, op->buf);
}

void sl_device_set_context(sl_dev_t *d, void *ctx) { d->context = ctx; }
void * sl_device_get_context(sl_dev_t *d) { return d->context; }
void sl_device_lock(sl_dev_t *d) { lock_lock(&d->lock); }
void sl_device_unlock(sl_dev_t *d) { lock_unlock(&d->lock); }
sl_irq_ep_t * sl_device_get_irq_ep(sl_dev_t *d) { return &d->irq_ep; }
sl_mapper_t * sl_device_get_mapper(sl_dev_t *d) { return d->mapper; }
void sl_device_set_mapper(sl_dev_t *d, sl_mapper_t *m) { d->mapper = m; }

void sl_device_set_worker(sl_dev_t *d, sl_worker_t *w, u32 epid) {
    d->worker = w;
    d->worker_epid = epid;
}

int sl_device_send_event_async(sl_dev_t *d, sl_event_t *ev) {
    if (d->worker == NULL) return SL_ERR_UNSUPPORTED;
    return sl_worker_event_enqueue_async(d->worker, ev);
}

static int device_ep_handle_event(sl_event_ep_t *ep, sl_event_t *ev) {
    if (ev->type != SL_MAP_EV_TYPE_UPDATE) {
        ev->err = SL_ERR_ARG;
        return 0;
    }
    sl_dev_t *d = containerof(ep, sl_dev_t, event_ep);
    return mapper_update(d->mapper, ev);
}

int sl_device_update_mapper_async(sl_dev_t *d, u32 ops, u32 count, sl_mapping_t *ent_list) {
    if (d->worker == NULL) return SL_ERR_UNSUPPORTED;
    if (d->mapper == NULL) return SL_ERR_UNSUPPORTED;

    sl_event_t *ev = calloc(1, sizeof(*ev));
    if (ev == NULL) return SL_ERR_MEM;

    ev->epid = d->worker_epid;
    ev->flags = SL_EV_FLAG_FREE;
    ev->type = SL_MAP_EV_TYPE_UPDATE;
    ev->arg[0] = ops;
    ev->arg[1] = count;
    ev->arg[2] = (uintptr_t)ent_list;
    int err = sl_worker_event_enqueue_async(d->worker, ev);
    if (err) free(ev);
    return err;
}

void device_embedded_shutdown(sl_dev_t *d) {
    sl_obj_embedded_shutdown(&d->obj_);
}

static void device_obj_shutdown(void *o) {
    sl_dev_t *d = o;
    d->ops.release(d->context);
    d->context = NULL;
    lock_destroy(&d->lock);
}

void sl_device_retain(sl_dev_t *d) { sl_obj_retain(&d->obj_); }
void sl_device_release(sl_dev_t *d) { sl_obj_release(&d->obj_); };

static const sl_obj_vtable_t device_vtab = {
    .type = SL_OBJ_TYPE_DEVICE,
    .shutdown = device_obj_shutdown,
};

static void device_init_common(sl_dev_t *d, u32 type, const sl_dev_ops_t *ops) {
    d->type = type;
    d->irq_ep.assert = device_accept_irq;
    lock_init(&d->lock);
    d->map_ep.io = device_mapper_ep_io;
    d->event_ep.handle = device_ep_handle_event;
    memcpy(&d->ops, ops, sizeof(*ops));
    if (d->ops.read == NULL) d->ops.read = dev_dummy_read;
    if (d->ops.write == NULL) d->ops.write = dev_dummy_write;
    if (d->ops.release == NULL) d->ops.release = dev_dummy_release;
}

void device_embedded_init(sl_dev_t *d, u32 type, const char *name, const sl_dev_ops_t *ops) {
    sl_obj_embedded_init(&d->obj_, name, &device_vtab);
    device_init_common(d, type, ops);
}

int sl_device_allocate(u32 type, const char *name, const sl_dev_ops_t *ops, sl_dev_t **dev_out) {
    sl_dev_t *d = sl_obj_allocate(sizeof(*d), name, &device_vtab);
    if (d == NULL) return SL_ERR_MEM;
    *dev_out = d;
    device_init_common(d, type, ops);
    return 0;
}
