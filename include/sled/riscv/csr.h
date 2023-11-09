// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

// mstatus / sstatus
// ****************************************************************************

#define RV_SR_STATUS_BIT_WPRI0 0
#define RV_SR_STATUS_BIT_SIE   1
#define RV_SR_STATUS_BIT_WPRI1 2
#define RV_SR_STATUS_BIT_MIE   3
#define RV_SR_STATUS_BIT_WPRI2 4
#define RV_SR_STATUS_BIT_SPIE  5
#define RV_SR_STATUS_BIT_UBE   6
#define RV_SR_STATUS_BIT_MPIE  7
#define RV_SR_STATUS_BIT_SPP   8
#define RV_SR_STATUS_BIT_VS    9
#define RV_SR_STATUS_BIT_MPP   11
#define RV_SR_STATUS_BIT_FS    13
#define RV_SR_STATUS_BIT_XS    15
#define RV_SR_STATUS_BIT_MPRV  17
#define RV_SR_STATUS_BIT_SUM   18
#define RV_SR_STATUS_BIT_MXR   19
#define RV_SR_STATUS_BIT_TVM   20
#define RV_SR_STATUS_BIT_TW    21
#define RV_SR_STATUS_BIT_TSR   22
#define RV_SR_STATUS_BIT_WPRI3 23
#define RV_SR_STATUS_BIT_SD    31   // 32-bit SD
// 64-bit only
#define RV_SR_STATUS_BIT_UXL   32
#define RV_SR_STATUS_BIT_SXL   34
#define RV_SR_STATUS_BIT_SBE   36
#define RV_SR_STATUS_BIT_MBE   37
#define RV_SR_STATUS_BIT_WPRI4 38
#define RV_SR_STATUS64_BIT_SD  63   // 64-bit SD

#define RV_SR_STATUS_WPRI0     (1ul << RV_SR_STATUS_BIT_WPRI0)
#define RV_SR_STATUS_SIE       (1ul << RV_SR_STATUS_BIT_SIE)
#define RV_SR_STATUS_WPRI1     (1ul << RV_SR_STATUS_BIT_WPRI1)
#define RV_SR_STATUS_MIE       (1ul << RV_SR_STATUS_BIT_MIE)
#define RV_SR_STATUS_WPRI2     (1ul << RV_SR_STATUS_BIT_WPRI2)
#define RV_SR_STATUS_SPIE      (1ul << RV_SR_STATUS_BIT_SPIE)
#define RV_SR_STATUS_UBE       (1ul << RV_SR_STATUS_BIT_UBE)
#define RV_SR_STATUS_MPIE      (1ul << RV_SR_STATUS_BIT_MPIE)
#define RV_SR_STATUS_SPP       (1ul << RV_SR_STATUS_BIT_SPP)
#define RV_SR_STATUS_MPRV      (1ul << RV_SR_STATUS_BIT_MPRV)
#define RV_SR_STATUS_SUM       (1ul << RV_SR_STATUS_BIT_SUM)
#define RV_SR_STATUS_MXR       (1ul << RV_SR_STATUS_BIT_MXR)
#define RV_SR_STATUS_TVM       (1ul << RV_SR_STATUS_BIT_TVM)
#define RV_SR_STATUS_TW        (1ul << RV_SR_STATUS_BIT_TW)
#define RV_SR_STATUS_TSR       (1ul << RV_SR_STATUS_BIT_TSR)
#define RV_SR_STATUS_SD        (1ul << RV_SR_STATUS_BIT_SD)
#define RV_SR_STATUS_SBE       (1ul << RV_SR_STATUS_BIT_SBE)
#define RV_SR_STATUS_MBE       (1ul << RV_SR_STATUS_BIT_MBE)
#define RV_SR_STATUS_WPRI4     (1ul << RV_SR_STATUS_BIT_WPRI4)
#define RV_SR_STATUS64_SD      (1ul << RV_SR_STATUS64_BIT_SD)

#define RV_SR_STATUS_VS_MASK    (3ul << RV_SR_STATUS_BIT_VS)
#define RV_SR_STATUS_MMP_MASK   (3ul << RV_SR_STATUS_BIT_MPP)
#define RV_SR_STATUS_FS_MASK    (3ul << RV_SR_STATUS_BIT_FS)
#define RV_SR_STATUS_XS_MASK    (3ul << RV_SR_STATUS_BIT_XS)
#define RV_SR_STATUS64_UXL_MASK (3ul << RV_SR_STATUS_BIT_UXL)
#define RV_SR_STATUS64_SXL_MASK (3ul << RV_SR_STATUS_BIT_SXL)

// CSR addresses in csr instruction
// ****************************************************************************

// Unprivileged Counter/Timers
#define RV_CSR_CYCLE            0xC00   // URO cycle - Cycle counter for RDCYCLE instruction
#define RV_CSR_TIME             0xC01   // URO time - Timer for RDTIME instruction
#define RV_CSR_INSTRET          0xC02   // URO instret - Instructions-retired counter for RDINSTRET instruction
#define RV_CSR_HPMCOUNTER3      0xC03   // URO hpmcounter3 - Performance-monitoring counter
#define RV_CSR_HPMCOUNTER4      0xC04   // URO hpmcounter4
                                        // ...
#define RV_CSR_HPMCOUNTER31     0xC1F   // URO hpmcounter31

#define RV_CSR_CYCLEH           0xC80   // URO cycleh - Upper 32 bits of cycle, RV32 only
#define RV_CSR_TIMEH            0xC81   // URO timeh - Upper 32 bits of time, RV32 only
#define RV_CSR_INSTRETH         0xC82   // URO instreth - Upper 32 bits of instret, RV32 only
#define RV_CSR_HPMCOUNTER3H     0xC83   // URO hpmcounter3h - Upper 32 bits of hpmcounter3, RV32 only
#define RV_CSR_HPMCOUNTER4H     0xC84   // URO hpmcounter4h
                                        // ...
#define RV_CSR_HPMCOUNTER31H    0xC9F   // URO hpmcounter31h


// Supervisor Trap Setup
#define RV_CSR_SSTATUS      0x100   // SRW sstatus - Supervisor status register.
#define RV_CSR_SIE          0x104   // SRW sie - Supervisor interrupt-enable register.
#define RV_CSR_STVEC        0x105   // SRW stvec - Supervisor trap handler base address.
#define RV_CSR_SCOUNTEREN   0x106   // SRW scounteren - Supervisor counter enable.
// Supervisor Configuration
#define RV_CSR_SENVCFG      0x10A   // SRW senvcfg - Supervisor environment configuration register.
// Supervisor Trap Handling
#define RV_CSR_SSCRATCH     0x140   // SRW sscratch - Scratch register for supervisor trap handlers.
#define RV_CSR_SEPC         0x141   // SRW sepc - Supervisor exception program counter.
#define RV_CSR_SCAUSE       0x142   // SRW scause - Supervisor trap cause.
#define RV_CSR_STVAL        0x143   // SRW stval - Supervisor bad address or instruction.
#define RV_CSR_SIP          0x144   // SRW sip - Supervisor interrupt pending.
// Supervisor address translation and protection.
#define RV_CSR_SATP         0x180   // SRW satp - Supervisor Protection and Translation
// Debug/Trace Registers
#define RV_CSR_SCONTEXT     0x5A8   // SRW scontext - Supervisor-mode context register.


// Machine Information Registers (MRO)
#define RV_CSR_MVENDORID        0xf11 // MRO mvendorid - Vendor ID.
#define RV_CSR_MARCHID          0xf12 // MRO marchid - Architecture ID.
#define RV_CSR_MIMPID           0xf13 // MRO mimpid - Implementation ID.
#define RV_CSR_MHARTID          0xf14 // MRO mhartid - Hardware thread ID.
#define RV_CSR_MCONFIGPTR       0xf15 // MRO mconfigptr - Pointer to configuration data structure.
// Machine Trap Setup (MRW)
#define RV_CSR_MSTATUS          0x300 // MRW mstatus - Machine status register.
#define RV_CSR_MISA             0x301 // MRW misa - ISA and extensions
#define RV_CSR_MEDELEG          0x302 // MRW medeleg - Machine exception delegation register.
#define RV_CSR_MIDELEG          0x303 // MRW mideleg - Machine interrupt delegation register.
#define RV_CSR_MIE              0x304 // MRW mie - Machine interrupt-enable register.
#define RV_CSR_MTVEC            0x305 // MRW mtvec - Machine trap-handler base address.
#define RV_CSR_MCOUNTEREN       0x306 // MRW mcounteren - Machine counter enable.
#define RV_CSR_MSTATUSH         0x310 // MRW mstatush - Additional machine status register, RV32 only.
// Machine Trap Handling (MRW)
#define RV_CSR_MSCRATCH         0x340 // MRW mscratch = Scratch register for machine trap handlers.
#define RV_CSR_MEPC             0x341 // MRW mepc = Machine exception program counter.
#define RV_CSR_MCAUSE           0x342 // MRW mcause = Machine trap cause.
#define RV_CSR_MTVAL            0x343 // MRW mtval = Machine bad address or instruction.
#define RV_CSR_MIP              0x344 // MRW mip = Machine interrupt pending.
#define RV_CSR_MTINST           0x34a // MRW mtinst = Machine trap instruction (transformed).
#define RV_CSR_MTVAL2           0x34b // MRW mtval2 = Machine bad guest physical address.
#define RV_CSR_MENVCFG          0x30a // MRW menvcfg - Machine environment configuration register.
#define RV_CSR_MENVCFGH         0x31a // MRW menvcfgh - Additional machine env. conf. register, RV32 only.
// Machine Configuration
#define RV_CSR_MSECCFG          0x747 // MRW mseccfg - Machine security configuration register.
#define RV_CSR_MSECCFGH         0x757 // MRW mseccfgh - Additional machine security conf. register, RV32 only.
#define RV_CSR_PMPCFG_BASE      0x3a0 // MRW pmpcfg0-15 - Physical memory protection configuration.
#define RV_CSR_PMPCFG_NUM       16
#define RV_CSR_PMPADDR_BASE     0x3b0 // MRW pmpaddr0-63 - Physical memory protection address register.
#define RV_CSR_PMPADDR_NUM      64
// Machine Counter/Timers
#define RV_CSR_MCYCLE           0xb00 // MRW mcycle - Machine cycle counter.
#define RV_CSR_MINSTRET         0xb02 // MRW minstret - Machine instructions-retired counter.
#define RV_CSR_MHPMCOUNTER3     0xb03 // MRW mhpmcounter3 - Machine performance-monitoring counter.
#define RV_CSR_MHPMCOUNTER_NUM  29
#define RV_CSR_MCYCLEH          0xb80 // MRW mcycleh - Upper 32 bits of mcycle, RV32 only.
#define RV_CSR_MINSTRETH        0xb82 // MRW minstreth - Upper 32 bits of minstret, RV32 only.
#define RV_CSR_MHPMCOUNTER3H    0xb83 // MRW mhpmcounter3h - Upper 32 bits of mhpmcounter3, RV32 only.
#define RV_CSR_MHPMCOUNTER4H    0xb84 // MRW mhpmcounter4h - Upper 32 bits of mhpmcounter4, RV32 only.
#define RV_CSR_MHPMCOUNTER31H   0xb9f // MRW mhpmcounter31h - Upper 32 bits of mhpmcounter31, RV32 only.
// Machine Counter Setup
#define RV_CSR_MCOUNTINHIBIT    0x320 // MRW mcountinhibit - Machine counter-inhibit register.
#define RV_CSR_MHPMEVENT3       0x323 // MRW mhpmevent3 - Machine performance-monitoring event selector.
#define RV_CSR_MHPMEVENT_NUM    29
// Debug/Trace Registers (shared with Debug Mode)
#define RV_CSR_TSELECT          0x7a0 // MRW tselect - Debug/Trace trigger register select.
#define RV_CSR_TDATA1           0x7a1 // MRW tdata1 - First Debug/Trace trigger data register.
#define RV_CSR_TDATA2           0x7a2 // MRW tdata2 - Second Debug/Trace trigger data register.
#define RV_CSR_TDATA3           0x7a3 // MRW tdata3 - Third Debug/Trace trigger data register.
#define RV_CSR_MCONTEXT         0x7a8 // MRW mcontext - Machine-mode context register.
// Debug Mode Registers
#define RV_CSR_DCSR             0x7b0 // DRW dcsr - Debug control and status register.
#define RV_CSR_DPC              0x7b1 // DRW dpc - Debug PC.
#define RV_CSR_DSCRATCH0        0x7b2 // DRW dscratch0 - Debug scratch register 0.
#define RV_CSR_DSCRATCH1        0x7b3 // DRW dscratch1 - Debug scratch register 1.


// fcsr - floating point control and status register
#define RV_CSR_FFLAGS           0x001
#define RV_CSR_FRM              0x002
#define RV_CSR_FCSR             0x003

// rounding modes
#define RV_FCSR_RM_RNE 0b000 // Round to Nearest, ties to Even
#define RV_FCSR_RM_RTZ 0b001 // Round towards Zero
#define RV_FCSR_RM_RDN 0b010 // Round Down (towards stem d086050f743ba2c7974a5de125f1962f)
#define RV_FCSR_RM_RUP 0b011 // Round Up (towards stem 743c6212f9443387ec5e6950b44c06db)
#define RV_FCSR_RM_RMM 0b100 // Round to Nearest, ties to Max Magnitude
#define RV_FCSR_RM_DYN 0b111 // In instruction’s rm field, selects dynamic rounding mode; In Rounding Mode register, reserved.

// flags
#define RV_FCSR_NV (1u << 4) // Invalid Operation
#define RV_FCSR_DZ (1u << 3) // Divide by Zero
#define RV_FCSR_OF (1u << 2) // Overflow
#define RV_FCSR_UF (1u << 1) // Underflow
#define RV_FCSR_NX (1u << 0) // Inexact

