// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/mapper.h>
#include <sled/list.h>

typedef struct {
    sl_list_node_t node;
    u64 base;
    u64 length;
    sl_map_ep_t ep;
    u8 data[];
} mem_region_t;

int mem_region_create(u64 base, u64 length, mem_region_t **m_out);
void mem_region_destroy(mem_region_t *m);
