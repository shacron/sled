// SPDX-License-Identifier: MIT License
// Copyright (c) 2024-2026 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SLAC_REG_DISCARD    0xff

#define SLAC_OP(type, func) (((type) << 10) | (func))
#define SLAC_TYPE(op) (op >> 10)
#define SLAC_FUNC(op) (op & 0x3ff)

// Mathmatical ops are unsigned by default

// condition flags
#define SLAC_FLAG_N         1   // negative
#define SLAC_FLAG_Z         2   // zero
#define SLAC_FLAG_C         4   // carry (unsigned overflow)
#define SLAC_FLAG_V         8   // overflow (signed)

#define SLAC_PRED_ALWAYS    0  // always
#define SLAC_PRED_EQ        1  // Z = 0 equal
#define SLAC_PREG_NE        2  // Z = 1 not equal
#define SLAC_PRED_HS        3  // C = 0 unsigned higher same (carry set)
#define SLAC_PRED_LO        4  // C = 1 unsigned lower (carry clear)
#define SLAC_PRED_POS       5  // N = 0 positive
#define SLAC_PRED_NEG       6  // N = 1 negative
#define SLAC_PRED_NOF       7  // V = 0 no signed overflow
#define SLAC_PRED_OF        8  // V = 1 signed overflow
#define SLAC_PRED_HI        9  // c = 1 & z = 0 unsigned higher
#define SLAC_PRED_LS        10  // c = 0 & z = 1 unsigned lower same
#define SLAC_PRED_GE        11  // n = v signed greater or equal
#define SLAC_PRED_LT        12  // n != v signed less than
#define SLAC_PRED_GT        13  // z = 0 & n = v signed greater than
#define SLAC_PRED_LE        14  // z = 1 | n != v signed less than or equal
#define SLAC_PRED_NEVER     15  // never

#define SLAC_TYPE_ALU        0  // arithmetic, logic
#define SLAC_TYPE_LD         1  // load
#define SLAC_TYPE_ST         2  // store
#define SLAC_TYPE_BR         3  // branch
#define SLAC_TYPE_FP32       4  // 32-bit floating point
#define SLAC_TYPE_FP64       5  // 64-bit floating point
#define SLAC_TYPE_VEC        6  // vector
#define SLAC_TYPE_SIMD       7  // simd
#define SLAC_TYPE_ATOMIC     8  // atomic
#define SLAC_TYPE_SYS        9  // system and control

#define SLAC_IN_LEN_1           0 // 1 byte
#define SLAC_IN_LEN_2           1 // 2 bytes
#define SLAC_IN_LEN_4           2 // 4 bytes
#define SLAC_IN_LEN_8           3 // 8 bytes
#define SLAC_IN_LEN_16          4 // 16 bytes
#define SLAC_IN_LEN_MODE        5 // current machine mode

#define SLAC_IN_ARG_NONE        (0)
#define SLAC_IN_ARG_D           (1u << 0)   // output to register
#define SLAC_IN_ARG_I           (1u << 1)   // use immediate value
#define SLAC_IN_ARG_R0          (0)         // no source registers
#define SLAC_IN_ARG_R1          (1u << 2)   // one source register
#define SLAC_IN_ARG_R2          (2u << 2)   // two source registers
#define SLAC_IN_ARG_R3          (3u << 2)   // three source registers
#define SLAC_IN_ARG_REG_MASK    (3u << 2)   // mask of register count
#define SLAC_IN_ARG_CC          (1u << 3)   // set condition code

#define SLAC_IN_ARG_DR          (SLAC_IN_ARG_D | SLAC_IN_ARG_R1)
#define SLAC_IN_ARG_DI          (SLAC_IN_ARG_D | SLAC_IN_ARG_I)
#define SLAC_IN_ARG_DRI         (SLAC_IN_ARG_D | SLAC_IN_ARG_R1 | SLAC_IN_ARG_I)
#define SLAC_IN_ARG_DRR         (SLAC_IN_ARG_D | SLAC_IN_ARG_R2)
#define SLAC_IN_ARG_RI          (SLAC_IN_ARG_R1 | SLAC_IN_ARG_I)
#define SLAC_IN_ARG_RRI         (SLAC_IN_ARG_R2 | SLAC_IN_ARG_I)

// alu
#define SLAC_FUNC_ADD          0x000
#define SLAC_FUNC_SUB          0x001
#define SLAC_FUNC_RSUB         0x002   // reverse subtract
#define SLAC_FUNC_AND          0x003
#define SLAC_FUNC_OR           0x004
#define SLAC_FUNC_XOR          0x005
#define SLAC_FUNC_NOT          0x006
#define SLAC_FUNC_SHL          0x007   // shift left
#define SLAC_FUNC_SHRS         0x008   // shift right signed
#define SLAC_FUNC_SHR          0x009   // shift right unsigned
#define SLAC_FUNC_CSELS        0x00a   // compare and select (signed)
#define SLAC_FUNC_CSEL         0x00b   // compare and select (unsigned)
#define SLAC_FUNC_MUL          0x00c
#define SLAC_FUNC_MULHSS       0x00d   // multiply high (signed * signed)
#define SLAC_FUNC_MULHSU       0x00e   // multiply high (signed * unsigned)
#define SLAC_FUNC_MULHUU       0x00f   // multiple high (unsigned * unsigned)
#define SLAC_FUNC_DIVS         0x010   // divide (signed * signed)
#define SLAC_FUNC_DIV          0x011   // divide (unsigned * unsigned)
#define SLAC_FUNC_MODS         0x012   // mod (signed * signed)
#define SLAC_FUNC_MOD          0x013   // mod (unsigned * unsigned)


// load store
#define SLAC_FUNC_LD1          0x000   // load byte
#define SLAC_FUNC_LD1S         0x001   // load byte (sign extend)
#define SLAC_FUNC_LD2          0x002   // load half
#define SLAC_FUNC_LD2S         0x003   // load half (sign extend)
#define SLAC_FUNC_LD4          0x004   // load word
#define SLAC_FUNC_LD4S         0x005   // load word (sign extend)
#define SLAC_FUNC_LD8          0x006   // load long
// #define SLAC_FUNC_LD8S         0x003   // load long (sign extend)

#define SLAC_FUNC_ST1          0x005   // store byte
#define SLAC_FUNC_ST2          0x006   // store half
#define SLAC_FUNC_ST4          0x007   // store word
#define SLAC_FUNC_ST8          0x008   // store long
// #define SLAC_FUNC_LDP          0x00a   // load pair
// #define SLAC_FUNC_STP          0x00b   // store pair
// #define SLAC_FUNC_LDM          0x00c   // load multiple
// #define SLAC_FUNC_STM          0x00d   // store multiple

// branch
#define SLAC_FUNC_B            0x000   // branch
#define SLAC_FUNC_BL           0x001   // branch and link
#define SLAC_FUNC_CBEQ         0x002   // compare and branch equal
#define SLAC_FUNC_CBNE         0x003   // compare and branch not equal
#define SLAC_FUNC_CBLTU        0x004   // compare and branch less than (unsigned)
#define SLAC_FUNC_CBLTS        0x005   // compare and branch less than (signed)
#define SLAC_FUNC_CBGEU        0x006   // compare and branch less than (unsigned)
#define SLAC_FUNC_CBGES        0x007   // compare and branch less than (signed)
// #define SLAC_FUNC_TB           0x008   // test and branch

// fp - these are shared for fp32 and fp64

#define SLAC_FUNC_FADD          0x000
#define SLAC_FUNC_FSUB          0x001
#define SLAC_FUNC_FMUL          0x002
#define SLAC_FUNC_FDIV          0x003
#define SLAC_FUNC_FSQRT         0x004
#define SLAC_FUNC_FMIN          0x005
#define SLAC_FUNC_FMAX          0x006
#define SLAC_FUNC_FEQ           0x007   // set register if equal
#define SLAC_FUNC_FLT           0x008   // set register if less than
#define SLAC_FUNC_FLE           0x009   // set register if less than or equal
#define SLAC_FUNC_FS            0x00a   // set sign from source
#define SLAC_FUNC_FSN           0x00b   // set not-sign from source
#define SLAC_FUNC_FSX           0x00c   // set xor of signs from sources
#define SLAC_FUNC_FCVT_S4_TO_F  0x00d    // convert from signed int4 to float
#define SLAC_FUNC_FCVT_U4_TO_F  0x00e   // convert from unsigned int4 to float
#define SLAC_FUNC_FCVT_S8_TO_F  0x00f   // convert from signed int8 to float
#define SLAC_FUNC_FCVT_U8_TO_F  0x010   // convert from unsigned int8 to float
#define SLAC_FUNC_FCVT_F_TO_S4  0x011   // convert from float to signed int4
#define SLAC_FUNC_FCVT_F_TO_U4  0x012   // convert from float to unsigned int4
#define SLAC_FUNC_FCVT_F_TO_S8  0x013   // convert from float to signed int8
#define SLAC_FUNC_FCVT_F_TO_U8  0x014   // convert from float to unsigned int8
#define SLAC_FUNC_FCVT_F32_TO_F64 0x015 // convert from float to double
#define SLAC_FUNC_FCLASS        0x016   // classify float type

#define SLAC_FUNC_FMADD         0x017   // multiply and add
#define SLAC_FUNC_FMSUB         0x018   // multiply and subtract
#define SLAC_FUNC_FNMADD        0x019   // multiply, negate, and add
#define SLAC_FUNC_FNMSUB        0x01a   // multiply, negate, and subtract

#define SLAC_FUNC_FMOV_F_TO_R   0x01b   // move from float to register
#define SLAC_FUNC_FMOV_R_TO_F   0x01c   // move from register to float

#define SLAC_FUNC_FLD           0x01d   // load float
#define SLAC_FUNC_FST           0x01e   // store float

// vec

// simd

// atomic
#define SLAC_FUNC_LX           0x000   // load exclusive
#define SLAC_FUNC_SX           0x001   // store exclusive
#define SLAC_FUNC_SWP          0x002   // atomic swap
#define SLAC_FUNC_CAS          0x003   // atomic compare and swap
#define SLAC_FUNC_ATADD        0x004   // atomic add
#define SLAC_FUNC_ATAND        0x005   // atomic and
#define SLAC_FUNC_ATOR         0x006   // atomic or
#define SLAC_FUNC_ATXOR        0x007   // atomic xor
#define SLAC_FUNC_ATMINU       0x008   // atomic min unsigned
#define SLAC_FUNC_ATMINS       0x009   // atomic min signed
#define SLAC_FUNC_ATMAXU       0x00a   // atomic max unsigned
#define SLAC_FUNC_ATMAXS       0x00b   // atomic max signed

// sys
#define SLAC_FUNC_MOVR         0x000   // move register to register
#define SLAC_FUNC_MOVI         0x001   // move immediate to register
#define SLAC_FUNC_ADR4K        0x002   // generate pc relative address at 4K alignment
#define SLAC_FUNC_MBAR         0x003   // memory barrier
#define SLAC_FUNC_IBAR         0x004   // instruction barrier
#define SLAC_FUNC_BREAK        0x005   // breakpoint

#define SLAC_FUNC_CSRRD        0x006   // csr read
#define SLAC_FUNC_CSRWR        0x007   // csr write
#define SLAC_FUNC_CSRSWP       0x008   // csr swap
#define SLAC_FUNC_CSROR        0x009   // csr or
#define SLAC_FUNC_CSRCLR       0x00a   // csr clear

#define SLAC_FUNC_NOP          0x3fd   // nop
#define SLAC_FUNC_UNDEF        0x3fe   // undefined instruction
#define SLAC_FUNC_INVALID      0x3ff   // invalid instruction

#define SLAC_IN_INVALID        0xffffffff


// Op codes
#define SLAC_OP_ADD     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_ADD)
#define SLAC_OP_SUB     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_SUB)
#define SLAC_OP_RSUB    SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_RSUB)
#define SLAC_OP_AND     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_AND)
#define SLAC_OP_OR      SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_OR)
#define SLAC_OP_XOR     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_XOR)
#define SLAC_OP_NOT     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_NOT)
#define SLAC_OP_SHL     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_SHL)
#define SLAC_OP_SHRS    SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_SHRS)
#define SLAC_OP_SHR     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_SHR)
#define SLAC_OP_CSELS   SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_CSELS)
#define SLAC_OP_CSEL    SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_CSEL)
#define SLAC_OP_MUL     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_MUL)
#define SLAC_OP_MULHSS  SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_MULHSS)
#define SLAC_OP_MULHSU  SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_MULHSU)
#define SLAC_OP_MULHUU  SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_MULHUU)
#define SLAC_OP_DIVS    SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_DIVS)
#define SLAC_OP_DIV     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_DIV)
#define SLAC_OP_MODS    SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_MODS)
#define SLAC_OP_MOD     SLAC_OP(SLAC_TYPE_ALU, SLAC_FUNC_MOD)

#define SLAC_OP_LD1     SLAC_OP(SLAC_TYPE_LD, SLAC_FUNC_LD1) // load byte
#define SLAC_OP_LD1S    SLAC_OP(SLAC_TYPE_LD, SLAC_FUNC_LD1S) // load byte (sign extend)
#define SLAC_OP_LD2     SLAC_OP(SLAC_TYPE_LD, SLAC_FUNC_LD2)  // load half
#define SLAC_OP_LD2S    SLAC_OP(SLAC_TYPE_LD, SLAC_FUNC_LD2S) // load half (sign extend)
#define SLAC_OP_LD4     SLAC_OP(SLAC_TYPE_LD, SLAC_FUNC_LD4)  // load word
#define SLAC_OP_LD4S    SLAC_OP(SLAC_TYPE_LD, SLAC_FUNC_LD4S) // load word (sign extend)
#define SLAC_OP_LD8     SLAC_OP(SLAC_TYPE_LD, SLAC_FUNC_LD8)  // load long

#define SLAC_OP_ST1     SLAC_OP(SLAC_TYPE_ST, SLAC_FUNC_ST1) // store byte
#define SLAC_OP_ST2     SLAC_OP(SLAC_TYPE_ST, SLAC_FUNC_ST2) // store half
#define SLAC_OP_ST4     SLAC_OP(SLAC_TYPE_ST, SLAC_FUNC_ST4) // store word
#define SLAC_OP_ST8     SLAC_OP(SLAC_TYPE_ST, SLAC_FUNC_ST8) // store long

#define SLAC_OP_B       SLAC_OP(SLAC_TYPE_BR, SLAC_FUNC_B)       // branch
#define SLAC_OP_BL      SLAC_OP(SLAC_TYPE_BR, SLAC_FUNC_BL)      // branch and link
#define SLAC_OP_CBEQ    SLAC_OP(SLAC_TYPE_BR, SLAC_FUNC_CBEQ)    // compare and branch equal
#define SLAC_OP_CBNE    SLAC_OP(SLAC_TYPE_BR, SLAC_FUNC_CBNE)    // compare and branch not equal
#define SLAC_OP_CBLTU   SLAC_OP(SLAC_TYPE_BR, SLAC_FUNC_CBLTU)   // compare and branch less than (unsigned)
#define SLAC_OP_CBLTS   SLAC_OP(SLAC_TYPE_BR, SLAC_FUNC_CBLTS)   // compare and branch less than (signed)
#define SLAC_OP_CBGEU   SLAC_OP(SLAC_TYPE_BR, SLAC_FUNC_CBGEU)   // compare and branch less than (unsigned)
#define SLAC_OP_CBGES   SLAC_OP(SLAC_TYPE_BR, SLAC_FUNC_CBGES)   // compare and branch less than (signed)


// fp32
#define SLAC_OP_FADD32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FADD)
#define SLAC_OP_FSUB32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FSUB)
#define SLAC_OP_FMUL32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FMUL)
#define SLAC_OP_FDIV32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FDIV)
#define SLAC_OP_FSQRT32     SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FSQRT)
#define SLAC_OP_FMIN32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FMIN)
#define SLAC_OP_FMAX32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FMAX)
#define SLAC_OP_FEQ32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FEQ)
#define SLAC_OP_FLT32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FLT)
#define SLAC_OP_FLE32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FLE)
#define SLAC_OP_FS32        SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FS)
#define SLAC_OP_FSN32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FSN)
#define SLAC_OP_FSX32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FSX)
#define SLAC_OP_FCVT_S4_TO_F32  SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCVT_S4_TO_F)
#define SLAC_OP_FCVT_U4_TO_F32  SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCVT_U4_TO_F)
#define SLAC_OP_FCVT_S8_TO_F32  SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCVT_S8_TO_F)
#define SLAC_OP_FCVT_U8_TO_F32  SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCVT_U8_TO_F)
#define SLAC_OP_FCVT_F32_TO_S4  SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCVT_F_TO_S4)
#define SLAC_OP_FCVT_F32_TO_U4  SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCVT_F_TO_U4)
#define SLAC_OP_FCVT_F32_TO_S8  SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCVT_F_TO_S8)
#define SLAC_OP_FCVT_F32_TO_U8  SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCVT_F_TO_U8)
#define SLAC_OP_FCLASS_F32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FCLASS)
#define SLAC_OP_FMADD_F32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FMADD)
#define SLAC_OP_FMSUB_F32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FMSUB)
#define SLAC_OP_FNMADD_F32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FNMADD)
#define SLAC_OP_FNMSUB_F32      SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FNMSUB)

#define SLAC_OP_MOV_RF32    SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FMOV_F_TO_R)
#define SLAC_OP_MOV_FR32    SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FMOV_R_TO_F)
#define SLAC_OP_FLD32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FLD)
#define SLAC_OP_FST32       SLAC_OP(SLAC_TYPE_FP32, SLAC_FUNC_FST)


// fp64
#define SLAC_OP_FADD64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FADD)
#define SLAC_OP_FSUB64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FSUB)
#define SLAC_OP_FMUL64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FMUL)
#define SLAC_OP_FDIV64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FDIV)
#define SLAC_OP_FSQRT64     SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FSQRT)
#define SLAC_OP_FMIN64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FMIN)
#define SLAC_OP_FMAX64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FMAX)
#define SLAC_OP_FEQ64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FEQ)
#define SLAC_OP_FLT64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FLT)
#define SLAC_OP_FLE64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FLE)
#define SLAC_OP_FS64        SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FS)
#define SLAC_OP_FSN64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FSN)
#define SLAC_OP_FSX64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FSX)
#define SLAC_OP_FCVT_S4_TO_F64  SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_S4_TO_F)
#define SLAC_OP_FCVT_U4_TO_F64  SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_U4_TO_F)
#define SLAC_OP_FCVT_S8_TO_F64  SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_S8_TO_F)
#define SLAC_OP_FCVT_U8_TO_F64  SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_U8_TO_F)
#define SLAC_OP_FCVT_F64_TO_S4  SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_F_TO_S4)
#define SLAC_OP_FCVT_F64_TO_U4  SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_F_TO_U4)
#define SLAC_OP_FCVT_F64_TO_S8  SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_F_TO_S8)
#define SLAC_OP_FCVT_F64_TO_U8  SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_F_TO_U8)
#define SLAC_OP_FCVT_F32_TO_F64 SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCVT_F32_TO_F64)
#define SLAC_OP_FCLASS_F64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FCLASS)
#define SLAC_OP_FMADD_F64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FMADD)
#define SLAC_OP_FMSUB_F64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FMSUB)
#define SLAC_OP_FNMADD_F64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FNMADD)
#define SLAC_OP_FNMSUB_F64      SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FNMSUB)

#define SLAC_OP_MOV_RF64    SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FMOV_F_TO_R)
#define SLAC_OP_MOV_FR64    SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FMOV_R_TO_F)
#define SLAC_OP_FLD64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FLD)
#define SLAC_OP_FST64       SLAC_OP(SLAC_TYPE_FP64, SLAC_FUNC_FST)


// #define SLAC_TYPE_SYS        8  // system and control

#define SLAC_OP_LX      SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_LX)      // load exclusive
#define SLAC_OP_SX      SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_SX)      // store exclusive
#define SLAC_OP_SWP     SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_SWP)     // atomic swap
#define SLAC_OP_CAS     SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_CAS)     // atomic compare and swap
#define SLAC_OP_ATADD   SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_ATADD)   // atomic add
#define SLAC_OP_ATAND   SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_ATAND)   // atomic and
#define SLAC_OP_ATOR    SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_ATOR)    // atomic or
#define SLAC_OP_ATXOR   SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_ATXOR)   // atomic xor
#define SLAC_OP_ATMINU  SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_ATMINU)  // atomic min unsigned
#define SLAC_OP_ATMINS  SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_ATMINS)  // atomic min signed
#define SLAC_OP_ATMAXU  SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_ATMAXU)  // atomic max unsigned
#define SLAC_OP_ATMAXS  SLAC_OP(SLAC_TYPE_ATOMIC, SLAC_FUNC_ATMAXS)  // atomic max signed

#define SLAC_OP_MOVR     SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_MOVR)      // mov register
#define SLAC_OP_MOVI     SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_MOVI)      // mov immediate
#define SLAC_OP_ADR4K   SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_ADR4K)      // generate pc relative address at 4K alignment
#define SLAC_OP_MBAR    SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_MBAR)       // memory barrier
#define SLAC_OP_IBAR    SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_IBAR)       // instruction barrier
#define SLAC_OP_BREAK   SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_BREAK)      // breakpoint

#define SLAC_OP_CSRRD  SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_CSRRD)       // csr read
#define SLAC_OP_CSRWR  SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_CSRWR)       // csr write
#define SLAC_OP_CSRSWP  SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_CSRSWP)     // csr swap
#define SLAC_OP_CSROR   SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_CSROR)      // csr or
#define SLAC_OP_CSRCLR  SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_CSRCLR)     // csr clear

#define SLAC_OP_NOP     SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_NOP)        // nop
#define SLAC_OP_UNDEF   SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_UNDEF)      // undefined instruction
#define SLAC_OP_INVALID SLAC_OP(SLAC_TYPE_SYS, SLAC_FUNC_INVALID)    // invalid instruction

int slac4_dispatch(sl_core_t *c, sl_slac_inst_t *si);
int slac8_dispatch(sl_core_t *c, sl_slac_inst_t *si);

#if SLAC_TRACE

#define SLAC_BUF_LEN    80

#define STRACE_DECL_OPSTR const char *opstr_
#define STRACE_OPSTR(s) do { opstr_ = s; } while (0)
#define STRACE_FORMAT(si, f) si->desc.print_format = f
#define STRACE(si, ...) \
    do { \
        si->desc.len += snprintf(si->desc.s + si->desc.len, SLAC_BUF_LEN - si->desc.len, __VA_ARGS__); \
    } while (0)

#else

#define STRACE_DECL_OPSTR
#define STRACE_OPSTR(s)
#define STRACE_FORMAT(si, f)
#define STRACE(si, ...)

#endif

struct sl_slac_desc {
    u4 machine_op;      // native instruction encoding

#if SLAC_TRACE
    u4 print_format;
    int len;
    char s[80];
#endif
};

struct sl_slac_inst {
    union {
        u4 raw;
        struct {
            u4 type   : 6;  // type of instruction
            u4 func   : 10; // function

            u4 arg    : 5;  // arguments used (should be implicit in op)
            u4 len    : 3;  // length of operands
            u4 sx8    : 1;  // sign extend to u8
            u4 sh     : 1;  // short instruction encoding (16 bit)
            u4 _unused: 6;  //
        };
    };

    u1 d0;
    u1 r0;
    u1 r1;
    u1 r2;          // for jump and link instructions, contains pc offset

    union {
        u8 uimm;
        i8 simm;
    };

    sl_slac_desc_t desc;
};

#ifdef __cplusplus
}
#endif
