// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Public core interface

typedef struct core core_t;
typedef struct sl_core_params sl_core_params_t;

#define SL_CORE_OPT_TRAP_SYSCALL           (1u << 0)
#define SL_CORE_OPT_TRAP_BREAKPOINT        (1u << 1)
#define SL_CORE_OPT_TRAP_ABORT             (1u << 2)
#define SL_CORE_OPT_TRAP_UNDEF             (1u << 3)
#define SL_CORE_OPT_TRAP_PREFETCH_ABORT    (1u << 4)
#define SL_CORE_OPT_ENDIAN_LITTLE          (1u << 30)
#define SL_CORE_OPT_ENDIAN_BIG             (1u << 31)

struct sl_core_params {
    uint8_t arch;
    uint8_t subarch;
    uint8_t id;
    uint32_t options;
    uint32_t arch_options;
};

// Special register defines to pass to set/get_reg()
#define SL_CORE_REG_PC     0xffff
#define SL_CORE_REG_SP     0xfffe
#define SL_CORE_REG_LR     0xfffd
#define SL_CORE_REG_ARG0   0xfffc
#define SL_CORE_REG_ARG1   0xfffb

#define SL_CORE_REG_INVALID 0xffffffff

// Architecture-specific registers definitions
// RISCV - add reg base to CSR address
#define SL_RV_CORE_REG_BASE     0x80000000
#define SL_RV_CORE_REG(csr)     (SL_RV_CORE_REG_BASE + (csr))

// Query register types
#define SL_CORE_REG_TYPE_INT    0
#define SL_CORE_REG_TYPE_FLOAT  1
#define SL_CORE_REG_TYPE_VECTOR 2
#define SL_CORE_REG_TYPE_MATRIX 3

// Core state args
#define SL_CORE_STATE_INTERRUPTS_EN    0
#define SL_CORE_STATE_64BIT            1
#define SL_CORE_STATE_ENDIAN_BIG       2

/*
There are two ways to run a core
Synchronous execution - this is accomplished by a series of iterated calls to
sl_core_step(). Between steps, all core accessor functions are safe to call.
Note that these functions are NOT generally thread safe and thus core_t must
be protected by an external lock if accessed from multiple threads.

Asynchronous execution - the involves a call to sl_core_dispatch_loop(). The
dispatch loop is a blocking call and should be done on a separate thread.
Control of the core is accomplished via async events. No other core accessor
functions may be called unless specified while the dispatch loop is running.
*/

void sl_core_set_reg(core_t *c, uint32_t reg, uint64_t value);
uint64_t sl_core_get_reg(core_t *c, uint32_t reg);
int sl_core_step(core_t *c, uint32_t num);
int sl_core_set_state(core_t *c, uint32_t state, bool enabled);

// memory access through core
int sl_core_mem_read(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf);
int sl_core_mem_write(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf);

uint64_t sl_core_get_cycles(core_t *c);


// Always safe to call on a valid core
uint8_t sl_core_get_arch(core_t *c);
uint32_t sl_core_get_reg_count(core_t *c, int type);


#ifdef __cplusplus
}
#endif
