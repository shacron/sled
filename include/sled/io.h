// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#define IO_DIR_IN   0
#define IO_DIR_OUT  1

typedef struct sl_io_op sl_io_op_t;
typedef struct sl_io_port sl_io_port_t;

struct sl_io_op {
    uint64_t addr;
    uint32_t count;
    uint16_t size;
    uint8_t direction;
    uint8_t align;
    void *buf;
    void *agent;
};