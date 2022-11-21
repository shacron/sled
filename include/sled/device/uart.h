#pragma once

#define UART_REG_DEV_TYPE      0x0  // RO
#define UART_REG_DEV_VERSION   0x4  // RO
#define UART_REG_CONFIG        0x8  // RW
#define UART_REG_STATUS        0xc  // RO
#define UART_REG_FIFO_READ     0x10 // RO
#define UART_REG_FIFO_WRITE    0x14 // WO

#define UART_APERTURE_LENGTH   0x18
