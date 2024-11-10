// SPDX-License-Identifier: MIT License
// Copyright (c) 2024 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SLAC_REG_DISCARD    0xff

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
#define SLAC_PRED_POS       5  // N = 0
#define SLAC_PRED_NEG       6  // N = 1
#define SLAC_PRED_NOF       7  // V = 0 no signed overflow
#define SLAC_PRED_OF        8  // V = 1 signed overflow
#define SLAC_PRED_HI        9  // c = 1 & z = 0 unsigned higher
#define SLAC_PRED_LS        10  // c = 0 & z = 1 unsigned lower same
#define SLAC_PRED_GE        11  // n = v signed greater or equal
#define SLAC_PRED_LT        12  // n != v signed less than
#define SLAC_PRED_GT        13  // z = 0 & n = v signed greater than
#define SLAC_PRED_LE        14  // z = 1 | n != v signed less than or equal
#define SLAC_PRED_NEVER     15  // never

#define SLAC_IN_TYPE_ALU        0  // arithmetic, logic
#define SLAC_IN_TYPE_LD         1  // load
#define SLAC_IN_TYPE_ST         2  // store
#define SLAC_IN_TYPE_BR         3  // branch
#define SLAC_IN_TYPE_FP         4  // floating point
#define SLAC_IN_TYPE_VEC        5  // vector
#define SLAC_IN_TYPE_SIMD       6  // simd
#define SLAC_IN_TYPE_ATOMIC     7  // atomic
#define SLAC_IN_TYPE_SYS        8  // system and control

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
#define SLAC_IN_OP_MOV          0x000
#define SLAC_IN_OP_ADD          0x001
#define SLAC_IN_OP_SUB          0x002
#define SLAC_IN_OP_RSUB         0x003
#define SLAC_IN_OP_AND          0x004
#define SLAC_IN_OP_OR           0x005
#define SLAC_IN_OP_XOR          0x006
#define SLAC_IN_OP_NOT          0x007
#define SLAC_IN_OP_SHL          0x008   // shift left
#define SLAC_IN_OP_SHRU         0x009   // shift right unsigned
#define SLAC_IN_OP_SHRS         0x00a   // shift right signed
#define SLAC_IN_OP_CSELS        0x00b   // compare and select (signed)
#define SLAC_IN_OP_CSELU        0x00c   // compare and select (unsigned)
#define SLAC_IN_OP_MUL          0x00d
#define SLAC_IN_OP_MULHSS       0x00e   // multiply high (signed * signed)
#define SLAC_IN_OP_MULHSU       0x00f   // multiply high (signed * unsigned)
#define SLAC_IN_OP_MULHUU       0x010   // multiple high (unsigned * unsigned)
#define SLAC_IN_OP_DIVS         0x011   // divide (signed * signed)
#define SLAC_IN_OP_DIVU         0x012   // divide (unsigned * unsigned)
#define SLAC_IN_OP_MODS         0x013   // mod (signed * signed)
#define SLAC_IN_OP_MODU         0x014   // mod (unsigned * unsigned)


// load store
#define SLAC_IN_OP_LDB          0x000   // load byte
#define SLAC_IN_OP_LDBS         0x001   // load byte (sign extend)
#define SLAC_IN_OP_LDH          0x002   // load half
#define SLAC_IN_OP_LDHS         0x003   // load half (sign extend)
#define SLAC_IN_OP_LDW          0x004   // load word
#define SLAC_IN_OP_LDWS         0x005   // load word (sign extend)
#define SLAC_IN_OP_LDX          0x006   // load long
// #define SLAC_IN_OP_LDXS         0x003   // load long (sign extend)

#define SLAC_IN_OP_STB          0x005   // store byte
#define SLAC_IN_OP_STH          0x006   // store half
#define SLAC_IN_OP_STW          0x007   // store word
#define SLAC_IN_OP_STX          0x008   // store long
// #define SLAC_IN_OP_LDP          0x00a   // load pair
// #define SLAC_IN_OP_STP          0x00b   // store pair
// #define SLAC_IN_OP_LDM          0x00c   // load multiple
// #define SLAC_IN_OP_STM          0x00d   // store multiple

// branch
#define SLAC_IN_OP_B            0x000   // branch
#define SLAC_IN_OP_BL           0x001   // branch and link
#define SLAC_IN_OP_CBEQ         0x002   // compare and branch equal
#define SLAC_IN_OP_CBNE         0x003   // compare and branch not equal
#define SLAC_IN_OP_CBLTU        0x004   // compare and branch less than (unsigned)
#define SLAC_IN_OP_CBLTS        0x005   // compare and branch less than (signed)
#define SLAC_IN_OP_CBGEU        0x006   // compare and branch less than (unsigned)
#define SLAC_IN_OP_CBGES        0x007   // compare and branch less than (signed)
// #define SLAC_IN_OP_TB           0x008   // test and branch

// fp

// vec

// simd

// atomic
#define SLAC_IN_OP_LX           0x000   // load exclusive
#define SLAC_IN_OP_SX           0x001   // store exclusive
#define SLAC_IN_OP_SWP          0x002   // atomic swap
#define SLAC_IN_OP_CAS          0x003   // atomic compare and swap
#define SLAC_IN_OP_ATADD        0x004   // atomic add
#define SLAC_IN_OP_ATAND        0x005   // atomic and
#define SLAC_IN_OP_ATOR         0x006   // atomic or
#define SLAC_IN_OP_ATXOR        0x007   // atomic xor
#define SLAC_IN_OP_ATMINU       0x008   // atomic min unsigned
#define SLAC_IN_OP_ATMINS       0x009   // atomic min signed
#define SLAC_IN_OP_ATMAXU       0x00a   // atomic max unsigned
#define SLAC_IN_OP_ATMAXS       0x00b   // atomic max signed


// sys
#define SLAC_IN_OP_ADR4K        0x000   // generate pc relative address at 4K alignment
#define SLAC_IN_OP_MBAR         0x001   // memory barrier
#define SLAC_IN_OP_IBAR         0x002   // instruction barrier

#define SLAC_IN_OP_NOP          0x3fd   // nop
#define SLAC_IN_OP_UNDEF        0x3fe   // undefined instruction
#define SLAC_IN_OP_INVALID      0x3ff   // invalid instruction

#define SLAC_IN_INVALID         0xffffffff


#define SLAC_IN_PRED_SHIFT      0
#define SLAC_IN_TYPE_SHIFT      4
#define SLAC_IN_ARG_SHIFT       8
#define SLAC_IN_OP_SHIFT        14
#define SLAC_IN_LEN_SHIFT       26

struct sl_slac_desc {
    u4 machine_op;      // native instruction encoding
    u4 print_type;      // arch-specific print format
    const char *str;
    // int (*print)(sl_core_t *c, sl_slac_inst_t *si);
};

struct sl_slac_inst {
    union {
        u4 raw;
        struct {
            u4 pred   : 4;  // 0
            u4 type   : 4;  // 4
            u4 arg    : 5;  // 8
            u4 op     : 10; // 13
            u4 len    : 3;  // 23
            u4 sx8    : 1;  // 26
            u4 _unused: 5;  // 27
        };
    };

    u1 d0;
    u1 r0;
    u1 r1;
    u1 r2;

    union {
        u8 uimm;
        i8 simm;
    };

    sl_slac_desc_t desc;
};

#ifdef __cplusplus
}
#endif
