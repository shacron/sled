// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sled/io.h>

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

struct sl_map_entry {
    uint64_t input_base;
    uint64_t length;
    uint64_t output_base;
    uint32_t domain;
    uint16_t permissions;
    sl_io_port_t *port;
    void *cookie;
};

// setup

int sl_mapper_create(sl_map_t **map_out);
void sl_mapper_destroy(sl_map_t *m);

// synchronous calls - should only be made in the mapping client context

int sl_mappper_add_mapping(sl_map_t *m, sl_map_entry_t *ent);
int sl_mapper_io(sl_map_t *m, sl_io_op_t *op);

// async calls can only be called with 

int sl_mapper_enable_async(sl_map_t *m, bool enable);

// int sl_mapper_clear_mappings_async(sl_map_t *m);
int sl_mapper_update_mappings_async(sl_map_t *m);


