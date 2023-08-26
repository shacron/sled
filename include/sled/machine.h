// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/core.h>
#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int sl_machine_create(sl_machine_t **m_out);

int sl_machine_add_core(sl_machine_t *m, sl_core_params_t *opts);
int sl_machine_add_device(sl_machine_t *m, u4 type, u8 base, const char *name);
int sl_machine_add_device_prefab(sl_machine_t *m, u8 base, sl_dev_t *d);
int sl_machine_add_mem(sl_machine_t *m, u8 base, u8 size);

int sl_machine_load_core(sl_machine_t *m, u4 id, sl_elf_obj_t *obj, bool configure);
int sl_machine_load_core_raw(sl_machine_t *m, u4 id, u8 addr, void *buf, u8 size);

sl_dev_t * sl_machine_get_device_for_name(sl_machine_t *m, const char *name);
sl_core_t * sl_machine_get_core(sl_machine_t *m, u4 id);
int sl_machine_set_interrupt(sl_machine_t *m, u4 irq, bool high);
sl_chrono_t * sl_machine_get_chrono(sl_machine_t *m);

void sl_machine_destroy(sl_machine_t *m);

#ifdef __cplusplus
}
#endif
