#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_BUS          0
#define DEVICE_CORE         1
#define DEVICE_MEM          2
#define DEVICE_ROM          3
#define DEVICE_INTC         4

#define DEVICE_UART         128
#define DEVICE_RTC          129


#define DEVICE_RESERVED     1024

typedef struct device device_t;

// common device calls

// uart
#define UART_IO_NULL 0
#define UART_IO_CONS 1
#define UART_IO_FILE 2
#define UART_IO_PORT 3
int sled_uart_set_channel(device_t *d, int io, int fd_in, int fd_out);


#ifdef __cplusplus
}
#endif
