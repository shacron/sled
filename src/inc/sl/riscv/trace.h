#pragma once

#ifdef RV_TRACE
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
