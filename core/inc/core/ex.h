// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#pragma once

typedef enum {
    EX_SYSCALL,
    EX_BREAKPOINT,
    EX_UNDEFINDED,
    EX_ABORT_LOAD,
    EX_ABORT_LOAD_ALIGN,
    EX_ABORT_STORE,
    EX_ABORT_STORE_ALIGN,
    EX_ABORT_INST,
    EX_ABORT_INST_ALIGN,

    EX_MATH_INTEGER,
    EX_MATH_FP,

    NUM_EXCPTIONS
} core_ex_t;

