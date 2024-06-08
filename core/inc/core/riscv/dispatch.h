// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/riscv/inst.h>
#include <core/riscv/rv.h>

int rv_exec_atomic(rv_core_t *c, rv_inst_t inst);
int rv_exec_ebreak(rv_core_t *c);
int rv_exec_mem(rv_core_t *c, rv_inst_t inst);
int rv_exec_system(rv_core_t *c, rv_inst_t inst);

int rv32_dispatch(rv_core_t *c, rv_inst_t inst);
int rv64_dispatch(rv_core_t *c, rv_inst_t inst);

static inline int rv_undef(rv_core_t *c, rv_inst_t inst) {
    return sl_core_synchronous_exception(&c->core, EX_UNDEFINDED, inst.raw, 0);
}
