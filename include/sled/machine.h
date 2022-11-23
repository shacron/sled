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

int machine_load_core(machine_t *m, uint32_t id, elf_object_t *obj);

device_t * machine_get_device_for_name(machine_t *m, const char *name);
core_t * machine_get_core(machine_t *m, uint32_t id);
int machine_set_interrupt(machine_t *m, uint32_t irq, bool high);

void machine_destroy(machine_t *m);

#ifdef __cplusplus
}
#endif
