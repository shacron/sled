#pragma once

#include <sl/core.h>
#include <sled/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RV_HAS_PRIV_LEVEL_USER       (1u << 0)
#define RV_HAS_PRIV_LEVEL_SUPERVISOR (1u << 1)
#define RV_HAS_PRIV_LEVEL_HYPERVISOR (1u << 2)

#define RV_REG_MSCRATCH 0x80000000
#define RV_REG_MEPC     0x80000001
#define RV_REG_MCAUSE   0x80000002
#define RV_REG_MTVAL    0x80000003
#define RV_REG_MIP      0x80000004

typedef struct bus bus_t;

int riscv_core_create(core_params_t *p, bus_t *bus, core_t **core_out);

#ifdef __cplusplus
}
#endif
