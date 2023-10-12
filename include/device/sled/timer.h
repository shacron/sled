// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

// Sled timer

// This device contains a set of asynchronous countdown timers.
// Each timer can run independently in either one shot or continuous modes.

// Timers are in host wall-clock microseconds. A scaler register is available
// to adjust the ratio of real time to simulated time.

#define TIMER_REG_DEV_TYPE                  0x0  // RO
#define TIMER_REG_DEV_VERSION               0x4  // RO
#define TIMER_REG_CONFIG                    0x8  // RW
#define TIMER_REG_STATUS                    0xc  // RO
// IRQ mask bitfield for each timer unit
// Writing 1 masks interrupt. 0 enables interrupt to proceed.
// Default value: all masked
#define TIMER_IRQ_MASK                      0x10 // RW
// IRQ status bitfield for each timer unit. Writing 1 clears a set bit, 0 retains it.
#define TIMER_IRQ_STATUS                    0x14 // RW

// scale the frequency of timers by multiple of real world clock
#define TIMER_REG_RT_SCALER_US              0x18 // RW

// number individual timer units
#define TIMER_REG_NUM_UNITS                 0x1c // RO

// Configuration register for timer i
#define TIMER_REG_UNIT_CONFIG(i)            (0x20 + (0x20 * i))

// Initial starting value of timer, in microseconds
#define TIMER_REG_UNIT_RESET_VAL_LO(i)      (0x24 + (0x20 * i))
#define TIMER_REG_UNIT_RESET_VAL_HI(i)      (0x28 + (0x20 * i))

// Current value of timer, in microseconds
#define TIMER_REG_UNIT_CURRENT_VAL_LO(i)    (0x2c + (0x20 * i))
#define TIMER_REG_UNIT_CURRENT_VAL_HI(i)    (0x30 + (0x20 * i))

// Start timer.
// Setting this bit starts the timer. If the timer was already
// running, no effect is observed.
// Clearing this bit stops the timer.
#define TIMER_UNIT_CONFIG_RUN               (1u << 0)

// Continuous timer.
// If set to 1, the timer will restart immediately when it reaches zero.
// This mode guarantees no timer drift.
// If set to 0, the timer will stop and the run bit will be cleared when
// zero is reached (one shot mode).
// Interrupts will be generated every time zero is reached, whether one
// shot or continuous.
// The looped bit will be set each time zero is reached.
#define TIMER_UNIT_CONFIG_CONTINUOUS        (1u << 1)

// Timer loop/end indicator.
// Set to 1 when timer has expired or looped.
// Cleared when a timer is started.
// Write 1 to clear, 0 to retain current value.
#define TIMER_UNIT_CONFIG_LOOPED            (1u << 2)


#define TIMER_APERTURE_LENGTH(num)          (0x20 * (num * 0x20))
