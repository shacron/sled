#pragma once

#include <core/types.h>

struct sl_sym_entry {
    u8 addr;
    u8 size;
    u4 flags;
    char *name;
};

struct sl_sym_list {
    sl_sym_list_t *next;
    char *name;
    u8 num;
    sl_sym_entry_t *ent;
};

int elf_read_symbols(sl_elf_obj_t *obj, sl_sym_list_t *list);

void sym_free(sl_sym_list_t *list);
