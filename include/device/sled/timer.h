// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project


#pragma once

#define TIMER_REG_DEV_TYPE                  0x0  // RO
#define TIMER_REG_DEV_VERSION               0x4  // RO
#define TIMER_REG_CONFIG                    0x8  // RW
#define TIMER_REG_STATUS                    0xc  // RO

// scale the frequency of timers by multiple of real world clock
#define TIMER_REG_RT_SCALER_US              0x10 // RW

// number individual timer units
#define TIMER_REG_NUM_UNITS                 0x14 // RO

#define TIMER_REG_UNIT_CONFIG(i)            (0x20 + (0x20 * i))
#define TIMER_REG_UNIT_RESET_VAL_LO(i)      (0x24 + (0x20 * i))
#define TIMER_REG_UNIT_RESET_VAL_HI(i)      (0x28 + (0x20 * i))
#define TIMER_REG_UNIT_CURRENT_VAL_LO(i)    (0x2c + (0x20 * i))
#define TIMER_REG_UNIT_CURRENT_VAL_HI(i)    (0x30 + (0x20 * i))

#define TIMER_UNIT_CONFIG_RUN               (1u << 0)
#define TIMER_UNIT_CONFIG_CONTINUOUS        (1u << 1)
#define TIMER_UNIT_CONFIG_LOOPED            (1u << 2)

#define TIMER_APERTURE_LENGTH(num)          (0x20 * (num * 0x20))
