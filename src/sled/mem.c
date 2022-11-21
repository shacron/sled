#include <stdlib.h>

#include <sl/mem.h>
#include <sled/error.h>

int mem_region_create(mem_region_t *m, uint64_t base, uint64_t length) {
    m->base = base;
    m->end = base + length;
    m->data = calloc(1, length);
    if (m->data == NULL) return SL_ERR_MEM;
    return 0;
}

int mem_region_destroy(mem_region_t *m) {
    if (m->data != NULL) free(m->data);
    m->data = NULL;
    return 0;
}

