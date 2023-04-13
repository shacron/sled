// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#include <sled/mapper.h>
#include <sled/list.h>

typedef struct {
    sl_list_node_t node;
    uint64_t base;
    uint64_t length;
    sl_map_ep_t ep;
    uint8_t data[];
} mem_region_t;

int mem_region_create(uint64_t base, uint64_t length, mem_region_t **m_out);
void mem_region_destroy(mem_region_t *m);
