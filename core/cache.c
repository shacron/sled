// SPDX-License-Identifier: MIT License
// Copyright (c) 2024-2025 Shac Ron and The Sled Project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <core/cache.h>
#include <sled/error.h>
#include <sled/slac.h>

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

int sl_cache_get_instruction(sl_cache_t *c, u8 addr, sl_slac_inst_t **inst_out) {
    const u1 shift = c->page_shift;
    const u8 base = addr >> shift;
    const u8 offset = addr - (base << shift);
    const u1 hash = base_hash(base);

    if (base != c->page[hash].base) {
        c->miss_addr = addr;
        return SL_ERR_NOT_FOUND;
    }
    *inst_out = &c->page[hash].decoded[offset / 2];
    return 0;
}

void sl_cache_set_data_page(sl_cache_t *c, u8 addr, void *buf) {
    const u1 shift = c->page_shift;
    const u8 base = addr >> shift;
    const u1 hash = base_hash(base);
    c->page[hash].base = base;
    c->page[hash].buf = buf;
}

void sl_cache_set_instruction_page(sl_cache_t *c, u8 addr, void *buf, bool overread) {
    const u1 shift = c->page_shift;
    const u8 base = addr >> shift;
    const u1 hash = base_hash(base);
    c->page[hash].base = base;
    c->page[hash].buf = buf;

    const u4 pg_size = 1u << shift;
    sl_slac_inst_t *d = c->page[hash].decoded;
    const u2 *inst = buf;
    int i;
    for (i = 0; i < (pg_size / 2) - 1; i++) {
        d[i].raw = SLAC_IN_INVALID;
        d[i].desc.machine_op = inst[0] | ((u4)inst[1] << 16);
        inst++;
    }
    d[i].raw = SLAC_IN_INVALID;
    if (overread) {
        // buffer extends beyond page size.
        // we can fetch the last bytes of unaligned instructions spanning page boundaries
        d[i].desc.machine_op = inst[0] | ((u4)inst[1] << 16);
    } else {
        d[i].desc.machine_op = inst[0];
    }
}

int sl_cache_init(sl_cache_t *c, u1 type) {
    c->page_shift = DEFAULT_CACHE_SHIFT;
    c->type = type;
    for (int i = 0; i < SL_CACHE_ENTS; i++)
        c->page[i].base = ~((u8)0);
    c->hash_hit = 0;
    c->hash_miss = 0;
    if (type == SL_CACHE_TYPE_INSTRUCTION) {
        const u4 pg_bytes = 1u << c->page_shift;
        const usize decode_size = (pg_bytes / 2) * sizeof(sl_slac_inst_t);
        void *buf = malloc(SL_CACHE_ENTS * decode_size);
        if (buf == NULL)
            return SL_ERR_MEM;
        for (int i = 0; i < SL_CACHE_ENTS; i++) {
            c->page[i].decoded = buf;
            buf += decode_size;
        }
    }
    return 0;
}

void sl_cache_shutdown(sl_cache_t *c) {}
