#pragma once

#include <stdint.h>

typedef struct elf_object elf_object_t;
typedef struct sym_list sym_list_t;

typedef struct {
    uint64_t addr;
    uint64_t size;
    uint32_t flags;
    char *name;
} sym_entry_t;

struct sym_list {
    sym_list_t *next;
    char *name;
    uint64_t num;
    sym_entry_t *ent;
};

int elf_read_symbols(elf_object_t *obj, sym_list_t *list);

void sym_free(sym_list_t *list);
