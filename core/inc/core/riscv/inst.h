// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdint.h>

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
        u2 imm0   : 1;
        u2 imm1   : 1;
        u2 imm2   : 4;
        u2 imm3   : 2;
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
        u2 imm0   : 1;
        u2 imm1   : 3;
        u2 imm2   : 1;
        u2 imm3   : 1;
        u2 imm4   : 1;
        u2 imm5   : 2;
        u2 imm6   : 1;
        u2 imm7   : 1;
        u2 funct3 : 3;
    } cj;
} rv_cinst_t;

// The ci immediate is a mess depending on the instruction

// ci format: imm1: imm[5], imm0: imm[4:0]
#define CI_IMM(ci) (ci.ci.imm0 | (ci.ci.imm1 << 5))

// ci format: imm1: imm[9], imm0: imm[4|6|8:7|5]
#define CI_ADDI16SP_IMM(ci) (((ci.ci.imm0 & 1) << 5) | ((ci.ci.imm0 & 6) << 6) | \
                             ((ci.ci.imm0 & 8) << 3) | (ci.ci.imm0 & 0x10) | (ci.ci.imm1 << 9))

// ci format: imm1: offset[5], imm0: offset[4:2|7:6]
#define CI4_IMM(ci) ((ci.ci4.off_6 << 6) | (ci.ci4.off_2 << 2) | (ci.ci4.off_5 << 5))

// ci format: imm1: offset[5], imm0: offset[4:3|8:6]
#define CI8_IMM(ci) ((ci.ci8.off_6 << 6) | (ci.ci8.off_3 << 3) | (ci.ci8.off_5 << 5))


#define CIWIMM(ci) ((ci.ciw.imm1 << 2) | (ci.ciw.imm0 << 3) | (ci.ciw.imm3 << 4) | (ci.ciw.imm2 << 6))
#define CJIMM(ci) ((ci.cj.imm0 << 5) | (ci.cj.imm1 << 1) | (ci.cj.imm2 << 7) | \
                   (ci.cj.imm3 << 6) | (ci.cj.imm4 << 10) | (ci.cj.imm5 << 8) | \
                   (ci.cj.imm6 << 4) | (ci.cj.imm7 << 11))

#define CBIMM(ci) ((ci.cb.off_5 << 5) | (ci.cb.off_1 << 1) | (ci.cb.off_6 << 6) | \
                   (ci.cb.off_3 << 3) | (ci.cb.off_8 << 8))
#define CBAIMM(ci) ((ci.cba.imm1 << 5) | ci.cba.imm0)
#define CSIMM_SCALED_W(ci) (((ci.cl.imm0 & 1) << 6) | ((ci.cl.imm0 & 2) << 1) | ((ci.cl.imm1) << 3))
#define CSIMM_SCALED_D(ci) ((ci.cl.imm0 << 6) | (ci.cl.imm1 << 3))

// CSS format has multiple shift patterns depending on the instruction
#define CSSIMM_SCALED_W(ci) (((ci.css.imm & 3) << 6) | (ci.css.imm & ~3))
#define CSSIMM_SCALED_D(ci) (((ci.css.imm & 7) << 6) | (ci.css.imm & ~7))
