// SPDX-License-Identifier: MIT License
// Copyright (c) 2024-2025 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>
#include <sled/list.h>

#define SL_CACHE_TYPE_INSTRUCTION   0
#define SL_CACHE_TYPE_DATA          1

#define SL_CACHE_ENTS   64

struct sl_cache_page {
    u8 base;
    void *buf;
    sl_slac_inst_t *decoded;
};

struct sl_cache {
    u1 page_shift;
    u1 type;
    u8 miss_addr;
    u8 hash_hit;
    u8 hash_miss;
    sl_cache_page_t page[SL_CACHE_ENTS];
};

int sl_cache_init(sl_cache_t *c, u1 type);
void sl_cache_shutdown(sl_cache_t *c);

int sl_cache_rw_single(sl_cache_t *c, u8 addr, usize size, void *buf, bool read);
int sl_cache_read(sl_cache_t *c, u8 addr, usize size, void *buf);
int sl_cache_write(sl_cache_t *c, u8 addr, usize size, void *buf);

int sl_cache_get_instruction(sl_cache_t *c, u8 addr, sl_slac_inst_t **inst_out);

void sl_cache_set_data_page(sl_cache_t *c, u8 base, void *buf);
void sl_cache_set_instruction_page(sl_cache_t *c, u8 addr, void *buf, bool overread);

void sl_cache_invalidate_page(sl_cache_t *c, u8 addr);
void sl_cache_invalidate_all(sl_cache_t *c);
