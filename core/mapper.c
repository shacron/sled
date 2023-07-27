// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/common.h>
#include <core/device.h>
#include <core/event.h>
#include <core/mapper.h>
#include <sled/error.h>
#include <sled/io.h>

#define MAP_ALLOC_INCREMENT 256

struct map_ent {
    u8 va_base;
    u8 va_end;
    u8 pa_base;
    u4 domain;
    u2 permissions;
    u1 type;
    sl_map_ep_t *ep;
};

static int ent_compare(const void *v0, const void *v1) {
    map_ent_t *a = *(map_ent_t **)v0;
    map_ent_t *b = *(map_ent_t **)v1;
    if (a->va_base < b->va_base) return -1;
    if (a->va_base > b->va_base) return 1;
    return 0;
}

static map_ent_t * create_map_ent(sl_mapping_t *m) {
    map_ent_t *n = malloc(sizeof(*n));
    if (n == NULL) return NULL;
    n->va_base = m->input_base;
    n->va_end = m->input_base + m->length;
    n->pa_base = m->output_base;
    n->domain = m->domain;
    n->permissions = m->permissions;
    n->type = m->type;
    n->ep = m->ep;
    return n;
}

static void finalize_mappings(sl_mapper_t *m) {
    qsort(m->list, m->num_ents, sizeof(map_ent_t *), ent_compare);
}

int sl_mappper_add_mapping(sl_mapper_t *m, sl_mapping_t *ent) {
    map_ent_t *n = create_map_ent(ent);
    if (n == NULL) return SL_ERR_MEM;

    if ((m->num_ents % MAP_ALLOC_INCREMENT) == 0) {
        void * p = realloc(m->list, (m->num_ents + MAP_ALLOC_INCREMENT) * sizeof(void *));
        if (p == NULL) {
            free(n);
            return SL_ERR_MEM;
        }
        m->list = p;
    }

    m->list[m->num_ents] = n;
    m->num_ents++;
    finalize_mappings(m);
    return 0;
}

static map_ent_t * ent_for_address(sl_mapper_t *m, u8 addr) {
    if (m->num_ents == 0) return NULL;

    u4 start, end, cur;
    start = 0;
    end = m->num_ents;

    do {
        cur = (start + end) / 2;
        map_ent_t *ent = m->list[cur];
        if (ent->va_base > addr) {
            end = cur;
        } else {
            if (ent->va_end > addr) return ent; // found
            if (start == cur) break;
            start = cur;
        }
    } while (start != end);
    return NULL;
}

void sl_mapper_set_mode(sl_mapper_t *m, int mode) { m->mode = mode; }
sl_mapper_t * sl_mapper_get_next(sl_mapper_t *m) { return m->next; }
sl_map_ep_t * sl_mapper_get_ep(sl_mapper_t *m) { return &m->ep; }

static int mapper_ep_io(sl_map_ep_t *ep, sl_io_op_t *op) {
    sl_mapper_t *m = containerof(ep, sl_mapper_t, ep);
    if (m->mode == SL_MAP_OP_MODE_BLOCK) return SL_ERR_IO_NOMAP;
    if (m->mode == SL_MAP_OP_MODE_PASSTHROUGH) return sl_mapper_io(m->next, op);

    int err = 0;
    u8 addr = op->addr;

    if (unlikely(IO_IS_ATOMIC(op->op))) {
        map_ent_t *e = ent_for_address(m, op->addr);
        if (e == NULL) return SL_ERR_IO_NOMAP;

        u8 offset = addr - e->va_base;
        op->addr = e->pa_base + offset;
        return e->ep->io(e->ep, op);
    }

    const u2 size = op->size;
    u8 len = size * op->count;
    sl_io_op_t subop = *op;

    while (len) {
        // todo: check alignment

        map_ent_t *e = ent_for_address(m, addr);
        if (e == NULL)
            return SL_ERR_IO_NOMAP;

        u8 offset = addr - e->va_base;
        u8 avail = e->va_end - e->va_base - offset;
        if (avail > len) avail = len;

        // todo: verify we are not lopping off trailing bytes

        subop.addr = e->pa_base + offset;
        subop.count = avail / size;

        if ((err = e->ep->io(e->ep, &subop))) return err;
        len -= avail;
        addr += avail;
        subop.buf += avail;
    }
    return 0;
}

int sl_mapper_io(void *ctx, sl_io_op_t *op) {
    sl_mapper_t *m = ctx;
    return mapper_ep_io(&m->ep, op);
}

int mapper_update(sl_mapper_t *m, sl_event_t *ev) {
    if (ev->type != SL_MAP_EV_TYPE_UPDATE) return SL_ERR_ARG;
    int err = 0;

    u4 op = ev->arg[0];
    if (op & SL_MAP_OP_REPLACE) {
        u4 count = ev->arg[1];
        sl_mapping_t *ent_list = (sl_mapping_t *)(ev->arg[2]);

        u4 size = ((count + MAP_ALLOC_INCREMENT - 1) / MAP_ALLOC_INCREMENT) * MAP_ALLOC_INCREMENT;
        map_ent_t **list = calloc(size, sizeof(map_ent_t *));
        if (list == NULL) return SL_ERR_MEM;

        for (u4 i = 0; i < count; i++) {
            list[i] = create_map_ent(ent_list + i);
            if (list[i] == NULL) {
                err = SL_ERR_MEM;
                break;
            }
        }

        u4 fnum = m->num_ents;
        map_ent_t **flist = m->list;
        if (err) {
            fnum = count;
            flist = list;
        }
        for (u4 i = 0; i < fnum; i++) {
            if (flist[i] != NULL) free(flist[i]);
        }
        free(flist);
        free(ent_list);
        if (err) return err;

        m->num_ents = count;
        m->list = list;
        finalize_mappings(m);
    }
    m->mode = op & SL_MAP_OP_MODE_MASK;
    return 0;
}

void mapper_init(sl_mapper_t *m) {
    memset(m, 0, sizeof(*m));
    m->ep.io = mapper_ep_io;
}

int sl_mapper_create(sl_mapper_t **map_out) {
    sl_mapper_t *m = calloc(1, sizeof(*m));
    if (m == NULL) return SL_ERR_MEM;
    m->ep.io = mapper_ep_io;
    *map_out = m;
    return 0;
}

void mapper_shutdown(sl_mapper_t *m) {
    for (u4 i = 0; i < m->num_ents; i++) {
        free(m->list[i]);
    }
    free(m->list);
}

void sl_mapper_destroy(sl_mapper_t *m) {
    if (m == NULL) return;
    mapper_shutdown(m);
    free(m);
}

void mapper_print_mappings(sl_mapper_t *m) {
    printf("mapper\n");
    bool has_next = false;
    switch (m->mode) {
    case SL_MAP_OP_MODE_PASSTHROUGH:
        printf("  passthrough\n");
        has_next = true;
        break;

    case SL_MAP_OP_MODE_BLOCK:
        printf("  blocked\n");
        break;

    case SL_MAP_OP_MODE_TRANSLATE:
        for (u4 i = 0; i < m->num_ents; i++) {
            sl_dev_t *d;
            map_ent_t *ent = m->list[i];
            printf("  %#20" PRIx64 " %#20" PRIx64 " %#20" PRIx64 "", ent->pa_base, ent->va_base, ent->va_end - 
            ent->va_base);

            switch (ent->type) {
            case SL_MAP_TYPE_MEMORY:
                printf(" memory\n");
                break;

            case SL_MAP_TYPE_DEVICE:
                d = device_get_for_ep(ent->ep);
                printf(" device: %s\n", d->name);
                break;

            case SL_MAP_TYPE_MAPPER:
                printf(" mapper\n");
                has_next = true;
                break;

            default:
                printf(" <unknown>\n");
                break;
            }
        }
        break;

    default:
        printf("  unhandled mode\n");
        break;
    }

    if (has_next) mapper_print_mappings(m->next);
}
