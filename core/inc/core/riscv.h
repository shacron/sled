// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>

#define RV_HAS_PRIV_LEVEL_USER       (1u << 0)
#define RV_HAS_PRIV_LEVEL_SUPERVISOR (1u << 1)
#define RV_HAS_PRIV_LEVEL_HYPERVISOR (1u << 2)

int riscv_core_create(sl_core_params_t *p, sl_bus_t *bus, sl_core_t **core_out);
