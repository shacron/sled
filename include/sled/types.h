// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// public types

typedef int8_t  i1;
typedef int16_t i2;
typedef int32_t i4;
typedef int64_t i8;

typedef uint8_t  u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;

// typedef ssize_t ssize;
typedef size_t  usize;

typedef intptr_t iptr;
typedef uintptr_t uptr;

typedef struct sl_bus sl_bus_t;
typedef struct sl_core sl_core_t;
typedef struct sl_core_params sl_core_params_t;
typedef struct sl_dev sl_dev_t;
typedef struct sl_dev_config sl_dev_config_t;
typedef struct sl_dev_ops sl_dev_ops_t;
typedef struct sl_elf_obj sl_elf_obj_t;
typedef struct sl_engine sl_engine_t;
typedef struct sl_engine_ops sl_engine_ops_t;
typedef struct sl_event sl_event_t;
typedef struct sl_event_ep sl_event_ep_t;
typedef struct sl_io_op sl_io_op_t;
typedef struct sl_irq_ep sl_irq_ep_t;
typedef struct sl_list sl_list_t;
typedef struct sl_list_node sl_list_node_t;
typedef struct sl_list_iterator sl_list_iterator_t;
typedef struct sl_machine sl_machine_t;
typedef struct sl_map_ep sl_map_ep_t;
typedef struct sl_mapper sl_mapper_t;
typedef struct sl_mapping sl_mapping_t;
typedef struct sl_sym_entry sl_sym_entry_t;
typedef struct sl_sym_list sl_sym_list_t;
typedef struct sl_chrono sl_chrono_t;
typedef struct sl_worker sl_worker_t;

typedef struct {
    u4 value;
    int err;
} result32_t;

typedef struct {
    u8 value;
    int err;
} result64_t;

#ifdef __cplusplus
}
#endif
