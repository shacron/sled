// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t value;
    int err;
} result32_t;

typedef struct {
    uint64_t value;
    int err;
} result64_t;

// private types
typedef struct core_ev core_ev_t;
typedef struct list list_t;
typedef struct queue queue_t;
typedef struct lock lock_t;
typedef struct cond cond_t;
typedef struct rv_core rv_core_t;
typedef struct sl_sem sl_sem_t;

// public types
typedef struct core core_t;
typedef struct sl_core_params sl_core_params_t;
typedef struct bus bus_t;
typedef struct sl_dev sl_dev_t;
typedef struct sl_elf_obj sl_elf_obj_t;
typedef struct sym_list sym_list_t;
typedef struct sym_entry sym_entry_t;


