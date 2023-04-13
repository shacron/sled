// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// sled core devices
#define SL_DEV_BUS          0
#define SL_DEV_CORE         1
#define SL_DEV_MEM          2
#define SL_DEV_ROM          3
#define SL_DEV_INTC         4

// sled devices
#define SL_DEV_UART         128
#define SL_DEV_RTC          129
#define SL_DEV_IRQGEN       130
#define SL_DEV_MPU          131

// user-defined devices
#define SL_DEV_RESERVED     1024

struct sl_dev_ops {
    int (*read)(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    int (*write)(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    void (*release)(void *ctx);
    uint32_t aperture;
};

int sl_device_allocate(uint32_t type, const char *name, const sl_dev_ops_t *ops, sl_dev_t **dev_out);
void sl_device_retain(sl_dev_t *d);
void sl_device_release(sl_dev_t *d);

void sl_device_set_context(sl_dev_t *d, void *ctx);
void * sl_device_get_context(sl_dev_t *d);

void sl_device_lock(sl_dev_t *d);
void sl_device_unlock(sl_dev_t *d);

void sl_device_set_event_queue(sl_dev_t *d, sl_event_queue_t *eq);

sl_irq_ep_t * sl_device_get_irq_ep(sl_dev_t *d);
sl_mapper_t * sl_device_get_mapper(sl_dev_t *d);
void sl_device_set_mapper(sl_dev_t *d, sl_mapper_t *m);

// async device ops

int sl_device_send_event_async(sl_dev_t *d, sl_event_t *ev);
int sl_device_update_mapper_async(sl_dev_t *d, uint32_t ops, uint32_t count, sl_mapping_t *ent_list);

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
