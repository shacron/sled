// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>

#include <core/arch.h>
#include <core/core.h>
#include <core/ex.h>
#include <sled/error.h>

int sl_core_synchronous_exception(sl_core_t *c, u8 ex, u8 value, u4 status) {
    u4 inst;

    // todo: check if nested exception

    const arch_ops_t *ops = c->arch_ops;

    switch (ex) {
    case EX_SYSCALL:
        if (c->options & SL_CORE_OPT_TRAP_SYSCALL) return SL_ERR_SYSCALL;
        return ops->exception_enter(c, ex, value);

    case EX_UNDEFINDED:
        if (c->options & SL_CORE_OPT_TRAP_UNDEF) {
            inst = (u4)value;
            printf("UNDEFINED instruction %08x at pc=%" PRIx64 "\n", inst, c->pc);
            sl_core_dump_state(c);
            return SL_ERR_UNDEF;
        } else {
            return ops->exception_enter(c, ex, value);
        }

    case EX_ABORT_LOAD:
        if (c->options & SL_CORE_OPT_TRAP_ABORT) {
            printf("LOAD FAULT (rd) at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            sl_core_dump_state(c);
            return status;
        } else {
            if (status == SL_ERR_IO_ALIGN) ex = EX_ABORT_LOAD_ALIGN;
            return ops->exception_enter(c, ex, value);
        }

    case EX_ABORT_STORE:
        if (c->options & SL_CORE_OPT_TRAP_ABORT) {
            printf("STORE FAULT at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            sl_core_dump_state(c);
            return status;
        } else {
            if (status == SL_ERR_IO_ALIGN) ex = EX_ABORT_STORE_ALIGN;
            return ops->exception_enter(c, ex, value);
        }

    case EX_ABORT_INST:
        if (c->options & SL_CORE_OPT_TRAP_PREFETCH_ABORT) {
            printf("PREFETCH FAULT at addr=%" PRIx64 ", pc=%" PRIx64 ", err=%s\n", value, c->pc, st_err(status));
            sl_core_dump_state(c);
            return status;
        } else {
            if (status == SL_ERR_IO_ALIGN) ex = EX_ABORT_INST_ALIGN;
            return ops->exception_enter(c, ex, value);
        }

    default:
        printf("\nUNHANDLED EXCEPTION type %" PRIx64 "\n", ex);
        return SL_ERR_UNIMPLEMENTED;
    }
}
