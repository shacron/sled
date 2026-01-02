// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2025 Shac Ron and The Sled Project

#pragma once

#include <core/riscv/inst.h>
#include <core/riscv/rv.h>

static inline int rv_undef(rv_core_t *c, rv_inst_t inst) {
    return sl_core_synchronous_exception(&c->core, EX_UNDEFINDED, inst.raw, 0);
}
