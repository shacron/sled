// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// public types

typedef struct sl_bus sl_bus_t;
typedef struct core core_t;
typedef struct sl_core_params sl_core_params_t;
typedef struct sl_dev sl_dev_t;
typedef struct sl_dev_ops sl_dev_ops_t;
typedef struct sl_elf_obj sl_elf_obj_t;
typedef struct sl_event sl_event_t;
typedef struct sl_event_queue sl_event_queue_t;
typedef struct sl_io_op sl_io_op_t;
typedef struct sl_irq_ep sl_irq_ep_t;
typedef struct sl_list_node sl_list_node_t;
typedef struct sl_list sl_list_t;
typedef struct sl_machine sl_machine_t;
typedef struct sl_mapper sl_mapper_t;
typedef struct sl_mapper_entry sl_mapper_entry_t;
typedef struct sym_list sym_list_t;
typedef struct sym_entry sym_entry_t;

typedef struct {
    uint32_t value;
    int err;
} result32_t;

typedef struct {
    uint64_t value;
    int err;
} result64_t;

typedef int (io_func_t)(void *ctx, sl_io_op_t *op);

#ifdef __cplusplus
}
#endif