// SPDX-License-Identifier: MIT License
// Copyright (c) 2024-2026 Shac Ron and The Sled Project

#include <math.h>

#include <core/core.h>
#include <core/ex.h>
#include <sled/error.h>
#include <sled/slac.h>

#define F32_SIGN_BIT    (1u << 31)
#define F64_SIGN_BIT    (1ul << 63)

#define FP_SET_RESULT   (1 << 0)
#define FP_SET_FLAGS    (1 << 1)

int slac_exec_fp32(sl_core_t *c, sl_slac_inst_t *si) {
    sl_fp_reg_t result = {};
    fexcept_t flags;
    u1 set_opts = FP_SET_RESULT | FP_SET_FLAGS;
    bool comp;

    feclearexcept(FE_ALL_EXCEPT);

    switch (si->func) {
    case SLAC_FUNC_FADD:
        result.f = c->f[si->r0].f + c->f[si->r1].f;
        break;

    case SLAC_FUNC_FSUB:
        result.f = c->f[si->r0].f - c->f[si->r1].f;
        break;

    case SLAC_FUNC_FMUL:
        result.f = c->f[si->r0].f * c->f[si->r1].f;
        break;

    case SLAC_FUNC_FDIV:
#if __x64_64__
        if (c->f[si->r1].f == 0)
            result.f = 0.0;
        else
#endif
        result.f = c->f[si->r0].f / c->f[si->r1].f;
        break;

    case SLAC_FUNC_FSQRT:
        result.f = sqrtf(c->f[si->r0].f);
        break;

    case SLAC_FUNC_FMIN:
        result.f = fminf(c->f[si->r0].f, c->f[si->r1].f);
        break;

    case SLAC_FUNC_FMAX:
        result.f = fmaxf(c->f[si->r0].f, c->f[si->r1].f);
        break;

    case SLAC_FUNC_FEQ:
        set_opts = FP_SET_FLAGS;
        comp = c->f[si->r0].f == c->f[si->r1].f;
        if (si->d0 != SLAC_REG_DISCARD)
            c->r[si->d0] = comp;
        break;

    case SLAC_FUNC_FLT:
        set_opts = FP_SET_FLAGS;
        comp = isless(c->f[si->r0].f, c->f[si->r1].f);
        if (si->d0 != SLAC_REG_DISCARD)
            c->r[si->d0] = comp;
        break;

    case SLAC_FUNC_FLE:
        set_opts = FP_SET_FLAGS;
        comp = islessequal(c->f[si->r0].f, c->f[si->r1].f);
        if (si->d0 != SLAC_REG_DISCARD)
            c->r[si->d0] = comp;
        break;

    case SLAC_FUNC_FS:
        set_opts = FP_SET_RESULT;
        result.u4 = c->f[si->r0].u4 & ~F32_SIGN_BIT;
        result.u4 |= (c->f[si->r1].u4 & F32_SIGN_BIT);
        break;

    case SLAC_FUNC_FSN:
        set_opts = FP_SET_RESULT;
        result.u4 = c->f[si->r0].u4 & ~F32_SIGN_BIT;
        result.u4 |= ((~c->f[si->r1].u4) & F32_SIGN_BIT);
        break;

    case SLAC_FUNC_FSX:
        set_opts = FP_SET_RESULT;
        result.u4 = c->f[si->r0].u4 & ~F32_SIGN_BIT;
        result.u4 |= ((c->f[si->r0].u4 ^ c->f[si->r1].u4) & F32_SIGN_BIT);
        break;

    case SLAC_FUNC_FMOV_F_TO_R:
        set_opts = 0;
        if (c->mode == SL_CORE_MODE_4)
            c->r[si->d0] = c->f[si->r0].u4;
        else
            c->r[si->d0] = (i8)(i4)c->f[si->r0].u4;
        break;

    case SLAC_FUNC_FMOV_R_TO_F:
        set_opts = FP_SET_RESULT;
        result.u4 = (u4)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_S4_TO_F:
        result.f = (float)(i4)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_U4_TO_F:
        result.f = (float)(u4)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_S8_TO_F:
        if (c->mode != SL_CORE_MODE_8) goto undef;
        result.f = (float)(i8)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_U8_TO_F:
        if (c->mode != SL_CORE_MODE_8) goto undef;
        // todo: check if sets flags
        result.f = (float)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_F_TO_S4:
        result.u4 = (i4)c->f[si->r0].f;
        break;

    case SLAC_FUNC_FCVT_F_TO_U4:
        result.u4 = (u4)c->f[si->r0].f;
        break;

    case SLAC_FUNC_FCVT_F_TO_S8:
        if (c->mode != SL_CORE_MODE_8) goto undef;
        result.u8 = (u8)(i8)c->f[si->r0].f;
        break;

    case SLAC_FUNC_FCVT_F_TO_U8:
        if (c->mode != SL_CORE_MODE_8) goto undef;
        result.u8 = (u8)c->f[si->r0].f;
        break;

    case SLAC_FUNC_FLD: {
        set_opts = 0;
        u8 target = c->r[si->r0] + si->simm;
        if (c->mode == SL_CORE_MODE_4)
            target &= 0xffffffff;
        u4 val;
        int err = sl_core_mem_read_single(c, target, 4, &val);
        if (err)
            return sl_core_synchronous_exception(c, EX_ABORT_LOAD, target, err);
        c->f[si->d0].u4 = val;
        break;
    }

    case SLAC_FUNC_FST: {
        set_opts = 0;
        u8 target = c->r[si->r0] + si->simm;
        if (c->mode == SL_CORE_MODE_4)
            target &= 0xffffffff;
        u4 val = c->f[si->d0].u4;
        int err = sl_core_mem_write_single(c, target, 4, &val);
        if (err)
            return sl_core_synchronous_exception(c, EX_ABORT_STORE, target, err);
        break;
    }

    case SLAC_FUNC_FCLASS: {
        set_opts = 0;
        u1 type = 0;
        int cl = fpclassify(c->f[si->r0].f);
        switch (cl) {
        case FP_INFINITE:   type = 0;   break;
        case FP_NORMAL:     type = 1;   break;
        case FP_SUBNORMAL:  type = 2;   break;
        case FP_ZERO:       type = 3;   break;
        case FP_NAN:        type = (c->f[si->r0].f == NAN) ? 9 : 8;   break;
        }
        if ((type < 8) && (signbit(c->f[si->r0].f) == 0))
            type = 7 - type;
        c->r[si->d0] = (1u << type);
        break;
    }

    case SLAC_FUNC_FMADD:
        result.f = (c->f[si->r0].f * c->f[si->r1].f) + c->f[si->r2].f;
        break;

    case SLAC_FUNC_FMSUB:
        result.f = (c->f[si->r0].f * c->f[si->r1].f) - c->f[si->r2].f;
        break;

    case SLAC_FUNC_FNMADD:
        result.f = -(c->f[si->r0].f * c->f[si->r1].f) + c->f[si->r2].f;
        break;

    case SLAC_FUNC_FNMSUB:
        result.f = -(c->f[si->r0].f * c->f[si->r1].f) - c->f[si->r2].f;
        break;

    default:
        return SL_ERR_UNIMPLEMENTED;
    }

    if (set_opts & FP_SET_FLAGS) {
        fegetexceptflag(&flags, FE_ALL_EXCEPT);
        c->fexc |= flags;
    }
    if (set_opts & FP_SET_RESULT) {
        c->f[si->d0].u8 = result.u8;
    }
    return 0;

undef:
    return SL_ERR_UNDEF;
}

int slac_exec_fp64(sl_core_t *c, sl_slac_inst_t *si) {
    sl_fp_reg_t result = {};
    fexcept_t flags;
    u1 set_opts = FP_SET_RESULT | FP_SET_FLAGS;
    bool comp;

    feclearexcept(FE_ALL_EXCEPT);

    switch (si->func) {
    case SLAC_FUNC_FADD:
        result.d = c->f[si->r0].d + c->f[si->r1].d;
        break;

    case SLAC_FUNC_FSUB:
        result.d = c->f[si->r0].d - c->f[si->r1].d;
        break;

    case SLAC_FUNC_FMUL:
        result.d = c->f[si->r0].d * c->f[si->r1].d;
        break;

    case SLAC_FUNC_FDIV:
#if __x64_64__
        if (c->f[si->r1].d == 0)
            result.d = 0.0;
        else
#endif
        result.d = c->f[si->r0].d / c->f[si->r1].d;
        break;

    case SLAC_FUNC_FSQRT:
        result.d = sqrt(c->f[si->r0].d);
        break;

    case SLAC_FUNC_FMIN:
        result.d = fmin(c->f[si->r0].d, c->f[si->r1].d);
        break;

    case SLAC_FUNC_FMAX:
        result.d = fmax(c->f[si->r0].d, c->f[si->r1].d);
        break;

    case SLAC_FUNC_FEQ:
        set_opts = FP_SET_FLAGS;
        comp = c->f[si->r0].d == c->f[si->r1].d;
        if (si->d0 != SLAC_REG_DISCARD)
            c->r[si->d0] = comp;
        break;

    case SLAC_FUNC_FLT:
        set_opts = FP_SET_FLAGS;
        comp = isless(c->f[si->r0].d, c->f[si->r1].d);
        if (si->d0 != SLAC_REG_DISCARD)
            c->r[si->d0] = comp;
        break;

    case SLAC_FUNC_FLE:
        set_opts = FP_SET_FLAGS;
        comp = islessequal(c->f[si->r0].d, c->f[si->r1].d);
        if (si->d0 != SLAC_REG_DISCARD)
            c->r[si->d0] = comp;
        break;

    case SLAC_FUNC_FS:
        set_opts = FP_SET_RESULT;
        result.u8 = c->f[si->r0].u8 & ~F64_SIGN_BIT;
        result.u8 |= (c->f[si->r1].u8 & F64_SIGN_BIT);
        break;

    case SLAC_FUNC_FSN:
        set_opts = FP_SET_RESULT;
        result.u8 = c->f[si->r0].u8 & ~F64_SIGN_BIT;
        result.u8 |= ((~c->f[si->r1].u8) & F64_SIGN_BIT);
        break;

    case SLAC_FUNC_FSX:
        set_opts = FP_SET_RESULT;
        result.u8 = c->f[si->r0].u8 & ~F64_SIGN_BIT;
        result.u8 |= ((c->f[si->r0].u8 ^ c->f[si->r1].u8) & F64_SIGN_BIT);
        break;

    case SLAC_FUNC_FMOV_F_TO_R:
        set_opts = 0;
        if (c->mode == SL_CORE_MODE_4)
            c->r[si->d0] = c->f[si->r0].u4;
        else
            c->r[si->d0] = c->f[si->r0].u8;
        break;

    case SLAC_FUNC_FMOV_R_TO_F:
        set_opts = FP_SET_RESULT;
        result.u8 = c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_S4_TO_F:
        result.d = (double)(i8)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_U4_TO_F:
        result.d = (double)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_S8_TO_F:
        if (c->mode != SL_CORE_MODE_8) goto undef;
        result.d = (double)(i8)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_U8_TO_F:
        if (c->mode != SL_CORE_MODE_8) goto undef;
        // todo: check if sets flags
        result.d = (double)c->r[si->r0];
        break;

    case SLAC_FUNC_FCVT_F_TO_S4:
        set_opts = FP_SET_FLAGS;
        c->r[si->d0] = (i4)c->f[si->r0].d;
        break;

    case SLAC_FUNC_FCVT_F_TO_U4:
        set_opts = FP_SET_FLAGS;
        c->r[si->d0] = (u4)c->f[si->r0].d;
        break;

    case SLAC_FUNC_FCVT_F_TO_S8:
        if (c->mode != SL_CORE_MODE_8) goto undef;
        set_opts = FP_SET_FLAGS;
        c->r[si->d0] = (i8)c->f[si->r0].d;
        break;

    case SLAC_FUNC_FCVT_F_TO_U8:
        if (c->mode != SL_CORE_MODE_8) goto undef;
        set_opts = FP_SET_FLAGS;
        c->r[si->d0] = (u8)c->f[si->r0].d;
        break;

    case SLAC_FUNC_FCVT_F32_TO_F64:
        result.d = (double)c->f[si->r0].f;
        break;

    case SLAC_FUNC_FLD: {
        set_opts = 0;
        u8 target = c->r[si->r0] + si->simm;
        if (c->mode == SL_CORE_MODE_4)
            target &= 0xffffffff;
        u8 val;
        int err = sl_core_mem_read_single(c, target, 8, &val);
        if (err)
            return sl_core_synchronous_exception(c, EX_ABORT_LOAD, target, err);
        c->f[si->d0].u8 = val;
        break;
    }

    case SLAC_FUNC_FST: {
        set_opts = 0;
        u8 target = c->r[si->r0] + si->simm;
        if (c->mode == SL_CORE_MODE_4)
            target &= 0xffffffff;
        u8 val = c->f[si->d0].u8;
        int err = sl_core_mem_write_single(c, target, 8, &val);
        if (err)
            return sl_core_synchronous_exception(c, EX_ABORT_STORE, target, err);
        break;
    }

    case SLAC_FUNC_FCLASS: {
        set_opts = 0;
        u1 type = 0;
        int cl = fpclassify(c->f[si->r0].d);
        switch (cl) {
        case FP_INFINITE:   type = 0;   break;
        case FP_NORMAL:     type = 1;   break;
        case FP_SUBNORMAL:  type = 2;   break;
        case FP_ZERO:       type = 3;   break;
        case FP_NAN:        type = (c->f[si->r0].d == NAN) ? 9 : 8;   break;
        }
        if ((type < 8) && (signbit(c->f[si->r0].d) == 0))
            type = 7 - type;
        c->r[si->d0] = (1u << type);
        break;
    }

    case SLAC_FUNC_FMADD:
        result.d = (c->f[si->r0].d * c->f[si->r1].d) + c->f[si->r2].d;
        break;

    case SLAC_FUNC_FMSUB:
        result.d = (c->f[si->r0].d * c->f[si->r1].d) - c->f[si->r2].d;
        break;

    case SLAC_FUNC_FNMADD:
        result.d = -(c->f[si->r0].d * c->f[si->r1].d) + c->f[si->r2].d;
        break;

    case SLAC_FUNC_FNMSUB:
        result.d = -(c->f[si->r0].d * c->f[si->r1].d) - c->f[si->r2].d;
        break;

    default:
        return SL_ERR_UNIMPLEMENTED;
    }

    if (set_opts & FP_SET_FLAGS) {
        fegetexceptflag(&flags, FE_ALL_EXCEPT);
        c->fexc |= flags;
    }
    if (set_opts & FP_SET_RESULT) {
        c->f[si->d0].u8 = result.u8;
    }
    return 0;

undef:
    return SL_ERR_UNDEF;
}

