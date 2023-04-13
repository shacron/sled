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
#include <sled/mapper.h>

// #define BUS_TRACE 1

#ifdef BUS_TRACE
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(format, ...) do {} while(0)
#endif

struct sl_bus {
    sl_dev_t dev;
    sl_list_t mem_list;
    sl_list_t dev_list;
};

static int bus_op_read(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    sl_bus_t *b = ctx;

    sl_io_op_t op = {};
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.direction = IO_DIR_IN;
    op.align = 0;
    op.buf = buf;
    op.agent = b;
    return sl_mapper_io(&b->dev.mapper, &op);
}

static int bus_op_write(void *ctx, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    sl_bus_t *b = ctx;

    TRACE("[[ bus: %#" PRIx64 "(%u@%u) ]]\n", addr, count, size);

    sl_io_op_t op = {};
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.direction = IO_DIR_OUT;
    op.align = 0;
    op.buf = buf;
    op.agent = b;
    return sl_mapper_io(&b->dev.mapper, &op);
}

int bus_add_mem_region(sl_bus_t *b, mem_region_t *r) {
    sl_mapping_t m = {};
    m.input_base = r->base;
    m.length = r->length;
    m.output_base = 0;
    m.ep = &r->ep;
    int err = sl_mappper_add_mapping(sl_device_get_mapper(&b->dev), &m);
    if (err) return err;
    sl_list_add_tail(&b->mem_list, &r->node);
    return 0;
}

int bus_add_device(sl_bus_t *b, sl_dev_t *dev, uint64_t base) {
    dev->base = base;
    sl_mapping_t m = {};
    m.input_base = dev->base;
    m.length = dev->ops.aperture;
    m.output_base = 0;
    m.ep = &dev->map_ep;
    int err = sl_mappper_add_mapping(sl_device_get_mapper(&b->dev), &m);
    if (err) return err;
    sl_device_retain(dev);
    sl_list_add_tail(&b->dev_list, &dev->node);
    return 0;
}

sl_mapper_t * bus_get_mapper(sl_bus_t *b) { return &b->dev.mapper; }

sl_dev_t * bus_get_device_for_name(sl_bus_t *b, const char *name) {
    sl_list_node_t *n = sl_list_peek_head(&b->dev_list);
    for ( ; n != NULL; n = n->next) {
        sl_dev_t *d = containerof(n, sl_dev_t, node);
        if (!strcmp(name, d->obj_.name)) return d;
    }
    return NULL;
}

void bus_destroy(sl_bus_t *bus) {
    sl_list_node_t *c;
    while ((c = sl_list_remove_head(&bus->dev_list)) != NULL)
        sl_device_release(containerof(c, sl_dev_t, node));
    while ((c = sl_list_remove_head(&bus->mem_list)) != NULL)
        mem_region_destroy((mem_region_t *)c);
    device_embedded_shutdown(&bus->dev);
    free(bus);
}

static const sl_dev_ops_t bus_ops = {
    .read = bus_op_read,
    .write = bus_op_write,
};

int bus_create(const char *name, sl_bus_t **bus_out) {
    sl_bus_t *b = calloc(1, sizeof(*b));
    if (b == NULL) return SL_ERR_MEM;

    device_embedded_init(&b->dev, SL_DEV_BUS, name, &bus_ops);
    sl_device_set_context(&b->dev, b);
    sl_device_set_mapper_mode(&b->dev, SL_MAP_OP_MODE_TRANSLATE);
    sl_list_init(&b->mem_list);
    sl_list_init(&b->dev_list);
    *bus_out = b;
    return 0;
}
