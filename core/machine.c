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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/bus.h>
#include <core/dev/intc.h>
#include <core/mem.h>
#include <core/riscv.h>
#include <sled/arch.h>
#include <sled/elf.h>
#include <sled/error.h>
#include <sled/machine.h>

#define MACHINE_MAX_CORES   4

struct machine {
    bus_t *bus;
    device_t *intc;
    core_t *core_list[MACHINE_MAX_CORES];
};

int machine_create(machine_t **m_out) {
    machine_t *m = calloc(1, sizeof(machine_t));
    if (m == NULL) {
        fprintf(stderr, "failed to allocate machine\n");
        return SL_ERR_MEM;
    }

    int err;
    if ((err = bus_create(&m->bus))) {
        free(m);
        return err;
    }
    *m_out = m;
    return 0;
}

int machine_add_mem(machine_t *m, uint64_t base, uint64_t size) {
    mem_region_t mem;
    int err;

    if ((err = mem_region_create(&mem, base, size))) {
        fprintf(stderr, "mem_region_create failed: %s\n", st_err(err));
        return err;
    }
    if ((err = bus_add_mem_region(m->bus, mem))) {
        fprintf(stderr, "bus_add_mem_region failed: %s\n", st_err(err));
        mem_region_destroy(&mem);
    }
    return err;
}

int machine_add_device(machine_t *m, uint32_t type, uint64_t base, const char *name) {
    device_t *d;
    int err;

    if ((err = device_create(type, name, &d))) {
        fprintf(stderr, "device_create failed: %s\n", st_err(err));
        goto out_err;
    }

    if ((err = bus_add_device(m->bus, d, base))) {
        fprintf(stderr, "bus_add_device failed: %s\n", st_err(err));
        d->destroy(d);
    }
    if (type == DEVICE_INTC) m->intc = d;

out_err:
    return err;
}

int machine_add_device_prefab(machine_t *m, uint64_t base, device_t *d) {
    int err;
    if ((err = bus_add_device(m->bus, d, base))) {
        fprintf(stderr, "bus_add_device failed: %s\n", st_err(err));
        d->destroy(d);
    }
    return err;
}


int machine_add_core(machine_t *m, core_params_t *opts) {
    core_t *c;
    int err;

    switch (opts->arch) {
    case ARCH_RISCV:
        err = riscv_core_create(opts, m->bus, &c);
        break;

    default:
        fprintf(stderr, "machine_add_core: unsupported architecture\n");
        return SL_ERR_ARG;
    }

    if (err) {
        fprintf(stderr, "core create failed: %s\n", st_err(err));
        return err;
    }

    for (int i = 0; i < MACHINE_MAX_CORES; i++) {
        if (m->core_list[i] == NULL) {
            m->core_list[i] = c;
            opts->id = i;
            irq_endpoint_set_client(&m->intc->irq_ep, &c->irq_ep, 11); // todo: get proper irq number
            return 0;
        }
    }
    c->ops.destroy(c);
    return SL_ERR_FULL;
}

core_t * machine_get_core(machine_t *m, uint32_t id) {
    if (id >= MACHINE_MAX_CORES) return NULL;
    return m->core_list[id];
}

device_t * machine_get_device_for_name(machine_t *m, const char *name) {
    return bus_get_device_for_name(m->bus, name);
}

int machine_set_interrupt(machine_t *m, uint32_t irq, bool high) {
    if (m->intc == NULL) return SL_ERR_NODEV;
    return irq_endpoint_assert(&m->intc->irq_ep, irq, high);
}

void machine_destroy(machine_t *m) {
    for (int i = 0; i < MACHINE_MAX_CORES; i++) {
        core_t *c = m->core_list[i];
        if (c != NULL) c->ops.destroy(c);
    }
    bus_destroy(m->bus);
    free(m);
}

int machine_load_core(machine_t *m, uint32_t id, elf_object_t *o) {
    core_t *c = machine_get_core(m, id);
    if (c == NULL) {
        fprintf(stderr, "invalid core id: %u\n", id);
        return SL_ERR_ARG;
    }

    int err;
    const bool is64 = elf_is_64bit(o);

    for (uint32_t i = 0; ; i++) {
        void *vph = elf_get_program_header(o, i);
        if (vph == NULL) break;

        Elf64_Word type;
        Elf64_Xword memsz, filesz;
        Elf64_Addr vaddr;
        Elf64_Off offset;
        if (is64) {
            Elf64_Phdr *ph = vph;
            type = ph->p_type;
            memsz = ph->p_memsz;
            vaddr = ph->p_vaddr;
            offset = ph->p_offset;
            filesz = ph->p_filesz;
        } else {
            Elf32_Phdr *ph = vph;
            type = ph->p_type;
            memsz = ph->p_memsz;
            vaddr = ph->p_vaddr;
            offset = ph->p_offset;
            filesz = ph->p_filesz;
        }

        // TODO: load data, init BSS

        // load PT_LOAD with X|W|R flags
        if (type != PT_LOAD) continue;
        if (memsz == 0) continue;
        void *p = elf_pointer_for_offset(o, offset);
        if ((err = core_mem_write(c, vaddr, 1, filesz, p))) {
            fprintf(stderr, "failed to load core memory: %s\n", st_err(err));
            fprintf(stderr, "  vaddr=%#" PRIx64 ", filesz=%#" PRIx64 "\n", vaddr, filesz);
            goto out_err;
        }
    }

    uint64_t entry = elf_get_entry(o);
    if (entry == 0) {
        fprintf(stderr, "no entry address for binary\n");
        err = SL_ERR_ARG;
        goto out_err;
    }

    if ((err = c->ops.set_state(c, CORE_STATE_64BIT, is64))) {
        fprintf(stderr, "failed to set core 64-bit state\n");
        goto out_err;
    }
    c->ops.set_reg(c, CORE_REG_PC, entry);
    err = 0;

out_err:
    return err;
}
