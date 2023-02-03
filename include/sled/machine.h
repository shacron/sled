// SPDX-License-Identifier: MIT License
// Copyright (c) 2022 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sled/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct elf_object elf_object_t;
typedef struct machine machine_t;

int machine_create(machine_t **m_out);

int machine_add_core(machine_t *m, core_params_t *opts);
int machine_add_device(machine_t *m, uint32_t type, uint64_t base, const char *name);
int machine_add_device_prefab(machine_t *m, uint64_t base, device_t *d);
int machine_add_mem(machine_t *m, uint64_t base, uint64_t size);

int machine_load_core(machine_t *m, uint32_t id, elf_object_t *obj, bool set_state);
int machine_load_core_raw(machine_t *m, uint32_t id, uint64_t addr, void *buf, uint64_t size);

device_t * machine_get_device_for_name(machine_t *m, const char *name);
core_t * machine_get_core(machine_t *m, uint32_t id);
int machine_set_interrupt(machine_t *m, uint32_t irq, bool high);

void machine_destroy(machine_t *m);

#ifdef __cplusplus
}
#endif
