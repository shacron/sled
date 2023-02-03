// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stddef.h>
#include <stdint.h>

#define countof(p) (sizeof(p) / sizeof(p[0]))
#define containerof(p, t, m) (t *)((uintptr_t)p - offsetof(t, m))

typedef struct {
    uint32_t value;
    int err;
} result32_t;

typedef struct {
    uint64_t value;
    int err;
} result64_t;

