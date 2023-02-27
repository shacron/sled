// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

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
    sl_dev_t dev;
} rtc_t;

static int rtc_read(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
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

static void rtc_destroy(sl_dev_t *dev) {
    if (dev == NULL) return;
    dev_shutdown(dev);
    free(FROMDEV(dev));
}

int rtc_create(sl_dev_t **dev_out) {
    rtc_t *u = calloc(1, sizeof(rtc_t));
    if (u == NULL) return SL_ERR_MEM;
    *dev_out = &u->dev;

    dev_init(&u->dev, SL_DEV_RTC);
    u->dev.length = RTC_APERTURE_LENGTH;
    u->dev.read = rtc_read;
    u->dev.destroy = rtc_destroy;
    return 0;
}
