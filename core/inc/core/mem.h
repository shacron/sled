// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

typedef struct {
    uint64_t base;
    uint64_t end;
    void *data;
} mem_region_t;

int mem_region_create(mem_region_t *m, uint64_t base, uint64_t length);
int mem_region_destroy(mem_region_t *m);
