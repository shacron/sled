// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

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

typedef struct sl_dev sl_dev_t;

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
