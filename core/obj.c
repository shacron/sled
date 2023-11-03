#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include <core/obj.h>
#include <core/types.h>
#include <sled/error.h>

const sl_obj_class_t * sl_obj_class_for_type(u1 type);

void sl_obj_retain(void *vo) {
    sl_obj_t *o = vo;
    assert(o != NULL);
    assert(o->refcount != 0);
    atomic_fetch_add_explicit((_Atomic u2 *)&o->refcount, 1, memory_order_relaxed);
}

void sl_obj_release(void *vo) {
    sl_obj_t *o = vo;
    assert(o != NULL);
    assert(o->refcount > 0);
    atomic_fetch_sub_explicit((_Atomic u2 *)&o->refcount, 1, memory_order_relaxed);
    if (o->refcount == 0) {
        const sl_obj_class_t *c = sl_obj_class_for_type(o->type);
        // printf("obj_release shutting down %s\n", o->name);
        c->shutdown(o);
        if ((o->flags & SL_OBJ_FLAG_EMBEDDED) == 0) free(o);
    }
}

static int obj_shared_init(sl_obj_t *o, uint8_t type, const char *name, void *cfg, uint8_t flags) {
    o->type = type;
    o->flags = flags;
    o->refcount = 1;
    const sl_obj_class_t *c = sl_obj_class_for_type(type);
    int err = c->init(o, name, cfg);
    if (err) free(o);
    return err;
}

int sl_obj_init(void *o, uint8_t type, const char *name, void *cfg) {
    return obj_shared_init(o, type, name, cfg, SL_OBJ_FLAG_EMBEDDED);
}

int sl_obj_alloc_init(uint8_t type, const char *name, void *cfg, void **o_out) {
    const sl_obj_class_t *c = sl_obj_class_for_type(type);
    sl_obj_t *o = calloc(1, c->size);
    if (o == NULL) return SL_ERR_MEM;
    int err = obj_shared_init(o, type, name, cfg, 0);
    if (err) {
        free(o);
        return err;
    }
    *o_out = o;
    return 0;
}
