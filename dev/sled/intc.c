// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <device/sled/intc.h>
#include <sled/device.h>
#include <sled/error.h>
#include <sled/irq.h>

#define INTC_TYPE 'intc'
#define INTC_VERSION 0

#define INTC_NUM_SUPPORTED  32

typedef struct {
    sl_dev_t *dev;
    sl_irq_ep_t *irq_ep;
} sled_intc_t;

int sled_intc_create(const char *name, sl_dev_config_t *cfg, sl_dev_t **dev_out);

sl_irq_ep_t * sled_intc_get_irq_ep(sl_dev_t *d) {
    sled_intc_t *ic = sl_device_get_context(d);
    return ic->irq_ep;
}

int sled_intc_set_input(sl_dev_t *intc, sl_dev_t *src, u4 num) {
    if (num >= INTC_NUM_SUPPORTED) return SL_ERR_RANGE;
    sled_intc_t *ic = sl_device_get_context(intc);
    return sl_irq_mux_set_client(sl_device_get_irq_mux(src), ic->irq_ep, num);
}

static int intc_read(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    u4 *val = buf;
    int err = 0;

    sled_intc_t *ic = ctx;
    sl_device_lock(ic->dev);
    switch (addr) {
    case INTC_REG_DEV_TYPE:     *val = INTC_TYPE;           break;
    case INTC_REG_DEV_VERSION:  *val = INTC_VERSION;        break;
    case INTC_REG_ASSERTED:     *val = sl_irq_endpoint_get_asserted(ic->irq_ep);    break;
    case INTC_REG_MASK:         *val = ~sl_irq_endpoint_get_enabled(ic->irq_ep);     break;
    default:                    err = SL_ERR_IO_INVALID;    break;
    }
    sl_device_unlock(ic->dev);
    return err;
}

static int intc_write(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    u4 val = *(u4 *)buf;
    int err = 0;

    sled_intc_t *ic = ctx;
    sl_device_lock(ic->dev);
    switch (addr) {
    case INTC_REG_DEV_TYPE:     err = SL_ERR_IO_NOWR;   break;
    case INTC_REG_DEV_VERSION:  err = SL_ERR_IO_NOWR;   break;
    case INTC_REG_ASSERTED:
        err = sl_irq_endpoint_clear(ic->irq_ep, val);
        break;

    case INTC_REG_MASK:
        err = sl_irq_endpoint_set_enabled(ic->irq_ep, ~val);
        break;

    default:    err = SL_ERR_IO_INVALID;    break;
    }
    sl_device_unlock(ic->dev);
    return err;
}

static int sled_intc_assert(sl_irq_ep_t *ep, u4 num, bool high) {
    if (num >= INTC_NUM_SUPPORTED) return SL_ERR_ARG;
    sled_intc_t *ic = sl_irq_endpoint_get_context(ep);

    sl_device_lock(ic->dev);
    int err = sl_irq_endpoint_assert(ep, num, high);
    sl_device_unlock(ic->dev);
    return err;
}

void intc_release(void *ctx) {
    sled_intc_t *ic = ctx;
    sl_obj_release(ic->irq_ep);
    sl_obj_release(ic->dev);
    free(ic);
}

static const sl_dev_ops_t intc_ops = {
    .type = SL_DEV_SLED_INTC,
    .read = intc_read,
    .write = intc_write,
    .create = sled_intc_create,
    .release = intc_release,
};

int sled_intc_create(const char *name, sl_dev_config_t *cfg, sl_dev_t **dev_out) {
    sled_intc_t *ic = calloc(1, sizeof(*ic));
    if (ic == NULL) return SL_ERR_MEM;

    int err = sl_device_allocate(name, cfg, INTC_APERTURE_LENGTH, &intc_ops, &ic->dev);
    if (err) {
        free(ic);
        return err;
    }

    if ((err = sl_obj_alloc_init(SL_OBJ_TYPE_IRQ_EP, name, cfg, (void **)&ic->irq_ep))) {
        sl_obj_release(ic->dev);
        free(ic);
        return err;
    }
    sl_irq_endpoint_set_context(ic->irq_ep, ic);
    sl_irq_endpoint_set_handler(ic->irq_ep, sled_intc_assert);

    *dev_out = ic->dev;
    sl_device_set_context(ic->dev, ic);
    return 0;
}

DECLARE_DEVICE(sled_intc, SL_DEV_INTC, &intc_ops);
