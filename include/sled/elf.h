// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>

#include <sled/elf/types.h>
#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sl_elf_obj sl_elf_obj_t;

int sl_elf_open(const char *filename, sl_elf_obj_t **obj_out);
int sl_elf_arch(sl_elf_obj_t *obj);
int sl_elf_subarch(sl_elf_obj_t *obj);
u32 sl_elf_arch_options(sl_elf_obj_t *obj);
bool sl_elf_is_64bit(sl_elf_obj_t *obj);
u64 sl_elf_get_entry(sl_elf_obj_t *obj);
ssize_t sl_elf_symbol_length(sl_elf_obj_t *obj, const char *name);
ssize_t sl_elf_read_symbol(sl_elf_obj_t *obj, const char *name, void *buf, size_t buflen);
void *sl_elf_get_program_header(sl_elf_obj_t *obj, u32 index);
void *sl_elf_pointer_for_offset(sl_elf_obj_t *obj, u64 offset);
void sl_elf_close(sl_elf_obj_t *obj);

#ifdef __cplusplus
}
#endif
