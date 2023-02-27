// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <core/common.h>
#include <core/dev/intc.h>
#include <device/sled/intc.h>
#include <sled/error.h>

#define INTC_TYPE 'intc'
#define INTC_VERSION 0

#define INTC_NUM_SUPPORTED  32

#define FROMDEV(d) containerof((d), intc_t, dev)

typedef struct {
    sl_dev_t dev;
} intc_t;

static int intc_read(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    uint32_t *val = buf;
    int err = 0;

    dev_lock(d);
    switch (addr) {
    case INTC_REG_DEV_TYPE:     *val = INTC_TYPE;           break;
    case INTC_REG_DEV_VERSION:  *val = INTC_VERSION;        break;
    case INTC_REG_ASSERTED:     *val = d->irq_ep.retained;  break;
    case INTC_REG_MASK:         *val = ~d->irq_ep.enabled;  break;
    default:                    err = SL_ERR_IO_INVALID;    break;
    }
    dev_unlock(d);
    return err;
}

static int intc_write(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    uint32_t val = *(uint32_t *)buf;
    int err = 0;

    dev_lock(d);
    switch (addr) {
    case INTC_REG_DEV_TYPE:     err = SL_ERR_IO_NOWR;   break;
    case INTC_REG_DEV_VERSION:  err = SL_ERR_IO_NOWR;   break;
    case INTC_REG_ASSERTED:
        err = irq_endpoint_clear(&d->irq_ep, val);
        break;

    case INTC_REG_MASK:
        err = irq_endpoint_set_enabled(&d->irq_ep, ~val);
        break;

    default:    err = SL_ERR_IO_INVALID;    break;
    }
    dev_unlock(d);
    return err;
}

void intc_destroy(sl_dev_t *d) {
    if (d == NULL) return;
    dev_shutdown(d);
    intc_t *ic = FROMDEV(d);
    free(ic);
}

int intc_create(sl_dev_t **dev_out) {
    intc_t *ic = calloc(1, sizeof(intc_t));
    if (ic == NULL) return SL_ERR_MEM;
    *dev_out = &ic->dev;

    dev_init(&ic->dev, SL_DEV_INTC);
    ic->dev.length = INTC_APERTURE_LENGTH;
    ic->dev.read = intc_read;
    ic->dev.write = intc_write;
    ic->dev.destroy = intc_destroy;
    return 0;    
}
