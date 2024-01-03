// SPDX-License-Identifier: MIT License
// Copyright (c) 2023-2024 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sl_list_node {
    struct sl_list_node *next;
};

struct sl_list {
    sl_list_node_t *first;
    sl_list_node_t *last;
};

struct sl_list_iterator {
    sl_list_t *list;
    sl_list_node_t *current;
    sl_list_node_t *previous;
};

void sl_list_init(sl_list_t *list);  // this is equivalent to zeroing the list struct
void sl_list_add_last(sl_list_t *list, sl_list_node_t *n);
void sl_list_add_first(sl_list_t *list, sl_list_node_t *n);
sl_list_node_t * sl_list_remove_first(sl_list_t *list);
sl_list_node_t * sl_list_remove_all(sl_list_t *list);
static inline sl_list_node_t * sl_list_peek_first(sl_list_t *list) { return list->first; }
static inline sl_list_node_t * sl_list_peek_last(sl_list_t *list) { return list->last; }
static inline bool sl_list_is_empty(sl_list_t *list) { return list->first == NULL; }

void sl_list_remove_node(sl_list_t *list, sl_list_node_t *n, sl_list_node_t *prev);

// finding requires traversing the list
int sl_list_find_and_remove(sl_list_t *list, sl_list_node_t *n);

void sl_list_insert_sorted(sl_list_t *list, int(*compare)(const sl_list_node_t *, const sl_list_node_t *), sl_list_node_t *n);

void sl_list_interator_begin(sl_list_iterator_t *iter, sl_list_t *list);
sl_list_node_t * sl_list_interator_next(sl_list_iterator_t *iter);
sl_list_node_t * sl_list_iterator_get_current(sl_list_iterator_t *iter);
void sl_list_iterator_remove_current(sl_list_iterator_t *iter);

#ifdef __cplusplus
}
#endif
