// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>

typedef struct sl_obj sl_obj_t;

struct sl_obj {
    u2 refcount;
    u1 _unused;
    u1 flags;
    u4 id;
    void (*shutdown)(void *o);
};

// Allocates an obj wrapper around item of size `size`.
// Allocated objects are retained. Calling sl_obj_release() will invoke the `shutdown`
// function and then free the memory.
sl_obj_t * sl_allocate_as_obj(size_t size, void (*shutdown)(void *o));
// Get pointer to the allocated item of the object
void * sl_obj_get_item(sl_obj_t *o);

// Increase the reference count of the object by one
void sl_obj_retain(sl_obj_t *o);
// Decrease the reference count by one. If reference count is zero, call the object's
// shutdown function and then free the memory.
void sl_obj_release(sl_obj_t *o);
