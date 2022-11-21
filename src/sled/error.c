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

    ESTR(SL_ERR_BUS),
};

const char *st_err(int err) {
    if (err > 0) return "UNDEFINED";
    err = -err;
    if (err > countof(err_str)) return "UNDEFINED";
    const char *e = err_str[err];
    if (!e) e = "ST_ERR_UNKNOWN";
    return e;
}
