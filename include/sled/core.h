// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Public core interface

// exception / privilege level
#define SL_CORE_EL_USER         0
#define SL_CORE_EL_SUPERVISOR   1
#define SL_CORE_EL_HYPERVISOR   2
#define SL_CORE_EL_MONITOR      3

#define SL_CORE_OPT_TRAP_SYSCALL           (1u << 0)
#define SL_CORE_OPT_TRAP_BREAKPOINT        (1u << 1)
#define SL_CORE_OPT_TRAP_ABORT             (1u << 2)
#define SL_CORE_OPT_TRAP_UNDEF             (1u << 3)
#define SL_CORE_OPT_TRAP_PREFETCH_ABORT    (1u << 4)
#define SL_CORE_OPT_ENDIAN_LITTLE          (1u << 30)
#define SL_CORE_OPT_ENDIAN_BIG             (1u << 31)

struct sl_core_params {
    u1 arch;
    u1 subarch;
    u1 id;
    u4 options;
    u4 arch_options;
    const char *name;
    sl_bus_t *bus;
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
#define SL_CORE_STATE_INTERRUPTS_EN     1
#define SL_CORE_STATE_WFI               2
#define SL_CORE_STATE_64BIT             3
#define SL_CORE_STATE_ENDIAN_BIG        4

/*
There are two ways to run a core
Synchronous execution - this is accomplished by iterated calls to
sl_core_step(). Between steps, all core accessor functions are safe to call.
Note that these functions are NOT generally thread safe and thus sl_core_t must
be protected by an external lock if accessed from multiple threads.

Asynchronous execution - the involves a call to sl_core_dispatch_loop(). The
dispatch loop is a blocking call and should be done on a separate thread.
Control of the core is accomplished via async events. No other core accessor
functions may be called unless specified while the dispatch loop is running.
*/

// Execute a number of instructions on current thread
int sl_core_step(sl_core_t *c, u8 num);

// Run dispatch loop on current thread - enter async execution mode
int sl_core_run(sl_core_t *c);

// ----------------------------------------------------------------------------
// Synchronous accessor functions
// ----------------------------------------------------------------------------

void sl_core_set_reg(sl_core_t *c, u4 reg, u8 value);
u8 sl_core_get_reg(sl_core_t *c, u4 reg);
int sl_core_set_state(sl_core_t *c, u4 state, bool enabled);
int sl_core_mem_read(sl_core_t *c, u8 addr, u4 size, u4 count, void *buf);
int sl_core_mem_write(sl_core_t *c, u8 addr, u4 size, u4 count, void *buf);
int sl_core_mem_atomic(sl_core_t *c, u8 addr, u4 size, u1 aop, u8 arg0, u8 arg1, u8 *result, u1 ord, u1 ord_fail);
u8 sl_core_get_cycles(sl_core_t *c);
int sl_core_set_mapper(sl_core_t *c, sl_dev_t *d);

// ----------------------------------------------------------------------------
// Async control functions
// ----------------------------------------------------------------------------
#define SL_CORE_CMD_RUN     0
#define SL_CORE_CMD_HALT    1
#define SL_CORE_CMD_EXIT    2

int sl_core_async_command(sl_core_t *c, u4 cmd, bool wait);

// ----------------------------------------------------------------------------
// Always safe to call on a valid core
// ----------------------------------------------------------------------------
u1 sl_core_get_arch(sl_core_t *c);
u4 sl_core_get_reg_count(sl_core_t *c, int type);

// debug feature: print the bus topology as seen by this core
// using the current translation regime.
void sl_core_print_bus_topology(sl_core_t *c);
void sl_core_print_config(sl_core_t *c);

#ifdef __cplusplus
}
#endif
