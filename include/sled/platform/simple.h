// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

#define PLAT_MEM_BASE 0x10000
#define PLAT_MEM_SIZE (5 * 1024 * 1024)

#define PLAT_CORE_ARCH      ARCH_RISCV
#define PLAT_CORE_SUBARCH   RISCV_SUBARCH_RV32I
#define PLAT_ARCH_OPTIONS   0

#define WITH_UART 1
#define PLAT_UART_BASE 0x5000000

#define PLAT_INTC_BASE 0x5010000
#define PLAT_RTC_BASE  0x5020000
