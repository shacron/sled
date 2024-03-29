// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/bus.h>
#include <core/common.h>
#include <sled/error.h>
#include <sled/io.h>


// #define BUS_TRACE 1

#ifdef BUS_TRACE
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(format, ...) do {} while(0)
#endif

static int bus_op_read(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    sl_bus_t *b = ctx;

    sl_io_op_t op = {};
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.op = IO_OP_IN;
    op.align = 0;
    op.buf = buf;
    op.agent = b;
    return sl_mapper_io(&b->mapper, &op);
}

static int bus_op_write(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    sl_bus_t *b = ctx;

    TRACE("[[ bus: %#" PRIx64 "(%u@%u) ]]\n", addr, count, size);

    sl_io_op_t op = {};
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.op = IO_OP_OUT;
    op.align = 0;
    op.buf = buf;
    op.agent = b;
    return sl_mapper_io(&b->mapper, &op);
}

int bus_add_mem_region(sl_bus_t *b, mem_region_t *r) {
    sl_mapping_t m = {};
    m.input_base = r->base;
    m.length = r->length;
    m.output_base = 0;
    m.type = SL_MAP_TYPE_MEMORY;
    m.ep = &r->ep;
    int err = sl_mappper_add_mapping(&b->mapper, &m);
    if (err) return err;
    sl_list_add_last(&b->mem_list, &r->node);
    return 0;
}

int bus_add_device(sl_bus_t *b, sl_dev_t *dev, u8 base) {
    dev->base = base;
    sl_mapping_t m = {};
    m.input_base = dev->base;
    m.length = dev->aperture;
    m.output_base = 0;
    m.type = SL_MAP_TYPE_DEVICE;
    m.ep = &dev->map_ep;
    return sl_mappper_add_mapping(&b->mapper, &m);
}

sl_mapper_t * bus_get_mapper(sl_bus_t *b) { return &b->mapper; }

static const sl_dev_ops_t bus_ops = {
    .type = SL_DEV_BUS,
    .read = bus_op_read,
    .write = bus_op_write,
};

int sl_bus_init(sl_bus_t *b, const char *name, sl_dev_config_t *cfg) {
    cfg->name = name;
    int err = sl_device_init(&b->dev, cfg);
    if (err) return err;
    mapper_init(&b->mapper);
    sl_mapper_set_mode(&b->mapper, SL_MAP_OP_MODE_TRANSLATE);
    sl_device_set_context(&b->dev, b);
    sl_device_set_mapper(&b->dev, &b->mapper);
    sl_list_init(&b->mem_list);
    return 0;
}

int sl_bus_create(const char *name, sl_dev_config_t *cfg, sl_bus_t **bus_out) {
    sl_bus_t *b = calloc(1, sizeof(*b));
    if (b == NULL) return SL_ERR_MEM;
    cfg->ops = &bus_ops;
    int err = sl_bus_init(b, name, cfg);
    if (err) {
        free(b);
        return err;
    }
    *bus_out = b;
    return 0;
}

void sl_bus_shutdown(sl_bus_t *b) {
    mapper_shutdown(&b->mapper);
    sl_list_node_t *c;
    while ((c = sl_list_remove_first(&b->mem_list)) != NULL)
        mem_region_destroy((mem_region_t *)c);
    sl_device_shutdown(&b->dev);
}

void sl_bus_destroy(sl_bus_t *b) {
    if (b == NULL) return;
    sl_bus_shutdown(b);
    free(b);
}
