// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// sled core devices
#define SL_DEV_NONE         0
#define SL_DEV_BUS          1
#define SL_DEV_CORE         2
#define SL_DEV_MEM          3
#define SL_DEV_ROM          4
#define SL_DEV_INTC         5

// sled devices
#define SL_DEV_UART         128
#define SL_DEV_RTC          129
#define SL_DEV_IRQGEN       130
#define SL_DEV_MPU          131

// user-defined devices
#define SL_DEV_RESERVED     1024

#define DECLARE_DEVICE(_name, _type, _ops) \
    const void * const _sl_device_dyn_ops_##_name = _ops;

struct sl_dev_config {
    sl_machine_t *machine;
    // todo:
    // power domains
    // clock domains
    // interrupt ep?
    // device-specific config
};

struct sl_dev_ops {
    u4 type;
    int (*read)(void *ctx, u8 addr, u4 size, u4 count, void *buf);
    int (*write)(void *ctx, u8 addr, u4 size, u4 count, void *buf);
    int (*create)(const char *name, sl_dev_config_t *cfg, sl_dev_t **dev_out);
    void (*release)(void *ctx);
};

int sl_device_allocate(u4 type, const char *name, u4 aperture, const sl_dev_ops_t *ops, sl_dev_t **dev_out);
void sl_device_retain(sl_dev_t *d);
void sl_device_release(sl_dev_t *d);

void sl_device_set_context(sl_dev_t *d, void *ctx);
void * sl_device_get_context(sl_dev_t *d);

void sl_device_lock(sl_dev_t *d);
void sl_device_unlock(sl_dev_t *d);

void sl_device_set_worker(sl_dev_t *d, sl_worker_t *w, u4 epid);

sl_irq_ep_t * sl_device_get_irq_ep(sl_dev_t *d);
sl_mapper_t * sl_device_get_mapper(sl_dev_t *d);
void sl_device_set_mapper(sl_dev_t *d, sl_mapper_t *m);

// async device ops

int sl_device_send_event_async(sl_dev_t *d, sl_event_t *ev);
int sl_device_update_mapper_async(sl_dev_t *d, u4 ops, u4 count, sl_mapping_t *ent_list);

// common device calls

// uart
#define UART_IO_NULL 0
#define UART_IO_CONS 1
#define UART_IO_FILE 2
#define UART_IO_PORT 3
int sled_uart_set_channel(sl_dev_t *d, int io, int fd_in, int fd_out);


#ifdef __cplusplus
}
#endif
