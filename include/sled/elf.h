// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sled/elf/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct elf_object elf_object_t;

int elf_open(const char *filename, elf_object_t **obj_out);
int elf_arch(elf_object_t *obj);
int elf_subarch(elf_object_t *obj);
uint32_t elf_arch_options(elf_object_t *obj);
bool elf_is_64bit(elf_object_t *obj);
uint64_t elf_get_entry(elf_object_t *obj);
ssize_t elf_symbol_length(elf_object_t *obj, const char *name);
ssize_t elf_read_symbol(elf_object_t *obj, const char *name, void *buf, size_t buflen);
void *elf_get_program_header(elf_object_t *obj, uint32_t index);
void *elf_pointer_for_offset(elf_object_t *obj, uint64_t offset);
void elf_close(elf_object_t *obj);

#ifdef __cplusplus
}
#endif
