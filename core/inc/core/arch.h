// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/arch.h>

typedef struct {
    u1 (*reg_index)(u4 reg);
    u4 (*reg_for_name)(const char *name);
    const char *(*name_for_reg)(u4 reg);
} arch_ops_t;

extern const arch_ops_t sl_arch_ops[];

u1 sl_arch_reg_index(u1 arch, u4 reg);
