// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Top level architecture
#define ARCH_MIPS     0
#define ARCH_ARM      1
#define ARCH_RISCV    2
#define ARCH_NUM      3
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

const char *arch_name(uint8_t arch);
uint32_t arch_reg_for_name(uint8_t arch, const char *name);

#ifdef __cplusplus
}
#endif
