// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/obj.h>

#define SL_OBJ_FLAG_EMBEDDED (1u << 0)

struct sl_obj {
    u2 refcount;
    u1 type;
    u1 flags;
    u4 id;
};

struct sl_obj_class {
    u4 size;
    int (*init)(void *o, const char *name, void *cfg);
    void (*shutdown)(void *o);
};

int sl_obj_init(void *o, u1 type, const char *name, void *cfg);

