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

// Top level architecture
#define ARCH_MIPS     0
#define ARCH_ARM      1
#define ARCH_RISCV    2
#define ARCH_UNKNOWN  0xff

// ARM subarch
#define SUBARCH_ARM   0
#define SUBARCH_ARM64 1

// RISCV subarch
#define SUBARCH_RV32 0
#define SUBARCH_RV64 1

// RISCV extensions
#define RISCV_EXT_M         (1u << 0)   // multiply/divide
#define RISCV_EXT_A         (1u << 1)   // atomic
#define RISCV_EXT_F         (1u << 2)   // float
#define RISCV_EXT_D         (1u << 3)   // double
#define RISCV_EXT_Q         (1u << 4)   // quad
#define RISCV_EXT_L         (1u << 5)   // decimal float
#define RISCV_EXT_C         (1u << 6)   // compressed
#define RISCV_EXT_B         (1u << 7)   // bit manipulation
#define RISCV_EXT_J         (1u << 8)   // dynamically translated
#define RISCV_EXT_T         (1u << 9)   // transactional memory
#define RISCV_EXT_P         (1u << 10)  // packed simd
#define RISCV_EXT_V         (1u << 11)  // vector
#define RISCV_EXT_N         (1u << 12)  // user-level interrupts

#ifdef __cplusplus
}
#endif
