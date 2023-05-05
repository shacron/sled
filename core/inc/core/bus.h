// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/device.h>
#include <core/mem.h>
#include <core/types.h>

int bus_create(const char *name, sl_bus_t **bus_out);
void bus_destroy(sl_bus_t *bus);

int bus_add_mem_region(sl_bus_t *b, mem_region_t *r);
int bus_add_device(sl_bus_t *b, sl_dev_t *dev, u64 base);
sl_dev_t * bus_get_device_for_name(sl_bus_t *b, const char *name);
sl_mapper_t * bus_get_mapper(sl_bus_t *b);
