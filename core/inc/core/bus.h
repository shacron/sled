// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#pragma once

#include <core/device.h>
#include <core/mem.h>
#include <core/types.h>

struct sl_bus {
    sl_dev_t dev;
    sl_mapper_t mapper;
    sl_list_t mem_list;
};

int sl_bus_create(const char *name, sl_dev_config_t *cfg, sl_bus_t **bus_out);
void sl_bus_destroy(sl_bus_t *b);

int sl_bus_init(sl_bus_t *b, const char *name, sl_dev_config_t *cfg);
void sl_bus_shutdown(sl_bus_t *b);

int bus_add_mem_region(sl_bus_t *b, mem_region_t *r);
int bus_add_device(sl_bus_t *b, sl_dev_t *dev, u8 base);
sl_dev_t * bus_get_device_for_name(sl_bus_t *b, const char *name);
sl_mapper_t * bus_get_mapper(sl_bus_t *b);
