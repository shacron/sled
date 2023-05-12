#include <stdlib.h>

#include <core/sym.h>

void sym_free(sym_list_t *list) {
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
