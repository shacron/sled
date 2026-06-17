// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2026 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// sl_sym_lookup_t

struct sl_sym_entry {
    u8 addr;
    u8 size;
    u4 flags;
    char *name;
};

// returns symbol entry containing 'addr'.
// if 'cached' is not null, it is searched first.
int sl_sym_entry_for_addr(sl_sym_list_t *sl, u8 addr, sl_sym_entry_t *cached, sl_sym_entry_t **ent_out);

#ifdef __cplusplus
}
#endif