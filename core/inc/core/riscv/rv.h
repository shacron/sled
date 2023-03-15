// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/core.h>
#include <core/ex.h>
#include <core/types.h>
#include <sled/riscv.h>

// #define RV_TRACE 1

#define RV_MODE_RV32    0
#define RV_MODE_RV64    1

#define RV_OP_MRET      RV_PL_MACHINE
#define RV_OP_SRET      RV_PL_SUPERVISOR

typedef struct {
    uint64_t scratch;
    uint64_t epc;
    uint64_t cause;
    uint64_t tval;
    uint64_t ip;
    uint64_t isa;
    uint64_t edeleg;
    uint64_t ideleg;
    uint64_t ie;
    uint64_t tvec;
    uint64_t counteren;
} rv_sr_pl_t;

typedef struct {
    result64_t (*csr_op)(rv_core_t *c, int op, uint32_t csr, uint64_t value);
    const char *(*name_for_sysreg)(rv_core_t *c, uint16_t num);
    void (*destroy)(void *ext_private);
} rv_isa_extension_t;

struct rv_core {
    core_t core;
    uint8_t mode;
    uint8_t pl;     // privilege level
    uint8_t jump_taken;
    uint8_t c_inst; // was the last dispatched instruction a C type short instruction?

    uint64_t pc;
    uint64_t r[32];

    // uint32_t irq_asserted;
    uint64_t status;

    // system registers
    rv_sr_pl_t sr_pl[3];
    uint64_t mvendorid;
    uint64_t marchid;
    uint64_t mimpid;
    uint64_t mhartid;
    uint64_t mconfigptr;

    uint64_t stap;

    // offsets for calculating these from running counters
    int64_t mcycle_offset;
    int64_t minstret_offset;

    uint32_t pmpcfg[16];
    uint64_t pmpaddr[64];
    uint64_t mhpmcounter[29];
    uint64_t mhpevent[29]; // mhpevent3-mhpevent31

    rv_isa_extension_t ext; // isa extension
    void *ext_private;
};

int rv_dispatch(rv_core_t *c, uint32_t instruction);

int rv_synchronous_exception(rv_core_t *c, core_ex_t ex, uint64_t value, uint32_t status);
int rv_exception_enter(rv_core_t *c, uint64_t cause, uint64_t addr);
int rv_exception_return(rv_core_t *c, uint8_t op);

rv_sr_pl_t* rv_get_pl_csrs(rv_core_t *c, uint8_t pl);

const char *rv_name_for_reg(uint32_t reg);
uint32_t rv_reg_for_name(const char *name);
