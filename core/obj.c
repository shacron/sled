#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include <core/obj.h>

static atomic_uint_least32_t g_obj_id = 0;

static void obj_init_common(sl_obj_t *o, const char *name, const sl_obj_vtable_t *vtab, uint8_t flags) {
    o->refcount = 1;
    o->type = vtab->type;
    o->flags = flags;
    o->id = atomic_fetch_add_explicit(&g_obj_id, 1, memory_order_relaxed);
    o->name = name;
    o->vtab = vtab;
}

void sl_obj_embedded_init(sl_obj_t *o, const char *name, const sl_obj_vtable_t *vtab) {
    obj_init_common(o, name, vtab, SL_OBJ_FLAG_EMBEDDED);
    // printf("sl_obj_embedded_init %s\n", o->name);
}

void * sl_obj_allocate(size_t size, const char *name, const sl_obj_vtable_t *vtab) {
    sl_obj_t *o = calloc(1, size);
    if (o == NULL) return NULL;
    obj_init_common(o, name, vtab, 0);
    // printf("sl_obj_allocate %s\n", o->name);
    return o;
}

void sl_obj_embedded_shutdown(sl_obj_t *o) {
    assert(o->flags & SL_OBJ_FLAG_EMBEDDED);
    assert(o->refcount == 1);

    // printf("sl_obj_embedded_shutdown shutting down %s\n", o->name);
    if (o->vtab->shutdown != NULL) o->vtab->shutdown(o);
    o->type = SL_OBJ_TYPE_INVALID;
    o->vtab = NULL;
}

void sl_obj_retain(sl_obj_t *o) {
    assert(o != NULL);
    assert(o->type != SL_OBJ_TYPE_INVALID);

    atomic_fetch_add_explicit((_Atomic uint16_t *)&o->refcount, 1, memory_order_relaxed);
}

void sl_obj_release(sl_obj_t *o) {
    if (o == NULL) return;
    assert(o->refcount > 0);
    atomic_fetch_sub_explicit((_Atomic uint16_t *)&o->refcount, 1, memory_order_relaxed);
    if (o->refcount == 0) {
        // printf("obj_release shutting down %s\n", o->name);
        if (o->vtab->shutdown != NULL) o->vtab->shutdown(o);
        if ((o->flags & SL_OBJ_FLAG_EMBEDDED) == 0) {
            // printf("obj_release freeing %s\n", o->name);
            free(o);
        }
    }
}
