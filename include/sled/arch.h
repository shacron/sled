// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Top level architecture
#define SL_ARCH_MIPS     0
#define SL_ARCH_ARM      1
#define SL_ARCH_RISCV    2
#define SL_ARCH_NUM      3
#define SL_ARCH_UNKNOWN  0xff

// MIPS subarch
#define SL_SUBARCH_MIPS   0
#define SL_SUBARCH_MIPS64 1

// ARM subarch
#define SL_SUBARCH_ARM   0
#define SL_SUBARCH_ARM64 1

// RISCV subarch
#define SL_SUBARCH_RV32 0
#define SL_SUBARCH_RV64 1

// RISCV extensions
#define SL_RISCV_EXT_M         (1u << 0)   // multiply/divide
#define SL_RISCV_EXT_A         (1u << 1)   // atomic
#define SL_RISCV_EXT_F         (1u << 2)   // float
#define SL_RISCV_EXT_D         (1u << 3)   // double
#define SL_RISCV_EXT_Q         (1u << 4)   // quad
#define SL_RISCV_EXT_L         (1u << 5)   // decimal float
#define SL_RISCV_EXT_C         (1u << 6)   // compressed
#define SL_RISCV_EXT_B         (1u << 7)   // bit manipulation
#define SL_RISCV_EXT_J         (1u << 8)   // dynamically translated
#define SL_RISCV_EXT_T         (1u << 9)   // transactional memory
#define SL_RISCV_EXT_P         (1u << 10)  // packed simd
#define SL_RISCV_EXT_V         (1u << 11)  // vector
#define SL_RISCV_EXT_N         (1u << 12)  // user-level interrupts

const char *sl_arch_name(u1 arch);
u4 sl_arch_reg_for_name(u1 arch, const char *name);
u4 sl_arch_get_reg_count(u1 arch, u1 subarch, int type);

#ifdef __cplusplus
}
#endif
