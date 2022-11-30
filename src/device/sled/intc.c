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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <sl/common.h>
#include <sl/dev/intc.h>
#include <sled/device/intc.h>
#include <sled/error.h>

#define INTC_TYPE 'intc'
#define INTC_VERSION 0

#define INTC_NUM_SUPPORTED  32

#define FROMDEV(d) containerof((d), intc_t, dev)

typedef struct {
    device_t dev;
} intc_t;

static int intc_read(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_BUS;
    if (count != 1) return SL_ERR_BUS;

    uint32_t *val = buf;
    int err = 0;

    dev_lock(d);
    switch (addr) {
    case INTC_REG_DEV_TYPE:
        *val = INTC_TYPE;
        break;

    case INTC_REG_DEV_VERSION:
        *val = INTC_VERSION;
        break;

    case INTC_REG_ASSERTED:
        *val = d->irq_ep.retained;
        break;

    case INTC_REG_MASK:
        *val = ~d->irq_ep.enabled;
        break;

    default:
        err = SL_ERR_BUS;
        break;
    }
    dev_unlock(d);
    return err;
}

static int intc_write(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_BUS;
    if (count != 1) return SL_ERR_BUS;

    uint32_t val = *(uint32_t *)buf;
    int err = 0;

    dev_lock(d);
    switch (addr) {
    case INTC_REG_ASSERTED:
        err = irq_endpoint_clear(&d->irq_ep, val);
        break;

    case INTC_REG_MASK:
        err = irq_endpoint_set_enabled(&d->irq_ep, ~val);
        break;

    default:
        err = SL_ERR_BUS;
        break;
    }
    dev_unlock(d);
    return err;
}

void intc_destroy(device_t *d) {
    if (d == NULL) return;
    dev_shutdown(d);
    intc_t *ic = FROMDEV(d);
    free(ic);
}

int intc_create(device_t **dev_out) {
    intc_t *ic = calloc(1, sizeof(intc_t));
    if (ic == NULL) return SL_ERR_MEM;
    *dev_out = &ic->dev;

    dev_init(&ic->dev, DEVICE_INTC);
    ic->dev.length = INTC_APERTURE_LENGTH;
    ic->dev.read = intc_read;
    ic->dev.write = intc_write;
    ic->dev.destroy = intc_destroy;
    return 0;    
}
