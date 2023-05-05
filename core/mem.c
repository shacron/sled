// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stdlib.h>
#include <string.h>

#include <core/common.h>
#include <core/mem.h>
#include <sled/error.h>
#include <sled/io.h>

static int mem_io(sl_map_ep_t *ep, sl_io_op_t *op) {
    mem_region_t *m = containerof(ep, mem_region_t, ep);
    void *data = m->data + op->addr;
    if (op->direction == IO_DIR_IN)
        memcpy(op->buf, data, op->count * op->size);
    else
        memcpy(data, op->buf, op->count * op->size);
    return 0;
}

int mem_region_create(u64 base, u64 length, mem_region_t **m_out) {
    const size_t size = length + sizeof(mem_region_t);
    mem_region_t *m = calloc(1, size);
    if (m == NULL) return SL_ERR_MEM;

    m->base = base;
    m->length = length;
    m->ep.io = mem_io;
    *m_out = m;
    return 0;
}

void mem_region_destroy(mem_region_t *m) {
    free(m);
}

