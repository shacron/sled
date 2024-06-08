// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#pragma once

#include <fenv.h>

#include <core/arch.h>
#include <core/irq.h>
#include <core/itrace.h>
#include <core/engine.h>
#include <core/types.h>
#include <sled/core.h>

#define MAX_PHYS_MEM_REGIONS    4
#define MAX_DEVICES             8 

#define BARRIER_LOAD    (1u << 0)
#define BARRIER_STORE   (1u << 1)
#define BARRIER_SYSTEM  (1u << 2)
#define BARRIER_SYNC    (1u << 3)

#define MONITOR_UNARMED 0
#define MONITOR_ARMED32 1
#define MONITOR_ARMED64 2

#define CORE_INT_ENABLED(s) (s & (1u << SL_CORE_STATE_INTERRUPTS_EN))
#define CORE_IS_WFI(s) (s & (1u << SL_CORE_STATE_WFI))

#define CORE_EV_IRQ     1
#define CORE_EV_RUNMODE 2

typedef union {
    u4 u4;
    float f;
    u8 u8;
    double d;
} sl_fp_reg_t;

typedef struct core_ops {
    void (*set_reg)(sl_core_t *c, u4 reg, u8 value);
    u8 (*get_reg)(sl_core_t *c, u4 reg);
    int (*set_state)(sl_core_t *c, u4 state, bool enabled);
    void (*shutdown)(sl_core_t *c);
    void (*destroy)(sl_core_t *c);
} core_ops_t;

struct sl_core {
    u1 el;              // exception level
    u1 mode;            // execution mode (register length)
    u1 prev_len;        // length of last instruction
    bool branch_taken;  // was the last instruction a branch

    u8 pc;
    u8 r[32];

    fexcept_t fexc; // host cumulative fp exception flags
    u1 frm;         // floating point rounding mode
    sl_fp_reg_t f[32];

    uint64_t monitor_addr;
    uint64_t monitor_value;
    uint8_t  monitor_status;

    u8 ticks;
    sl_mapper_t *mapper;

    sl_engine_t engine;

    itrace_t *trace;
    const core_ops_t *ops;
    const arch_ops_t *arch_ops;

    u1 arch;
    u1 subarch;
    u1 id;             // numerical id of this core instance
    u4 options;
    u4 arch_options;
    const char *name;
#if WITH_SYMBOLS
    sl_sym_list_t *symbols;
#endif
};

// ----------------------------------------------------------------------------
// setup functions
// ----------------------------------------------------------------------------

// Setup functions may only be called when the core dispatch loop is not
// running.

int sl_core_init(sl_core_t *c, sl_core_params_t *p, sl_mapper_t *m);
void sl_core_shutdown(sl_core_t *c);
void sl_core_destroy(sl_core_t *c);

void sl_core_config_get(sl_core_t *c, sl_core_params_t *p);
int sl_core_config_set(sl_core_t *c, sl_core_params_t *p);

void sl_core_add_symbols(sl_core_t *c, sl_sym_list_t *list);

// ----------------------------------------------------------------------------
// async functions
// ----------------------------------------------------------------------------

// Core Events are a way to send events to the core's dispatch loop.
// Events are a means to synchronize asynchronous inputs to the core.
// Events can be interrupts or other changes initiated from outside the
// dispatch loop. Events will be handled before the next instruction is
// dispatched.

// int sl_core_event_send_async(sl_core_t *c, sl_event_t *ev);

// ----------------------------------------------------------------------------
// synchronous functions
// ----------------------------------------------------------------------------

// These functions may only be invoked by the core dispatch loop

void sl_core_interrupt_set(sl_core_t *c, bool enable);
int sl_core_endian_set(sl_core_t *c, bool big);
void sl_core_instruction_barrier(sl_core_t *c);
void sl_core_memory_barrier(sl_core_t *c, u4 type);

void sl_core_next_pc(sl_core_t *c);
int sl_core_load_pc(sl_core_t *c, u4 *inst);

// ----------------------------------------------------------------------------
// Misc
// ----------------------------------------------------------------------------

// safe to call in any context as long as the core is not shut down.
sl_sym_entry_t *sl_core_get_sym_for_addr(sl_core_t *c, u8 addr);
