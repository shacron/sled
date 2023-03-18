// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct sl_list sl_list_t;

typedef struct sl_list_node_s {
    struct sl_list_node_s *next;
} sl_list_node_t;

struct sl_list {
    sl_list_node_t *head;
    sl_list_node_t *tail;
};

void sl_list_init(sl_list_t *c);  // this is equivalent to zeroing the list struct
void sl_list_add_tail(sl_list_t *c, sl_list_node_t *n);
void sl_list_add_head(sl_list_t *c, sl_list_node_t *n);
sl_list_node_t * sl_list_remove_head(sl_list_t *c);
sl_list_node_t * sl_list_remove_all(sl_list_t *c);
static inline sl_list_node_t * sl_list_peek_head(sl_list_t *c) { return c->head; }
static inline sl_list_node_t * sl_list_peek_tail(sl_list_t *c) { return c->tail; }
static inline bool sl_list_is_empty(sl_list_t *c) { return c->head == NULL; }

void sl_list_remove_node(sl_list_t *c, sl_list_node_t *n, sl_list_node_t *prev);
void sl_list_insert_sorted(sl_list_t *c, int(*compare)(const sl_list_node_t *, const sl_list_node_t *), sl_list_node_t *n);
