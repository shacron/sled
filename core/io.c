// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <stdatomic.h>
#include <string.h>

#include <core/common.h>
#include <sled/error.h>
#include <sled/io.h>

#define ATOMIC_CAS(d, expected, desired, o, of) atomic_compare_exchange_strong_explicit(d, &expected, desired, o, of)

// always perform the exchange even when it is seemingly pointless to synchronize the observable
// value with the memory barriers

#define ATOMIC_OP(op, data, utype, stype) { \
    const utype v = op->arg[0]; \
    utype e = op->arg[1]; \
    _Atomic utype *d = data; \
    _Atomic stype *sd = data; \
    utype result; \
    switch (op->op) { \
    case IO_OP_ATOMIC_SWAP: result = atomic_exchange_explicit(d, v, op->order); break; \
    case IO_OP_ATOMIC_CAS:  result = (ATOMIC_CAS(d, e, v, op->order, op->order_fail) ? 0 : 1);  break; \
    case IO_OP_ATOMIC_ADD:  result = atomic_fetch_add_explicit(d, v, op->order); break; \
    case IO_OP_ATOMIC_SUB:  result = atomic_fetch_sub_explicit(d, v, op->order); break; \
    case IO_OP_ATOMIC_AND:  result = atomic_fetch_and_explicit(d, v, op->order); break; \
    case IO_OP_ATOMIC_OR:   result = atomic_fetch_or_explicit(d, v, op->order); break; \
    case IO_OP_ATOMIC_XOR:  result = atomic_fetch_xor_explicit(d, v, op->order); break; \
    case IO_OP_ATOMIC_SMAX: \
        for (bool success = false; !success; ) { \
            stype cur = *(stype *)data; \
            const stype max = MAX((stype)v, cur); \
            success = ATOMIC_CAS((_Atomic stype *)data, cur, max, op->order, memory_order_relaxed); \
            result = max; \
        } \
        break; \
    case IO_OP_ATOMIC_SMIN: \
        for (bool success = false; !success; ) { \
            stype cur = *(stype *)data; \
            const stype min = MIN((stype)v, cur); \
            success = ATOMIC_CAS(sd, cur, min, op->order, memory_order_relaxed); \
            result = min; \
        } \
        break; \
    case IO_OP_ATOMIC_UMAX: \
        for (bool success = false; !success; ) { \
            utype cur = *(utype *)data; \
            const utype max = MAX(v, cur); \
            success = ATOMIC_CAS(d, cur, max, op->order, memory_order_relaxed); \
            result = max; \
        } \
        break; \
    case IO_OP_ATOMIC_UMIN: \
        for (bool success = false; !success; ) { \
            utype cur = *(utype *)data; \
            const utype min = MIN(v, cur); \
            success = ATOMIC_CAS(d, cur, min, op->order, memory_order_relaxed); \
            result = min; \
        } \
        break; \
    default: \
        return SL_ERR_IO_INVALID; \
    } \
    op->arg[0] = result; }


int sl_io_for_data(void *data, sl_io_op_t *op) {
    if (op->op == IO_OP_IN) {
        memcpy(op->buf, data, op->count * op->size);
        return 0;
    }
    if (op->op == IO_OP_OUT) {
        memcpy(data, op->buf, op->count * op->size);
        return 0;
    }

    if ((uptr)data & (sizeof(op->size) - 1)) return SL_ERR_IO_ALIGN;

    switch (op->size) {
    case 1: ATOMIC_OP(op, data, u8, i8);      break;
    case 2: ATOMIC_OP(op, data, u16, i16);    break;
    case 4: ATOMIC_OP(op, data, u32, i32);    break;
    case 8: ATOMIC_OP(op, data, u64, i64);    break;
    default:
        return SL_ERR_IO_SIZE;
    }
    return 0;
}
