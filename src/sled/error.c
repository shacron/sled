// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#include <sled/error.h>

#define countof(ar) (sizeof(ar) / sizeof(ar[0]))

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
    ESTR(SL_ERR_NODEV),

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
};

const char *st_err(int err) {
    if (err > 0) return "UNDEFINED";
    err = -err;
    if (err > countof(err_str)) return "UNDEFINED";
    const char *e = err_str[err];
    if (!e) e = "ST_ERR_UNKNOWN";
    return e;
}
