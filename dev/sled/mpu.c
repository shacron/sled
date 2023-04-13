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
    uint8_t config;
    uint32_t map_len[MPU_MAX_MAPPINGS];
    uint64_t va_base[MPU_MAX_MAPPINGS];
    uint64_t pa_base[MPU_MAX_MAPPINGS];
} sled_mpu_t;

static int mpu_read(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;
    if (addr & 3) return SL_ERR_IO_ALIGN;

    int err = 0;
    sled_mpu_t *m = ctx;

    sl_device_lock(m->dev);
    uint32_t *val = buf;
    switch (addr) {
    case MPU_REG_DEV_TYPE:      *val = MPU_TYPE;            goto out;
    case MPU_REG_DEV_VERSION:   *val = MPU_VERSION;         goto out;
    case MPU_REG_CONFIG:        *val = m->config;           goto out;
    case MPU_REG_STATUS:        *val = m->config & MPU_CONFIG_ENABLE;   goto out;
    case MPU_REG_MAP_ENTS:      *val = MPU_MAX_MAPPINGS;    goto out;
    default:    break;
    }
    if ((addr >= MPU_REG_MAP_VA_BASE_LO(0)) && (addr < MPU_REG_MAP_VA_BASE_LO(MPU_MAX_MAPPINGS))) {
        const uint32_t index = (addr - MPU_REG_MAP_VA_BASE_LO(0)) >> 2;
        uint32_t *p = (uint32_t *)m->va_base;
        *val = p[index];
        goto out;
    }
    if ((addr >= MPU_REG_MAP_PA_BASE_LO(0)) && (addr < MPU_REG_MAP_PA_BASE_LO(MPU_MAX_MAPPINGS))) {
        const uint32_t index = (addr - MPU_REG_MAP_PA_BASE_LO(0)) >> 2;
        uint32_t *p = (uint32_t *)m->pa_base;
        *val = p[index];
        goto out;
    }
    if ((addr >= MPU_REG_MAP_LEN(0)) && (addr < MPU_REG_MAP_LEN(MPU_MAX_MAPPINGS))) {
        const uint32_t index = (addr - MPU_REG_MAP_LEN(0)) >> 2;
        *val = m->map_len[index];
        goto out;
    }
    err = SL_ERR_IO_INVALID;
out:
    sl_device_unlock(m->dev);
    return err;
}

static void clear_entries(sled_mpu_t *m) {
    const size_t size = (sizeof(uint64_t) * 2 * MPU_MAX_MAPPINGS) + (sizeof(uint32_t) * MPU_MAX_MAPPINGS);
    memset(m->map_len, 0, size);
}

static int update_config(sled_mpu_t *m, uint32_t val) {
    int err = 0;
    uint32_t config = m->config;
    uint32_t ops = 0;
    uint32_t ent_count = 0;
    sl_mapping_t *ent_list = NULL;

    if (val & MPU_CONFIG_ENABLE)
        config |= MPU_CONFIG_ENABLE;
    else
        config &= ~MPU_CONFIG_ENABLE;

    if (val & MPU_CONFIG_APPLY) {
        ent_list = calloc(MPU_MAX_MAPPINGS, sizeof(sl_mapping_t));
        if (ent_list == NULL) return SL_ERR_MEM;
        sl_mapper_t *map = sl_device_get_mapper(m->dev);
        sl_mapper_t *next = sl_mapper_get_next(map);
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

static int mpu_write(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;
    if (addr & 3) return SL_ERR_IO_ALIGN;

    int err = 0;
    sled_mpu_t *m = ctx;
    uint32_t val = *(uint32_t *)buf;

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
        const uint32_t index = (addr - MPU_REG_MAP_VA_BASE_LO(0)) >> 2;
        uint32_t *p = (uint32_t *)m->va_base;
        p[index] = val;
        goto out;
    }
    if ((addr >= MPU_REG_MAP_PA_BASE_LO(0)) && (addr < MPU_REG_MAP_PA_BASE_LO(MPU_MAX_MAPPINGS))) {
        const uint32_t index = (addr - MPU_REG_MAP_PA_BASE_LO(0)) >> 2;
        uint32_t *p = (uint32_t *)m->pa_base;
        p[index] = val;
        goto out;
    }
    if ((addr >= MPU_REG_MAP_LEN(0)) && (addr < MPU_REG_MAP_LEN(MPU_MAX_MAPPINGS))) {
        const uint32_t index = (addr - MPU_REG_MAP_LEN(0)) >> 2;
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
    free(m);
}

static const sl_dev_ops_t mpu_ops = {
    .read = mpu_read,
    .write = mpu_write,
    .release = mpu_release,
    .aperture = MPU_APERTURE_LENGTH,
};

int sled_mpu_create(const char *name, sl_dev_t **dev_out) {
    int err = 0;

    sled_mpu_t *m = calloc(1, sizeof(sled_mpu_t));
    if (m == NULL) return SL_ERR_MEM;

    if ((err = sl_device_allocate(SL_DEV_MPU, name, &mpu_ops, &m->dev))) goto out_err;
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
