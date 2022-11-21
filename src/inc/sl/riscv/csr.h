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
