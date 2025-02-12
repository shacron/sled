// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <assert.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>

#include <core/arch.h>
#include <core/device.h>
#include <core/core.h>
#include <core/mapper.h>
#include <core/sym.h>
#include <sled/error.h>
#include <sled/io.h>

int sl_core_async_command(sl_core_t *c, u4 cmd, bool wait) {
    return sl_engine_async_command(&c->engine, cmd, wait);
}

u1 sl_core_get_arch(sl_core_t *c) {
    return c->arch;
}

void sl_core_config_get(sl_core_t *c, sl_core_params_t *p) {
    p->arch = c->arch;
    p->subarch = c->subarch;
    p->id = c->id;
    p->options = c->options;
    p->arch_options = c->arch_options;
    p->name = c->name;
}

static void config_set_internal(sl_core_t *c, sl_core_params_t *p) {
    c->arch = p->arch;
    c->subarch = p->subarch;
    c->id = p->id;
    c->options = p->options;
    c->arch_options = p->arch_options;
    c->name = p->name;
    c->arch_ops = arch_get_ops(c->arch);
}

int sl_core_config_set(sl_core_t *c, sl_core_params_t *p) {
    if (c->arch != p->arch) return SL_ERR_ARG;
    config_set_internal(c, p);
    return 0;
}

void sl_core_print_config(sl_core_t *c) {
    printf("core '%s'\n", c->name);
    printf("  arch: %s\n", sl_arch_name(c->arch));
    printf("  subarch: %u\n", c->subarch);
    printf("  options: %x\n", c->options);
    printf("  arch_options: %x\n", c->arch_options);
}

const core_ops_t * core_get_ops(sl_core_t *c) {
    return c->ops;
}

u8 sl_core_get_cycles(sl_core_t *c) {
    return c->ticks;
}

void sl_core_interrupt_set(sl_core_t *c, bool enable) {
    sl_engine_interrupt_set(&c->engine, enable);
}

int sl_core_endian_set(sl_core_t *c, bool big) {
    if (big) c->state |= SL_CORE_STATE_ENDIAN_BIG;
    else c->state &= ~SL_CORE_STATE_ENDIAN_BIG;
    return 0;
}

void sl_core_instruction_barrier(sl_core_t *c) {
    atomic_thread_fence(memory_order_acquire);
}

void sl_core_memory_barrier(sl_core_t *c, u4 type) {
    // currently ignoring system and sync
    type &= (BARRIER_LOAD | BARRIER_STORE);
    switch (type) {
    case 0: return;
    case BARRIER_LOAD:
        atomic_thread_fence(memory_order_acquire);
        break;
    case BARRIER_STORE:
        atomic_thread_fence(memory_order_release);
        break;
    case (BARRIER_LOAD | BARRIER_STORE):
        atomic_thread_fence(memory_order_acq_rel);
        break;
    }
}

int sl_core_mem_read(sl_core_t *c, u8 addr, u4 size, u4 count, void *buf) {
    sl_io_op_t op;
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.op = IO_OP_IN;
    op.align = 1;
    op.buf = buf;
    op.agent = c;
    return sl_mapper_io(c->mapper, &op);
}

int sl_core_mem_write(sl_core_t *c, u8 addr, u4 size, u4 count, void *buf) {
    sl_io_op_t op;
    op.addr = addr;
    op.count = count;
    op.size = size;
    op.op = IO_OP_OUT;
    op.align = 1;
    op.buf = buf;
    op.agent = c;
    return sl_mapper_io(c->mapper, &op);
}

int sl_core_mem_atomic(sl_core_t *c, u8 addr, u4 size, u1 aop, u8 arg0, u8 arg1, u8 *result, u1 ord, u1 ord_fail) {
    sl_io_op_t op;
    op.addr = addr;
    op.size = size;
    op.op = aop;
    op.align = 1;
    op.order = ord;
    op.order_fail = ord_fail;
    op.arg[0] = arg0;
    op.arg[1] = arg1;
    op.agent = c;
    int err = sl_mapper_io(c->mapper, &op);
    if (err) return err;
    *result = op.arg[0];
    return 0;
}

void sl_core_set_reg(sl_core_t *c, u4 reg, u8 value) {
    c->ops->set_reg(c, reg, value);
}

u8 sl_core_get_reg(sl_core_t *c, u4 reg) {
    return c->ops->get_reg(c, reg);
}

int sl_core_set_mapper(sl_core_t *c, sl_dev_t *d) {
    sl_mapper_t *m = sl_device_get_mapper(d);
    m->next = c->mapper;
    c->mapper = m;

    u4 id;
    int err = sl_worker_add_event_endpoint(c->engine.worker, &d->event_ep, &id);
    if (err) return err;
    sl_device_set_worker(d, c->engine.worker, id);
    return id;
}

int sl_core_step(sl_core_t *c, u8 num) {
    return sl_engine_step(&c->engine, num);
}

int sl_core_run(sl_core_t *c) {
    return sl_engine_run(&c->engine);
}

void sl_core_set_mode(sl_core_t *c, u1 mode) {
    c->mode = mode;
}

u4 sl_core_get_reg_count(sl_core_t *c, int type) {
    return sl_arch_get_reg_count(c->arch, c->subarch, type);
}

#if WITH_SYMBOLS
void sl_core_add_symbols(sl_core_t *c, sl_sym_list_t *list) {
    list->next = c->symbols;
    c->symbols = list;
}

sl_sym_entry_t *sl_core_get_sym_for_addr(sl_core_t *c, u8 addr) {
    sl_sym_entry_t *nearest = NULL;
    u8 distance = ~0;
    for (sl_sym_list_t *list = c->symbols; list != NULL; list = list->next) {
        for (u8 i = 0; i < list->num; i++) {
            if (list->ent[i].addr > addr) continue;
            if (list->ent[i].addr == addr) return &list->ent[i];
            u8 d = addr - list->ent[i].addr;
            if (d > distance) continue;
            distance = d;
            nearest = &list->ent[i];
        }
    }
    return nearest;
}
#endif

void sl_core_next_pc(sl_core_t *c) {
    c->pc += c->prev_len;
    c->prev_len = 4;       // todo: fix me in decoder
}

int sl_core_load_pc(sl_core_t * restrict c, u4 * restrict inst) {
    // todo extras: check alignment of pc

    for (int i = 0; i < 3; i++) {
        int err = sl_cache_read(&c->icache, c->pc, 4, inst);
        if (err != SL_ERR_NOT_FOUND) return err;

        // filled in both cache pages, all data should be found
        if (i == 2) return SL_ERR_STATE;

        sl_cache_page_t *pg;
        const u8 miss_addr = c->icache.miss_addr;
        if ((err = sl_cache_alloc_page(&c->icache, miss_addr, &pg))) return err;

        const u1 shift = c->icache.page_shift;
        const u8 pg_size = 1u << shift;
        const u8 base = (miss_addr >> shift) << shift;
        if ((err = sl_core_mem_read(c, base, 1, pg_size, pg->buffer))) {
            sl_cache_discard_unfilled_page(&c->icache, pg);
            return err;
        }
        sl_cache_fill_page(&c->icache, pg);
    }
    return 0;
}

int sl_core_init(sl_core_t *c, sl_core_params_t *p, sl_mapper_t *m) {
    c->mapper = m;
    c->el = SL_CORE_EL_MONITOR;
    c->mode = SL_CORE_MODE_4;
    c->prev_len = 0;
    config_set_internal(c, p);
    sl_cache_init(&c->icache);
    sl_engine_init(&c->engine, "core_eng", NULL);
    sl_irq_endpoint_set_enabled(&c->engine.irq_ep, SL_IRQ_VEC_ALL);
    return 0;
}

void sl_core_shutdown(sl_core_t *c) {
    c->ops->shutdown(c);
    sl_engine_shutdown(&c->engine);
    sl_cache_shutdown(&c->icache);
#if WITH_SYMBOLS
    sl_sym_list_t *n = NULL;
    for (sl_sym_list_t *s = c->symbols; s != NULL; s = n) {
        n = s->next;
        sym_free(s);
    }
#endif
}

void sl_core_destroy(sl_core_t *c) {
    c->ops->destroy(c);
}

void sl_core_print_bus_topology(sl_core_t *c) {
    mapper_print_mappings(c->mapper);
}

void sl_core_dump_state(sl_core_t *c) {
    const arch_ops_t *ops = c->arch_ops;
    const u1 sp = ops->reg_index(SL_CORE_REG_SP);
    const u1 lr = ops->reg_index(SL_CORE_REG_LR);
    const char *lr_name = ops->name_for_reg(SL_CORE_REG_LR);

    printf("pc=%" PRIx64 ", sp=%" PRIx64 ", %s=%" PRIx64 ", ticks=%" PRIu64 "\n", c->pc, c->r[sp], lr_name, c->r[lr], c->ticks);
    for (u4 i = 0; i < 32; i += 4) {
        if (i < 10) printf(" ");
        printf("r%u: %16" PRIx64 "  %16" PRIx64 "  %16" PRIx64 "  %16" PRIx64 "\n", i, c->r[i], c->r[i+1], c->r[i+2], c->r[i+3]);
    }
    // todo: dump fp registers
}
