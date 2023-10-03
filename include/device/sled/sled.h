// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// common headers for sled generic devices
// todo: break this up

// uart
#define UART_IO_NULL 0
#define UART_IO_CONS 1
#define UART_IO_FILE 2
#define UART_IO_PORT 3
int sled_uart_set_channel(sl_dev_t *d, int io, int fd_in, int fd_out);

// intc
sl_irq_ep_t * sled_intc_get_irq_ep(sl_dev_t *d);

#ifdef __cplusplus
}
#endif
