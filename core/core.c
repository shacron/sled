// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <stdatomic.h>
#include <stdio.h>

#include <core/device.h>
#include <core/core.h>
#include <core/mapper.h>
#include <core/sym.h>
#include <sled/arch.h>
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
    return c->ops->set_state(c, SL_CORE_STATE_ENDIAN_BIG, big);
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

int sl_core_set_state(sl_core_t *c, u4 state, bool enabled) {
    return c->ops->set_state(c, state, enabled);
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

int sl_core_init(sl_core_t *c, sl_core_params_t *p, sl_mapper_t *m) {
    c->mapper = m;
    c->el = SL_CORE_EL_MONITOR;
    c->mode = SL_CORE_MODE_32;
    config_set_internal(c, p);
    sl_engine_init(&c->engine, "core_eng", NULL);
    sl_irq_endpoint_set_enabled(&c->engine.irq_ep, SL_IRQ_VEC_ALL);
    return 0;
}

void sl_core_shutdown(sl_core_t *c) {
    c->ops->shutdown(c);
    sl_engine_shutdown(&c->engine);
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
