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

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include <sled/error.h>
#include <sl/riscv/csr.h>
#include <sl/riscv/inst.h>
#include <sl/riscv/rv.h>

static void rv_dump_core_state(rv_core_t *c) {
    printf("pc = %"PRIx64", sp = %"PRIx64", ra = %"PRIx64"\n", c->pc, c->r[RV_SP], c->r[RV_RA]);
    for (uint32_t i = 0; i < 32; i += 4) {
        if (i < 10) printf(" ");
        printf("x%u: %16"PRIx64"  %16"PRIx64"  %16"PRIx64"  %16"PRIx64"\n", i, c->r[i], c->r[i+1], c->r[i+2], c->r[i+3]);
    }
}

rv_sr_level_t* rv_get_level_csrs(rv_core_t *c, uint8_t level) {
    assert(c->level != 0);
    return &c->sr_mode[level - 1];
}

int rv_exception_enter(rv_core_t *c, uint64_t cause, uint64_t addr) {
    rv_sr_level_t *r = rv_get_level_csrs(c, RV_PRIV_LEVEL_MACHINE);
    r->cause = cause;
    r->epc = c->pc;
    r->tval = addr;

    // update status register
    csr_status_t s;
    s.raw = c->status;
    s.m_mpie = s.m_mie;           // previous interrupt state
    s.m_mie = 0;                  // disable interrupts
    s.m_mpp = c->level;           // previous priv level
    c->status = s.raw;

    c->level = RV_PRIV_LEVEL_MACHINE;      // todo: fix me

    uint64_t tvec = r->tvec;
    if (cause & RV_CAUSE_INT64) {
        const uint64_t ci = cause & ~RV_CAUSE_INT64;
        // m->ip = 1ull << ci;
        if (tvec & 1) tvec += (ci << 2);
    }
    c->pc = tvec;
    c->jump_taken = 1;
    core_interrupt_set(&c->core, false);
    return 0;
}

int rv_exception_return(rv_core_t *c) {
    bool int_enabled = true;
    uint8_t dest_pl;
    csr_status_t s;
    s.raw = c->status;

    if (c->level == RV_PRIV_LEVEL_MACHINE) {
        // mret
        dest_pl = s.m_mpp;
        s.m_mie = s.m_mpie;
        s.m_mpie = 1;
        s.m_mpp = 0;
        int_enabled = s.m_mie;
    } else {
        // todo: sret
        return SL_ERR_UNIMPLEMENTED;
    }
    c->status = s.raw;

    rv_sr_level_t *r = rv_get_level_csrs(c, c->level);
    c->pc = r->epc;
    c->level = dest_pl;
    c->jump_taken = 1;

    core_interrupt_set(&c->core, int_enabled);
    return 0;
}

int rv_synchronous_exception(rv_core_t *c, core_ex_t ex, uint64_t value, uint32_t status) {
    rv_inst_t inst;

    // todo: check if nested exception

    switch (ex) {
    case EX_SYSCALL:
        if (c->core.options & CORE_OPT_TRAP_SYSCALL) {
            return SL_ERR_SYSCALL;
        } else {
            return rv_exception_enter(c, RV_EX_CALL_FROM_U + c->level, value);
        }

    case EX_UNDEFINDED:
        if (c->core.options & CORE_OPT_TRAP_UNDEF) {
            inst.raw = (uint32_t)value;
            printf("UNDEFINED instruction %08x (op=%x) at pc=%" PRIx64 "\n", inst.raw, inst.b.opcode, c->pc);
            rv_dump_core_state(c);
            return SL_ERR_UNDEF;
        } else {
            return rv_exception_enter(c, RV_EX_INST_ILLEGAL, value);
        }

    case EX_ABORT_LOAD:
        if (c->core.options & CORE_OPT_TRAP_ABORT) {
            printf("LOAD FAULT (rd) at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            rv_dump_core_state(c);
            return status;
        } else {
            uint32_t fault = RV_EX_LOAD_FAULT;
            if (status == SL_ERR_IO_ALIGN) fault = RV_EX_LOAD_ALIGN;
            return rv_exception_enter(c, fault, value);
        }

    case EX_ABORT_STORE:
        if (c->core.options & CORE_OPT_TRAP_ABORT) {
            printf("STORE FAULT at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            rv_dump_core_state(c);
            return status;
        } else {
            uint32_t fault = RV_EX_STORE_FAULT;
            if (status == SL_ERR_IO_ALIGN) fault = RV_EX_STORE_ALIGN;
            return rv_exception_enter(c, fault, value);
        }

    case EX_ABORT_INST:
        if (c->core.options & CORE_OPT_TRAP_PREFETCH_ABORT) {
            printf("PREFETCH FAULT at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            rv_dump_core_state(c);
            return status;
        } else {
            uint32_t fault = RV_EX_INST_FAULT;
            if (status == SL_ERR_IO_ALIGN) fault = RV_EX_INST_ALIGN;
            return rv_exception_enter(c, fault, value);
        }

    default:
        printf("\nUNHANDLED EXCEPTION type %u\n", ex);
        return SL_ERR_UNIMPLEMENTED;
    }
}
