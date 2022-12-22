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

#include <stdint.h>

#include <core/device.h>
#include <core/mem.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bus bus_t;

int bus_create(bus_t **bus_out);
void bus_destroy(bus_t *bus);

int bus_read(bus_t *b, uint64_t addr, uint32_t size, uint32_t count, void *buf);
int bus_write(bus_t *b, uint64_t addr, uint32_t size, uint32_t count, void *buf);

int bus_add_mem_region(bus_t *b, mem_region_t r);
int bus_add_device(bus_t *b, device_t *dev, uint64_t base);
device_t * bus_get_device_for_name(bus_t *b, const char *name);

#ifdef __cplusplus
}
#endif
