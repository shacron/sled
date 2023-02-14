// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#include <stdbool.h>
#include <stdio.h>

#include <core/core.h>
#include <core/riscv/csr.h>
#include <core/riscv/rv.h>
#include <riscv/csr.h>
#include <sled/error.h>

/*
mstatus64 SD - MBE SBE SXL[1:0] UXL[1:0] - TSR TW TVM MXR SUM MPRV XS[1:0] FS[1:0] MPP[1:0] VS[1:0] SPP MPIE UBE SPIE - MIE - SIE
sstatus64 SD ------------------ UXL[1:0] ------------ MXR SUM ---- XS[1:0] FS[1:0] -------- VS[1:0] SPP ---- UBE SPIE ------- SIE
*/

#define STATUS_MASK_M \
    (RV_SR_STATUS_SIE | RV_SR_STATUS_MIE | RV_SR_STATUS_SPIE | RV_SR_STATUS_UBE | RV_SR_STATUS_MPIE | RV_SR_STATUS_SPP | \
     RV_SR_STATUS_VS_MASK | RV_SR_STATUS_MMP_MASK | RV_SR_STATUS_FS_MASK | RV_SR_STATUS_XS_MASK | RV_SR_STATUS_MPRV | \
     RV_SR_STATUS_SUM | RV_SR_STATUS_MXR | RV_SR_STATUS_TVM | RV_SR_STATUS_TW | RV_SR_STATUS_TSR | RV_SR_STATUS64_UXL_MASK | \
     RV_SR_STATUS64_SXL_MASK | RV_SR_STATUS_SBE | RV_SR_STATUS_MBE | RV_SR_STATUS64_SD)

// #define STATUS_MASK_H STATUS_MASK_M

#define STATUS_MASK_S \
    (RV_SR_STATUS_SIE | RV_SR_STATUS_SPIE | RV_SR_STATUS_UBE | RV_SR_STATUS_SPP | RV_SR_STATUS_VS_MASK | \
     RV_SR_STATUS_FS_MASK | RV_SR_STATUS_XS_MASK | RV_SR_STATUS_SUM | RV_SR_STATUS_MXR | RV_SR_STATUS64_UXL_MASK | \
     RV_SR_STATUS64_SD)

// plain register modification with no side effects
result64_t rv_csr_update(rv_core_t *c, int op, uint64_t *reg, uint64_t value) {
    result64_t result = {};
    switch (op) {
    case RV_CSR_OP_WRITE:       *reg = value;                            break;
    case RV_CSR_OP_READ:        result.value = *reg;                     break;
    case RV_CSR_OP_SWAP:        result.value = *reg;    *reg = value;    break;
    case RV_CSR_OP_READ_SET:    result.value = *reg;    *reg |= value;   break;
    case RV_CSR_OP_READ_CLEAR:  result.value = *reg;    *reg &= ~value;  break;
    default:                    result.err = SL_ERR_UNDEF;               break;
    }
    return result;
}

static uint64_t status_for_pl(uint64_t s, uint8_t pl) {
    switch (pl) {
    case RV_PL_MACHINE:     return s & STATUS_MASK_M;
    // case RV_PL_HYPERVISOR:  return s & STATUS_MASK_H;
    case RV_PL_SUPERVISOR:  return s & STATUS_MASK_S;
    default:                return 0;
    }
}

static result64_t rv_status_csr(rv_core_t *c, int op, uint64_t value) {
    result64_t result = {};

    uint64_t s = status_for_pl(c->status, c->pl);

    if (op == RV_CSR_OP_READ) {
        result.value = s;
        goto fixup;
    }

    if (c->mode == RV_MODE_RV32)
        value = ((value & RV_SR_STATUS_SD) << 32) | (value & ~(RV_SR_STATUS_SD));

    value = status_for_pl(value, c->pl);

    uint64_t changed_bits;

    switch (op) {
    case RV_CSR_OP_READ_SET:
        changed_bits = (~s) & value;
        if (changed_bits & RV_SR_STATUS_MIE) core_interrupt_set(&c->core, true);
        if (changed_bits & RV_SR_STATUS_UBE) core_endian_set(&c->core, true);
        // todo: other fields
        c->status |= value;
        break;

    case RV_CSR_OP_READ_CLEAR:
        changed_bits = s & value;
        if (changed_bits & RV_SR_STATUS_MIE) core_interrupt_set(&c->core, false);
        if (changed_bits & RV_SR_STATUS_UBE) core_endian_set(&c->core, false);
        // todo: other fields
        c->status &= ~value;
        break;

    case RV_CSR_OP_SWAP:
        result.value = s;
        // fall through
    case RV_CSR_OP_WRITE:
        changed_bits = s ^ value;
        if (changed_bits & RV_SR_STATUS_MIE) core_interrupt_set(&c->core, (bool)(value & RV_SR_STATUS_MIE));
        if (changed_bits & RV_SR_STATUS_UBE) core_endian_set(&c->core, (bool)(value & RV_SR_STATUS_UBE));
        // todo: other fields
        c->status = value;
        break;

    default:
        result.err = SL_ERR_UNDEF;
        goto out;
    }

fixup:
    if (c->mode == RV_MODE_RV32)
        result.value = (uint32_t)(((result.value & RV_SR_STATUS64_SD) >> 32) | (result.value & ~(RV_SR_STATUS64_SD)));
out:
    return result;
}

static result64_t rv_mcause_csr(rv_core_t *c, int op, uint64_t *reg, uint64_t value) {
    result64_t result = {};
    if (c->mode == RV_MODE_RV32) {
        value = ((value & RV_CAUSE_INT32) << 32) | (value & ~RV_CAUSE_INT32);
        result = rv_csr_update(c, op, reg, value);
        result.value = (uint32_t)(((result.value & RV_CAUSE_INT64) >> 32) | (result.value & 0x7fffffff));
    } else {
        result = rv_csr_update(c, op, reg, value);
    }
    return result;
}

static result64_t rv_tick_csr(rv_core_t *c, int op, int64_t *offset, uint64_t value) {
    result64_t result = {};
    uint64_t ticks = c->core.ticks;

    switch (op) {
    case RV_CSR_OP_READ:
        result.value = ticks - *offset;
        break;

    case RV_CSR_OP_SWAP:
        result.value = ticks - *offset;
        // fall through
    case RV_CSR_OP_WRITE:
        *offset = ticks - value;
        break;

    case RV_CSR_OP_READ_SET:
    case RV_CSR_OP_READ_CLEAR:
        // bitwise operations on a counter are a bad idea
        result.err = SL_ERR_UNIMPLEMENTED;
        break;

    default:
        result.err = SL_ERR_UNDEF;
        break;
    }
    return result;
}

result64_t rv_csr_op(rv_core_t *c, int op, uint32_t csr, uint64_t value) {
    result64_t result = {};
    csr_addr_t addr;
    addr.raw = csr;

    if (addr.f.level > c->pl) goto undef;
    if (op != RV_CSR_OP_READ) {
        if (addr.f.type == 3) goto undef;
    }

    if (addr.f.level == RV_PL_MACHINE) {
        rv_sr_pl_t *r = rv_get_pl_csrs(c, RV_PL_MACHINE);

        // machine level
        switch (addr.raw) {
        // Machine Information Registers (MRO)
        case RV_CSR_MVENDORID:  result.value = c->mvendorid;  goto out;
        case RV_CSR_MARCHID:    result.value = c->marchid;    goto out;
        case RV_CSR_MIMPID:     result.value = c->mimpid;     goto out;
        case RV_CSR_MHARTID:    result.value = c->mhartid;    goto out;
        case RV_CSR_MCONFIGPTR: result.value = c->mconfigptr; goto out;
        // Machine Trap Setup (MRW)
        case RV_CSR_MSTATUS:    result = rv_status_csr(c, op, value);               goto out;
        case RV_CSR_MISA:       result = rv_csr_update(c, op, &r->isa, value);      goto out;
        case RV_CSR_MEDELEG:    result = rv_csr_update(c, op, &r->edeleg, value);   goto out;
        case RV_CSR_MIDELEG:    result = rv_csr_update(c, op, &r->ideleg, value);   goto out;
        case RV_CSR_MIE:        result = rv_csr_update(c, op, &r->ie, value);       goto out;
        case RV_CSR_MTVEC:      result = rv_csr_update(c, op, &r->tvec, value);     goto out;
        case RV_CSR_MCOUNTEREN: result = rv_csr_update(c, op, &r->counteren, value);    goto out;

        case RV_CSR_MSTATUSH:
            if (c->mode != RV_MODE_RV32) goto undef;
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case RV_CSR_MSCRATCH:   result = rv_csr_update(c, op, &r->scratch, value);  goto out;
        case RV_CSR_MEPC:       result = rv_csr_update(c, op, &r->epc, value);      goto out;
        case RV_CSR_MCAUSE:     result = rv_mcause_csr(c, op, &r->cause, value);    goto out;
        case RV_CSR_MTVAL:      result = rv_csr_update(c, op, &r->tval, value);     goto out;
        case RV_CSR_MIP:        result = rv_csr_update(c, op, &r->ip, value);       goto out;

        case RV_CSR_MTINST:
        case RV_CSR_MTVAL2:
        case RV_CSR_MENVCFG:
            result.err = SL_ERR_UNIMPLEMENTED;  goto out;

        case RV_CSR_MENVCFGH:
            if (c->mode != RV_MODE_RV32) goto undef;
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case RV_CSR_MSECCFG:    result.err = SL_ERR_UNIMPLEMENTED;      goto out;

        case RV_CSR_MSECCFGH:
            if (c->mode != RV_MODE_RV32) goto undef;
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case RV_CSR_MCYCLE:     result = rv_tick_csr(c, op, &c->mcycle_offset, value);      goto out;
        case RV_CSR_MINSTRET:   result = rv_tick_csr(c, op, &c->minstret_offset, value);    goto out;

        default:                break;
        }

        if ((addr.raw >= RV_CSR_PMPCFG_BASE) && (addr.raw < (RV_CSR_PMPCFG_BASE + RV_CSR_PMPCFG_NUM))) {
            const uint32_t i = addr.raw - RV_CSR_PMPCFG_BASE;
            result = rv_csr_update(c, op, &c->pmpcfg[i], value);
            goto out;
        }

        if ((addr.raw >= RV_CSR_PMPADDR_BASE) && (addr.raw < (RV_CSR_PMPADDR_BASE + RV_CSR_PMPADDR_NUM))) {
            const uint32_t i = addr.raw - RV_CSR_PMPADDR_BASE;
            result = rv_csr_update(c, op, &c->pmpaddr[i], value);
            goto out;
        }

        goto undef;
    }

    // hypervisor level
    if (addr.f.level == RV_PL_HYPERVISOR) {
        switch (addr.raw) {
        case 0x240: // vsscratch
        case 0x241: // vsepc
        case 0x242: // vscause
        case 0x243: // vstval
        case 0x244: // vsip

        case 0x643: // htval
        case 0x644: // hip
        case 0x645: // hvip
        case 0x64a: // htinst

        case 0xe12: // hgeip (HRO)

        // Virtual Supervisor Trap Registers (HRW)
        case 0x280: // vsatp    // ?

        // Virtual Supervisor Registers (HRW)
        case 0x200: // vsstatus
        case 0x204: // vsie
        case 0x205: // vstvec

        // Hypervisor Trap Setup (HRW)
        case 0x600: // hstatus
        case 0x602: // hedeleg
        case 0x603: // hideleg
        case 0x604: // hie
        case 0x606: // hcounteren
        case 0x607: // hgeie

        // Hypervisor Configuration (HRW)
        case 0x60a: // henvcfg
        case 0x61a: // henvcfgh

        // Hypervisor Protection and Translation (HRW)
        case 0x680: // hgatp

        // Debug/Trace Registers
        case 0x6a8: // hcontext

        // Hypervisor Counter/Timer Virtualization Registers (HRW)
        case 0x605: // htimedelta
        case 0x615: // hitimedeltah (32-bit only)
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        default:            break;
        }

        if ((addr.raw >= 0xc03) && (addr.raw <= 0xc1f)) {
            // hpmcounter3...31 (URO)
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;
        }

        // rv32 only
        if (c->mode != RV_MODE_RV32) goto undef;
        if ((addr.raw >= 0xc83) && (addr.raw <= 0xc9f)) {
            // hpmcounter3h...31h (URO)
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;
        }
        goto undef;
    }

    // supervisor level
    if (addr.f.level == RV_PL_SUPERVISOR) {
        rv_sr_pl_t *r = rv_get_pl_csrs(c, RV_PL_HYPERVISOR);

        switch (addr.raw) {
        // Supervisor Trap Setup (SRW)
        case RV_CSR_SSTATUS:    result = rv_status_csr(c, op, value);               goto out;
        case RV_CSR_SIE:        result = rv_csr_update(c, op, &r->ie, value);       goto out;
        case RV_CSR_STVEC:      result = rv_csr_update(c, op, &r->tvec, value);     goto out;
        case RV_CSR_SCOUNTEREN: result = rv_csr_update(c, op, &r->counteren, value);    goto out;

        // Supervisor Configuration (SRW)
        case RV_CSR_SENVCFG: // senvcfg
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        // Supervisor Trap Handling
        case RV_CSR_SSCRATCH:   result = rv_csr_update(c, op, &r->scratch, value);  goto out;
        case RV_CSR_SEPC:       result = rv_csr_update(c, op, &r->epc, value);      goto out;
        case RV_CSR_SCAUSE:     result = rv_csr_update(c, op, &r->cause, value);    goto out;
        case RV_CSR_STVAL:      result = rv_csr_update(c, op, &r->tval, value);     goto out;
        case RV_CSR_SIP:        result = rv_csr_update(c, op, &r->ip, value);       goto out;

        // Supervisor Protection and Translation (SRW)
        case RV_CSR_SATP: // satp
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        // Debug/Trace Registers (SRW)
        case RV_CSR_SCONTEXT: // scontext
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        default:
            goto undef;
        }
    }

    // user level
    switch (addr.raw) {
    // Unprivileged Floating-Point CSRs (URW)
    case 0x001: // fflags
    case 0x002: // frm
    case 0x003: // fcsr
        result.err = SL_ERR_UNIMPLEMENTED;
        goto out;

    // Unprivileged Counter/Timers (URO)
    case 0xc00: // cycle
    case 0xc01: // time
    case 0xc02: // instret
        result.err = SL_ERR_UNIMPLEMENTED;
        goto out;

    // rv32 only (URO)
    case 0xc80: // cycleh
    case 0xc81: // timeh
    case 0xc82: // instreth
        if (c->mode != RV_MODE_RV32) goto undef;
        result.err = SL_ERR_UNIMPLEMENTED;
        goto out;

    default:
        goto undef;
    }

undef:
    if (c->ext.csr_op != NULL) result = c->ext.csr_op(c, op, csr, value);
    else result.err = SL_ERR_UNDEF;

out:
    return result;
}
