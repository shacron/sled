#pragma once

#include <sled/sym.h>

struct sl_sym_list {
    sl_sym_list_t *next;
    char *name;
    u8 num;
    sl_sym_entry_t *ent;
};

int sl_elf_symbol_list_load(sl_elf_obj_t *obj, sl_sym_list_t *list);
void sl_symbol_list_free(sl_sym_list_t *list);
