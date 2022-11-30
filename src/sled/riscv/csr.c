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

#include <stdbool.h>
#include <stdio.h>

#include <sl/core.h>
#include <sled/error.h>
#include <sl/riscv/csr.h>
#include <sl/riscv/rv.h>

// plain register modification with no side effects
result64_t rv_csr_update(rv_core_t *c, int op, uint64_t *reg, uint64_t update_value) {
    result64_t result = {};
    switch (op) {
    case RV_CSR_OP_WRITE:
        *reg = update_value;
        break;

    case RV_CSR_OP_READ:
        result.value = *reg;
        break;

    case RV_CSR_OP_SWAP:
        result.value = *reg;
        *reg = update_value;
        break;

    case RV_CSR_OP_READ_SET:
        result.value = *reg;
        *reg |= update_value;
        break;

    case RV_CSR_OP_READ_CLEAR:
        result.value = *reg;
        *reg &= ~update_value;
        break;

    default:
        result.err = SL_ERR_UNDEF;
        break;
    }
    return result;
}

static uint64_t rv_status_internal_to_64(uint64_t is) {
    return ((is & RV_SR_STATUS_SD) << 32) | (is & ~(RV_SR_STATUS_SD));
}

static uint64_t rv_status64_to_internal(uint64_t s) {
    return ((s & RV_SR_STATUS64_SD) >> 32) | (s & ~(RV_SR_STATUS64_SD));
}

static result64_t rv_mstatus_csr(rv_core_t *c, int op, uint64_t update_value) {
    result64_t result = {};

    // Our status register is 64-bits, but keeps SD in bit 32 for compatibility
    // with the 32 bit version.

    if (op == RV_CSR_OP_READ) {
        result.value = c->status;
        goto fixup;
    }

    if (c->mode == RV_MODE_RV64) {
        update_value = rv_status64_to_internal(update_value);
    }

    const uint64_t wpri_mask = (RV_SR_STATUS_WPRI0 | RV_SR_STATUS_WPRI1 | RV_SR_STATUS_WPRI2 | (0xff << RV_SR_STATUS_BIT_WPRI3) | (0x1fffffful << RV_SR_STATUS_BIT_WPRI4));

    update_value &= ~wpri_mask;
    uint64_t changed_bits;

    switch (op) {
    case RV_CSR_OP_READ_SET:
        changed_bits = (~c->status) & update_value;
        if (changed_bits & RV_SR_STATUS_MIE) core_interrupt_set(&c->core, true);
        if (changed_bits & RV_SR_STATUS_UBE) core_endian_set(&c->core, true);
        // todo: other fields
        c->status |= update_value;
        break;

    case RV_CSR_OP_READ_CLEAR:
        changed_bits = c->status & update_value;
        if (changed_bits & RV_SR_STATUS_MIE) core_interrupt_set(&c->core, false);
        if (changed_bits & RV_SR_STATUS_UBE) core_endian_set(&c->core, false);
        // todo: other fields
        c->status &= ~update_value;
        break;

    case RV_CSR_OP_SWAP:
        result.value = c->status;
        // fall through
    case RV_CSR_OP_WRITE:
        changed_bits = c->status ^ update_value;
        if (changed_bits & RV_SR_STATUS_MIE) core_interrupt_set(&c->core, (bool)(update_value & RV_SR_STATUS_MIE));
        if (changed_bits & RV_SR_STATUS_UBE) core_endian_set(&c->core, (bool)(update_value & RV_SR_STATUS_UBE));
        // todo: other fields
        c->status = update_value;
        break;

    default:
        result.err = SL_ERR_UNDEF;
        goto out;
    }

fixup:
    if (c->mode == RV_MODE_RV32) result.value = (uint32_t)result.value;
    else result.value = rv_status_internal_to_64(result.value);
out:
    return result;
}

result64_t rv_mcycle_csr(rv_core_t *c, int op, uint64_t value) {
    result64_t result = {};
    uint64_t ticks = c->core.ticks;

    switch (op) {
    case RV_CSR_OP_READ:
        result.value = ticks - c->mcycle_offset;
        break;

    case RV_CSR_OP_SWAP:
        result.value = ticks - c->mcycle_offset;
        // fall through
    case RV_CSR_OP_WRITE:
        c->mcycle_offset = ticks - value;
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

result64_t rv_mcinstret_csr(rv_core_t *c, int op, uint64_t value) {
    result64_t result = {};
    uint64_t ticks = c->core.ticks;

    switch (op) {
    case RV_CSR_OP_READ:
        result.value = ticks - c->minstret_offset;
        break;

    case RV_CSR_OP_SWAP:
        result.value = ticks - c->minstret_offset;
        // fall through
    case RV_CSR_OP_WRITE:
        c->minstret_offset = ticks - value;
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

    if (addr.f.level > c->priv_level) goto undef;
    if (op != RV_CSR_OP_READ) {
        if (addr.f.type == 3) goto undef;
    }

    if (addr.f.level == RV_PRIV_LEVEL_MACHINE) {
        rv_sr_mode_t *m = rv_get_mode_registers(c, RV_PRIV_LEVEL_MACHINE);

        // machine level
        switch (addr.raw) {
        // Machine Information Registers (MRO)
        case 0xF11: // MRO mvendorid - Vendor ID.
            result.value = c->mvendorid;  goto out;
        case 0xF12: // MRO marchid - Architecture ID.
            result.value = c->marchid;    goto out;
        case 0xF13: // MRO mimpid - Implementation ID.
            result.value = c->mimpid;     goto out;
        case 0xF14: // MRO mhartid - Hardware thread ID.
            result.value = c->mhartid;    goto out;
        case 0xF15: // MRO mconfigptr - Pointer to configuration data structure.
            result.value = c->mconfigptr; goto out;

        // Machine Trap Setup (MRW)
        case 0x300: // MRW mstatus - Machine status register.
            result = rv_mstatus_csr(c, op, value);
            goto out;

        case 0x301: // MRW misa - ISA and extensions
            result = rv_csr_update(c, op, &m->isa, value);
            goto out;

        case 0x302: // MRW medeleg - Machine exception delegation register.
            result = rv_csr_update(c, op, &m->edeleg, value);
            goto out;

        case 0x303: // MRW mideleg - Machine interrupt delegation register.
            result = rv_csr_update(c, op, &m->ideleg, value);
            goto out;

        case 0x304: // MRW mie - Machine interrupt-enable register.
            result = rv_csr_update(c, op, &m->ie, value);
            goto out;

        case 0x305: // MRW mtvec - Machine trap-handler base address.
            result = rv_csr_update(c, op, &m->tvec, value);
            goto out;

        case 0x306: // MRW mcounteren - Machine counter enable.
            result = rv_csr_update(c, op, &m->counteren, value);
            goto out;

        case 0x30A: // MRW menvcfg - Machine environment configuration register.
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case 0x310: // MRW mstatush - Additional machine status register, RV32 only.
            if (c->mode != RV_MODE_RV32) goto undef;
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case 0x31A: // MRW menvcfgh - Additional machine env. conf. register, RV32 only.
            if (c->mode != RV_MODE_RV32) goto undef;
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case 0x340: // MRW mscratch = Scratch register for machine trap handlers.
            result = rv_csr_update(c, op, &m->scratch, value);
            goto out;

        case 0x341: // MRW mepc = Machine exception program counter.
            result = rv_csr_update(c, op, &m->epc, value);
            goto out;

        case 0x342: // MRW mcause = Machine trap cause.
            result = rv_csr_update(c, op, &m->cause, value);
            goto out;

        case 0x343: // MRW mtval = Machine bad address or instruction.
            result = rv_csr_update(c, op, &m->tval, value);
            goto out;

        case 0x344: // MRW mip = Machine interrupt pending.
            result = rv_csr_update(c, op, &m->ip, value);
            goto out;

        case 0x34A: // MRW mtinst = Machine trap instruction (transformed).
        case 0x34B: // MRW mtval2 = Machine bad guest physical address.
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        // Machine Configuration
        case 0x747: // MRW mseccfg - Machine security configuration register.
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case 0x757: // MRW mseccfgh - Additional machine security conf. register, RV32 only.
            if (c->mode != RV_MODE_RV32) goto undef;
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case 0xB00: // MRW mcycle - Machine cycle counter
            result = rv_mcycle_csr(c, op, value);
            goto out;

        case 0xB02: // MRW minstret - Machine instructions-retired counter.
            result = rv_mcinstret_csr(c, op, value);
            goto out;

        default:
            break;
        }

        // MRW pmpcfg0-15 - Physical memory protection configuration.
        if ((addr.raw >= 0x3A0) && (addr.raw <= 0x3AF)) {
            const uint32_t i = addr.raw - 0x3A0;
            result = rv_csr_update(c, op, &c->pmpcfg[i], value);
            goto out;
        }

        // MRW pmpaddr0-63 - Physical memory protection address register.
        if ((addr.raw >= 0x3B0) && (addr.raw <= 0x3EF)) {
            const uint32_t i = addr.raw - 0x3B0;
            result = rv_csr_update(c, op, &c->pmpaddr[i], value);
            goto out;
        }

        goto undef;
    }

    // hypervisor level
    if (addr.f.level == RV_PRIV_LEVEL_HYPERVISOR) {
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

        default:
            break;
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
    if (addr.f.level == RV_PRIV_LEVEL_SUPERVISOR) {
        rv_sr_mode_t *m = rv_get_mode_registers(c, RV_PRIV_LEVEL_HYPERVISOR);

        switch (addr.raw) {
        case 0x140: // sscratch
            result = rv_csr_update(c, op, &m->scratch, value);
            goto out;

        case 0x141: // sepc
            result = rv_csr_update(c, op, &m->epc, value);
            goto out;

        case 0x142: // scause
            result = rv_csr_update(c, op, &m->cause, value);
            goto out;

        case 0x143: // stval
            result = rv_csr_update(c, op, &m->tval, value);
            goto out;

        case 0x144: // sip
            result = rv_csr_update(c, op, &m->ip, value);
            goto out;

        // Supervisor Trap Setup (SRW)
        case 0x100: // sstatus
        case 0x104: // sie
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        case 0x105: // stvec
            result = rv_csr_update(c, op, &m->tvec, value);
            goto out;

        case 0x106: // scounteren
        // Supervisor Configuration (SRW)
        case 0x10a: // senvcfg
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        // Supervisor Protection and Translation (SRW)
        case 0x180: // satp
            result.err = SL_ERR_UNIMPLEMENTED;
            goto out;

        // Debug/Trace Registers (SRW)
        case 0x5a8: // scontext
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
