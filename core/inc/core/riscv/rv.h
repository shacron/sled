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
    u64 scratch;
    u64 epc;
    u64 cause;
    u64 tval;
    u64 ip;
    u64 isa;
    u64 edeleg;
    u64 ideleg;
    u64 ie;
    u64 tvec;
    u64 counteren;
} rv_sr_pl_t;

typedef struct {
    result64_t (*csr_op)(rv_core_t *c, int op, u32 csr, u64 value);
    const char *(*name_for_sysreg)(rv_core_t *c, u16 num);
    void (*destroy)(void *ext_private);
} rv_isa_extension_t;

struct rv_core {
    core_t core;
    u8 mode;
    u8 pl;     // privilege level
    u8 jump_taken;
    u8 c_inst; // was the last dispatched instruction a C type short instruction?

    u64 pc;
    u64 r[32];

    u64 status;

    uint64_t monitor_addr;
    uint64_t monitor_value;
    uint8_t  monitor_status;

    // system registers
    rv_sr_pl_t sr_pl[3];
    u64 mvendorid;
    u64 marchid;
    u64 mimpid;
    u64 mhartid;
    u64 mconfigptr;

    u64 stap;

    // offsets for calculating these from running counters
    i64 mcycle_offset;
    i64 minstret_offset;

    u32 pmpcfg[16];
    u64 pmpaddr[64];
    u64 mhpmcounter[29];
    u64 mhpevent[29]; // mhpevent3-mhpevent31

    rv_isa_extension_t ext; // isa extension
    void *ext_private;
};

int rv_dispatch(rv_core_t *c, u32 instruction);

int rv_synchronous_exception(rv_core_t *c, core_ex_t ex, u64 value, u32 status);
int rv_exception_enter(rv_core_t *c, u64 cause, u64 addr);
int rv_exception_return(rv_core_t *c, u8 op);

rv_sr_pl_t* rv_get_pl_csrs(rv_core_t *c, u8 pl);

const char *rv_name_for_reg(u32 reg);
u32 rv_reg_for_name(const char *name);
