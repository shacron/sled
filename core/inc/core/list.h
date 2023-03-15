// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <stddef.h>

#include <core/types.h>

typedef struct list_node_s {
    struct list_node_s *next;
} list_node_t;

struct list {
    list_node_t *head;
    list_node_t *tail;
};

void list_init(list_t *c);  // this is equivalent to zeroing the list struct
void list_add_tail(list_t *c, list_node_t *n);
void list_add_head(list_t *c, list_node_t *n);
list_node_t * list_remove_head(list_t *c);
list_node_t * list_remove_all(list_t *c);
static inline list_node_t * list_peek_head(list_t *c) { return c->head; }
static inline list_node_t * list_peek_tail(list_t *c) { return c->tail; }
static inline bool list_is_empty(list_t *c) { return c->head == NULL; }

void list_remove_node(list_t *c, list_node_t *n, list_node_t *prev);
void list_insert_sorted(list_t *c, int(*compare)(const list_node_t *, const list_node_t *), list_node_t *n);
