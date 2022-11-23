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

int elf_symbols(elf_object_t *obj);

#ifdef __cplusplus
}
#endif
