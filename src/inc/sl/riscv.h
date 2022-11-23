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

#include <sl/core.h>
#include <sled/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RV_HAS_PRIV_LEVEL_USER       (1u << 0)
#define RV_HAS_PRIV_LEVEL_SUPERVISOR (1u << 1)
#define RV_HAS_PRIV_LEVEL_HYPERVISOR (1u << 2)

#define RV_REG_MSCRATCH 0x80000000
#define RV_REG_MEPC     0x80000001
#define RV_REG_MCAUSE   0x80000002
#define RV_REG_MTVAL    0x80000003
#define RV_REG_MIP      0x80000004

typedef struct bus bus_t;

int riscv_core_create(core_params_t *p, bus_t *bus, core_t **core_out);

#ifdef __cplusplus
}
#endif
