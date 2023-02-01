// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/core.h>
#include <sled/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RV_HAS_PRIV_LEVEL_USER       (1u << 0)
#define RV_HAS_PRIV_LEVEL_SUPERVISOR (1u << 1)
#define RV_HAS_PRIV_LEVEL_HYPERVISOR (1u << 2)

typedef struct bus bus_t;

int riscv_core_create(core_params_t *p, bus_t *bus, core_t **core_out);

#ifdef __cplusplus
}
#endif
