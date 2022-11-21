#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t base;
    uint64_t end;
    void *data;
} mem_region_t;

int mem_region_create(mem_region_t *m, uint64_t base, uint64_t length);
int mem_region_destroy(mem_region_t *m);

#ifdef __cplusplus
}
#endif
