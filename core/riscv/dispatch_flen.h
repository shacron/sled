#pragma once

#include <stdint.h>
#include <math.h>

#include <core/riscv.h>
#include <core/riscv/dispatch.h>
#include <core/riscv/trace.h>
#include <sled/arch.h>
#include <sled/error.h>

#if USING_FP32
float typedef flen_t;
#define FLEN        32
#define FLEN_TYPE   float
#define FLEN_PREFIX(name) rv_fp32_ ## name
#define FLEN_REG(r) r.f
#define FF          f
#define FU          u4
#define FI          i4
#define FLEN_CHAR   's'
#define FLEN_S      "s"
#define FLEN_SIGN_BIT (1u << 31)
#define FLEN_SQRT   sqrtf
#define FLEN_FMIN   fminf
#define FLEN_FMAX   fmaxf
#define FLEN_NAN    0x7fc00000
#else
double typedef flen_t;
#define FLEN        64
#define FLEN_TYPE   double
#define FLEN_PREFIX(name) rv_fp64_ ## name
#define FLEN_REG(r) r.d
#define FF          d
#define FU          u8
#define FI          i8
#define FLEN_CHAR   'd'
#define FLEN_S      "d"
#define FLEN_SIGN_BIT (1ul << 63)
#define FLEN_SQRT   sqrt
#define FLEN_FMIN   fmin
#define FLEN_FMAX   fmax
#define FLEN_NAN    0x7ff8000000000000
#endif

int FLEN_PREFIX(exec_fp)(rv_core_t *c, rv_inst_t inst) {
    sl_fp_reg_t result = {};
    fexcept_t flags;
    const u1 rd = inst.r.rd;
    const u1 set_result = (1 << 0);
    const u1 set_flags = (1 << 1);
    const u1 set_result_and_flags = set_result | set_flags;

    u1 set_opts = set_result_and_flags;

    feclearexcept(FE_ALL_EXCEPT);

    u8 uval;
    switch (inst.r.funct7 >> 2) {
    // 0000000 rs2     rs1  rm   rd  1010011  FADD.S
    case 0b00000:
        result.FF = c->core.f[inst.r.rs1].FF + c->core.f[inst.r.rs2].FF;
        RV_TRACE_PRINT(c, "fadd." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
        break;

    // 0000100 rs2     rs1  rm   rd  1010011  FSUB.S
    case 0b00001:
        result.FF = c->core.f[inst.r.rs1].FF - c->core.f[inst.r.rs2].FF;
        RV_TRACE_PRINT(c, "fsub." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
        break;

    // 0001000 rs2     rs1  rm   rd  1010011  FMUL.S
    case 0b00010:
        result.FF = c->core.f[inst.r.rs1].FF * c->core.f[inst.r.rs2].FF;
        RV_TRACE_PRINT(c, "fmul." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
        break;

    // 0001100 rs2     rs1  rm   rd  1010011  FDIV.S
    case 0b00011:
#if __x64_64__
        if (c->core.f[inst.r.rs2].FF == 0) result.FF = 0.0;
        else
#endif
        result.FF = c->core.f[inst.r.rs1].FF / c->core.f[inst.r.rs2].FF;
        RV_TRACE_PRINT(c, "fdiv." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
        break;

    // 01011 size 00000   rs1  rm   rd  1010011  FSQRT.S
    case 0b01011:
        result.FF = FLEN_SQRT(c->core.f[inst.r.rs1].FF);
        RV_TRACE_PRINT(c, "fsqrt." FLEN_S " f%u, f%u", rd, inst.r.rs1);
        break;

    // 00100 size rs2     rs1  000  rd  1010011  FSGNJ.S
    // 00100 size rs2     rs1  001  rd  1010011  FSGNJN.S
    // 00100 size rs2     rs1  010  rd  1010011  FSGNJX.S
    case 0b00100:
        set_opts = set_result;
        result.FU = c->core.f[inst.r.rs1].FU & ~FLEN_SIGN_BIT;
        switch (inst.r.funct3) {
        case 0b000:
            result.FU |= (c->core.f[inst.r.rs2].FU & FLEN_SIGN_BIT);
            RV_TRACE_PRINT(c, "fsgnj." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        case 0b001:
            result.FU |= ((~c->core.f[inst.r.rs2].FU) & FLEN_SIGN_BIT);
            RV_TRACE_PRINT(c, "fsgnjn." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        case 0b010:
            result.FU |= ((c->core.f[inst.r.rs1].FU ^ c->core.f[inst.r.rs2].FU) & FLEN_SIGN_BIT);
            RV_TRACE_PRINT(c, "fsgnjx." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        default:    goto undef;
        }
        break;

    // 00101 size rs2     rs1  000  rd  1010011  FMIN.S
    // 00101 size rs2     rs1  001  rd  1010011  FMAX.S
    case 0b00101:
        switch (inst.r.funct3) {
        case 0b000:
            result.FF = FLEN_FMIN(c->core.f[inst.r.rs1].FF, c->core.f[inst.r.rs2].FF);
            RV_TRACE_PRINT(c, "fmin." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        case 0b001:
            result.FF = FLEN_FMAX(c->core.f[inst.r.rs1].FF, c->core.f[inst.r.rs2].FF);
            RV_TRACE_PRINT(c, "fmaxf." FLEN_S " f%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        default:    goto undef;
        }
        break;

    case 0b11000:
        set_opts = set_flags;
        // todo: set correct rounding mode
        switch (inst.r.funct3) {
        // 11000 size 00000   rs1  rm   rd  1010011  FCVT.W.S/D
        case 0b00000:
            uval = (u4)(FI)c->core.f[inst.r.rs1].FF;
            RV_TRACE_PRINT(c, "fcvt.w." FLEN_S " x%u, f%u", rd, inst.r.rs1);
            break;

        // 11000 size 00001   rs1  rm   rd  1010011  FCVT.WU.S
        case 0b00001:
            uval = (u4)c->core.f[inst.r.rs1].FF;
            RV_TRACE_PRINT(c, "fcvt.wu." FLEN_S " x%u, f%u", rd, inst.r.rs1);
            break;

        // 11000 size 00010   rs1  rm   rd  1010011  FCVT.L.S/D
        case 0b00010:
            if (c->core.mode != SL_CORE_MODE_64) goto undef;
            uval = (u8)(i8)c->core.f[inst.r.rs1].FF;
            RV_TRACE_PRINT(c, "fcvt.l." FLEN_S " x%u, f%u", rd, inst.r.rs1);
            break;

        // 11000 size 00011   rs1  rm   rd  1010011  FCVT.LU.S/D
        case 0b11000:
            if (c->core.mode != SL_CORE_MODE_64) goto undef;
            uval = (u8)c->core.f[inst.r.rs1].FF;
            RV_TRACE_PRINT(c, "fcvt.lu." FLEN_S " x%u, f%u", rd, inst.r.rs1);
            break;

        default:    goto undef;
        }
        if (rd != RV_ZERO) {
            c->core.r[rd] = uval;
            RV_TRACE_RD(c, rd, c->core.r[rd]);
        }
        break;

    // 10100 size rs2     rs1  010  rd  1010011  FEQ.S
    // 10100 size rs2     rs1  001  rd  1010011  FLT.S
    // 10100 size rs2     rs1  000  rd  1010011  FLE.S
    case 0b10100:
        switch (inst.r.funct3) {
        case 0b010:
            uval = c->core.f[inst.r.rs1].FF == c->core.f[inst.r.rs2].FF;
            RV_TRACE_PRINT(c, "feq." FLEN_S " x%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        case 0b001:
            uval = isless(c->core.f[inst.r.rs1].FF, c->core.f[inst.r.rs2].FF);
            RV_TRACE_PRINT(c, "flt." FLEN_S " x%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        case 0b000:
            uval = islessequal(c->core.f[inst.r.rs1].FF, c->core.f[inst.r.rs2].FF);
            RV_TRACE_PRINT(c, "fle." FLEN_S " x%u, f%u, f%u", rd, inst.r.rs1, inst.r.rs2);
            break;

        default:    goto undef;
        }
        if (rd != RV_ZERO) {
            c->core.r[rd] = uval ? 1 : 0;
            RV_TRACE_RD(c, rd, c->core.r[rd]);
        }
        set_opts = set_flags;
        break;

    case 0b11100:
        set_opts = 0;
        switch (inst.r.funct3) {
        case 0b000: {
#if USING_FP32
            // 11100 size 00000   rs1  000  rd  1010011  FMV.X.W
            RV_TRACE_PRINT(c, "fmv.x.w x%u, f%u", rd, inst.r.rs1);
            if (rd == RV_ZERO) break;
            if (c->core.mode == SL_CORE_MODE_32) c->core.r[rd] = c->core.f[inst.r.rs1].u4;
            else c->core.r[rd] = (i8)(i4)c->core.f[inst.r.rs1].u4;
#else
            // 11100 size 00000   rs1  000  rd  1010011  FMV.X.D
            if (c->core.mode != SL_CORE_MODE_64) goto undef;
            RV_TRACE_PRINT(c, "fmv.x.d x%u, f%u", rd, inst.r.rs1);
            if (rd == RV_ZERO) break;
            c->core.r[rd] = c->core.f[inst.r.rs1].u8;
#endif
            RV_TRACE_RD(c, rd, c->core.r[rd]);
            break;
        }

        // 11100 size 00000   rs1  001  rd  1010011  FCLASS.S/D
        case 0b001: {
            RV_TRACE_PRINT(c, "fclass." FLEN_S " x%u, f%u", rd, inst.r.rs1);
            if (rd == RV_ZERO) break;
            u1 type = 0;
            int cl = fpclassify(c->core.f[inst.r.rs1].FF);
            switch (cl) {
            case FP_INFINITE:   type = 0;   break;
            case FP_NORMAL:     type = 1;   break;
            case FP_SUBNORMAL:  type = 2;   break;
            case FP_ZERO:       type = 3;   break;
            case FP_NAN:        type = (c->core.f[inst.r.rs1].FF == NAN) ? 9 : 8;   break;
            }
            if ((type < 8) && (signbit(c->core.f[inst.r.rs1].FF) == 0)) type = 7 - type;
            c->core.r[rd] = (1u << type);
            RV_TRACE_RD(c, rd, c->core.r[rd]);
            break;
        }
        default:    goto undef;
        }
        break;

    case 0b11010:
        switch (inst.r.rs2) {
        // 11010 00 00000   rs1  rm   rd  1010011  FCVT.S.W
        // 11010 01 00000   rs1  rm   rd  1010011  FCVT.D.W
        case 0b00000:
            result.FF = (FLEN_TYPE)(i4)c->core.r[inst.r.rs1];
            RV_TRACE_PRINT(c, "fcvt." FLEN_S ".w f%u, x%u", rd, inst.r.rs1);
            break;

        // 11010 size 00001   rs1  rm   rd  1010011  FCVT.S.WU
        // 11010 size 00001   rs1  rm   rd  1010011  FCVT.D.WU
        case 0b00001:
            result.FF = (FLEN_TYPE)(u4)c->core.r[inst.r.rs1];
            RV_TRACE_PRINT(c, "fcvt." FLEN_S ".wu f%u, x%u", rd, inst.r.rs1);
            break;

        // 11010 size 00010   rs1  rm   rd  1010011  FCVT.S.L
        case 0b00010:
            if (c->core.mode != SL_CORE_MODE_64) goto undef;
            result.FF = (FLEN_TYPE)(i8)c->core.r[inst.r.rs1];
            RV_TRACE_PRINT(c, "fcvt." FLEN_S ".l f%u, x%u", rd, inst.r.rs1);
            break;

        // 11010 size 00011   rs1  rm   rd  1010011  FCVT.S.LU
        case 0b00011:
            if (c->core.mode != SL_CORE_MODE_64) goto undef;
            result.FF = (FLEN_TYPE)c->core.r[inst.r.rs1];
            RV_TRACE_PRINT(c, "fcvt." FLEN_S ".lu f%u, x%u", rd, inst.r.rs1);
            break;

        default:    goto undef;
        }
        break;

    case 0b01000: {
        // the following instructions abuse the size field. Both require the D ext.
#if USING_FP32 // i.e. size = 0
        if ((c->core.arch_options & SL_RISCV_EXT_D) == 0) goto undef;
        // 01000 00 00001 rs1 rm rd 1010011 FCVT.S.D
        if (inst.r.rs2 != 1) goto undef;
        result.f = c->core.f[inst.r.rs1].d;
        RV_TRACE_PRINT(c, "fcvt.s.d f%u, f%u", rd, inst.r.rs1);
        set_opts = set_result_and_flags;
#else
        // 01000 01 00000 rs1 rm rd 1010011 FCVT.D.S
        if (inst.r.rs2 != 0) goto undef;
        result.d = c->core.f[inst.r.rs1].f;
        RV_TRACE_PRINT(c, "fcvt.d.s f%u, f%u", rd, inst.r.rs1);
        set_opts = set_result;
#endif
        break;
    }

    case 0b11110:
        if (inst.r.funct3 != 000) goto undef;
#if USING_FP32
        // 11110 size 00000   rs1  000  rd  1010011  FMV.W.X
        result.u4 = (u4)c->core.r[inst.r.rs1];
        RV_TRACE_PRINT(c, "fmv.w.x f%u, x%u", rd, inst.r.rs1);
#else
        // 11110 size 00000   rs1  000  rd  1010011  FMV.D.X
        if (c->core.mode != SL_CORE_MODE_64) goto undef;
        result.u8 = c->core.r[inst.r.rs1];
        RV_TRACE_PRINT(c, "fmv.d.x f%u, x%u", rd, inst.r.rs1);
#endif
        set_opts = set_result;
        break;

    default: goto undef;
    }

    if (set_opts & set_flags) {
        fegetexceptflag(&flags, FE_ALL_EXCEPT);
        c->core.fexc |= flags;
    }
    if (set_opts & set_result) {
        c->core.f[rd].u8 = result.u8;
        RV_TRACE_RDF(c, rd, c->core.f[rd].FF);
    }
    return 0;

undef:
    return rv_undef(c, inst);
}
