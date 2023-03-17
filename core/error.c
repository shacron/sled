// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <core/common.h>
#include <sled/error.h>

#define ESTR(e) [-(e)] = #e

static const char *err_str[] = {
    [SL_OK] = "SL_OK",
    ESTR(SL_ERR),
    ESTR(SL_ERR_ARG),
    ESTR(SL_ERR_MEM),
    ESTR(SL_ERR_UNSUPPORTED),
    ESTR(SL_ERR_UNIMPLEMENTED),
    ESTR(SL_ERR_FULL),
    ESTR(SL_ERR_RANGE),
    ESTR(SL_ERR_STATE),
    ESTR(SL_ERR_TIMEOUT),
    ESTR(SL_ERR_BUSY),
    ESTR(SL_ERR_EXITED),

    ESTR(SL_ERR_UNDEF),
    ESTR(SL_ERR_ABORT),
    ESTR(SL_ERR_SYSCALL),
    ESTR(SL_ERR_BREAKPOINT),

    // ESTR(SL_ERR_BUS),
    ESTR(SL_ERR_IO_NODEV),
    ESTR(SL_ERR_IO_ALIGN),
    ESTR(SL_ERR_IO_SIZE),
    ESTR(SL_ERR_IO_COUNT),
    ESTR(SL_ERR_IO_PERM),
    ESTR(SL_ERR_IO_NOWR),
    ESTR(SL_ERR_IO_NORD),
    ESTR(SL_ERR_IO_INVALID),
    ESTR(SL_ERR_IO_NOMAP),
};

const char *st_err(int err) {
    if (err > 0) return "UNDEFINED";
    err = -err;
    if (err > countof(err_str)) return "UNDEFINED";
    const char *e = err_str[err];
    if (!e) e = "ST_ERR_UNKNOWN";
    return e;
}
