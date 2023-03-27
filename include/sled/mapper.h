// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

/*
A mapper is an abstract address translation device.

Examples of translators:
    Bus - map access from bus address to the target device
    MPU - memory protection unit performs static region mapping and protection
    MMU - memory manager unit performs dynamic page mapping and protection

The core mapper is implemented in sled-core and used by the generic bus.
Other implementations exist as device drivers, called map drivers.

A mapper instance operates in the context of the mapping client, typically
the associated CPU core or a chained mapper. Map drivers operate in device context.
Device context may or may not be the same as the mapping client. To synchronize
changes from the map driver to the mapper, the mapping client's async command queue
must be used. The map driver must not block the mapping client from completing its work.
*/

#define SL_MAP_EV_TYPE_UPDATE       0x1000

#define SL_MAP_OP_MODE_BLOCK        (0u)
#define SL_MAP_OP_MODE_PASSTHROUGH  (1u)
#define SL_MAP_OP_MODE_TRANSLATE    (2u)
#define SL_MAP_OP_MODE_MASK         (3u)

#define SL_MAP_OP_REPLACE           (1u << 2)

struct sl_mapper_entry {
    uint64_t input_base;
    uint64_t length;
    uint64_t output_base;
    uint32_t domain;
    uint16_t permissions;
    io_func_t *io;
    void *context;
};

// setup

int sl_mapper_create(sl_mapper_t **map_out);
void sl_mapper_destroy(sl_mapper_t *m);

int sl_mappper_add_mapping(sl_mapper_t *m, sl_mapper_entry_t *ent);
int sl_mapper_io(void *ctx, sl_io_op_t *op);
sl_mapper_t * sl_mapper_get_next(sl_mapper_t *m);