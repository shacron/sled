// SPDX-License-Identifier: MIT License
// Copyright (c) 2023-2025 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#define IO_OP_IN                        0
#define IO_OP_OUT                       1

#define IO_OP_RESOLVE                   2

// Atomic Ops. These use arg[] instead of buf and order instead of count
#define IO_OP_ATOMIC_SWAP               3
#define IO_OP_ATOMIC_CAS                4   // compare and swap
#define IO_OP_ATOMIC_ADD                5
#define IO_OP_ATOMIC_SUB                6
#define IO_OP_ATOMIC_AND                7
#define IO_OP_ATOMIC_OR                 8
#define IO_OP_ATOMIC_XOR                9
#define IO_OP_ATOMIC_SMAX               10
#define IO_OP_ATOMIC_SMIN               11
#define IO_OP_ATOMIC_UMAX               12
#define IO_OP_ATOMIC_UMIN               13

#define IO_IS_ATOMIC(op) (op >= IO_OP_ATOMIC_SWAP)

typedef struct sl_io_op sl_io_op_t;
typedef struct sl_io_port sl_io_port_t;

// sl_io_op: an io operation
// When called with op IO_OP_RESOLVE
//  'size' and 'count' should be 1, 'buf' is unused
// On successful return:
//  arg[0] will contain the host machine pointer to the data
//  arg[1] will contain the length of the data

struct sl_io_op {
    u8 addr;   // bus address of target data
    u2 size;   // size of a single entry in bytes. should be power of 2.
    u1 op;      // IO_OP
    u1 align;   // enforce alignment - boolean value, always true for atomics
    union {
        u4 count;          // number of 'size' entries - used for IN, OUT
        struct {
            u1 order;       // memory order - used for all atomics
            u1 order_fail;  // only used for IO_OP_ATOMIC_CAS
        };
    };
    union {
        void *buf;          // io buffer, used for IN, OUT
        u8 arg[2];         // arg[0] used for all atomics, arg[1] for IO_OP_ATOMIC_CAS
    };
    void *agent;            // io source, used for permission checking and attribution
};

int sl_io_for_data(void *data, sl_io_op_t *op);
