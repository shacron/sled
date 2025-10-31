// SPDX-License-Identifier: MIT License
// Copyright (c) 2024-2025 Shac Ron and The Sled Project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <core/cache.h>
#include <sled/error.h>

#define DEFAULT_CACHE_SHIFT 12  // 4096

static inline u1 base_hash(u8 base) {
    return base & (SL_CACHE_ENTS - 1);
}

int sl_cache_rw_single(sl_cache_t *c, u8 addr, usize size, void *buf, bool read) {
    const u1 shift = c->page_shift;
    const u8 base = addr >> shift;
    const u8 offset = addr - (base << shift);

    // is current page?
    const u1 hash = base_hash(base);
    if (base == c->page[hash].base) {
        c->hash_hit++;
        goto current_page;
    }

    c->miss_addr = addr;
    c->hash_miss++;
    return SL_ERR_NOT_FOUND;

current_page:
    if (read)
        memcpy(buf, c->page[hash].buf + offset, size);
    else
        memcpy(c->page[hash].buf + offset, buf, size);
    return 0;
}

static int cache_rw(sl_cache_t *c, u8 addr, usize size, void *buf, bool read) {
    const u8 pg_size = (1u << c->page_shift);
    usize remain = pg_size - (addr & (pg_size - 1));
    if (remain >= size)
        return sl_cache_rw_single(c, addr, size, buf, read);

    while (size) {
        int err = sl_cache_rw_single(c, addr, remain, buf, read);
        if (err)
            return err;
        addr += remain;
        size -= remain;
        buf += remain;
        remain = pg_size;
        if (size < pg_size)
            remain = size;
    }
    return 0;
}

int sl_cache_read(sl_cache_t *c, u8 addr, usize size, void *buf) {
    return cache_rw(c, addr, size, buf, true);
}

int sl_cache_write(sl_cache_t *c, u8 addr, usize size, void *buf) {
    return cache_rw(c, addr, size, buf, false);
}

void sl_cache_set_page(sl_cache_t *c, u8 addr, void *buf) {
    const u1 shift = c->page_shift;
    const u8 base = addr >> shift;
    const u1 hash = base_hash(base);
    c->page[hash].base = base;
    c->page[hash].buf = buf;
}

void sl_cache_init(sl_cache_t *c) {
    c->page_shift = DEFAULT_CACHE_SHIFT;
    for (int i = 0; i < SL_CACHE_ENTS; i++)
        c->page[i].base = ~((u8)0);
    c->hash_hit = 0;
    c->hash_miss = 0;
}

void sl_cache_shutdown(sl_cache_t *c) {}
