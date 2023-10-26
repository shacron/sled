// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#define PLAT_MEM_BASE       0x10000
#define PLAT_MEM_SIZE       (5 * 1024 * 1024)

#define PLAT_CORE_ARCH      SL_ARCH_RISCV
#define PLAT_CORE_SUBARCH   SL_SUBARCH_RV32
#define PLAT_ARCH_OPTIONS   0

// map of interrupt vectors to devices in intc
#define PLAT_INTC_TIMER_IRQ_BIT     0
#define PLAT_INTC_UART_IRQ_BIT      1

#define WITH_UART 1
#define PLAT_UART_BASE      0x5000000
#define PLAT_INTC_BASE      0x5010000
#define PLAT_RTC_BASE       0x5020000
#define PLAT_MPU_BASE       0x5030000
#define PLAT_TIMER_BASE     0x5040000
