// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

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

#define CORE_INT_ENABLED(s) (s & (1u << SL_CORE_STATE_INTERRUPTS_EN))
#define CORE_IS_WFI(s) (s & (1u << SL_CORE_STATE_WFI))

#define CORE_EV_IRQ     1
#define CORE_EV_RUNMODE 2

typedef struct core_ops {
    void (*set_reg)(core_t *c, u4 reg, u8 value);
    u8 (*get_reg)(core_t *c, u4 reg);
    int (*set_state)(core_t *c, u4 state, bool enabled);
} core_ops_t;

struct core {
    sl_engine_t engine;

    u8 ticks;

    sl_mapper_t *mapper;
    itrace_t *trace;
    core_ops_t ops;

    u1 arch;
    u1 subarch;
    u1 id;             // numerical id of this core instance
    u4 options;
    u4 arch_options;
#if WITH_SYMBOLS
    sym_list_t *symbols;
#endif
};

// ----------------------------------------------------------------------------
// setup functions
// ----------------------------------------------------------------------------

// Setup functions may only be called when the core dispatch loop is not
// running.

int core_init(core_t *c, sl_core_params_t *p, sl_bus_t *b);
int core_shutdown(core_t *c);

void core_config_get(core_t *c, sl_core_params_t *p);
int core_config_set(core_t *c, sl_core_params_t *p);

void core_add_symbols(core_t *c, sym_list_t *list);

// ----------------------------------------------------------------------------
// async functions
// ----------------------------------------------------------------------------

// Core Events are a way to send events to the core's dispatch loop.
// Events are a means to synchronize asynchronous inputs to the core.
// Events can be interrupts or other changes initiated from outside the
// dispatch loop. Events will be handled before the next instruction is
// dispatched.
int core_event_send_async(core_t *c, sl_event_t *ev);

// ----------------------------------------------------------------------------
// dispatch functions
// ----------------------------------------------------------------------------

// These functions may only be invoked by the core dispatch loop

void core_interrupt_set(core_t *c, bool enable);
int core_endian_set(core_t *c, bool big);
void core_instruction_barrier(core_t *c);
void core_memory_barrier(core_t *c, u4 type);

// ----------------------------------------------------------------------------
// Misc
// ----------------------------------------------------------------------------

// safe to call in any context as long as the core is not shut down.
sym_entry_t *core_get_sym_for_addr(core_t *c, u8 addr);
