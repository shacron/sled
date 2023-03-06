// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <assert.h>

#include <core/list.h>

void list_init(list_t *c) {
    c->head = NULL;
    c->tail = NULL;
}

void list_add_tail(list_t *c, list_node_t *n) {
    n->next = NULL;
    if (c->head == NULL) {
        c->head = n;
        c->tail = n;
    } else {
        c->tail->next = n;
        c->tail = n;
    }
}

void list_add_head(list_t *c, list_node_t *n) {
    n->next = c->head;
    c->head = n;
    if (c->tail == NULL) c->tail = n;
}

list_node_t * list_remove_head(list_t *c) {
    if (c->head == NULL) return NULL;
    list_node_t *n = c->head;
    c->head = n->next;
    if(c->head == NULL) c->tail = NULL;
    return n;
}

list_node_t * list_remove_all(list_t *c) {
    list_node_t *n = c->head;
    c->head = NULL;
    c->tail = NULL;
    return n;
}

void list_insert_sorted(list_t *c, int(*compare)(const list_node_t *, const list_node_t *), list_node_t *n) {
    if (c->head == NULL) {
        list_add_head(c, n);
        return;
    }

    if (compare(n, c->head) <= 0) {
        n->next = c->head;
        c->head = n;
        return;
    }

    list_node_t *cur = c->head->next;
    list_node_t *prev = c->head;
    for ( ; cur != NULL; cur = cur->next) {
        if (compare(n, cur) <= 0) {
            n->next = cur;
            prev->next = n;
            return;
        }
        prev = cur;
    }
    list_add_head(c, n);
}

void list_remove_node(list_t *c, list_node_t *n, list_node_t *prev) {
    if (c->head == NULL) return;
    list_node_t *next;
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

