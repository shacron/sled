// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    uint32_t raw;
    struct {
        uint32_t opcode : 7;
        uint32_t imm3   : 1;
        uint32_t imm1   : 4;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t imm2   : 6;
        uint32_t imm4   : 1;
    } b;
    struct {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t imm    : 12;
    } i;
    struct {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t imm3   : 8;
        uint32_t imm2   : 1;
        uint32_t imm1   : 10;
        uint32_t imm4   : 1;
    } j;
    struct {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t funct7 : 7;
    } r;
    struct {
        uint32_t opcode : 7;
        uint32_t imm1   : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t imm2   : 7;
    } s;
    struct {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t imm    : 20;
    } u;
} rv_inst_t;

typedef union {
    uint16_t raw;
    struct {
        uint16_t opcode : 2;
        uint16_t rs2    : 5;
        uint16_t rsd    : 5;
        uint16_t funct4 : 1;    // this is different from the spec definition of funct4
        uint16_t funct3 : 3;
    } cr;
    struct {
        uint16_t opcode : 2;
        uint16_t imm0   : 5;
        uint16_t rsd    : 5;
        uint16_t imm1   : 1;
        uint16_t funct3 : 3;
    } ci;
    struct {
        uint16_t opcode : 2;
        uint16_t off_6  : 2;
        uint16_t off_2  : 3;
        uint16_t rd     : 5;
        uint16_t off_5  : 1;
        uint16_t funct3 : 3;
    } cilwsp;
    struct {
        uint16_t opcode : 2;
        uint16_t off_6  : 3;
        uint16_t off_3  : 2;
        uint16_t rd     : 5;
        uint16_t off_5  : 1;
        uint16_t funct3 : 3;
    } cildsp;
    struct {
        uint16_t opcode : 2;
        uint16_t rs2    : 5;
        uint16_t imm    : 6;
        uint16_t funct3 : 3;
    } css;
    struct {
        uint16_t opcode : 2;
        uint16_t rd     : 3;
        uint16_t imm0   : 1;
        uint16_t imm1   : 1;
        uint16_t imm2   : 4;
        uint16_t imm3   : 2;
        uint16_t funct3 : 3;
    } ciw;
    struct {
        uint16_t opcode : 2;
        uint16_t rd     : 3;
        uint16_t imm0   : 2;
        uint16_t rs     : 3;
        uint16_t imm1   : 3;
        uint16_t funct3 : 3;
    } cl;
    struct {
        uint16_t opcode : 2;
        uint16_t rs2    : 3;
        uint16_t imm0   : 2;
        uint16_t rs1    : 3;
        uint16_t imm1   : 3;
        uint16_t funct3 : 3;
    } cs;
    struct {
        uint16_t opcode : 2;
        uint16_t off_5  : 1;
        uint16_t off_1  : 2;
        uint16_t off_6  : 2;
        uint16_t rs     : 3;
        uint16_t off_3  : 2;
        uint16_t off_8  : 1;
        uint16_t funct3 : 3;
    } cb;
    struct {
        uint16_t opcode : 2;
        uint16_t imm0   : 5;
        uint16_t rsd    : 3;
        uint16_t funct2 : 2;
        uint16_t imm1   : 1;
        uint16_t funct3 : 3;
    } cba; // CB-ALU format
    struct {
        uint16_t opcode : 2;
        uint16_t imm0   : 1;
        uint16_t imm1   : 3;
        uint16_t imm2   : 1;
        uint16_t imm3   : 1;
        uint16_t imm4   : 1;
        uint16_t imm5   : 2;
        uint16_t imm6   : 1;
        uint16_t imm7   : 1;
        uint16_t funct3 : 3;
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

#ifdef __cplusplus
}
#endif
