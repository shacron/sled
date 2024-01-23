// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/common.h>
#include <core/device.h>
#include <core/event.h>
#include <sled/error.h>

#define DEV_MAGIC           0x91919192

int device_mapper_ep_io(sl_map_ep_t *ep, sl_io_op_t *op) {
    sl_dev_t *d = containerof(ep, sl_dev_t, map_ep);
    if (op->op == IO_OP_IN) {
        if (d->ops->read == NULL) return SL_ERR_IO_NORD;
        return d->ops->read(d->context, op->addr, op->size, op->count, op->buf);
    }
    if (op->op == IO_OP_OUT) {
        if (d->ops->write == NULL) return SL_ERR_IO_NOWR;
        return d->ops->write(d->context, op->addr, op->size, op->count, op->buf);
    }
    return SL_ERR_IO_NOATOMIC;
}

void sl_device_set_context(sl_dev_t *d, void *ctx) { d->context = ctx; }
void * sl_device_get_context(sl_dev_t *d) { return d->context; }

void sl_device_lock(sl_dev_t *d) { sl_lock_lock(&d->lock); }
void sl_device_unlock(sl_dev_t *d) { sl_lock_unlock(&d->lock); }
sl_irq_mux_t * sl_device_get_irq_mux(sl_dev_t *d) { return &d->irq_mux; }
sl_mapper_t * sl_device_get_mapper(sl_dev_t *d) { return d->mapper; }
void sl_device_set_mapper(sl_dev_t *d, sl_mapper_t *m) { d->mapper = m; }

sl_dev_t * device_get_for_ep(sl_map_ep_t *ep) {
    return containerof(ep, sl_dev_t, map_ep);
}

void sl_device_set_worker(sl_dev_t *d, sl_worker_t *w, u4 epid) {
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

int sl_device_update_mapper_async(sl_dev_t *d, u4 ops, u4 count, sl_mapping_t *ent_list) {
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

int sl_device_init(sl_dev_t *d, sl_dev_config_t *cfg) {
    d->magic = DEV_MAGIC;
    d->ops = cfg->ops;
    d->name = cfg->name;
    d->aperture = cfg->aperture;
    d->map_ep.io = device_mapper_ep_io;
    d->event_ep.handle = device_ep_handle_event;
    sl_lock_init(&d->lock);
    return 0;
}

int sl_device_create(sl_dev_config_t *cfg, sl_dev_t **dev_out) {
    sl_dev_t *d = calloc(1, sizeof(*d));
    if (d == NULL) return SL_ERR_MEM;
    int err = sl_device_init(d, cfg);
    if (err) {
        free(d);
        return err;
    }
    if (d->ops->create != NULL) {
        err = d->ops->create(d, cfg);
        if (err) {
            sl_device_destroy(d);
            return err;
        }
    }
    d->aperture = cfg->aperture;
    *dev_out = d;
    return 0;
}

void sl_device_shutdown(sl_dev_t *d) {
    assert(d->magic == DEV_MAGIC);
    if (d->ops->destroy != NULL) d->ops->destroy(d);
    sl_lock_destroy(&d->lock);
}

void sl_device_destroy(sl_dev_t *d) {
    if (d == NULL) return;
    assert(d->magic == DEV_MAGIC);
    sl_device_shutdown(d);
    free(d);
}
