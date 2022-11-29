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

struct intc {
    device_t dev;
    irq_handler_t self;
    irq_handler_t *target;
    uint32_t target_num;
    uint32_t asserted;
    uint32_t mask;
    bool high;
};

static void intc_update_level(intc_t *ic) {
    uint32_t active = ic->asserted & ~(ic->mask);
    if (active == 0) {
        if (ic->high == true) {
            ic->target->irq(ic->target, ic->target_num, false);
            ic->high = false;
        }
    } else {
        if (ic->high == false) {
            ic->target->irq(ic->target, ic->target_num, true);
            ic->high = true;
        }
    }
}

static int intc_read(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_BUS;
    if (count != 1) return SL_ERR_BUS;

    intc_t *ic = FROMDEV(d);
    uint32_t *val = buf;

    switch (addr) {
    case INTC_REG_DEV_TYPE:
        *val = INTC_TYPE;
        return 0;

    case INTC_REG_DEV_VERSION:
        *val = INTC_VERSION;
        return 0;

    case INTC_REG_ASSERTED:
        *val = ic->asserted;
        return 0;

    case INTC_REG_MASK:
        *val = ic->mask;
        return 0;

    default:
        return SL_ERR_BUS;
    }
}

static int intc_write(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_BUS;
    if (count != 1) return SL_ERR_BUS;

    intc_t *ic = FROMDEV(d);
    uint32_t val = *(uint32_t *)buf;

    switch (addr) {
    case INTC_REG_ASSERTED:
        ic->asserted &= ~val;
        intc_update_level(ic);
        return 0;

    case INTC_REG_MASK:
        ic->mask = val;
        intc_update_level(ic);
        return 0;

    default:
        return SL_ERR_BUS;
    }
}

static int intc_handle_irq(irq_handler_t *h, uint32_t num, bool high) {
    if (num > 31) return -1;
    intc_t *ic = containerof(h, intc_t, self);
    const uint32_t bit = (1u << num);
    if (ic->asserted & bit) return 0;
    ic->asserted |= bit;
    intc_update_level(ic);
    return 0;
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
    ic->self.irq = intc_handle_irq;
    ic->mask = 0xffffffff;
    ic->dev.length = INTC_APERTURE_LENGTH;
    ic->dev.read = intc_read;
    ic->dev.write = intc_write;
    ic->dev.destroy = intc_destroy;
    return 0;    
}

int intc_add_target(device_t *dev, irq_handler_t *target, uint32_t num) {
    intc_t *ic = FROMDEV(dev);
    if (num > 31) return -1;
    assert(ic->target == NULL); // todo: support multiple interrupt targets
    ic->target = target;
    ic->target_num = num;
    return 0;
}

irq_handler_t * intc_get_irq_handler(device_t *dev) {
    intc_t *ic = FROMDEV(dev);
    return &ic->self;
}
