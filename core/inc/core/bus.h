// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/device.h>
#include <core/mem.h>
#include <core/types.h>

int bus_create(const char *name, bus_t **bus_out);
void bus_destroy(bus_t *bus);

int bus_add_mem_region(bus_t *b, mem_region_t *r);
int bus_add_device(bus_t *b, sl_dev_t *dev, uint64_t base);
sl_dev_t * bus_get_device_for_name(bus_t *b, const char *name);
sl_io_port_t * bus_get_port(bus_t *b);
