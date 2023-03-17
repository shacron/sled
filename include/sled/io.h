// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>

// IO port interface - implemented by devices that
// implement chained memory-addressed IO

#define IO_DIR_IN   0
#define IO_DIR_OUT  1

struct sl_io_op {
    uint64_t addr;
    uint32_t count;
    uint16_t size;
    uint8_t direction;
    uint8_t align;
    void *buf;
    void *agent;
};

struct sl_io_port {
    int (*io)(sl_io_port_t *p, sl_io_op_t *op, void *cookie);
};

