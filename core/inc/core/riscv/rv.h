// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

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

#define RV_NAN_S        0x7fc00000
// #define RV_NAN_D        

typedef struct {
    u8 scratch;
    u8 epc;
    u8 cause;
    u8 tval;
    u8 ip;
    u8 isa;
    u8 edeleg;
    u8 ideleg;
    u8 ie;
    u8 tvec;
    u8 counteren;
} rv_sr_pl_t;

typedef struct {
    result64_t (*csr_op)(rv_core_t *c, int op, u4 csr, u8 value);
    const char *(*name_for_sysreg)(rv_core_t *c, u2 num);
    void (*destroy)(void *ext_private);
} rv_isa_extension_t;

struct rv_core {
    sl_core_t core;

    u8 status;

    // system registers
    rv_sr_pl_t sr_pl[3];
    u8 mvendorid;
    u8 marchid;
    u8 mimpid;
    u8 mhartid;
    u8 mconfigptr;

    u8 stap;

    // offsets for calculating these from running counters
    i8 mcycle_offset;
    i8 minstret_offset;

    u4 pmpcfg[16];
    u8 pmpaddr[64];
    u8 mhpmcounter[29];
    u8 mhpevent[29]; // mhpevent3-mhpevent31

    rv_isa_extension_t ext; // isa extension
    void *ext_private;
};

int rv_dispatch(rv_core_t *c, u4 instruction);

int rv_exception_enter(sl_core_t *c, u8 cause, u8 addr);
int rv_exception_return(rv_core_t *c, u1 op);

rv_sr_pl_t* rv_get_pl_csrs(rv_core_t *c, u1 pl);

u1 rv_reg_index(u4 reg);
const char *rv_name_for_reg(u4 reg);
u4 rv_reg_for_name(const char *name);
