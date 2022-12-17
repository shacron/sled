// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

#define RV_PRIV_LEVEL_USER       0
#define RV_PRIV_LEVEL_SUPERVISOR 1
#define RV_PRIV_LEVEL_HYPERVISOR 2
#define RV_PRIV_LEVEL_MACHINE    3


// Exception values found in mcause

// High bit is set if cause is interrupt
#define RV_CAUSE_INT64          (1ul << 63)
#define RV_CAUSE_INT32          (1ul << 31)

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

