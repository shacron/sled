// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>

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
    u32 raw;
    struct {
        u32 _wpri0:1;  // write preserve, read ignore
        u32 sie:1;     // s-mode interrupts enabled
        u32 _wpri1:1;  // -------------
        u32 m_mie:1;   // m-mode interrupts enabled
        u32 _wpri2:1;  // -------------
        u32 spie:1;    // s-mode interrupt state prior to exception
        u32 ube:1;     // big endian
        u32 m_mpie:1;  // m-mode interrupt state prior to exception
        u32 spp:1;     // s-mode prior privilege
        u32 vs:2;      // vector state
        u32 m_mpp:2;   // m-mode prior privilege
        u32 fs:2;      // floating point state
        u32 xs:2;      // user mode extension state
        u32 m_mprv:1;  // effective memory privilege mode
        u32 sum:1;     // supervisor user memory access
        u32 mxr:1;     // make executable readable
        u32 m_tvm:1;   // trap virtual memory
        u32 m_tw:1;    // timeout wait
        u32 m_tsr:1;   // trap SRET
        u32 _wpri3:8;  // -------------
        u32 sd:1;      // (32-bit only) something dirty (fs/vs/xs)
    };
} csr_status_t;

typedef struct {
    u32 _wpri0:4;  // write preserve, read ignore
    u32 sbe:1;     // s-mode big endian
    u32 m_mbe:1;   // m-mode big endian
    u32 _wpri1:26; // -------------
} csr_statush_t;

typedef union {
    u64 raw;
    struct {
        u64 _wpri0:1;  // write preserve, read ignore
        u64 sie:1;     // s-mode interrupts enabled
        u64 _wpri1:1;  // -------------
        u64 m_mie:1;   // m-mode interrupts enabled
        u64 _wpri2:1;  // -------------
        u64 spie:1;    // s-mode interrupt state prior to exception
        u64 ube:1;     // big endian
        u64 m_mpie:1;  // m-mode interrupt state prior to exception
        u64 spp:1;     // s-mode prior privilege
        u64 vs:2;      // vector state
        u64 m_mpp:2;   // m-mode prior privilege
        u64 fs:2;      // floating point state
        u64 xs:2;      // user mode extension state
        u64 m_mprv:1;  // effective memory privilege mode
        u64 sum:1;     // supervisor user memory access
        u64 mxr:1;     // make executable readable
        u64 m_tvm:1;   // trap virtual memory
        u64 m_tw:1;    // timeout wait
        u64 m_tsr:1;   // trap SRET
        u64 _wpri3:9;  // -------------
        u64 uxl:2;     // 64-bit only: u-mode xlen
        u64 sxl:2;     // 64-bit only: s-mode xlen
        u64 sbe:1;     // s-mode big endian
        u64 m_mbe:1;   // m-mode big endian
        u64 _wpri4:25; // -------------
        u64 sd:1;      // something dirty (fs/vs/xs)
    };
} csr_status64_t;

typedef struct {
    u32 cy:1;
    u32 tm:1;
    u32 ir:1;
    u32 hpm:29; // hpm3 ... hpm31
} csr_counteren_t;


typedef union {
    u32 raw;
    struct {
        u32 num:4;
        u32 func:4;
        u32 level:2;
        u32 type:2;
        u32 _unused:20;
    } f;
} csr_addr_t;

result64_t rv_csr_op(rv_core_t *c, int op, u32 csr, u64 value);
result64_t rv_csr_update(rv_core_t *c, int op, u64 *reg, u64 update_value);

const char *rv_name_for_sysreg(rv_core_t *c, u16 num);
