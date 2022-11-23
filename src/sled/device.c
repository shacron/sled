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

#include <stdatomic.h>
#include <stdio.h>

#include <sl/device.h>
#include <sled/error.h>

int intc_create(device_t **dev_out);
int rtc_create(device_t **dev_out);
int uart_create(device_t **dev_out);

static atomic_uint_least32_t device_id = 0;

int device_create(uint32_t type, const char *name, device_t **dev_out) {
    int err;
    device_t *d;

    switch (type) {
    case DEVICE_UART: err = uart_create(&d); break;
    case DEVICE_INTC: err = intc_create(&d); break;
    case DEVICE_RTC:  err = rtc_create(&d);  break;
    default:
        return SL_ERR_ARG;
    }
    if (err) return err;
    d->name = name;
    *dev_out = d;
    return 0;
}

void dev_init(device_t *d, uint32_t type) {
    d->type = type;
    d->id = atomic_fetch_add_explicit(&device_id, 1, memory_order_relaxed);
}
