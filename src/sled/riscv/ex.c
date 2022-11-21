#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include <sled/error.h>
#include <sl/riscv/csr.h>
#include <sl/riscv/inst.h>
#include <sl/riscv/rv.h>

static void rv_dump_core_state(rv_core_t *c) {
    printf("pc = %" PRIx64 "\n", c->pc);
    printf("sp = %" PRIx64 "\n", c->r[RV_SP]);
    printf("ra = %" PRIx64 "\n\n", c->r[RV_RA]);

    for (int i = 0; i < 32; i++) {
        printf("x%i %" PRIx64 "\n", i, c->r[i]);
    }
}

rv_sr_mode_t* rv_get_mode_registers(rv_core_t *c, uint8_t priv_level) {
    assert(c->priv_level != 0);
    return &c->sr_mode[priv_level - 1];
}

int rv_exception_enter(rv_core_t *c, uint64_t cause, bool irq) {
    rv_sr_mode_t *m = rv_get_mode_registers(c, RV_PRIV_LEVEL_MACHINE);
    m->epc = c->pc;
    m->tval = 0;
    m->ip = 0;
    if (irq) {
        m->ip = 1;
        if (c->mode == RV_MODE_RV64) cause |= (1ull << 63);
        else                         cause |= (1u << 31);
    }
    m->cause = cause;

    // update status register
    csr_status_t s;
    s.raw = c->status;
    s.m_mpie = s.m_mie;           // previous interrupt state
    s.m_mie = 0;                  // disable interrupts

    s.m_mpp = c->priv_level;      // previous priv level
    c->status = s.raw;

    c->priv_level = RV_PRIV_LEVEL_MACHINE;      // todo: fix me
    c->pc = m->tvec;

    core_interrupt_set(&c->core, false);
    return 0;
}

int rv_exception_return(rv_core_t *c) {
    bool int_enabled = true;
    uint8_t dest_pl;
    csr_status_t s;
    s.raw = c->status;

    if (c->priv_level == RV_PRIV_LEVEL_MACHINE) {
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

    rv_sr_mode_t *m = rv_get_mode_registers(c, c->priv_level);
    c->pc = m->epc;
    c->priv_level = dest_pl;
    c->jump_taken = 1;

    core_interrupt_set(&c->core, int_enabled);
    return 0;
}

int rv_synchronous_exception(rv_core_t *c, core_ex_t ex, uint64_t value, uint32_t status) {
    rv_inst_t inst;

    // todo: check if nested exception

    switch (ex) {
    case EX_SYSCALL:
        // printf("\nSYSCALL %" PRIx64 "\n", value);
        if (c->core.options & CORE_OPT_TRAP_SYSCALL)
            return SL_ERR_SYSCALL;
        return SL_ERR_UNIMPLEMENTED;
        // return rv_syscall(c);

    case EX_UNDEFINDED:
        inst.raw = (uint32_t)value;
        printf("UNDEFINED instruction %08x (op=%x) at pc=%" PRIx64 "\n", inst.raw, inst.b.opcode, c->pc);
        rv_dump_core_state(c);
        return SL_ERR_UNDEF;

    case EX_ABORT_READ:
        printf("DATA ABORT (rd) at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
        rv_dump_core_state(c);
        return status;

    case EX_ABORT_WRITE:
        printf("DATA ABORT (wr) at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
        rv_dump_core_state(c);
        return status;

    default:
        printf("\nUNHANDLED EXCEPTION type %u\n", ex);
        return SL_ERR_UNIMPLEMENTED;
    }
}
