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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sl/bus.h>
#include <sl/device.h>
#include <sled/error.h>
#include <sl/mem.h>

// #define BUS_TRACE 1

#ifdef BUS_TRACE
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(format, ...) do {} while(0)
#endif

#define MAX_MEM_REGIONS     4
#define MAX_DEVICES         8

struct bus {
    device_t dev;
    mem_region_t mem[MAX_MEM_REGIONS];
    device_t *dev_list[MAX_DEVICES];
};

static mem_region_t * get_mem_region_for_addr(bus_t *b, uint64_t addr) {
    for (int i = 0; i < MAX_MEM_REGIONS; i++) {
        mem_region_t *r = &b->mem[i];
        if ((r->base <= addr) && (r->end > addr)) return r;
        if (r->base > addr) break;
    }
    return NULL;
}

static device_t * get_device_for_addr(bus_t *b, uint64_t addr) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        device_t *d = b->dev_list[i];
        if (d == NULL) continue;
        if ((d->base <= addr) && ((d->base + d->length) > addr)) return d;
    }
    return NULL;
}

int bus_read(bus_t *b, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    mem_region_t *r = get_mem_region_for_addr(b, addr);
    if (r != NULL) {
        const uint64_t offset = addr - r->base;
        memcpy(buf, r->data + offset, size * count);
        return 0;
    }

    device_t *d = get_device_for_addr(b, addr);
    if (d == NULL) return SL_ERR_BUS;
    return d->read(d, addr - d->base, size, count, buf);
}

int bus_write(bus_t *b, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    TRACE("[[ bus: %#" PRIx64 "(%u@%u) ]]\n", addr, count, size);

    mem_region_t *r = get_mem_region_for_addr(b, addr);
    if (r != NULL) {
        const uint64_t offset = addr - r->base;
        memcpy(r->data + offset, buf, size * count);
        return 0;
    }

    device_t *d = get_device_for_addr(b, addr);
    if (d == NULL) return SL_ERR_BUS;
    return d->write(d, addr - d->base, size, count, buf);
}

int bus_add_mem_region(bus_t *b, mem_region_t r) {
    int push = -1;
    for (int i = 0; i < MAX_MEM_REGIONS; i++) {
        // todo: check for overlaps

        if (b->mem[i].base == 0) {
            b->mem[i] = r;
            return 0;
        }
        if (b->mem[i].base == r.base) {
            b->mem[i] = r;
            return 0;
        }
        if (b->mem[i].base > r.base) {
            push = i;
            break;
        }
    }
    if (push == -1) return SL_ERR_FULL;

    for (int i = MAX_MEM_REGIONS - 1; i > push; i--) {
        b->mem[i] = b->mem[i-1];
    }
    b->mem[push] = r;
    return 0;
}

// todo: check for overlaps
int bus_add_device(bus_t *b, device_t *dev, uint64_t base) {
    // if ((base < b->device_base) || (base > (b->device_base + b->device_len))) return SL_ERR_ARG;

    dev->base = base;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (b->dev_list[i] == NULL) {
            b->dev_list[i] = dev;
            return 0;
        }
    }
    return SL_ERR_FULL;
}

device_t * bus_get_device_for_name(bus_t *b, const char *name) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        device_t *d = b->dev_list[i];
        if (d != NULL) {
            if (!strcmp(name, d->name)) return d;
        }
    }
    return NULL;
}

int bus_create(bus_t **bus_out) {
    bus_t *b = calloc(1, sizeof(*b));
    if (b == NULL) return SL_ERR_MEM;
    dev_init(&b->dev, DEVICE_BUS);
    *bus_out = b;
    return 0;
}

void bus_destroy(bus_t *bus) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        device_t *d = bus->dev_list[i];
        if (d != NULL) d->destroy(d);
    }

    for (int i = 0; i < MAX_MEM_REGIONS; i++)
        mem_region_destroy(&bus->mem[i]);

    free(bus);
}
