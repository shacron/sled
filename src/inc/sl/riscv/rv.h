#pragma once

#include <sl/common.h>
#include <sl/core.h>
#include <sl/ex.h>

#ifdef __cplusplus
extern "C" {
#endif

// #define RV_TRACE 1

typedef struct rv_core rv_core_t;

#define RV_ZERO 0   // zero
#define RV_RA   1   // return address
#define RV_SP   2   // stack pointer
#define RV_GP   3   // global pointer
#define RV_TP   4   // thread pointer
#define RV_T0   5   // Temporary/alternate link register
#define RV_T1   6   // Temporary
#define RV_T2   7   // Temporary
#define RV_FP   8   // Frame pointer
#define RV_S0   8   // Saved register (same as FP)
#define RV_S1   9   // Saved register
#define RV_A0   10  // Function arguments/return values
#define RV_A1   11  // Function arguments/return values
#define RV_A2   12  // Function argument
#define RV_A3   13  // Function argument
#define RV_A4   14  // Function argument
#define RV_A5   15  // Function argument
#define RV_A6   16  // Function argument
#define RV_A7   17  // Function argument
#define RV_S2   18  // Saved register
#define RV_S3   19  // Saved register
#define RV_S4   20  // Saved register
#define RV_S5   21  // Saved register
#define RV_S6   22  // Saved register
#define RV_S7   23  // Saved register
#define RV_S8   24  // Saved register
#define RV_S9   25  // Saved register
#define RV_S10  26  // Saved register
#define RV_S11  27  // Saved register
#define RV_T3   28  // Temporary
#define RV_T4   29  // Temporary
#define RV_T5   30  // Temporary
#define RV_T6   31  // Temporary

#define RV_PRIV_LEVEL_USER       0
#define RV_PRIV_LEVEL_SUPERVISOR 1
#define RV_PRIV_LEVEL_HYPERVISOR 2
#define RV_PRIV_LEVEL_MACHINE    3

#define RV_INT_SUPER_SW         1
#define RV_INT_MACHINE_SW       3
#define RV_INT_SUPER_TIMER      5
#define RV_INT_MACHINE_TIMER    7
#define RV_INT_SUPER_EXTERNAL   9
#define RV_INT_MACHINE_EXTERNAL 11

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

    uint32_t pending_irq;
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
int rv_exception_enter(rv_core_t *c, uint64_t cause, bool irq);
int rv_exception_return(rv_core_t *c);

rv_sr_mode_t* rv_get_mode_registers(rv_core_t *c, uint8_t priv_level);

const char *rv_name_for_reg(uint32_t reg);

#ifdef __cplusplus
}
#endif
