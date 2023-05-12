#pragma once

#include <core/types.h>

struct sym_entry {
    u8 addr;
    u8 size;
    u4 flags;
    char *name;
};

struct sym_list {
    sym_list_t *next;
    char *name;
    u8 num;
    sym_entry_t *ent;
};

int elf_read_symbols(sl_elf_obj_t *obj, sym_list_t *list);

void sym_free(sym_list_t *list);
