// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// sled core devices
#define SL_DEV_NONE         0
#define SL_DEV_BUS          1
#define SL_DEV_REG_VIEW     2

// sled devices
#define SL_DEV_SLED_UART         128
#define SL_DEV_SLED_RTC          129
#define SL_DEV_SLED_INTC         130
#define SL_DEV_SLED_MPU          131
#define SL_DEV_SLED_TIMER        132

// user-defined devices
#define SL_DEV_RESERVED     1024

#define DECLARE_DEVICE(_name, _type, _ops) \
    const void * const _sl_device_dyn_ops_##_name = _ops;

struct sl_dev_config {
    const sl_dev_ops_t *ops;
    const char *name;
    u4 aperture;

    sl_machine_t *machine;
    // todo:
    // power domains
    // clock domains
    // interrupt ep?
    // device-specific config
};

struct sl_dev_ops {
    u4 type;
    // mmio read transaction
    int (*read)(void *ctx, u8 addr, u4 size, u4 count, void *buf);
    // mmio write transaction
    int (*write)(void *ctx, u8 addr, u4 size, u4 count, void *buf);
    // create driver-specific context
    int (*create)(sl_dev_t *d, sl_dev_config_t *cfg);
    // destroy driver-specific context
    void (*destroy)(sl_dev_t *d);
};

// allocate and call ops->create()
int sl_device_create(sl_dev_config_t *cfg, sl_dev_t **dev_out);
// free device and call ops->destroy()
void sl_device_destroy(sl_dev_t *d);


void sl_device_set_context(sl_dev_t *d, void *ctx);
void * sl_device_get_context(sl_dev_t *d);

void sl_device_lock(sl_dev_t *d);
void sl_device_unlock(sl_dev_t *d);

void sl_device_set_worker(sl_dev_t *d, sl_worker_t *w, u4 epid);

sl_irq_mux_t * sl_device_get_irq_mux(sl_dev_t *d);

sl_mapper_t * sl_device_get_mapper(sl_dev_t *d);
void sl_device_set_mapper(sl_dev_t *d, sl_mapper_t *m);

// async device ops

int sl_device_send_event_async(sl_dev_t *d, sl_event_t *ev);
int sl_device_update_mapper_async(sl_dev_t *d, u4 ops, u4 count, sl_mapping_t *ent_list);

#ifdef __cplusplus
}
#endif
