#pragma once

#include <stdint.h>

#define TRACE_STR_LEN   64

#define ITRACE_OPT_INST_STORE   (1u << 0)
#define ITRACE_OPT_INST16       (1u << 1)

typedef struct {
    uint64_t pc;
    uint64_t sp;
    uint32_t opcode;
    uint16_t rd;
    uint16_t options;
    uint64_t rd_value;
    uint64_t addr;
    uint32_t cur;
    char opstr[TRACE_STR_LEN];
} itrace_t;
