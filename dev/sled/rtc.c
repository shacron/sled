// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stdlib.h>
#include <sys/time.h>

#include <device/sled/rtc.h>
#include <sled/device.h>
#include <sled/error.h>

#define RTC_TYPE 'rtcs'
#define RTC_VERSION 0

static int rtc_read(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (count != 1) return SL_ERR_IO_COUNT;

    u4 *val = buf;
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
    u8 us = tp.tv_usec + (tp.tv_sec * 1000000);

    switch (addr) {
    case RTC_REG_MONOTONIC64:
        if (size != 8) return SL_ERR_IO_SIZE;
        *(u8 *)buf = us;
        return 0;

    case RTC_REG_MONOTONIC_LO:
        if (size != 4) return SL_ERR_IO_SIZE;
        *val = (u4)us;
        return 0;

    case RTC_REG_MONOTONIC_HI:
        if (size != 4) return SL_ERR_IO_SIZE;
        *val = (u4)(us >> 32);
        return 0;

    default:
        return SL_ERR_IO_INVALID;
    }
}

static int sled_rtc_create(sl_dev_t *d, sl_dev_config_t *cfg) {
    cfg->aperture = RTC_APERTURE_LENGTH;
    return 0;
}

static const sl_dev_ops_t rtc_ops = {
    .type = SL_DEV_SLED_RTC,
    .read = rtc_read,
    .create = sled_rtc_create,
};

DECLARE_DEVICE(sled_rtc, SL_DEV_RTC, &rtc_ops);
