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

#include <stdlib.h>
#include <sys/time.h>

#include <core/common.h>
#include <core/device.h>
#include <device/sled/rtc.h>
#include <sled/error.h>

#define RTC_TYPE 'time'
#define RTC_VERSION 0

#define FROMDEV(d) ((rtc_t *)(d))

typedef struct {
    device_t dev;
} rtc_t;

static int rtc_read(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (count != 1) return SL_ERR_IO_COUNT;

    uint32_t *val = buf;
    switch (addr) {
    case RTC_REG_DEV_TYPE:
        if (size != 4) return SL_ERR_IO_SIZE;
        *val = RTC_TYPE;
        return 0;

    case RTC_REG_DEV_VERSION:
        if (size != 4) return SL_ERR_IO_SIZE;
        *val = RTC_VERSION;
        return 0;

    default:
        break;
    }

    struct timeval tp;
    gettimeofday(&tp, NULL);
    uint64_t us = tp.tv_usec + (tp.tv_sec * 1000000);

    switch (addr) {
    case RTC_REG_MONOTONIC64:
        if (size != 8) return SL_ERR_IO_SIZE;
        *(uint64_t *)buf = us;
        return 0;

    case RTC_REG_MONOTONIC_LO:
        if (size != 4) return SL_ERR_IO_SIZE;
        *val = (uint32_t)us;
        return 0;

    case RTC_REG_MONOTONIC_HI:
        if (size != 4) return SL_ERR_IO_SIZE;
        *val = (uint32_t)(us >> 32);
        return 0;

    default:
        return SL_ERR_IO_INVALID;
    }
}

static void rtc_destroy(device_t *dev) {
    if (dev == NULL) return;
    dev_shutdown(dev);
    free(FROMDEV(dev));
}

int rtc_create(device_t **dev_out) {
    rtc_t *u = calloc(1, sizeof(rtc_t));
    if (u == NULL) return SL_ERR_MEM;
    *dev_out = &u->dev;

    dev_init(&u->dev, DEVICE_RTC);
    u->dev.length = RTC_APERTURE_LENGTH;
    u->dev.read = rtc_read;
    u->dev.destroy = rtc_destroy;
    return 0;
}
