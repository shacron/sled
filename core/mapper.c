// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <stdlib.h>
#include <string.h>

#include <core/common.h>
#include <core/event.h>
#include <core/mapper.h>
#include <sled/error.h>
#include <sled/io.h>

#define MAP_ALLOC_INCREMENT 256

struct map_ent {
    uint64_t va_base;
    uint64_t va_end;
    uint64_t pa_base;
    uint32_t domain;
    uint16_t permissions;
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

static map_ent_t * ent_for_address(sl_mapper_t *m, uint64_t addr) {
    if (m->num_ents == 0) return NULL;

    uint32_t start, end, cur;
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

void mapper_set_mode(sl_mapper_t *m, int mode) { m->mode = mode; }
sl_mapper_t * sl_mapper_get_next(sl_mapper_t *m) { return m->next; }
sl_map_ep_t * sl_mapper_get_ep(sl_mapper_t *m) { return &m->ep; }

static int mapper_ep_io(sl_map_ep_t *ep, sl_io_op_t *op) {
    sl_mapper_t *m = containerof(ep, sl_mapper_t, ep);
    if (m->mode == SL_MAP_OP_MODE_BLOCK) return SL_ERR_IO_NOMAP;
    if (m->mode == SL_MAP_OP_MODE_PASSTHROUGH) return sl_mapper_io(m->next, op);

    const uint16_t size = op->size;
    uint32_t count = op->count;
    uint64_t len = count * size;
    uint64_t addr = op->addr;
    sl_io_op_t subop = *op;
    int err = 0;

    while (len) {
        // todo: check alignment

        map_ent_t *e = ent_for_address(m, addr);
        if (e == NULL)
            return SL_ERR_IO_NOMAP;

        uint64_t offset = addr - e->va_base;
        uint64_t avail = e->va_end - e->va_base - offset;
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

    uint32_t op = ev->arg[0];
    if (op & SL_MAP_OP_REPLACE) {
        uint32_t count = ev->arg[1];
        sl_mapping_t *ent_list = (sl_mapping_t *)(ev->arg[2]);

        uint32_t size = ((count + MAP_ALLOC_INCREMENT - 1) / MAP_ALLOC_INCREMENT) * MAP_ALLOC_INCREMENT;
        map_ent_t **list = calloc(size, sizeof(map_ent_t *));
        if (list == NULL) return SL_ERR_MEM;

        for (uint32_t i = 0; i < count; i++) {
            list[i] = create_map_ent(ent_list + i);
            if (list[i] == NULL) {
                err = SL_ERR_MEM;
                break;
            }
        }

        uint32_t fnum = m->num_ents;
        map_ent_t **flist = m->list;
        if (err) {
            fnum = count;
            flist = list;
        }
        for (uint32_t i = 0; i < fnum; i++) {
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
    for (uint32_t i = 0; i < m->num_ents; i++) {
        free(m->list[i]);
    }
    free(m->list);
}

void sl_mapper_destroy(sl_mapper_t *m) {
    if (m == NULL) return;
    mapper_shutdown(m);
    free(m);
}

