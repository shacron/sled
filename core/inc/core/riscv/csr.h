// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#include <core/common.h>

typedef struct rv_core rv_core_t;

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
    uint32_t raw;
    struct {
        uint32_t _wpri0:1;  // write preserve, read ignore
        uint32_t sie:1;     // s-mode interrupts enabled
        uint32_t _wpri1:1;  // -------------
        uint32_t m_mie:1;   // m-mode interrupts enabled
        uint32_t _wpri2:1;  // -------------
        uint32_t spie:1;    // s-mode interrupt state prior to exception
        uint32_t ube:1;     // big endian
        uint32_t m_mpie:1;  // m-mode interrupt state prior to exception
        uint32_t ssp:1;     // s-mode prior privilege
        uint32_t vs:2;      // vector state
        uint32_t m_mpp:2;   // m-mode prior privilege
        uint32_t fs:2;      // floating point state
        uint32_t xs:2;      // user mode extension state
        uint32_t m_mprv:1;  // effective memory privilege mode
        uint32_t sum:1;     // supervisor user memory access
        uint32_t mxr:1;     // make executable readable
        uint32_t m_tvm:1;   // trap virtual memory
        uint32_t m_tw:1;    // timeout wait
        uint32_t m_tsr:1;   // trap SRET
        uint32_t _wpri3:8;  // -------------
        uint32_t sd:1;      // (32-bit only) something dirty (fs/vs/xs)
    };
} csr_status_t;

typedef struct {
    uint32_t _wpri0:4;  // write preserve, read ignore
    uint32_t sbe:1;     // s-mode big endian
    uint32_t m_mbe:1;   // m-mode big endian
    uint32_t _wpri1:26; // -------------
} csr_statush_t;

typedef union {
    uint64_t raw;
    struct {
        uint64_t _wpri0:1;  // write preserve, read ignore
        uint64_t sie:1;     // s-mode interrupts enabled
        uint64_t _wpri1:1;  // -------------
        uint64_t m_mie:1;   // m-mode interrupts enabled
        uint64_t _wpri2:1;  // -------------
        uint64_t spie:1;    // s-mode interrupt state prior to exception
        uint64_t ube:1;     // big endian
        uint64_t m_mpie:1;  // m-mode interrupt state prior to exception
        uint64_t ssp:1;     // s-mode prior privilege
        uint64_t vs:2;      // vector state
        uint64_t m_mpp:2;   // m-mode prior privilege
        uint64_t fs:2;      // floating point state
        uint64_t xs:2;      // user mode extension state
        uint64_t m_mprv:1;  // effective memory privilege mode
        uint64_t sum:1;     // supervisor user memory access
        uint64_t mxr:1;     // make executable readable
        uint64_t m_tvm:1;   // trap virtual memory
        uint64_t m_tw:1;    // timeout wait
        uint64_t m_tsr:1;   // trap SRET
        uint64_t _wpri3:9;  // -------------
        uint64_t uxl:2;     // 64-bit only: u-mode xlen
        uint64_t sxl:2;     // 64-bit only: s-mode xlen
        uint64_t sbe:1;     // s-mode big endian
        uint64_t m_mbe:1;   // m-mode big endian
        uint64_t _wpri4:25; // -------------
        uint64_t sd:1;      // something dirty (fs/vs/xs)
    };
} csr_status64_t;

typedef struct {
    uint32_t cy:1;
    uint32_t tm:1;
    uint32_t ir:1;
    uint32_t hpm:29; // hpm3 ... hpm31
} csr_counteren_t;


typedef union {
    uint32_t raw;
    struct {
        uint32_t num:4;
        uint32_t func:4;
        uint32_t level:2;
        uint32_t type:2;
        uint32_t _unused:20;
    } f;
} csr_addr_t;

result64_t rv_csr_op(rv_core_t *c, int op, uint32_t csr, uint64_t value);
result64_t rv_csr_update(rv_core_t *c, int op, uint64_t *reg, uint64_t update_value);

const char *rv_name_for_sysreg(rv_core_t *c, uint16_t num);
