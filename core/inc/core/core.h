// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#include <core/bus.h>
#include <core/irq.h>
#include <core/itrace.h>
#include <core/queue.h>
#include <sled/core.h>

#define MAX_PHYS_MEM_REGIONS    4
#define MAX_DEVICES             8 

#define BARRIER_LOAD    (1u << 0)
#define BARRIER_STORE   (1u << 1)
#define BARRIER_SYSTEM  (1u << 2)
#define BARRIER_SYNC    (1u << 3)

#define SL_CORE_STATE_WFI   31

#define CORE_PENDING_IRQ    (1u << 0)
#define CORE_PENDING_EVENT  (1u << 1)

#define CORE_INT_ENABLED(s) (s & (1u << SL_CORE_STATE_INTERRUPTS_EN))
#define CORE_IS_WFI(s) (s & (1u << SL_CORE_STATE_WFI))

#define CORE_EV_IRQ     1

typedef struct sym_list sym_list_t;
typedef struct sym_entry sym_entry_t;

typedef struct {
    list_node_t node;
    uint32_t type;
    uint32_t option;
    uint64_t arg[2];
} core_ev_t;

typedef struct core_ops {
    void (*set_reg)(core_t *c, uint32_t reg, uint64_t value);
    uint64_t (*get_reg)(core_t *c, uint32_t reg);
    int (*step)(core_t *c, uint32_t num);
    int (*run)(core_t *c);
    int (*set_state)(core_t *c, uint32_t state, bool enabled);
    int (*destroy)(core_t *c);
} core_ops_t;

struct core {
    uint8_t arch;
    uint8_t subarch;
    uint8_t id;             // numerical id of this core instance
    uint32_t options;
    uint32_t arch_options;

    uint32_t state;         // current running state
    uint64_t ticks;

    bus_t *bus;
    itrace_t *trace;
    core_ops_t ops;

    sl_irq_ep_t irq_ep;
    queue_t event_q;

#if WITH_SYMBOLS
    sym_list_t *symbols;
#endif
};

// setup functions
int core_init(core_t *c, sl_core_params_t *p, bus_t *b);
int core_shutdown(core_t *c);

void core_config_get(core_t *c, sl_core_params_t *p);
int core_config_set(core_t *c, sl_core_params_t *p);

void core_add_symbols(core_t *c, sym_list_t *list);

// Core Events are a way to send events to the core's dispatch loop.
// Events are a means to synchronize asynchronous inputs to the core.
// Events can be interrupts or other changes initiated from outside the
// dispatch loop. Events will be handled before the next instruction is
// dispatched.
void core_event_send(core_t *c, core_ev_t *ev);

// runtime functions
// these may only be called by the core's dispatch loop
void core_interrupt_set(core_t *c, bool enable);
int core_endian_set(core_t *c, bool big);
void core_instruction_barrier(core_t *c);
void core_memory_barrier(core_t *c, uint32_t type);
int core_wait_for_interrupt(core_t *c);
int core_handle_irq_event(core_t *c, core_ev_t *ev);

sym_entry_t *core_get_sym_for_addr(core_t *c, uint64_t addr);
