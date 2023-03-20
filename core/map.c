// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <stdlib.h>
#include <string.h>

#include <core/common.h>
#include <core/event.h>
#include <sled/error.h>
#include <sled/io.h>
#include <sled/map.h>

#define MAP_ALLOC_INCREMENT 256

typedef struct {
    uint64_t va_base;
    uint64_t va_end;
    uint64_t pa_base;
    uint32_t domain;
    uint16_t permissions;
    sl_io_port_t *port;
    void *cookie;
} map_ent_t;

struct sl_map {
    uint32_t num_ents;
    map_ent_t **list;
    sl_io_port_t port;
    sl_event_queue_t *q;
};

static int ent_compare(const void *v0, const void *v1) {
    map_ent_t *a = *(map_ent_t **)v0;
    map_ent_t *b = *(map_ent_t **)v1;
    if (a->va_base < b->va_base) return -1;
    if (a->va_base > b->va_base) return 1;
    return 0;
}

int sl_mappper_add_mapping(sl_map_t *m, sl_map_entry_t *ent) {
    map_ent_t *n = malloc(sizeof(*n));
    if (n == NULL) return SL_ERR_MEM;
    n->va_base = ent->input_base;
    n->va_end = ent->input_base + ent->length;
    n->pa_base = ent->output_base;
    n->domain = ent->domain;
    n->permissions = ent->permissions;
    n->port = ent->port;
    n->cookie = ent->cookie;

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
    qsort(m->list, m->num_ents, sizeof(map_ent_t *), ent_compare);
    return 0;
}

static map_ent_t * ent_for_address(sl_map_t *m, uint64_t addr) {
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

static int sl_mapper_port_io(sl_io_port_t *p, sl_io_op_t *op, void *cookie) {
    sl_map_t *m = containerof(p, sl_map_t, port);
    return sl_mapper_io(m, op);
}

int sl_mapper_io(sl_map_t *m, sl_io_op_t *op) {
    const uint16_t size = op->size;
    uint32_t count = op->count;
    uint64_t len = count * size;
    uint64_t addr = op->addr;
    sl_io_op_t subop = *op;
    int err = 0;

    while (len) {
        // todo: check alignment

        map_ent_t *e = ent_for_address(m, addr);
        if (e == NULL) return SL_ERR_IO_NOMAP;

        uint64_t offset = addr - e->va_base;
        uint64_t avail = e->va_end - e->va_base - offset;
        if (avail > len) avail = len;

        // todo: verify we are not lopping off trailing bytes

        subop.addr = e->pa_base + offset;
        subop.count = avail / size;

        if ((err = e->port->io(e->port, &subop, e->cookie))) {
            return err;
        }
        len -= avail;
        addr += avail;
        subop.buf += avail;
    }
    return 0;
}

void sl_mapper_set_event_queue(sl_map_t *m, sl_event_queue_t *eq) { m->q = eq; }

int sl_mapper_event_send_async(sl_map_t *m, sl_event_t *ev) {
    if (m->q == NULL) return SL_ERR_UNSUPPORTED;
    return sl_event_send_async(m->q, ev);
}

int sl_mapper_create(sl_map_t **map_out) {
    sl_map_t *m = calloc(1, sizeof(*m));
    if (m == NULL) return SL_ERR_MEM;
    m->port.io = sl_mapper_port_io;
    *map_out = m;
    return 0;
}

void sl_mapper_destroy(sl_map_t *m) {
    if (m == NULL) return;
    for (uint32_t i = 0; i < m->num_ents; i++) {
        free(m->list[i]);
    }
    free(m->list);
    free(m);
}

