// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

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
