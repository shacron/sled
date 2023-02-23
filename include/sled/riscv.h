// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

// Standard RISCV defines

// ------------------------------------------------------------------
// registers
// ------------------------------------------------------------------

#define RV_ZERO 0   // zero
#define RV_RA   1   // return address
#define RV_SP   2   // stack pointer
#define RV_GP   3   // global pointer
#define RV_TP   4   // thread pointer
#define RV_T0   5   // Temporary/alternate link register
#define RV_T1   6   // Temporary
#define RV_T2   7   // Temporary
#define RV_S0   8   // Saved register
#define RV_FP   RV_S0  // Frame pointer
#define RV_S1   9   // Saved register
#define RV_A0   10  // Function arguments/return values
#define RV_A1   11  // Function arguments/return values
#define RV_A2   12  // Function argument
#define RV_A3   13  // Function argument
#define RV_A4   14  // Function argument
#define RV_A5   15  // Function argument
#define RV_A6   16  // Function argument
#define RV_A7   17  // Function argument
#define RV_S2   18  // Saved register
#define RV_S3   19  // Saved register
#define RV_S4   20  // Saved register
#define RV_S5   21  // Saved register
#define RV_S6   22  // Saved register
#define RV_S7   23  // Saved register
#define RV_S8   24  // Saved register
#define RV_S9   25  // Saved register
#define RV_S10  26  // Saved register
#define RV_S11  27  // Saved register
#define RV_T3   28  // Temporary
#define RV_T4   29  // Temporary
#define RV_T5   30  // Temporary
#define RV_T6   31  // Temporary

// ------------------------------------------------------------------
// privilege levels
// ------------------------------------------------------------------
#define RV_PL_USER       0
#define RV_PL_SUPERVISOR 1
#define RV_PL_HYPERVISOR 2
#define RV_PL_MACHINE    3

// ------------------------------------------------------------------
// exceptions
// ------------------------------------------------------------------
// High bit is set if cause is interrupt
#define RV_CAUSE64_INT          (1ul << 63)
#define RV_CAUSE32_INT          (1ul << 31)

// cause when interrupt bit is set
#define RV_INT_SW_S             1   // software interrupt from supervisor mode
#define RV_INT_SW_M             3   // software interrupt from machine mode
#define RV_INT_TIMER_S          5   // timer interrupt from supervisor mode
#define RV_INT_TIMER_M          7   // timer interrupt from machine mode
#define RV_INT_EXTERNAL_S       9   // external interrupt from supervisor mode
#define RV_INT_EXTERNAL_M       11  // external interrupt from machine mode

// cause when interrupt bit is not set
#define RV_EX_INST_ALIGN        0
#define RV_EX_INST_FAULT        1
#define RV_EX_INST_ILLEGAL      2
#define RV_EX_BREAK             3
#define RV_EX_LOAD_ALIGN        4
#define RV_EX_LOAD_FAULT        5
#define RV_EX_STORE_ALIGN       6
#define RV_EX_STORE_FAULT       7
#define RV_EX_CALL_FROM_U       8
#define RV_EX_CALL_FROM_S       9
#define RV_EX_CALL_FROM_H       10
#define RV_EX_CALL_FROM_M       11
#define RV_EX_INST_PAGE_FAULT   12
#define RV_EX_LOAD_PAGE_FAULT   13
#define RV_EX_STORE_PAGE_FAULT  15
