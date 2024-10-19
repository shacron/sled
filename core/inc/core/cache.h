// SPDX-License-Identifier: MIT License
// Copyright (c) 2024 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>
#include <sled/list.h>

#define SL_CACHE_TYPE_INSTRUCTION   0
#define SL_CACHE_TYPE_DATA          1

#define SL_CACHE_ENTS   8

struct sl_cache_page {
    sl_list_node_t node;
    u8 base;
    u1 buffer[];
};

struct sl_cache {
    u1 page_shift;
    u1 type;
    u8 base[SL_CACHE_ENTS];
    sl_cache_page_t *page[SL_CACHE_ENTS];
    u8 miss_addr;
    sl_list_t allocated_list;
    sl_list_t free_list;
};

void sl_cache_init(sl_cache_t *c, u1 type);
void sl_cache_shutdown(sl_cache_t *c);

int sl_cache_get_instruction(sl_cache_t *c, u8 addr, sl_slac_inst_t **inst_out);
sl_cache_page_t * sl_cache_get_page(sl_cache_t *c, u8 addr);
int sl_cache_alloc_page(sl_cache_t *c, u8 addr, sl_cache_page_t **pg_out);
void sl_cache_fill_page(sl_cache_t *c, sl_cache_page_t *pg);
void sl_cache_discard_unfilled_page(sl_cache_t *c, sl_cache_page_t *pg);
sl_slac_inst_t * sl_cache_page_slac_inst(sl_cache_t *c, sl_cache_page_t *pg);

void sl_cache_invalidate_page(sl_cache_t *c, u8 addr);
void sl_cache_invalidate_all(sl_cache_t *c);
