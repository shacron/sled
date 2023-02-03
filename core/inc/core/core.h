// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#include <core/bus.h>
#include <core/irq.h>
#include <core/itrace.h>
#include <sled/core.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CORE_STATE_INTERRUPTS_EN    0
#define CORE_STATE_64BIT            1
#define CORE_STATE_ENDIAN_BIG       2

#define MAX_PHYS_MEM_REGIONS    4
#define MAX_DEVICES             8 

#define BARRIER_LOAD    (1u << 0)
#define BARRIER_STORE   (1u << 1)
#define BARRIER_SYSTEM  (1u << 2)
#define BARRIER_SYNC    (1u << 3)

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

    lock_t lock;
    cond_t cond_int_asserted;
    irq_endpoint_t irq_ep;
    uint32_t pending_irq;       // only updated under lock
};

// setup functions
int core_init(core_t *c, core_params_t *p, bus_t *b);
int core_shutdown(core_t *c);

void core_config_get(core_t *c, core_params_t *p);
int core_config_set(core_t *c, core_params_t *p);

void core_interrupt_set(core_t *c, bool enable);

// runtime functions
int core_endian_set(core_t *c, bool big);
void core_instruction_barrier(core_t *c);
void core_memory_barrier(core_t *c, uint32_t type);
int core_wait_for_interrupt(core_t *c);

// memory access through core
int core_mem_read(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf);
int core_mem_write(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf);

#ifdef __cplusplus
}
#endif
