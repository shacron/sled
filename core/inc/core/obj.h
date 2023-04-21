// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <stddef.h>
#include <stdint.h>

#define SL_OBJ_TYPE_OBJ         0
#define SL_OBJ_TYPE_CORE        1
#define SL_OBJ_TYPE_DEVICE      2
#define SL_OBJ_TYPE_WORKER      3
#define SL_OBJ_TYPE_ENGINE      4
#define SL_OBJ_TYPE_INVALID     0xff

#define SL_OBJ_FLAG_EMBEDDED   (1u << 0)

typedef struct sl_obj sl_obj_t;
typedef struct sl_obj_vtable sl_obj_vtable_t;

struct sl_obj_vtable {
    uint8_t type;
    void (*shutdown)(void *o);
};

struct sl_obj {
    uint16_t refcount;
    uint8_t type;
    uint8_t flags;
    uint32_t id;
    const char *name;
    const sl_obj_vtable_t *vtab;
};

void sl_obj_embedded_init(sl_obj_t *o, const char *name, const sl_obj_vtable_t *vtab);
void sl_obj_embedded_shutdown(sl_obj_t *o);

void * sl_obj_allocate(size_t size, const char *name, const sl_obj_vtable_t *vtab);
void sl_obj_retain(sl_obj_t *o);
void sl_obj_release(sl_obj_t *o);
