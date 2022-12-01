// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#pragma once

#include <stdint.h>

#include <sl/common.h>

#ifdef __cplusplus
extern "C" {
#endif

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

// Machine Information Registers (MRO)
#define RV_CSR_MVENDORID    0xf11 // MRO mvendorid - Vendor ID.
#define RV_CSR_MARCHID      0xf12 // MRO marchid - Architecture ID.
#define RV_CSR_MIMPID       0xf13 // MRO mimpid - Implementation ID.
#define RV_CSR_MHARTID      0xf14 // MRO mhartid - Hardware thread ID.
#define RV_CSR_MCONFIGPTR   0xf15 // MRO mconfigptr - Pointer to configuration data structure.
// Machine Trap Setup (MRW)
#define RV_CSR_MSTATUS      0x300 // MRW mstatus - Machine status register.
#define RV_CSR_MISA         0x301 // MRW misa - ISA and extensions
#define RV_CSR_MEDELEG      0x302 // MRW medeleg - Machine exception delegation register.
#define RV_CSR_MIDELEG      0x303 // MRW mideleg - Machine interrupt delegation register.
#define RV_CSR_MIE          0x304 // MRW mie - Machine interrupt-enable register.
#define RV_CSR_MTVEC        0x305 // MRW mtvec - Machine trap-handler base address.
#define RV_CSR_MCOUNTEREN   0x306 // MRW mcounteren - Machine counter enable.
#define RV_CSR_MSTATUSH     0x310 // MRW mstatush - Additional machine status register, RV32 only.
// Machine Trap Handling (MRW)
#define RV_CSR_MSCRATCH     0x340 // MRW mscratch = Scratch register for machine trap handlers.
#define RV_CSR_MEPC         0x341 // MRW mepc = Machine exception program counter.
#define RV_CSR_MCAUSE       0x342 // MRW mcause = Machine trap cause.
#define RV_CSR_MTVAL        0x343 // MRW mtval = Machine bad address or instruction.
#define RV_CSR_MIP          0x344 // MRW mip = Machine interrupt pending.
#define RV_CSR_MTINST       0x34a // MRW mtinst = Machine trap instruction (transformed).
#define RV_CSR_MTVAL2       0x34b // MRW mtval2 = Machine bad guest physical address.
#define RV_CSR_MENVCFG      0x30a // MRW menvcfg - Machine environment configuration register.
#define RV_CSR_MENVCFGH     0x31a // MRW menvcfgh - Additional machine env. conf. register, RV32 only.
// Machine Configuration
#define RV_CSR_MSECCFG      0x747 // MRW mseccfg - Machine security configuration register.
#define RV_CSR_MSECCFGH     0x757 // MRW mseccfgh - Additional machine security conf. register, RV32 only.
#define RV_CSR_PMPCFG_BASE  0x3a0 // MRW pmpcfg0-15 - Physical memory protection configuration.
#define RV_CSR_PMPCFG_NUM   16
#define RV_CSR_PMPADDR_BASE 0x3b0 // MRW pmpaddr0-63 - Physical memory protection address register.
#define RV_CSR_PMPADDR_NUM  64
// Machine Counter/Timers
#define RV_CSR_MCYCLE       0xb00 // MRW mcycle - Machine cycle counter.
#define RV_CSR_MINSTRET     0xb02 // MRW minstret - Machine instructions-retired counter.
#define RV_CSR_MHPMCOUNTER3 0xb03 // MRW mhpmcounter3 - Machine performance-monitoring counter.
#define RV_CSR_MHPMCOUNTER4 0xb04 // MRW mhpmcounter4 - Machine performance-monitoring counter.
#define RV_CSR_MHPMCOUNTER31    0xb1f // MRW mhpmcounter31 - Machine performance-monitoring counter.
#define RV_CSR_MCYCLEH      0xb80 // MRW mcycleh - Upper 32 bits of mcycle, RV32 only.
#define RV_CSR_MINSTRETH    0xb82 // MRW minstreth - Upper 32 bits of minstret, RV32 only.
#define RV_CSR_MHPMCOUNTER3H    0xb83 // MRW mhpmcounter3h - Upper 32 bits of mhpmcounter3, RV32 only.
#define RV_CSR_MHPMCOUNTER4H    0xb84 // MRW mhpmcounter4h - Upper 32 bits of mhpmcounter4, RV32 only.
#define RV_CSR_MHPMCOUNTER31H   0xb9f // MRW mhpmcounter31h - Upper 32 bits of mhpmcounter31, RV32 only.
// Machine Counter Setup
#define RV_CSR_MCOUNTINHIBIT    0x320 // MRW mcountinhibit - Machine counter-inhibit register.
#define RV_CSR_MHPMEVENT3       0x323 // MRW mhpmevent3 - Machine performance-monitoring event selector.
#define RV_CSR_MHPMEVENT4       0x324 // MRW mhpmevent4 - Machine performance-monitoring event selector.
#define RV_CSR_MHPMEVENT31      0x33f // MRW mhpmevent31 - Machine performance-monitoring event selector.
// Debug/Trace Registers (shared with Debug Mode)
#define RV_CSR_TSELECT      0x7a0 // MRW tselect - Debug/Trace trigger register select.
#define RV_CSR_TDATA1       0x7a1 // MRW tdata1 - First Debug/Trace trigger data register.
#define RV_CSR_TDATA2       0x7a2 // MRW tdata2 - Second Debug/Trace trigger data register.
#define RV_CSR_TDATA3       0x7a3 // MRW tdata3 - Third Debug/Trace trigger data register.
#define RV_CSR_MCONTEXT     0x7a8 // MRW mcontext - Machine-mode context register.
// Debug Mode Registers
#define RV_CSR_DCSR         0x7b0 // DRW dcsr - Debug control and status register.
#define RV_CSR_DPC          0x7b1 // DRW dpc - Debug PC.
#define RV_CSR_DSCRATCH0    0x7b2 // DRW dscratch0 - Debug scratch register 0.
#define RV_CSR_DSCRATCH1    0x7b3 // DRW dscratch1 - Debug scratch register 1.



#define RV_SR_STATUS_BIT_WPRI0 0
#define RV_SR_STATUS_BIT_SIE   1
#define RV_SR_STATUS_BIT_WPRI1 2
#define RV_SR_STATUS_BIT_MIE   3
#define RV_SR_STATUS_BIT_WPRI2 4
#define RV_SR_STATUS_BIT_SPIE  5
#define RV_SR_STATUS_BIT_UBE   6
#define RV_SR_STATUS_BIT_MPIE  7
#define RV_SR_STATUS_BIT_SPP   8
#define RV_SR_STATUS_BIT_VS    9
#define RV_SR_STATUS_BIT_MPP   11
#define RV_SR_STATUS_BIT_FS    13
#define RV_SR_STATUS_BIT_XS    15
#define RV_SR_STATUS_BIT_MPRV  17
#define RV_SR_STATUS_BIT_SUM   18
#define RV_SR_STATUS_BIT_MXR   19
#define RV_SR_STATUS_BIT_TVM   20
#define RV_SR_STATUS_BIT_TW    21
#define RV_SR_STATUS_BIT_TSR   22
#define RV_SR_STATUS_BIT_WPRI3 23
#define RV_SR_STATUS_BIT_SD    31   // in 64 bit mode this is elsewhere
#define RV_SR_STATUS_BIT_UXL   32
#define RV_SR_STATUS_BIT_SXL   34
#define RV_SR_STATUS_BIT_SBE   36
#define RV_SR_STATUS_BIT_MBE   37
#define RV_SR_STATUS_BIT_WPRI4 38
#define RV_SR_STATUS64_BIT_SD  63

#define RV_SR_STATUS_WPRI0     (1ul << RV_SR_STATUS_BIT_WPRI0)
#define RV_SR_STATUS_SIE       (1ul << RV_SR_STATUS_BIT_SIE)
#define RV_SR_STATUS_WPRI1     (1ul << RV_SR_STATUS_BIT_WPRI1)
#define RV_SR_STATUS_MIE       (1ul << RV_SR_STATUS_BIT_MIE)
#define RV_SR_STATUS_WPRI2     (1ul << RV_SR_STATUS_BIT_WPRI2)
#define RV_SR_STATUS_SPIE      (1ul << RV_SR_STATUS_BIT_SPIE)
#define RV_SR_STATUS_UBE       (1ul << RV_SR_STATUS_BIT_UBE)
#define RV_SR_STATUS_MPIE      (1ul << RV_SR_STATUS_BIT_MPIE)
#define RV_SR_STATUS_SPP       (1ul << RV_SR_STATUS_BIT_SPP)
#define RV_SR_STATUS_MPRV      (1ul << RV_SR_STATUS_BIT_MPRV)
#define RV_SR_STATUS_SUM       (1ul << RV_SR_STATUS_BIT_SUM)
#define RV_SR_STATUS_MXR       (1ul << RV_SR_STATUS_BIT_MXR)
#define RV_SR_STATUS_TVM       (1ul << RV_SR_STATUS_BIT_TVM)
#define RV_SR_STATUS_TW        (1ul << RV_SR_STATUS_BIT_TW)
#define RV_SR_STATUS_TSR       (1ul << RV_SR_STATUS_BIT_TSR)
#define RV_SR_STATUS_SD        (1ul << RV_SR_STATUS_BIT_SD)
#define RV_SR_STATUS_SBE       (1ul << RV_SR_STATUS_BIT_SBE)
#define RV_SR_STATUS_MBE       (1ul << RV_SR_STATUS_BIT_MBE)
#define RV_SR_STATUS_WPRI4     (1ul << RV_SR_STATUS_BIT_WPRI4)
#define RV_SR_STATUS64_SD      (1ul << RV_SR_STATUS64_BIT_SD)

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

typedef struct {
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
} csr_status64_t;

// mcause where interrupt = 1
#define RV_SR_MCAUSE_SW_INT_S       1
#define RV_SR_MCAUSE_SW_INT_M       3
#define RV_SR_MCAUSE_TIMER_S        5
#define RV_SR_MCAUSE_TIMER_M        7
#define RV_SR_MCAUSE_EXT_S          9
#define RV_SR_MCAUSE_EXT_M          11

// mcause where interrupt = 0
#define RV_SR_MCAUSE_INST_ALIGN     0
#define RV_SR_MCAUSE_INST_ACCESS    1
#define RV_SR_MCAUSE_INST_ILLEGAL   2
#define RV_SR_MCAUSE_BREAKPOINT     3
#define RV_SR_MCAUSE_LOAD_ALIGN     4
#define RV_SR_MCAUSE_LOAD_ACCESS    5
#define RV_SR_MCAUSE_STORE_ALIGN    6
#define RV_SR_MCAUSE_STORE_FAULT    7
#define RV_SR_MCAUSE_CALL_FROM_U    8
#define RV_SR_MCAUSE_CALL_FROM_S    9
// 10 - reserved
#define RV_SR_MCAUSE_CALL_FROM_M    11
#define RV_SR_MCAUSE_INST_PAGE_FAULT    12
#define RV_SR_MCAUSE_LOAD_PAGE_FAULT    13
// 14 - reserved
#define RV_SR_MCAUSE_STORE_PAGE_FAULT   15


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

// mcause where interrupt = 1
#define RV_SR_MCAUSE_SW_INT_S       1
#define RV_SR_MCAUSE_SW_INT_M       3
#define RV_SR_MCAUSE_TIMER_S        5
#define RV_SR_MCAUSE_TIMER_M        7
#define RV_SR_MCAUSE_EXT_S          9
#define RV_SR_MCAUSE_EXT_M          11

// mcause where interrupt = 0
#define RV_SR_MCAUSE_INST_ALIGN     0
#define RV_SR_MCAUSE_INST_ACCESS    1
#define RV_SR_MCAUSE_INST_ILLEGAL   2
#define RV_SR_MCAUSE_BREAKPOINT     3
#define RV_SR_MCAUSE_LOAD_ALIGN     4
#define RV_SR_MCAUSE_LOAD_ACCESS    5
#define RV_SR_MCAUSE_STORE_ALIGN    6
#define RV_SR_MCAUSE_STORE_FAULT    7
#define RV_SR_MCAUSE_CALL_FROM_U    8
#define RV_SR_MCAUSE_CALL_FROM_S    9
// 10 - reserved
#define RV_SR_MCAUSE_CALL_FROM_M    11
#define RV_SR_MCAUSE_INST_PAGE_FAULT    12
#define RV_SR_MCAUSE_LOAD_PAGE_FAULT    13
// 14 - reserved
#define RV_SR_MCAUSE_STORE_PAGE_FAULT   15


result64_t rv_csr_op(rv_core_t *c, int op, uint32_t csr, uint64_t value);
result64_t rv_csr_update(rv_core_t *c, int op, uint64_t *reg, uint64_t update_value);

const char *rv_name_for_sysreg(rv_core_t *c, uint16_t num);

#ifdef __cplusplus
}
#endif
