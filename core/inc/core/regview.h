// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/obj.h>
#include <sled/regview.h>

#define HASH_ENTS   256
#define MAX_DEVS    40

typedef struct {
    u4 index;
    u4 value;
    sl_dev_t *dev;  // todo: optimize storage
} hash_item_t;

typedef struct {
    u4 offset;
    u4 count;
} hash_ent_t;

typedef struct {
    u4 count;
    hash_item_t *items;
    hash_ent_t ent[HASH_ENTS];
} hash_t;

struct sl_reg_view {
    sl_obj_t obj_;
    sl_dev_t dev;
    hash_t hash;
    sl_dev_ops_t ops;
    u4 dev_count;
    u4 dev_view_offset[MAX_DEVS];
    sl_dev_t *dev_list[MAX_DEVS];
};
