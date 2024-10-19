// SPDX-License-Identifier: MIT License
// Copyright (c) 2024 Shac Ron and The Sled Project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <core/cache.h>
#include <sled/error.h>
#include <sled/slac.h>

#define DEFAULT_CACHE_SHIFT 12  // 4096

sl_cache_page_t * sl_cache_get_page(sl_cache_t *c, u8 addr) {
    const u1 shift = c->page_shift;
    const u8 base = addr >> shift;

    // is current page?
    const u1 hash = base & (SL_CACHE_ENTS - 1);
    if (base == c->base[hash]) return c->page[hash];

    // is in allocated list?
    // todo: split lists by hash
    for (sl_list_node_t *n = sl_list_peek_first(&c->allocated_list); n != NULL; n = n->next) {
        sl_cache_page_t *pg = (sl_cache_page_t *)n;
        if (pg->base == base) {
            c->base[hash] = base;
            c->page[hash] = pg;
            return c->page[hash];
        }
    }
    return NULL;
}

int sl_cache_get_instruction(sl_cache_t *c, u8 addr, sl_slac_inst_t **inst_out) {
    sl_cache_page_t *pg = sl_cache_get_page(c, addr);
    if (pg == NULL) {
        c->miss_addr = addr;
        return SL_ERR_NOT_FOUND;
    }
    const u8 offset = (addr & ((1u << c->page_shift) - 1)) >> 1;
    sl_slac_inst_t *si = sl_cache_page_slac_inst(c, pg);
    *inst_out = &si[offset];
    return 0;
}

sl_slac_inst_t * sl_cache_page_slac_inst(sl_cache_t *c, sl_cache_page_t *pg) {
    const u4 pg_bytes = 1u << c->page_shift;
    return (sl_slac_inst_t *)(pg->buffer + pg_bytes);
}

int sl_cache_alloc_page(sl_cache_t *c, u8 addr, sl_cache_page_t **pg_out) {
    const u4 pg_bytes = 1u << c->page_shift;
    usize size = sizeof(sl_cache_page_t) + pg_bytes;
    if (c->type == SL_CACHE_TYPE_INSTRUCTION)
        size += (pg_bytes / 2) * sizeof(sl_slac_inst_t);
    sl_cache_page_t *pg = malloc(size);
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

void sl_cache_init(sl_cache_t *c, u1 type) {
    c->page_shift = DEFAULT_CACHE_SHIFT;
    c->type = type;
    for (int i = 0; i < SL_CACHE_ENTS; i++)
        c->base[i] = ~((u8)0);
    sl_list_init(&c->allocated_list);
    sl_list_init(&c->free_list);
}

void sl_cache_shutdown(sl_cache_t *c) {
    sl_list_node_t *n;
    for (n = sl_list_remove_first(&c->allocated_list); n != NULL; n = sl_list_remove_first(&c->allocated_list))
        free(n);
    for (n = sl_list_remove_first(&c->free_list); n != NULL; n = sl_list_remove_first(&c->free_list))
        free(n);
}
