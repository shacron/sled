#include <stdlib.h>

#include <core/sym.h>
#include <sled/error.h>

void sl_symbol_list_free(sl_sym_list_t *list) {
    if (list == NULL) return;
    if (list->name) free(list->name);
    if (list->ent != NULL) {
        for (u8 i = 0; i < list->num; i++) {
            if (list->ent[i].name != NULL) free(list->ent[i].name);
        }
        free(list->ent);
    }
    free(list);
}

int sl_sym_entry_for_addr(sl_sym_list_t *sl, u8 addr, sl_sym_entry_t *cached, sl_sym_entry_t **ent_out) {
    if (cached != NULL) {
        if ((cached->addr <= addr) && ((cached->addr + cached->size) > addr)) {
            *ent_out = cached;
            return 0;
        }
    }
    for (u4 i = 0; i < sl->num; i++) {
        sl_sym_entry_t *ent = &sl->ent[i];
        if ((ent->addr <= addr) && ((ent->addr + ent->size) > addr)) {
            *ent_out = ent;
            return 0;
        }
    }
    return SL_ERR_NOT_FOUND;
}
