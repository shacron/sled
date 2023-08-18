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

int sled_intc_create(const char *name, sl_dev_t **dev_out);

static int intc_read(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    sl_dev_t *d = ctx;

    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    u4 *val = buf;
    int err = 0;

    sl_irq_ep_t *ep = sl_device_get_irq_ep(d);

    sl_device_lock(d);
    switch (addr) {
    case INTC_REG_DEV_TYPE:     *val = INTC_TYPE;           break;
    case INTC_REG_DEV_VERSION:  *val = INTC_VERSION;        break;
    case INTC_REG_ASSERTED:     *val = sl_irq_endpoint_get_asserted(ep);    break;
    case INTC_REG_MASK:         *val = sl_irq_endpoint_get_enabled(ep);     break;
    default:                    err = SL_ERR_IO_INVALID;    break;
    }
    sl_device_unlock(d);
    return err;
}

static int intc_write(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    sl_dev_t *d = ctx;

    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    u4 val = *(u4 *)buf;
    int err = 0;

    sl_irq_ep_t *ep = sl_device_get_irq_ep(d);

    sl_device_lock(d);
    switch (addr) {
    case INTC_REG_DEV_TYPE:     err = SL_ERR_IO_NOWR;   break;
    case INTC_REG_DEV_VERSION:  err = SL_ERR_IO_NOWR;   break;
    case INTC_REG_ASSERTED:
        err = sl_irq_endpoint_clear(ep, val);
        break;

    case INTC_REG_MASK:
        err = sl_irq_endpoint_set_enabled(ep, ~val);
        break;

    default:    err = SL_ERR_IO_INVALID;    break;
    }
    sl_device_unlock(d);
    return err;
}

static const sl_dev_ops_t intc_ops = {
    .type = SL_DEV_INTC,
    .read = intc_read,
    .write = intc_write,
    .create = sled_intc_create,
};

int sled_intc_create(const char *name, sl_dev_t **dev_out) {
    sl_dev_t *d;
    int err = sl_device_allocate(SL_DEV_INTC, name, INTC_APERTURE_LENGTH, &intc_ops, &d);
    if (err) return err;
    *dev_out = d;
    sl_device_set_context(d, d);
    return 0;
}

DECLARE_DEVICE(sled_intc, SL_DEV_INTC, &intc_ops);
