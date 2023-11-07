// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <core/common.h>
#include <core/device.h>
#include <core/regview.h>
#include <sled/error.h>

sl_dev_t * sl_reg_view_get_dev(sl_reg_view_t *rv) { return &rv->dev; };

static inline u4 hash_bits(u4 val) {
    return (val >> 2) & 0xff;
}

static hash_item_t * hash_lookup(hash_t *h, u4 i) {
    const u4 bits = hash_bits(i);
    hash_ent_t *e = &h->ent[bits];
    hash_item_t *it = &h->items[e->offset];
    for (u4 c = 0; c < e->count; c++) {
        if (it[c].index == i) return &it[c];
    }
    return NULL;
}

static void hash_insert_item(hash_t *h, u4 index, u4 value, sl_dev_t *dev) {
    const u4 bits = hash_bits(index);
    hash_ent_t *e = &h->ent[bits];
    hash_item_t *it = &h->items[e->offset];
    it[e->count].index = index;
    it[e->count].value = value;
    it[e->count].dev = dev;
    e->count++;
    h->count++;
}

int sl_reg_view_add_mapping(sl_reg_view_t *rv, sl_dev_t *dev, u4 view_offset, const u4 *view_addr, const u4 *dev_addr, u4 addr_count) {
    if (addr_count == 0) return SL_ERR_ARG;

    if (rv->dev_count == MAX_DEVS) return SL_ERR_FULL;

    // todo: verify there are no conflicts

    hash_t *h = &rv->hash;
    hash_item_t *prev_items = h->items;
    u4 prev_count = h->count;

    h->items = malloc((addr_count + prev_count) * sizeof(hash_item_t));
    if (h->items == NULL) {
        h->items = prev_items;
        return SL_ERR_MEM;
    }

    for (int i = 0; i < addr_count; i++) {
        const u4 bits = hash_bits(view_addr[i] + view_offset);
        h->ent[bits].count++;
    }

    u4 offset = 0;
    for (int i = 0; i < HASH_ENTS; i++) {
        h->ent[i].offset = offset;
        offset += h->ent[i].count;
        h->ent[i].count = 0;
    }

    h->count = 0;

    // copy existing entries
    for (int i = 0; i < prev_count; i++)
        hash_insert_item(h, prev_items[i].index, prev_items[i].value, prev_items[i].dev);
    free(prev_items);

    u4 max_vaddr = 0;
    for (int i = 0; i < addr_count; i++) {
        u4 addr;
        if (dev_addr == NULL) addr = i; // linear addressing
        else addr = dev_addr[i];
        const u4 vaddr = view_addr[i] + view_offset;
        hash_insert_item(h, vaddr, addr, dev);
        if (vaddr > max_vaddr) max_vaddr = vaddr;
    }
    rv->dev.aperture = max_vaddr + 4;
    rv->dev_view_offset[rv->dev_count] = view_offset;
    rv->dev_list[rv->dev_count] = dev;
    rv->dev_count++;
    return 0;
}

static int reg_view_device_read(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;
    if (addr & 3) return SL_ERR_IO_ALIGN;

    sl_reg_view_t *rv = ctx;
    hash_item_t *it = hash_lookup(&rv->hash, addr);
    if (it == NULL) return SL_ERR_IO_INVALID;
    sl_dev_t *d = it->dev;
    return d->ops.read(d->context, it->value << 2, size, count, buf);
}

static int reg_view_device_write(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;
    if (addr & 3) return SL_ERR_IO_ALIGN;

    sl_reg_view_t *rv = ctx;
    hash_item_t *it = hash_lookup(&rv->hash, addr);
    if (it == NULL) return SL_ERR_IO_INVALID;
    sl_dev_t *d = it->dev;
    return d->ops.write(d->context, it->value << 2, size, count, buf);
}

void reg_view_obj_shutdown(void *o) {
    sl_reg_view_t *rv = o;
    for (u4 i = 0; i < rv->dev_count; i++)
        sl_obj_release(rv->dev_list[i]);
    sl_obj_release(&rv->dev);
    free(rv->hash.items);
}

int reg_view_obj_init(void *o, const char *name, void *cfg) {
    sl_reg_view_t *rv = o;
    rv->ops.type = SL_DEV_REG_VIEW;
    rv->ops.read = reg_view_device_read;
    rv->ops.write = reg_view_device_write;
    int err = sl_obj_init(&rv->dev, SL_OBJ_TYPE_DEVICE, name, &rv->ops);
    if (err) return err;
    return 0;
}

int sl_reg_view_allocate(const char *name, sl_reg_view_t **rv_out) {
    sl_reg_view_t *rv;
    int err = sl_obj_alloc_init(SL_OBJ_TYPE_REGVIEW, name, NULL, (void **)&rv);
    if (err) return err;
    *rv_out = rv;
    sl_device_set_context(&rv->dev, rv);
    return 0;
}

void sl_reg_view_print_mappings(sl_dev_t *d, u8 base) {
    sl_reg_view_t *rv = containerof(d, sl_reg_view_t, dev);
    for (u4 i = 0; i < rv->dev_count; i++) {
        printf("                     > %#20" PRIx64 "                      %s\n", base + rv->dev_view_offset[i], rv->dev_list[i]->name);
    }
}
