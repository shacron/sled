#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include <core/obj.h>

static atomic_uint_least32_t g_obj_id = 0;

sl_obj_t * sl_allocate_as_obj(usize size, void (*shutdown)(void *o)) {
    size += sizeof(sl_obj_t);
    sl_obj_t *o = calloc(1, size);
    if (o == NULL) return NULL;
    o->refcount = 1;
    o->flags = 0;
    o->shutdown = shutdown;
    o->id = atomic_fetch_add_explicit(&g_obj_id, 1, memory_order_relaxed);
    return o;
}

void * sl_obj_get_item(sl_obj_t *o) {
    assert(o != NULL);
    return o+1;
}

void sl_obj_retain(sl_obj_t *o) {
    assert(o != NULL);
    assert(o->refcount != 0);
    atomic_fetch_add_explicit((_Atomic u2 *)&o->refcount, 1, memory_order_relaxed);
}

void sl_obj_release(sl_obj_t *o) {
    // if (o == NULL) return;
    assert(o != NULL);
    assert(o->refcount > 0);
    atomic_fetch_sub_explicit((_Atomic u2 *)&o->refcount, 1, memory_order_relaxed);
    if (o->refcount == 0) {
        // printf("obj_release shutting down %s\n", o->name);
        if (o->shutdown != NULL) o->shutdown(sl_obj_get_item(o));
        free(o);
    }
}
