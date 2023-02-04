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
typedef struct core_params core_params_t;

#define CORE_OPT_TRAP_SYSCALL           (1u << 0)
#define CORE_OPT_TRAP_BREAKPOINT        (1u << 1)
#define CORE_OPT_TRAP_ABORT             (1u << 2)
#define CORE_OPT_TRAP_UNDEF             (1u << 3)
#define CORE_OPT_TRAP_PREFETCH_ABORT    (1u << 4)
#define CORE_OPT_ENDIAN_LITTLE          (1u << 30)
#define CORE_OPT_ENDIAN_BIG             (1u << 31)

struct core_params {
    uint8_t arch;
    uint8_t subarch;
    uint8_t id;
    uint32_t options;
    uint32_t arch_options;
};

// Special register defines to pass to set/get_reg()
#define CORE_REG_PC     0xffff
#define CORE_REG_SP     0xfffe
#define CORE_REG_LR     0xfffd
#define CORE_REG_ARG0   0xfffc
#define CORE_REG_ARG1   0xfffb

#define CORE_REG_INVALID 0xffffffff

// Architecture-specific registers definitions
// RISCV - add reg base to CSR address
#define RV_CORE_REG_BASE     0x80000000
#define RV_CORE_REG(csr) (RV_CORE_REG_BASE + (csr))

// Query register types
#define CORE_REG_TYPE_INT    0
#define CORE_REG_TYPE_FLOAT  1
#define CORE_REG_TYPE_VECTOR 2
#define CORE_REG_TYPE_MATRIX 3

// Core state args
#define CORE_STATE_INTERRUPTS_EN    0
#define CORE_STATE_64BIT            1
#define CORE_STATE_ENDIAN_BIG       2

uint8_t core_get_arch(core_t *c);
uint32_t core_get_reg_count(core_t *c, int type);

void core_set_reg(core_t *c, uint32_t reg, uint64_t value);
uint64_t core_get_reg(core_t *c, uint32_t reg);
int core_step(core_t *c, uint32_t num);
int core_run(core_t *c);
int core_set_state(core_t *c, uint32_t state, bool enabled);

// memory access through core
int core_mem_read(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf);
int core_mem_write(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf);

uint64_t core_get_cycles(core_t *c);

#ifdef __cplusplus
}
#endif
