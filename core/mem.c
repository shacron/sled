// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stdlib.h>

#include <core/mem.h>
#include <sled/error.h>

int mem_region_create(uint64_t base, uint64_t length, mem_region_t **m_out) {
    const size_t size = length + sizeof(mem_region_t);
    mem_region_t *m = calloc(1, size);
    if (m == NULL) return SL_ERR_MEM;

    m->base = base;
    m->length = length;
    *m_out = m;
    return 0;
}

void mem_region_destroy(mem_region_t *m) {
    free(m);
}

