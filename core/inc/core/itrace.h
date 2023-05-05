// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

#define TRACE_STR_LEN   64

#define ITRACE_OPT_INST_STORE   (1u << 0)
#define ITRACE_OPT_INST16       (1u << 1)
#define ITRACE_OPT_SYSREG       (1u << 2)

typedef struct {
    u64 pc;
    u64 sp;
    u32 opcode;
    u16 rd;
    u16 options;
    u8 pl;
    u64 rd_value;
    u64 addr;
    u64 aux_value;
    u32 cur;
    char opstr[TRACE_STR_LEN];
} itrace_t;
