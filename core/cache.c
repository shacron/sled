// SPDX-License-Identifier: MIT License
// Copyright (c) 2024 Shac Ron and The Sled Project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <core/cache.h>
#include <sled/error.h>

#define DEFAULT_CACHE_SHIFT 12  // 4096

static int sl_cache_read_one(sl_cache_t *c, u8 addr, usize size, void *buf) {
    const u1 shift = c->page_shift;
    const u8 base = addr >> shift;
    const u8 offset = addr - (base << shift);

    // is current page?
    const u1 hash = base & (SL_CACHE_ENTS - 1);
    if (base == c->base[hash]) goto current_page;

    // is in allocated list?
    // todo: split lists by hash
    for (sl_list_node_t *n = sl_list_peek_first(&c->allocated_list); n != NULL; n = n->next) {
        sl_cache_page_t *pg = (sl_cache_page_t *)n;
        if (pg->base == base) {
            c->base[hash] = base;
            c->page[hash] = pg;
            goto current_page;
        }
    }
    c->miss_addr = addr;
    return SL_ERR_NOT_FOUND;

current_page:
    memcpy(buf, &c->page[hash]->buffer[offset], size);
    return 0;
}

int sl_cache_read(sl_cache_t *c, u8 addr, usize size, void *buf) {
    const u8 pg_size = (1u << c->page_shift);
    usize remain = pg_size - (addr & (pg_size - 1));
    if (remain >= size) return sl_cache_read_one(c, addr, size, buf);

    while (size) {
        int err = sl_cache_read_one(c, addr, remain, buf);
        if (err) return err;
        addr += remain;
        size -= remain;
        buf += remain;
        remain = pg_size;
        if (size < pg_size) remain = size;
    }
    return 0;
}

int sl_cache_alloc_page(sl_cache_t *c, u8 addr, sl_cache_page_t **pg_out) {
    sl_cache_page_t *pg = malloc(sizeof(*pg) + (1u << c->page_shift));
    if (pg == NULL) return SL_ERR_MEM;
    pg->base = addr >> c->page_shift;
    *pg_out = pg;
    return 0;
}

void sl_cache_fill_page(sl_cache_t *c, sl_cache_page_t *pg) {
    sl_list_add_first(&c->allocated_list, &pg->node);
}

void sl_cache_discard_unfilled_page(sl_cache_t *c, sl_cache_page_t *pg) {
    free(pg);
}

void sl_cache_init(sl_cache_t *c) {
    c->page_shift = DEFAULT_CACHE_SHIFT;
    for (int i = 0; i < SL_CACHE_ENTS; i++)
        c->base[i] = ~((u8)0);
    sl_list_init(&c->allocated_list);
    sl_list_init(&c->free_list);
}
