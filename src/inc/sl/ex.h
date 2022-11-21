#pragma once

typedef enum {
    EX_SYSCALL,
    EX_BREAKPOINT,
    EX_UNDEFINDED,
    EX_ABORT_READ,
    EX_ABORT_WRITE,

    EX_MATH_INTEGER,
    EX_MATH_FP,

    NUM_EXCPTIONS
} core_ex_t;

