// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/device.h>
#include <core/mem.h>
#include <core/obj.h>
#include <core/types.h>
// #include <sled/mapper.h>

struct sl_bus {
    sl_obj_t obj_;
    sl_dev_t dev;
    sl_mapper_t mapper;
    sl_list_t mem_list;
    sl_list_t dev_list;
};

int bus_create(const char *name, sl_dev_config_t *cfg, sl_bus_t **bus_out);

int bus_add_mem_region(sl_bus_t *b, mem_region_t *r);
int bus_add_device(sl_bus_t *b, sl_dev_t *dev, u8 base);
sl_dev_t * bus_get_device_for_name(sl_bus_t *b, const char *name);
sl_mapper_t * bus_get_mapper(sl_bus_t *b);
