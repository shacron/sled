// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#include <core/list.h>

typedef struct {
    list_node_t node;
    uint64_t base;
    uint64_t length;
    uint8_t data[];
} mem_region_t;

int mem_region_create(uint64_t base, uint64_t length, mem_region_t **m_out);
void mem_region_destroy(mem_region_t *m);
