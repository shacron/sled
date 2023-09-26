// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/bus.h>
#include <core/core.h>
#include <core/mem.h>
#include <core/riscv.h>
#include <core/sym.h>
#include <sled/arch.h>
#include <sled/core.h>
#include <sled/elf.h>
#include <sled/error.h>
#include <sled/machine.h>
#include <sled/chrono.h>

#define MACHINE_MAX_CORES   4

typedef struct {
    sl_core_t *core;
    sl_worker_t *worker;
    u4 epid;
    // sl_mapper_t *mapper;
} machine_core_t;

struct sl_machine {
    sl_bus_t *bus;
    sl_dev_t *intc;
    sl_chrono_t *chrono;
    u4 core_count;
    machine_core_t mc[MACHINE_MAX_CORES];
};

extern const void * dyn_dev_ops_list[];

sl_chrono_t * sl_machine_get_chrono(sl_machine_t *m) { return m->chrono; }

static const sl_dev_ops_t * get_ops_for_device(u4 type) {
    for (int i = 0; dyn_dev_ops_list[i] != NULL; i++) {
        const sl_dev_ops_t * const *p = dyn_dev_ops_list[i];
        const sl_dev_ops_t *ops = *p;
        if (ops->type == type) return ops;
    }
    return NULL;
}

static int machine_create_device(sl_machine_t *m, u4 type, const char *name, sl_dev_t **dev_out) {
    const sl_dev_ops_t *ops = get_ops_for_device(type);
    if (ops == NULL) {
        fprintf(stderr, "failed to locate device type %u (%s)\n", type, name);
        return SL_ERR_ARG;
    }

    sl_dev_t *d;
    sl_dev_config_t cfg;
    cfg.machine = m;
    int err = ops->create(name, &cfg, &d);
    if (!err) *dev_out = d;
    return err;
}

int sl_machine_create(sl_machine_t **m_out) {
    sl_machine_t *m = calloc(1, sizeof(sl_machine_t));
    if (m == NULL) {
        fprintf(stderr, "failed to allocate machine\n");
        return SL_ERR_MEM;
    }

    int err;
    sl_dev_config_t cfg;
    cfg.machine = m;
    if ((err = bus_create("bus0", &cfg, &m->bus))) goto out_err;
    if ((err = sl_obj_alloc_init(SL_OBJ_TYPE_CHRONO, "tm0", (void **)&m->chrono))) goto out_err;
    if ((err = sl_chrono_run(m->chrono))) goto out_err;

    *m_out = m;
    return 0;

out_err:
    if (m->chrono != NULL) sl_obj_release(m->chrono);
    if (m->bus != NULL) bus_destroy(m->bus);
    free(m);
    return err;
}

int sl_machine_add_mem(sl_machine_t *m, u8 base, u8 size) {
    mem_region_t *mem;
    int err;

    if ((err = mem_region_create(base, size, &mem))) {
        fprintf(stderr, "mem_region_create failed: %s\n", st_err(err));
        return err;
    }
    if ((err = bus_add_mem_region(m->bus, mem))) {
        fprintf(stderr, "bus_add_mem_region failed: %s\n", st_err(err));
        mem_region_destroy(mem);
    }
    return err;
}

int sl_machine_add_device(sl_machine_t *m, u4 type, u8 base, const char *name) {
    sl_dev_t *d;
    int err;

    if ((err = machine_create_device(m, type, name, &d))) {
        fprintf(stderr, "device_create failed: %s\n", st_err(err));
        return err;
    }

    if ((err = bus_add_device(m->bus, d, base))) {
        fprintf(stderr, "bus_add_device failed: %s\n", st_err(err));
        goto out_err;
    }
    if (type == SL_DEV_INTC) m->intc = d;

out_err:
    sl_obj_release(d);
    return err;
}

int sl_machine_add_device_prefab(sl_machine_t *m, u8 base, sl_dev_t *d) {
    int err;
    if ((err = bus_add_device(m->bus, d, base))) {
        fprintf(stderr, "bus_add_device failed: %s\n", st_err(err));
    }
    return err;
}

int sl_machine_add_core(sl_machine_t *m, sl_core_params_t *opts) {
    if (m->core_count >= MACHINE_MAX_CORES) return SL_ERR_FULL;

    machine_core_t *mc = &m->mc[m->core_count];
    // mc->mapper = bus_get_mapper(m->bus);
    mc->core = NULL;
    mc->worker = NULL;

    int err;
    if ((err = sl_obj_alloc_init(SL_OBJ_TYPE_WORKER, "core_worker", (void **)&mc->worker))) {
        fprintf(stderr, "sl_worker_create failed: %s\n", st_err(err));
        return err;
    }

    switch (opts->arch) {
    case SL_ARCH_RISCV:
        err = riscv_core_create(opts, m->bus, &mc->core);
        break;

    default:
        fprintf(stderr, "sl_machine_add_core: unsupported architecture\n");
        err = SL_ERR_ARG;
        goto out_err;
    }

    if (err) {
        fprintf(stderr, "core create failed: %s\n", st_err(err));
        goto out_err;
    }

    if ((err = sl_worker_add_engine(mc->worker, &mc->core->engine, &mc->epid))) {
        fprintf(stderr, "sl_worker_add_engine failed: %s\n", st_err(err));
        goto out_err;
    }

    opts->id = m->core_count;
    if (m->intc != NULL)
        sl_irq_endpoint_set_client(&m->intc->irq_ep, &mc->core->engine.irq_ep, 11); // todo: get proper irq number
    m->core_count++;
    return 0;

out_err:
    if (mc->worker) sl_obj_release(mc->worker);
    if (mc->core) sl_obj_release(mc->core);
    return err;
}

sl_core_t * sl_machine_get_core(sl_machine_t *m, u4 id) {
    if (id >= m->core_count) return NULL;
    return m->mc[id].core;
}

sl_dev_t * sl_machine_get_device_for_name(sl_machine_t *m, const char *name) {
    return bus_get_device_for_name(m->bus, name);
}

int sl_machine_set_interrupt(sl_machine_t *m, u4 irq, bool high) {
    if (m->intc == NULL) return SL_ERR_IO_NODEV;
    return sl_irq_endpoint_assert(&m->intc->irq_ep, irq, high);
}

void sl_machine_destroy(sl_machine_t *m) {
    for (int i = 0; i < m->core_count; i++) {
        sl_obj_release(m->mc[i].core);
    }
    bus_destroy(m->bus);
    sl_chrono_stop(m->chrono);
    sl_obj_release(m->chrono);
    free(m);
}

int sl_machine_load_core(sl_machine_t *m, u4 id, sl_elf_obj_t *o, bool configure) {
    sl_core_t *c = sl_machine_get_core(m, id);
    if (c == NULL) {
        fprintf(stderr, "invalid core id: %u\n", id);
        return SL_ERR_ARG;
    }

    int err;
    const bool is64 = sl_elf_is_64bit(o);
#if WITH_SYMBOLS
    sl_sym_list_t *sl = NULL;
#endif

    for (u4 i = 0; ; i++) {
        void *vph = sl_elf_get_program_header(o, i);
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

        // load PT_LOAD with X|W|R flags
        if (type != PT_LOAD) continue;
        if (memsz == 0) continue;
        void *p = sl_elf_pointer_for_offset(o, offset);
        if ((err = sl_core_mem_write(c, vaddr, 1, filesz, p))) {
            fprintf(stderr, "failed to load core memory: %s\n", st_err(err));
            fprintf(stderr, "  vaddr=%#" PRIx64 ", filesz=%#" PRIx64 "\n", vaddr, filesz);
            goto out_err;
        }
    }

#if WITH_SYMBOLS
    sl = calloc(1, sizeof(*sl));
    if (sl == NULL) {
        printf("failed to allocate symbol list\n");
        goto out_err;
    }
    if ((err = elf_read_symbols(o, sl))) {
        printf("failed to read symbols\n");
        goto out_err;
    }

    sl_core_add_symbols(c, sl);
#endif

    err = 0;
    if (!configure) goto out_err;

    sl_core_params_t params = {};
    sl_core_config_get(c, &params);

    if (params.arch != sl_elf_arch(o)) {
        fprintf(stderr, "elf architecture does not match core\n");
        err = SL_ERR_ARG;
        goto out_err;
    }
    params.subarch = sl_elf_subarch(o);
    params.arch_options = sl_elf_arch_options(o);

    if ((err = sl_core_config_set(c, &params))) {
        fprintf(stderr, "sl_core_config_set failed: %s\n", st_err(err));
        goto out_err;
    }

    u8 entry = sl_elf_get_entry(o);
    if (entry == 0) {
        fprintf(stderr, "no entry address for binary\n");
        err = SL_ERR_ARG;
        goto out_err;
    }
    sl_core_set_reg(c, SL_CORE_REG_PC, entry);

    if ((err = sl_core_set_state(c, SL_CORE_STATE_64BIT, is64))) {
        fprintf(stderr, "failed to set core 64-bit state\n");
        goto out_err;
    }
    err = 0;

out_err:
#if WITH_SYMBOLS
    if (err) sym_free(sl);
#endif
    return err;
}

int sl_machine_load_core_raw(sl_machine_t *m, u4 id, u8 addr, void *buf, u8 size) {
    sl_core_t *c = sl_machine_get_core(m, id);
    if (c == NULL) {
        fprintf(stderr, "invalid core id: %u\n", id);
        return SL_ERR_ARG;
    }
    int err;
    if ((err = sl_core_mem_write(c, addr, 1, size, buf))) {
        fprintf(stderr, "failed to load core memory: %s\n", st_err(err));
        fprintf(stderr, "  addr=%#" PRIx64 ", size=%#" PRIx64 "\n", addr, size);
    }
    return err;
}

