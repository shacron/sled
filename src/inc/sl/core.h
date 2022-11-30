// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#pragma once

#include <stdint.h>

#include <sl/bus.h>
#include <sl/irq.h>
#include <sl/itrace.h>
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
    irq_endpoint_t irq_ep;
    uint32_t pending_irq;       // only updated under lock
};

int core_init(core_t *c, core_params_t *p, bus_t *b);
int core_shutdown(core_t *c);

void core_interrupt_set(core_t *c, bool enable);
int core_endian_set(core_t *c, bool big);

uint32_t core_state_get(core_t *c);
int core_state_set(core_t *c, uint32_t state);

void core_instruction_barrier(core_t *c);
void core_memory_barrier(core_t *c, uint32_t type);

int core_mem_read(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf);
int core_mem_write(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf);

#ifdef __cplusplus
}
#endif
