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

#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sl/common.h>
#include <sl/core.h>
#include <sled/arch.h>
#include <sled/error.h>

static int core_accept_irq(irq_endpoint_t *ep, uint32_t num, bool high) {
    core_t *c = containerof(ep, core_t, irq_ep);

    lock_lock(&c->lock);
    int err = irq_endpoint_assert(ep, num, high);
    // if (!err) c->pending_irq = irq_endpoint_get_active(ep);
    lock_unlock(&c->lock);

    return err;
}

int core_init(core_t *c, core_params_t *p, bus_t *b) {
    c->arch = p->arch;
    c->subarch = p->subarch;
    c->options = p->options;
    c->arch_options = p->arch_options;
    c->id = p->id;
    c->bus = b;
    c->irq_ep.assert = core_accept_irq;
    lock_init(&c->lock);
    irq_endpoint_set_enabled(&c->irq_ep, IRQ_VEC_ALL);
    return 0;
}

int core_shutdown(core_t *c) {
    lock_destroy(&c->lock);
    return 0;
}

const core_ops_t * core_get_ops(core_t *c) {
    return &c->ops;
}

uint64_t core_get_cycles(core_t *c) {
    return c->ticks;
}

void core_interrupt_set(core_t *c, bool enable) {
    uint32_t bit = (1u << CORE_STATE_INTERRUPTS_EN);
    if (enable) c->state |= bit;
    else c->state &= ~bit;
}

int core_endian_set(core_t *c, bool big) {
    c->ops.set_state(c, CORE_OPT_ENDIAN_BIG, big);
    return 0;
}

void core_instruction_barrier(core_t *c) {
    atomic_thread_fence(memory_order_acquire);
}

void core_memory_barrier(core_t *c, uint32_t type) {
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

int core_mem_read(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    bus_t *b = c->bus;
    if (b == NULL) return SL_ERR_BUS;
    return bus_read(b, addr, size, count, buf);
}

int core_mem_write(core_t *c, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    bus_t *b = c->bus;
    if (b == NULL) return SL_ERR_BUS;
    return bus_write(b, addr, size, count, buf);
}

void core_set_reg(core_t *c, uint32_t reg, uint64_t value) {
    c->ops.set_reg(c, reg, value);
}

uint64_t core_get_reg(core_t *c, uint32_t reg) {
    return c->ops.get_reg(c, reg);
}

int core_step(core_t *c, uint32_t num) {
    return c->ops.step(c, num);
}

int core_run(core_t *c) {
    return c->ops.run(c);
}

int core_set_state(core_t *c, uint32_t state, bool enabled) {
    return c->ops.set_state(c, state, enabled);
}

