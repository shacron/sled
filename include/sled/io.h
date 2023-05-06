// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#define IO_OP_IN                        0
#define IO_OP_OUT                       1

// Atomic Ops. These use arg[] instead of buf and order instead of count
#define IO_OP_ATOMIC_SWAP               2
#define IO_OP_ATOMIC_CAS                3   // compare and swap
#define IO_OP_ATOMIC_ADD                4
#define IO_OP_ATOMIC_SUB                5
#define IO_OP_ATOMIC_AND                6
#define IO_OP_ATOMIC_OR                 7
#define IO_OP_ATOMIC_XOR                8
#define IO_OP_ATOMIC_SMAX               9
#define IO_OP_ATOMIC_SMIN               10
#define IO_OP_ATOMIC_UMAX               11
#define IO_OP_ATOMIC_UMIN               12

typedef struct sl_io_op sl_io_op_t;
typedef struct sl_io_port sl_io_port_t;

struct sl_io_op {
    u64 addr;   // bus address of target data
    u16 size;   // size of a single entry in bytes. should be power of 2.
    u8 op;      // IO_OP
    u8 align;   // enforce alignment - boolean value, always true for atomics
    union {
        u32 count;          // number of 'size' entries - used for IN, OUT
        struct {
            u8 order;       // memory order - used for all atomics
            u8 order_fail;  // only used for IO_OP_ATOMIC_CAS
        };
    };
    union {
        void *buf;          // io buffer, used for IN, OUT
        u64 arg[2];         // arg[0] used for all atomics, arg[1] for IO_OP_ATOMIC_CAS
    };
    void *agent;            // io source, used for permission checking and attribution
};

int sl_io_for_data(void *data, sl_io_op_t *op);
