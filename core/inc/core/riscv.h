// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#define RV_HAS_PRIV_LEVEL_USER       (1u << 0)
#define RV_HAS_PRIV_LEVEL_SUPERVISOR (1u << 1)
#define RV_HAS_PRIV_LEVEL_HYPERVISOR (1u << 2)

typedef struct bus bus_t;
typedef struct core core_t;
typedef struct core_params core_params_t;

int riscv_core_create(core_params_t *p, bus_t *bus, core_t **core_out);
