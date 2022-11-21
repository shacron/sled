#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct core core_t;
typedef struct core_params core_params_t;

#define CORE_OPT_TRAP_SYSCALL    (1u << 0)
#define CORE_OPT_TRAP_BREAKPOINT (1u << 1)
#define CORE_OPT_ENDIAN_LITTLE  (1u << 30)
#define CORE_OPT_ENDIAN_BIG     (1u << 31)

struct core_params {
    uint8_t arch;
    uint8_t subarch;
    uint8_t id;
    uint32_t options;
    uint32_t arch_options;
};

// Special register defines to pass to set/get_reg()
#define CORE_REG_PC     0xffff
#define CORE_REG_SP     0xfffe
#define CORE_REG_LR     0xfffd
#define CORE_REG_ARG0   0xfffc
#define CORE_REG_ARG1   0xfffb

void core_set_reg(core_t *c, uint32_t reg, uint64_t value);
uint64_t core_get_reg(core_t *c, uint32_t reg);
int core_step(core_t *c, uint32_t num);
int core_run(core_t *c);
int core_set_state(core_t *c, uint32_t state, bool enabled);

uint64_t core_get_cycles(core_t *c);

#ifdef __cplusplus
}
#endif
