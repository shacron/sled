// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

#include <riscv/exception.h>
#include <riscv/register.h>
#include <sl/common.h>
#include <sl/core.h>
#include <sl/ex.h>

// #define RV_TRACE 1

typedef struct rv_core rv_core_t;

#define RV_MODE_RV32    0
#define RV_MODE_RV64    1

typedef struct {
    uint64_t scratch;
    uint64_t epc;
    uint64_t cause;
    uint64_t tval;
    uint64_t ip;
    // uint64_t status;
    uint64_t isa;
    uint64_t edeleg;
    uint64_t ideleg;
    uint64_t ie;
    uint64_t tvec;
    uint64_t counteren;
} rv_sr_mode_t;

typedef struct {
    result64_t (*csr_op)(rv_core_t *c, int op, uint32_t csr, uint64_t value);
    const char *(*name_for_sysreg)(rv_core_t *c, uint16_t num);
    void (*destroy)(void *ext_private);
} rv_isa_extension_t;

struct rv_core {
    core_t core;
    uint8_t mode;
    uint8_t priv_level;
    uint8_t jump_taken;
    uint8_t c_inst; // was the last dispatched instruction a C type short instruction?

    uint64_t pc;
    uint64_t r[32];

    uint64_t status;

    // system registers
    rv_sr_mode_t sr_mode[3];
    uint64_t mvendorid;
    uint64_t marchid;
    uint64_t mimpid;
    uint64_t mhartid;
    uint64_t mconfigptr;

    // offsets for calculating these from running counters
    int64_t mcycle_offset;
    int64_t minstret_offset;

    uint64_t pmpcfg[16];
    uint64_t pmpaddr[64];

    rv_isa_extension_t ext; // isa extension
    void *ext_private;
};

int rv_dispatch(rv_core_t *c, uint32_t instruction);

int rv_synchronous_exception(rv_core_t *c, core_ex_t ex, uint64_t value, uint32_t status);
int rv_exception_enter(rv_core_t *c, uint64_t cause, uint64_t addr);
int rv_exception_return(rv_core_t *c);
int rv_wait_for_interrupt(rv_core_t *c);

rv_sr_mode_t* rv_get_mode_registers(rv_core_t *c, uint8_t priv_level);

const char *rv_name_for_reg(uint32_t reg);
