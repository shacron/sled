// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <core/bus.h>
#include <core/common.h>
#include <core/riscv.h>
#include <core/riscv/csr.h>
#include <core/riscv/rv.h>
#include <sled/arch.h>
#include <sled/error.h>
#include <sled/riscv/csr.h>

int riscv_core_exception_enter(sl_core_t *core, u8 cause, u8 addr);
static void riscv_core_shutdown(sl_core_t *c);
static void riscv_core_destroy(sl_core_t *c);

static void riscv_core_set_reg(sl_core_t *c, u4 reg, u8 value) {
    rv_core_t *rc = (rv_core_t *)c;

    if (reg == 0) return;  // always zero
    if (reg < 32) {
        rc->core.r[reg] = value;
        return;
    }

    rv_sr_pl_t *sr = NULL;
    if (reg >= SL_RV_CORE_REG_BASE) sr = rv_get_pl_csrs(rc, RV_PL_MACHINE);

    switch (reg) {
    case SL_CORE_REG_PC:   rc->core.pc = value;         break;
    case SL_CORE_REG_SP:   rc->core.r[RV_SP] = value;   break;
    case SL_CORE_REG_LR:   rc->core.r[RV_RA] = value;   break;
    case SL_RV_CORE_REG(RV_CSR_MTVEC):     sr->tvec = value;       break;
    case SL_RV_CORE_REG(RV_CSR_MSCRATCH):  sr->scratch = value;    break;
    case SL_RV_CORE_REG(RV_CSR_MEPC):      sr->epc = value;        break;
    case SL_RV_CORE_REG(RV_CSR_MCAUSE):    sr->cause = value;      break;
    case SL_RV_CORE_REG(RV_CSR_MTVAL):     sr->tval = value;       break;
    case SL_RV_CORE_REG(RV_CSR_MIP):       sr->ip = value;         break;
    default:
        assert(false);
        break;
    }
}

static u8 riscv_core_get_reg(sl_core_t *c, u4 reg) {
    rv_core_t *rc = (rv_core_t *)c;

    if (reg == 0) return 0;  // always zero
    if (reg < 32)
        return rc->core.r[reg];

    switch (reg) {
    case SL_CORE_REG_PC: return rc->core.pc;
    case SL_CORE_REG_SP: return rc->core.r[RV_SP];
    case SL_CORE_REG_LR: return rc->core.r[RV_RA];
    case SL_CORE_REG_ARG0: return rc->core.r[RV_A0];
    case SL_CORE_REG_ARG1: return rc->core.r[RV_A1];
    default:
        assert(false);
        return 0xbaddbaddbaddbadd;
    }
}

// Synchronous irq handler - invokes an exception before the next instruction is dispatched.
static int riscv_interrupt(sl_engine_t *e) {
    sl_irq_ep_t *ep = &e->irq_ep;
    rv_core_t *rc = containerof(e, rv_core_t, core.engine);

    static const u1 irq_pri[] = {
        RV_INT_EXTERNAL_M, RV_INT_TIMER_M, RV_INT_SW_M, RV_INT_EXTERNAL_S, RV_INT_TIMER_S, RV_INT_SW_S
    };

    for (int i = 0; i < 6; i++) {
        const u1 bit = irq_pri[i];
        const u4 num = (1u << bit);
        if (ep->asserted & num)
            return riscv_core_exception_enter(&rc->core, bit | RV_CAUSE64_INT, 0);
    }
    return SL_ERR_STATE;
}

int sl_riscv_core_create(sl_core_params_t *p, sl_core_t **core_out) {
    rv_core_t *rc = calloc(1, sizeof(*rc));
    if (rc == NULL) return SL_ERR_MEM;
    int err = sl_core_init(&rc->core, p, bus_get_mapper(p->bus));
    if (err) {
        free(rc);
        return err;
    }
    *core_out = &rc->core;
    rc->core.exception_enter = riscv_core_exception_enter;
    rc->core.set_reg = riscv_core_set_reg;
    rc->core.get_reg = riscv_core_get_reg;
    rc->core.shutdown = riscv_core_shutdown;
    rc->core.destroy = riscv_core_destroy;

    rc->core.options |= SL_CORE_OPT_ENDIAN_LITTLE;
    rc->mhartid = p->id;
    rc->core.engine.ops.interrupt = riscv_interrupt;
    rc->mimpid = 'sled';
    rc->ext.name_for_sysreg = rv_name_for_sysreg;
    return 0;
}

static void riscv_core_shutdown(sl_core_t *c) {
    rv_core_t *rc = (rv_core_t *)c;
    if (rc->ext.destroy != NULL) rc->ext.destroy(rc->ext_private);
}

static void riscv_core_destroy(sl_core_t *c) {
    if (c == NULL) return;
    sl_core_shutdown(c);
    rv_core_t *rc = (rv_core_t *)c;
    free(rc);
}


typedef struct {
    char *name;
    u4 ext;
    u2 version;
    u2 patch;
} rv_extension_t;

typedef struct {
    const char *name;
    u4 option;
} rv_extension_map_t;

static const rv_extension_map_t ext_map[] = {
  { "m", SL_RISCV_EXT_M },
  { "a", SL_RISCV_EXT_A },
  { "f", SL_RISCV_EXT_F | SL_RISCV_EXT_ZICSR },
  { "d", SL_RISCV_EXT_D | SL_RISCV_EXT_F | SL_RISCV_EXT_ZICSR },
  { "c", SL_RISCV_EXT_C },
  { "zicsr", SL_RISCV_EXT_ZICSR },
};

static int parse_attribute(char *s, rv_extension_t *ex) {
    int len = strlen(s);
    if (len == 0) goto malformed;
    char *p = strrchr(s, 'p');
    if ((p == NULL) || (p == s)) goto malformed;
    char *endp;
    u4 patch = strtoul(p + 1, &endp, 10);
    if (endp != s + len) goto malformed;
    *p = '\0';
    char *v = p - 1;
    for ( ; v != s; v--) {
        if (!isdigit(*v)) break;
    }
    if (v == p - 1) goto malformed;
    u4 version = strtoul(v + 1, &endp, 10);
    if (endp != p) goto malformed;
    v[1] = '\0';

    ex->name = s;
    ex->version = version;
    ex->patch = patch;
    return 0;

malformed:
    return SL_ERR_ARG;
}

int riscv_decode_attributes(const char *attrib, u4 *arch_options_out) {
    int err = 0;
    char *at = strdup(attrib);
    if (at == NULL) return SL_ERR_MEM;
    u4 options = 0;

    printf("RISCV attributes: %s\n", at);

    const char *sep = "_";
    char *lasts;
    char *s = strtok_r(at, sep, &lasts);
    if (s == NULL) {
        printf("invalid format string\n");
        err = SL_ERR_ARG;
        goto out;
    }

    rv_extension_t ex;
    if ((err = parse_attribute(s, &ex))) {
        printf("invalid attribute string segment '%s'\n", s);
        goto out;
    }

    if (!strcmp(ex.name, "rv64i")) {
        options = SL_CORE_MODE_8;
    } else {
        if (strcmp(ex.name, "rv32i")) {
            printf("unexpected arch mode: %s\n", ex.name);
            err = SL_ERR_ARG;
            goto out;
        }
        options = SL_CORE_MODE_4;
    }

    err = SL_ERR_ARG;
    while ((s = strtok_r(NULL, sep, &lasts))) {
        if (s[0] == '\0') continue;
        rv_extension_t ex = {};
        if ((err = parse_attribute(s, &ex))) {
            printf("invalid attribute string segment '%s'\n", s);
            goto out;
        }

        bool found = false;
        for (int i = 0; i < countof(ext_map); i++) {
            if (!strcmp(ex.name, ext_map[i].name)) {
                // printf("attribute %s (%up%u)\n", ex.name, ex.version, ex.patch);
                options |= ext_map[i].option;
                found = true;
                break;
            }
        }

        if (!found) {
            printf("unhandled extension: %s\n", ex.name);
            err = SL_ERR_UNSUPPORTED;
            goto out;
        }
    }
    err = 0;
    *arch_options_out = options;

out:
    free(at);
    return err;
}

