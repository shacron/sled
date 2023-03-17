// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/bus.h>
#include <core/common.h>
#include <core/device.h>
#include <core/mem.h>
#include <sled/error.h>
#include <sled/io.h>
#include <sled/map.h>

// #define BUS_TRACE 1

#ifdef BUS_TRACE
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(format, ...) do {} while(0)
#endif

#define MAX_DEVICES         8

static int mem_port_io(sl_io_port_t *p, sl_io_op_t *op, void *cookie);

sl_io_port_t mem_port = {
    .io = mem_port_io,
};

struct bus {
    sl_dev_t *dev;
    sl_map_t *map;
    sl_io_port_t port;      // incoming io port

    list_t mem_list;
    sl_dev_t *dev_list[MAX_DEVICES];
};

sl_io_port_t * bus_get_port(bus_t *b) { return &b->port; }

// emulate an io port for memory
static int mem_port_io(sl_io_port_t *p, sl_io_op_t *op, void *cookie) {
    mem_region_t *m = cookie;
    void *data = m->data + op->addr;
    if (op->direction == IO_DIR_IN)
        memcpy(op->buf, data, op->count * op->size);
    else
        memcpy(data, op->buf, op->count * op->size);
    return 0;
}

static int bus_port_io(sl_io_port_t *p, sl_io_op_t *op, void *cookie) {
    bus_t *b = containerof(p, bus_t, port);
    return sl_mapper_io(b->map, op);
}

static int bus_op_read(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    bus_t *b = ctx;

    sl_io_op_t op = {};
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.direction = IO_DIR_IN;
    op.align = 0;
    op.buf = buf;
    op.agent = b;
    return bus_port_io(&b->port, &op, NULL);
}

static int bus_op_write(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    bus_t *b = ctx;

    TRACE("[[ bus: %#" PRIx64 "(%u@%u) ]]\n", addr, count, size);

    sl_io_op_t op = {};
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.direction = IO_DIR_OUT;
    op.align = 0;
    op.buf = buf;
    op.agent = b;
    return bus_port_io(&b->port, &op, NULL);
}

int bus_add_mem_region(bus_t *b, mem_region_t *r) {
    sl_map_entry_t ent = {};
    ent.input_base = r->base;
    ent.length = r->length;
    ent.output_base = 0;
    ent.port = &mem_port;
    ent.cookie = r;
    int err = sl_mappper_add_mapping(b->map, &ent);
    if (err) return err;
    list_add_tail(&b->mem_list, &r->node);
    return 0;
}

int bus_add_device(bus_t *b, sl_dev_t *dev, uint64_t base) {
    int index = -1;

    dev->base = base;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (b->dev_list[i] == NULL) {
            b->dev_list[i] = dev;
            index = i;
            break;
        }
    }
    if (index == -1) return SL_ERR_FULL;

    sl_map_entry_t ent = {};
    ent.input_base = dev->base;
    ent.length = dev->ops.aperture;
    ent.output_base = 0;
    ent.port = &dev->port;
    int err = sl_mappper_add_mapping(b->map, &ent);
    if (err) {
        b->dev_list[index] = NULL;
        return err;
    }
    return 0;
}

sl_dev_t * bus_get_device_for_name(bus_t *b, const char *name) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        sl_dev_t *d = b->dev_list[i];
        if (d != NULL) {
            if (!strcmp(name, d->name)) return d;
        }
    }
    return NULL;
}

static void bus_op_destroy(void *ctx) {
    bus_t *bus = ctx;
    for (int i = 0; i < MAX_DEVICES; i++) {
        sl_dev_t *d = bus->dev_list[i];
        if (d != NULL) sl_device_destroy(d);
    }

    list_node_t *c;
    while ((c = list_remove_head(&bus->mem_list)) != NULL)
        mem_region_destroy((mem_region_t *)c);

    sl_mapper_destroy(bus->map);
    free(bus);
}

void bus_destroy(bus_t *bus) {
    sl_device_destroy(bus->dev);
}

static const sl_dev_ops_t bus_ops = {
    .read = bus_op_read,
    .write = bus_op_write,
    .destroy = bus_op_destroy,
};

int bus_create(const char *name, bus_t **bus_out) {
    int err = 0;

    bus_t *b = calloc(1, sizeof(*b));
    if (b == NULL) return SL_ERR_MEM;

    sl_map_t *m = NULL;
    if ((err = sl_mapper_create(&m))) goto out_err;
    b->map = m;

    if ((err = sl_device_create(SL_DEV_BUS, name, &bus_ops, &b->dev))) goto out_err;

    sl_device_set_context(b->dev, b);
    list_init(&b->mem_list);
    b->port.io = bus_port_io;
    *bus_out = b;
    return 0;

out_err:
    sl_mapper_destroy(m);
    free(b);
    return err;
}
