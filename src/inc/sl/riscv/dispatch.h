#pragma once

#include <sl/riscv/inst.h>
#include <sl/riscv/rv.h>

int rv_exec_ebreak(rv_core_t *c);
int rv_exec_mem(rv_core_t *c, rv_inst_t inst);
int rv_exec_system(rv_core_t *c, rv_inst_t inst);

int rv32_dispatch(rv_core_t *c, rv_inst_t inst);
int rv64_dispatch(rv_core_t *c, rv_inst_t inst);

static inline int rv_undef(rv_core_t *c, rv_inst_t inst) {
    return rv_synchronous_exception(c, EX_UNDEFINDED, inst.raw, 0);
}
