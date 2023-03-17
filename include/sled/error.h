// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define SL_OK                 0
#define SL_ERR               -1 // generic error

// user errors
#define SL_ERR_ARG           -2 // invalid argument
#define SL_ERR_MEM           -3 // no memory
#define SL_ERR_UNSUPPORTED   -4 // operation not supported
#define SL_ERR_UNIMPLEMENTED -5 // operation not implemented
#define SL_ERR_FULL          -6 // queue is full
#define SL_ERR_RANGE         -7 // request out of range
#define SL_ERR_STATE         -8 // bad state
#define SL_ERR_TIMEOUT       -9 // timeout
#define SL_ERR_BUSY          -10 // busy
#define SL_ERR_EXITED        -11 // exited cleanly

// instruction execution errors
#define SL_ERR_UNDEF        -16 // undefined (illegal) instruction
#define SL_ERR_ABORT        -17 // instruction or data load/store failure
#define SL_ERR_SYSCALL      -18 // system call
#define SL_ERR_BREAKPOINT   -19 // breakpoint encountered

// IO errors
// #define SL_ERR_BUS          -32
#define SL_ERR_IO_NODEV     -33 // device not found
#define SL_ERR_IO_ALIGN     -34 // invalid address alignment
#define SL_ERR_IO_SIZE      -35 // invalid io size
#define SL_ERR_IO_COUNT     -36 // invalid io count
#define SL_ERR_IO_PERM      -37 // no permission
#define SL_ERR_IO_NOWR      -38 // no write allowed
#define SL_ERR_IO_NORD      -39 // no read allowed
#define SL_ERR_IO_INVALID   -40 // invalid io operation
#define SL_ERR_IO_NOMAP     -41 // no valid mapping

const char *st_err(int err);

#ifdef __cplusplus
}
#endif
