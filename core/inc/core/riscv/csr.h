// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>
#include <sled/riscv/csr.h>

#define RV_CSR_OP_READ       0b000
#define RV_CSR_OP_SWAP       0b001 // CSRRW/CSRRWI
#define RV_CSR_OP_READ_SET   0b010 // CSRRS/CSRRSI
#define RV_CSR_OP_READ_CLEAR 0b011 // CSRRC/CSRRCI
#define RV_CSR_OP_WRITE      0b100

// offsets in rv_sr_trap_t
// These match the CSR num field (lower 4 bits)
#define RV_SR_SCRATCH   0
#define RV_SR_EPC       1
#define RV_SR_CAUSE     2
#define RV_SR_TVAL      3
#define RV_SR_IP        4
// These don't match the num field. They are machine mode only
#define RV_SR_TINST     5
#define RV_SR_TVAL2     6

// offsets in sr_minfo + 1
// These match the CSR num field
#define RV_SR_MVENDORID     1
#define RV_SR_MARCHID       2
#define RV_SR_MIMPID        3
#define RV_SR_HARTID        4
#define RV_SR_MCONFIGPTR    5


// fields starting with m_ are only available in M mode, otherwise wpri

typedef union {
    u4 raw;
    struct {
        u4 _wpri0:1;  // write preserve, read ignore
        u4 sie:1;     // s-mode interrupts enabled
        u4 _wpri1:1;  // -------------
        u4 m_mie:1;   // m-mode interrupts enabled
        u4 _wpri2:1;  // -------------
        u4 spie:1;    // s-mode interrupt state prior to exception
        u4 ube:1;     // big endian
        u4 m_mpie:1;  // m-mode interrupt state prior to exception
        u4 spp:1;     // s-mode prior privilege
        u4 vs:2;      // vector state
        u4 m_mpp:2;   // m-mode prior privilege
        u4 fs:2;      // floating point state
        u4 xs:2;      // user mode extension state
        u4 m_mprv:1;  // effective memory privilege mode
        u4 sum:1;     // supervisor user memory access
        u4 mxr:1;     // make executable readable
        u4 m_tvm:1;   // trap virtual memory
        u4 m_tw:1;    // timeout wait
        u4 m_tsr:1;   // trap SRET
        u4 _wpri3:8;  // -------------
        u4 sd:1;      // (32-bit only) something dirty (fs/vs/xs)
    };
} csr_status_t;

typedef struct {
    u4 _wpri0:4;  // write preserve, read ignore
    u4 sbe:1;     // s-mode big endian
    u4 m_mbe:1;   // m-mode big endian
    u4 _wpri1:26; // -------------
} csr_statush_t;

typedef union {
    u8 raw;
    struct {
        u8 _wpri0:1;  // write preserve, read ignore
        u8 sie:1;     // s-mode interrupts enabled
        u8 _wpri1:1;  // -------------
        u8 m_mie:1;   // m-mode interrupts enabled
        u8 _wpri2:1;  // -------------
        u8 spie:1;    // s-mode interrupt state prior to exception
        u8 ube:1;     // big endian
        u8 m_mpie:1;  // m-mode interrupt state prior to exception
        u8 spp:1;     // s-mode prior privilege
        u8 vs:2;      // vector state
        u8 m_mpp:2;   // m-mode prior privilege
        u8 fs:2;      // floating point state
        u8 xs:2;      // user mode extension state
        u8 m_mprv:1;  // effective memory privilege mode
        u8 sum:1;     // supervisor user memory access
        u8 mxr:1;     // make executable readable
        u8 m_tvm:1;   // trap virtual memory
        u8 m_tw:1;    // timeout wait
        u8 m_tsr:1;   // trap SRET
        u8 _wpri3:9;  // -------------
        u8 uxl:2;     // 64-bit only: u-mode xlen
        u8 sxl:2;     // 64-bit only: s-mode xlen
        u8 sbe:1;     // s-mode big endian
        u8 m_mbe:1;   // m-mode big endian
        u8 _wpri4:25; // -------------
        u8 sd:1;      // something dirty (fs/vs/xs)
    };
} csr_status64_t;

typedef struct {
    u4 cy:1;
    u4 tm:1;
    u4 ir:1;
    u4 hpm:29; // hpm3 ... hpm31
} csr_counteren_t;


typedef union {
    u4 raw;
    struct {
        u4 num:4;
        u4 func:4;
        u4 level:2;
        u4 type:2;
        u4 _unused:20;
    } f;
} csr_addr_t;

result64_t rv_csr_op(rv_core_t *c, int op, u4 csr, u8 value);
result64_t rv_csr_update(rv_core_t *c, int op, u8 *reg, u8 update_value);

const char *rv_name_for_sysreg(rv_core_t *c, u2 num);
