// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

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
#define SL_ERR_NODEV         -8 // device does not exist

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

const char *st_err(int err);

#ifdef __cplusplus
}
#endif
