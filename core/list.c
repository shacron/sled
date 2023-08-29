// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <assert.h>

#include <sled/list.h>

void sl_list_init(sl_list_t *list) {
    list->first = NULL;
    list->last = NULL;
}

void sl_list_add_last(sl_list_t *list, sl_list_node_t *n) {
    n->next = NULL;
    if (list->first == NULL) {
        list->first = n;
        list->last = n;
    } else {
        list->last->next = n;
        list->last = n;
    }
}

void sl_list_add_first(sl_list_t *list, sl_list_node_t *n) {
    n->next = list->first;
    list->first = n;
    if (list->last == NULL) list->last = n;
}

sl_list_node_t * sl_list_remove_first(sl_list_t *list) {
    if (list->first == NULL) return NULL;
    sl_list_node_t *n = list->first;
    list->first = n->next;
    if(list->first == NULL) list->last = NULL;
    return n;
}

sl_list_node_t * sl_list_remove_all(sl_list_t *list) {
    sl_list_node_t *n = list->first;
    list->first = NULL;
    list->last = NULL;
    return n;
}

void sl_list_insert_sorted(sl_list_t *list, int(*compare)(const sl_list_node_t *, const sl_list_node_t *), sl_list_node_t *n) {
    if (list->first == NULL) {
        sl_list_add_first(list, n);
        return;
    }

    if (compare(n, list->first) <= 0) {
        n->next = list->first;
        list->first = n;
        return;
    }

    sl_list_node_t *cur = list->first->next;
    sl_list_node_t *prev = list->first;
    for ( ; cur != NULL; cur = cur->next) {
        if (compare(n, cur) <= 0) {
            n->next = cur;
            prev->next = n;
            return;
        }
        prev = cur;
    }
    sl_list_add_first(list, n);
}

void sl_list_remove_node(sl_list_t *list, sl_list_node_t *n, sl_list_node_t *prev) {
    if (list->first == NULL) return;
    sl_list_node_t *next;
    if (prev == NULL) {
        assert(n == list->first);
        next = list->first->next;
        list->first = next;
    } else {
        next = n->next;
        prev->next = next;
    }
    if (next == NULL) list->last = prev;
    n->next = NULL;
}

