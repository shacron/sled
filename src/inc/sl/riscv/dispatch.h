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

#include <sl/riscv/inst.h>
#include <sl/riscv/rv.h>

int rv_exec_ebreak(rv_core_t *c);
int rv_exec_mem(rv_core_t *c, rv_inst_t inst);
int rv_exec_system(rv_core_t *c, rv_inst_t inst);

int rv32_dispatch(rv_core_t *c, rv_inst_t inst);
int rv64_dispatch(rv_core_t *c, rv_inst_t inst);

static inline int rv_undef(rv_core_t *c, rv_inst_t inst) {
    return rv_synchronous_exception(c, EX_UNDEFINDED, inst.raw, 0);
}
