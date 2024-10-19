// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

// opcodes
#define OP_ALU          0b0110011   // ALU REG
#define OP_ALU32        0b0111011   // rv64 ALU 32
#define OP_IMM          0b0010011   // ALU IMM
#define OP_IMM32        0b0011011   // rv64 ALU IMM 32
#define OP_LUI          0b0110111
#define OP_AUIPC        0b0010111
#define OP_JAL          0b1101111
#define OP_JALR         0b1100111
#define OP_BRANCH       0b1100011
#define OP_LOAD         0b0000011
#define OP_STORE        0b0100011
#define OP_MISC_MEM     0b0001111 // fence
#define OP_SYSTEM       0b1110011 // csr ecall ebreak

#define OP_AMO          0b0101111 // RV32A / RV64A extension
#define OP_FP           0b1010011 // FP32/64
#define OP_FP_LOAD      0b0000111 // FP32/64 load
#define OP_FP_STORE     0b0100111 // FP32/64 store

#define OP_FMADD_S      0b1000011 // FMADD.S
#define OP_FMSUB_S      0b1000111 // FMSUB.S
#define OP_FNMSUB_S     0b1001011 // FNMSUB.S
#define OP_FNMADD_S     0b1001111 // FNMADD.S

typedef union {
    u4 raw;
    struct {
        u4 opcode : 7;
        u4 imm3   : 1;
        u4 imm1   : 4;
        u4 funct3 : 3;
        u4 rs1    : 5;
        u4 rs2    : 5;
        u4 imm2   : 6;
        u4 imm4   : 1;
    } b;
    struct {
        u4 opcode : 7;
        u4 rd     : 5;
        u4 funct3 : 3;
        u4 rs1    : 5;
        u4 imm    : 12;
    } i;
    struct {
        u4 opcode : 7;
        u4 rd     : 5;
        u4 imm3   : 8;
        u4 imm2   : 1;
        u4 imm1   : 10;
        u4 imm4   : 1;
    } j;
    struct {
        u4 opcode : 7;
        u4 rd     : 5;
        u4 funct3 : 3;
        u4 rs1    : 5;
        u4 rs2    : 5;
        u4 funct7 : 7;
    } r;
    struct {
        u4 opcode : 7;
        u4 imm1   : 5;
        u4 funct3 : 3;
        u4 rs1    : 5;
        u4 rs2    : 5;
        u4 imm2   : 7;
    } s;
    struct {
        u4 opcode : 7;
        u4 rd     : 5;
        u4 imm    : 20;
    } u;
    struct {
        u4 opcode : 7;
        u4 rd     : 5;
        u4 rm     : 3;
        u4 rs1    : 5;
        u4 rs2    : 5;
        u4 fmt    : 2;
        u4 funct5 : 5; // rs3
    } r4;
} rv_inst_t;

typedef union {
    u2 raw;
    struct {
        u2 opcode : 2;
        u2 rs2    : 5;
        u2 rsd    : 5;
        u2 funct4 : 1;    // this is different from the spec definition of funct4
        u2 funct3 : 3;
    } cr;
    struct {
        u2 opcode : 2;
        u2 imm0   : 5;
        u2 rsd    : 5;
        u2 imm1   : 1;
        u2 funct3 : 3;
    } ci;
    struct {
        u2 opcode : 2;
        u2 off_6  : 2;
        u2 off_2  : 3;
        u2 rd     : 5;
        u2 off_5  : 1;
        u2 funct3 : 3;
    } ci4;
    struct {
        u2 opcode : 2;
        u2 off_6  : 3;
        u2 off_3  : 2;
        u2 rd     : 5;
        u2 off_5  : 1;
        u2 funct3 : 3;
    } ci8;
    struct {
        u2 opcode : 2;
        u2 rs2    : 5;
        u2 imm    : 6;
        u2 funct3 : 3;
    } css;
    struct {
        u2 opcode : 2;
        u2 rd     : 3;
        u2 off_3  : 1;
        u2 off_2  : 1;
        u2 off_6  : 4;
        u2 off_4  : 2;
        u2 funct3 : 3;
    } ciw;
    struct {
        u2 opcode : 2;
        u2 rd     : 3;
        u2 imm0   : 2;
        u2 rs     : 3;
        u2 imm1   : 3;
        u2 funct3 : 3;
    } cl;
    struct {
        u2 opcode : 2;
        u2 rs2    : 3;
        u2 imm0   : 2;
        u2 rs1    : 3;
        u2 imm1   : 3;
        u2 funct3 : 3;
    } cs;
    struct {
        u2 opcode : 2;
        u2 off_5  : 1;
        u2 off_1  : 2;
        u2 off_6  : 2;
        u2 rs     : 3;
        u2 off_3  : 2;
        u2 off_8  : 1;
        u2 funct3 : 3;
    } cb;
    struct {
        u2 opcode : 2;
        u2 imm0   : 5;
        u2 rsd    : 3;
        u2 funct2 : 2;
        u2 imm1   : 1;
        u2 funct3 : 3;
    } cba; // CB-ALU format
    struct {
        u2 opcode : 2;
        u2 off_5  : 1;
        u2 off_1  : 3;
        u2 off_7  : 1;
        u2 off_6  : 1;
        u2 off_10 : 1;
        u2 off_8  : 2;
        u2 off_4  : 1;
        u2 off_11 : 1;
        u2 funct3 : 3;
    } cj;
} rv_cinst_t;

// The CI, CS, and CSS immediate format varies depending on the instruction
// This is an attempt to make extracting them more sane.

// ci format: imm1: imm[5], imm0: imm[4:0]
#define CI_IMM(ci) (ci.ci.imm0 | (ci.ci.imm1 << 5))

// ci format: imm1: imm[9], imm0: imm[4|6|8:7|5]
#define CI_ADDI16SP_IMM(ci) (((ci.ci.imm0 & 1) << 5) | ((ci.ci.imm0 & 6) << 6) | \
                             ((ci.ci.imm0 & 8) << 3) | (ci.ci.imm0 & 0x10) | (ci.ci.imm1 << 9))

// ci format: imm1: offset[5], imm0: offset[4:2|7:6]
#define CI_IMM_SCALED_4(ci) ((ci.ci4.off_6 << 6) | (ci.ci4.off_2 << 2) | (ci.ci4.off_5 << 5))

// ci format: imm1: offset[5], imm0: offset[4:3|8:6]
#define CI_IMM_SCALED_8(ci) ((ci.ci8.off_6 << 6) | (ci.ci8.off_3 << 3) | (ci.ci8.off_5 << 5))

#define CIW_IMM(ci) ((ci.ciw.off_2 << 2) | (ci.ciw.off_3 << 3) | (ci.ciw.off_4 << 4) | (ci.ciw.off_6 << 6))

#define CJ_IMM(ci) ((ci.cj.off_5 << 5) | (ci.cj.off_1 << 1) | (ci.cj.off_7 << 7) | \
                    (ci.cj.off_6 << 6) | (ci.cj.off_10 << 10) | (ci.cj.off_8 << 8) | \
                    (ci.cj.off_4 << 4) | (ci.cj.off_11 << 11))

#define CB_IMM(ci) ((ci.cb.off_5 << 5) | (ci.cb.off_1 << 1) | (ci.cb.off_6 << 6) | \
                    (ci.cb.off_3 << 3) | (ci.cb.off_8 << 8))

// same as CI_IMM
#define CBA_IMM(ci) (ci.cba.imm0 | (ci.cba.imm1 << 5))

#define CS_IMM_SCALED_4(ci) (((ci.cl.imm0 & 1) << 6) | ((ci.cl.imm0 & 2) << 1) | ((ci.cl.imm1) << 3))
#define CS_IMM_SCALED_8(ci) ((ci.cl.imm0 << 6) | (ci.cl.imm1 << 3))

#define CSS_IMM_SCALED_4(ci) (((ci.css.imm & 3) << 6) | (ci.css.imm & ~3))
#define CSS_IMM_SCALED_8(ci) (((ci.css.imm & 7) << 6) | (ci.css.imm & ~7))
