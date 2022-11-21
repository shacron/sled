#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define SL_OK                0
#define SL_ERR              -1

// user errors
#define SL_ERR_ARG           -2
#define SL_ERR_MEM           -3
#define SL_ERR_UNSUPPORTED   -4
#define SL_ERR_UNIMPLEMENTED -5
#define SL_ERR_FULL          -6
#define SL_ERR_RANGE         -7
#define SL_ERR_NODEV         -8

// instruction execution errors
#define SL_ERR_UNDEF        -16
#define SL_ERR_ABORT        -17
#define SL_ERR_SYSCALL      -18
#define SL_ERR_BREAKPOINT   -19

// IO errors
#define SL_ERR_BUS          -32

const char *st_err(int err);

#ifdef __cplusplus
}
#endif
