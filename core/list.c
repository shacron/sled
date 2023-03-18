// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <assert.h>

#include <sled/list.h>

void sl_list_init(sl_list_t *c) {
    c->head = NULL;
    c->tail = NULL;
}

void sl_list_add_tail(sl_list_t *c, sl_list_node_t *n) {
    n->next = NULL;
    if (c->head == NULL) {
        c->head = n;
        c->tail = n;
    } else {
        c->tail->next = n;
        c->tail = n;
    }
}

void sl_list_add_head(sl_list_t *c, sl_list_node_t *n) {
    n->next = c->head;
    c->head = n;
    if (c->tail == NULL) c->tail = n;
}

sl_list_node_t * sl_list_remove_head(sl_list_t *c) {
    if (c->head == NULL) return NULL;
    sl_list_node_t *n = c->head;
    c->head = n->next;
    if(c->head == NULL) c->tail = NULL;
    return n;
}

sl_list_node_t * sl_list_remove_all(sl_list_t *c) {
    sl_list_node_t *n = c->head;
    c->head = NULL;
    c->tail = NULL;
    return n;
}

void sl_list_insert_sorted(sl_list_t *c, int(*compare)(const sl_list_node_t *, const sl_list_node_t *), sl_list_node_t *n) {
    if (c->head == NULL) {
        sl_list_add_head(c, n);
        return;
    }

    if (compare(n, c->head) <= 0) {
        n->next = c->head;
        c->head = n;
        return;
    }

    sl_list_node_t *cur = c->head->next;
    sl_list_node_t *prev = c->head;
    for ( ; cur != NULL; cur = cur->next) {
        if (compare(n, cur) <= 0) {
            n->next = cur;
            prev->next = n;
            return;
        }
        prev = cur;
    }
    sl_list_add_head(c, n);
}

void sl_list_remove_node(sl_list_t *c, sl_list_node_t *n, sl_list_node_t *prev) {
    if (c->head == NULL) return;
    sl_list_node_t *next;
    if (prev == NULL) {
        assert(n == c->head);
        next = c->head->next;
        c->head = next;
    } else {
        next = n->next;
        prev->next = next;
    }
    if (next == NULL) c->tail = prev;
    n->next = NULL;
}

