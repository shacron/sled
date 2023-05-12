// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#define TRACE_STR_LEN   64

#define ITRACE_OPT_INST_STORE   (1u << 0)
#define ITRACE_OPT_INST16       (1u << 1)
#define ITRACE_OPT_SYSREG       (1u << 2)

typedef struct {
    u8 pc;
    u8 sp;
    u4 opcode;
    u2 rd;
    u2 options;
    u1 pl;
    u8 rd_value;
    u8 addr;
    u8 aux_value;
    u4 cur;
    char opstr[TRACE_STR_LEN];
} itrace_t;
