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

typedef union {
    u32 raw;
    struct {
        u32 opcode : 7;
        u32 imm3   : 1;
        u32 imm1   : 4;
        u32 funct3 : 3;
        u32 rs1    : 5;
        u32 rs2    : 5;
        u32 imm2   : 6;
        u32 imm4   : 1;
    } b;
    struct {
        u32 opcode : 7;
        u32 rd     : 5;
        u32 funct3 : 3;
        u32 rs1    : 5;
        u32 imm    : 12;
    } i;
    struct {
        u32 opcode : 7;
        u32 rd     : 5;
        u32 imm3   : 8;
        u32 imm2   : 1;
        u32 imm1   : 10;
        u32 imm4   : 1;
    } j;
    struct {
        u32 opcode : 7;
        u32 rd     : 5;
        u32 funct3 : 3;
        u32 rs1    : 5;
        u32 rs2    : 5;
        u32 funct7 : 7;
    } r;
    struct {
        u32 opcode : 7;
        u32 imm1   : 5;
        u32 funct3 : 3;
        u32 rs1    : 5;
        u32 rs2    : 5;
        u32 imm2   : 7;
    } s;
    struct {
        u32 opcode : 7;
        u32 rd     : 5;
        u32 imm    : 20;
    } u;
} rv_inst_t;

typedef union {
    u16 raw;
    struct {
        u16 opcode : 2;
        u16 rs2    : 5;
        u16 rsd    : 5;
        u16 funct4 : 1;    // this is different from the spec definition of funct4
        u16 funct3 : 3;
    } cr;
    struct {
        u16 opcode : 2;
        u16 imm0   : 5;
        u16 rsd    : 5;
        u16 imm1   : 1;
        u16 funct3 : 3;
    } ci;
    struct {
        u16 opcode : 2;
        u16 off_6  : 2;
        u16 off_2  : 3;
        u16 rd     : 5;
        u16 off_5  : 1;
        u16 funct3 : 3;
    } cilwsp;
    struct {
        u16 opcode : 2;
        u16 off_6  : 3;
        u16 off_3  : 2;
        u16 rd     : 5;
        u16 off_5  : 1;
        u16 funct3 : 3;
    } cildsp;
    struct {
        u16 opcode : 2;
        u16 rs2    : 5;
        u16 imm    : 6;
        u16 funct3 : 3;
    } css;
    struct {
        u16 opcode : 2;
        u16 rd     : 3;
        u16 imm0   : 1;
        u16 imm1   : 1;
        u16 imm2   : 4;
        u16 imm3   : 2;
        u16 funct3 : 3;
    } ciw;
    struct {
        u16 opcode : 2;
        u16 rd     : 3;
        u16 imm0   : 2;
        u16 rs     : 3;
        u16 imm1   : 3;
        u16 funct3 : 3;
    } cl;
    struct {
        u16 opcode : 2;
        u16 rs2    : 3;
        u16 imm0   : 2;
        u16 rs1    : 3;
        u16 imm1   : 3;
        u16 funct3 : 3;
    } cs;
    struct {
        u16 opcode : 2;
        u16 off_5  : 1;
        u16 off_1  : 2;
        u16 off_6  : 2;
        u16 rs     : 3;
        u16 off_3  : 2;
        u16 off_8  : 1;
        u16 funct3 : 3;
    } cb;
    struct {
        u16 opcode : 2;
        u16 imm0   : 5;
        u16 rsd    : 3;
        u16 funct2 : 2;
        u16 imm1   : 1;
        u16 funct3 : 3;
    } cba; // CB-ALU format
    struct {
        u16 opcode : 2;
        u16 imm0   : 1;
        u16 imm1   : 3;
        u16 imm2   : 1;
        u16 imm3   : 1;
        u16 imm4   : 1;
        u16 imm5   : 2;
        u16 imm6   : 1;
        u16 imm7   : 1;
        u16 funct3 : 3;
    } cj;
} rv_cinst_t;

#define CIIMM(ci) (ci.ci.imm0 | (ci.ci.imm1 << 5))
#define CIMM_ADDI16SP(ci) (((ci.ci.imm0 & 1) << 5) | ((ci.ci.imm0 & 6) << 6) | \
                           ((ci.ci.imm0 & 8) << 3) | (ci.ci.imm0 & 0x10) | (ci.ci.imm1 << 9))
#define CILWSPIMM(ci) ((ci.cilwsp.off_6 << 6) | (ci.cilwsp.off_2 << 2) | (ci.cilwsp.off_5 << 5))
#define CLIDSPIMM(ci) ((ci.cildsp.off_6 << 6) | (ci.cildsp.off_3 << 3) | (ci.cildsp.off_5 << 5))

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
