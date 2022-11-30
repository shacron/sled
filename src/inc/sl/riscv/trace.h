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

#pragma once

#ifdef RV_TRACE

#include <stdio.h>

#define RV_TRACE_DECL_OPSTR const char *opstr
#define RV_TRACE_OPSTR(s) do { opstr = s; } while (0)
#define RV_TRACE_RD(c, r, v) do { c->core.trace->rd = r; c->core.trace->rd_value = v; } while (0)
#define RV_TRACE_STORE(c, a, r, v) \
    do { \
        c->core.trace->options |= ITRACE_OPT_INST_STORE; \
        c->core.trace->addr = a; c->core.trace->rd = r; c->core.trace->rd_value = v; \
    } while (0)
#define RV_TRACE_OPT(c, o) do { c->core.trace->options |= o; } while (0)
#define RV_TRACE_PRINT(c, ...) \
    do { \
        itrace_t *_tr = c->core.trace; \
        int _len = snprintf(_tr->opstr + _tr->cur, TRACE_STR_LEN - _tr->cur, __VA_ARGS__); \
        _tr->cur += _len; \
    } while (0)
#else
#define RV_TRACE_DECL_OPSTR do {} while (0)
#define RV_TRACE_OPSTR(s) do {} while (0)
#define RV_TRACE_RD(c, r, v) do {} while (0)
#define RV_TRACE_STORE(c, a, r, v) do {} while (0)
#define RV_TRACE_OPT(c, o) do {} while (0)
#define RV_TRACE_PRINT(rc, format, ...) do {} while (0)
#endif
