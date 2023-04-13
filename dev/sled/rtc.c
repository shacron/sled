// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stdlib.h>
#include <sys/time.h>

#include <device/sled/rtc.h>
#include <sled/device.h>
#include <sled/error.h>

#define RTC_TYPE 'time'
#define RTC_VERSION 0

static int rtc_read(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
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

static const sl_dev_ops_t rtc_ops = {
    .read = rtc_read,
    .aperture = RTC_APERTURE_LENGTH,
};

int rtc_create(const char *name, sl_dev_t **dev_out) {
    return sl_device_allocate(SL_DEV_RTC, name, &rtc_ops, dev_out);
}
