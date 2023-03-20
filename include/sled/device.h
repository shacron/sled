// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SL_DEV_BUS          0
#define SL_DEV_CORE         1
#define SL_DEV_MEM          2
#define SL_DEV_ROM          3
#define SL_DEV_INTC         4

#define SL_DEV_UART         128
#define SL_DEV_RTC          129
#define SL_DEV_IRQGEN       130


#define SL_DEV_RESERVED     1024

struct sl_dev_ops {
    int (*read)(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    int (*write)(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    void (*destroy)(void *ctx);
    uint32_t aperture;
};

int sl_device_create(uint32_t type, const char *name, const sl_dev_ops_t *ops, sl_dev_t **dev_out);

void sl_device_set_context(sl_dev_t *d, void *ctx);
void * sl_device_get_context(sl_dev_t *d);

void sl_device_lock(sl_dev_t *d);
void sl_device_unlock(sl_dev_t *d);

sl_irq_ep_t * sl_device_get_irq_ep(sl_dev_t *d);

void sl_device_destroy(sl_dev_t *dev);

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
