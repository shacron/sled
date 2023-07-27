// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <device/sled/mpu.h>
#include <sled/device.h>
#include <sled/error.h>
#include <sled/event.h>
#include <sled/io.h>
#include <sled/mapper.h>

// sled MPU device

#define MPU_TYPE 'mpux'
#define MPU_VERSION 0

typedef struct {
    sl_dev_t *dev;
    sl_mapper_t *mapper;

    bool enabled;
    u1 config;
    u4 map_len[MPU_MAX_MAPPINGS];
    u8 va_base[MPU_MAX_MAPPINGS];
    u8 pa_base[MPU_MAX_MAPPINGS];
} sled_mpu_t;

static int mpu_read(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;
    if (addr & 3) return SL_ERR_IO_ALIGN;

    int err = 0;
    sled_mpu_t *m = ctx;

    sl_device_lock(m->dev);
    u4 *val = buf;
    switch (addr) {
    case MPU_REG_DEV_TYPE:      *val = MPU_TYPE;            goto out;
    case MPU_REG_DEV_VERSION:   *val = MPU_VERSION;         goto out;
    case MPU_REG_CONFIG:        *val = m->config;           goto out;
    case MPU_REG_STATUS:        *val = m->config & MPU_CONFIG_ENABLE;   goto out;
    case MPU_REG_MAP_ENTS:      *val = MPU_MAX_MAPPINGS;    goto out;
    default:    break;
    }
    if ((addr >= MPU_REG_MAP_VA_BASE_LO(0)) && (addr < MPU_REG_MAP_VA_BASE_LO(MPU_MAX_MAPPINGS))) {
        const u4 index = (addr - MPU_REG_MAP_VA_BASE_LO(0)) >> 2;
        u4 *p = (u4 *)m->va_base;
        *val = p[index];
        goto out;
    }
    if ((addr >= MPU_REG_MAP_PA_BASE_LO(0)) && (addr < MPU_REG_MAP_PA_BASE_LO(MPU_MAX_MAPPINGS))) {
        const u4 index = (addr - MPU_REG_MAP_PA_BASE_LO(0)) >> 2;
        u4 *p = (u4 *)m->pa_base;
        *val = p[index];
        goto out;
    }
    if ((addr >= MPU_REG_MAP_LEN(0)) && (addr < MPU_REG_MAP_LEN(MPU_MAX_MAPPINGS))) {
        const u4 index = (addr - MPU_REG_MAP_LEN(0)) >> 2;
        *val = m->map_len[index];
        goto out;
    }
    err = SL_ERR_IO_INVALID;
out:
    sl_device_unlock(m->dev);
    return err;
}

static void clear_entries(sled_mpu_t *m) {
    const size_t size = (sizeof(u8) * 2 * MPU_MAX_MAPPINGS) + (sizeof(u4) * MPU_MAX_MAPPINGS);
    memset(m->map_len, 0, size);
}

static int update_config(sled_mpu_t *m, u4 val) {
    int err = 0;
    u4 config = m->config;
    u4 ops = 0;
    u4 ent_count = 0;
    sl_mapping_t *ent_list = NULL;

    if (val & MPU_CONFIG_ENABLE)
        config |= MPU_CONFIG_ENABLE;
    else
        config &= ~MPU_CONFIG_ENABLE;

    if (val & MPU_CONFIG_APPLY) {
        ent_list = calloc(MPU_MAX_MAPPINGS, sizeof(sl_mapping_t));
        if (ent_list == NULL) return SL_ERR_MEM;
        sl_mapper_t *next = sl_mapper_get_next(m->mapper);
        for (int i = 0; i < MPU_MAX_MAPPINGS; i++) {
            if (m->map_len[i] == 0) continue;
            sl_mapping_t *ent = &ent_list[ent_count];
            ent->input_base = m->va_base[i];
            ent->length = m->map_len[i];
            ent->output_base = m->pa_base[i];
            ent->ep = sl_mapper_get_ep(next);
            ent_count++;
        }
        if (ent_count > 0) ops |= SL_MAP_OP_REPLACE;
    }

    if (config & MPU_CONFIG_ENABLE) ops |= SL_MAP_OP_MODE_TRANSLATE;
    else ops |= SL_MAP_OP_MODE_PASSTHROUGH;

    // todo: add completion callback to update_async to delay updating config
    if ((err = sl_device_update_mapper_async(m->dev, ops, ent_count, ent_list))) goto out_err;

    if (val & MPU_CONFIG_CLEAR) clear_entries(m);
    m->config = config;
    return 0;

out_err:
    free(ent_list);
    return err;
}

static int mpu_write(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;
    if (addr & 3) return SL_ERR_IO_ALIGN;

    int err = 0;
    sled_mpu_t *m = ctx;
    u4 val = *(u4 *)buf;

    sl_device_lock(m->dev);
    switch (addr) {
    case MPU_REG_CONFIG:
        err = update_config(m, val);
        goto out;

    case MPU_REG_DEV_TYPE:
    case MPU_REG_DEV_VERSION:
    case MPU_REG_STATUS:
    case MPU_REG_MAP_ENTS:
        err = SL_ERR_IO_NOWR;
        goto out;

    default:
        break;
    }
    if ((addr >= MPU_REG_MAP_VA_BASE_LO(0)) && (addr < MPU_REG_MAP_VA_BASE_LO(MPU_MAX_MAPPINGS))) {
        const u4 index = (addr - MPU_REG_MAP_VA_BASE_LO(0)) >> 2;
        u4 *p = (u4 *)m->va_base;
        p[index] = val;
        goto out;
    }
    if ((addr >= MPU_REG_MAP_PA_BASE_LO(0)) && (addr < MPU_REG_MAP_PA_BASE_LO(MPU_MAX_MAPPINGS))) {
        const u4 index = (addr - MPU_REG_MAP_PA_BASE_LO(0)) >> 2;
        u4 *p = (u4 *)m->pa_base;
        p[index] = val;
        goto out;
    }
    if ((addr >= MPU_REG_MAP_LEN(0)) && (addr < MPU_REG_MAP_LEN(MPU_MAX_MAPPINGS))) {
        const u4 index = (addr - MPU_REG_MAP_LEN(0)) >> 2;
        m->map_len[index] = val;
        goto out;
    }
    err = SL_ERR_IO_INVALID;
out:
    sl_device_unlock(m->dev);
    return err;
}

static void mpu_release(void *ctx) {
    if (ctx == NULL) return;
    sled_mpu_t *m = ctx;
    sl_mapper_destroy(m->mapper);
    free(m);
}

static const sl_dev_ops_t mpu_ops = {
    .read = mpu_read,
    .write = mpu_write,
    .release = mpu_release,
};

int sled_mpu_create(const char *name, sl_dev_t **dev_out) {
    int err = 0;

    sled_mpu_t *m = calloc(1, sizeof(sled_mpu_t));
    if (m == NULL) return SL_ERR_MEM;

    if ((err = sl_device_allocate(SL_DEV_MPU, name, MPU_APERTURE_LENGTH, &mpu_ops, &m->dev))) goto out_err;
    if ((err = sl_mapper_create(&m->mapper))) goto out_err;

    *dev_out = m->dev;
    sl_device_set_context(m->dev, m);
    sl_mapper_set_mode(m->mapper, SL_MAP_OP_MODE_PASSTHROUGH);
    sl_device_set_mapper(m->dev, m->mapper);
    return 0;

out_err:
    if (m->dev) sl_device_release(m->dev);
    free(m);
    return err;
}
