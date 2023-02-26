// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#include <core/device.h>
#include <core/mem.h>

typedef struct bus bus_t;

int bus_create(bus_t **bus_out);
void bus_destroy(bus_t *bus);

int bus_read(bus_t *b, uint64_t addr, uint32_t size, uint32_t count, void *buf);
int bus_write(bus_t *b, uint64_t addr, uint32_t size, uint32_t count, void *buf);

int bus_add_mem_region(bus_t *b, mem_region_t r);
int bus_add_device(bus_t *b, device_t *dev, uint64_t base);
device_t * bus_get_device_for_name(bus_t *b, const char *name);
