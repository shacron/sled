// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#pragma once

#include <stddef.h>

#define countof(p) (sizeof(p) / sizeof(p[0]))
#define containerof(p, t, m) (t *)((uintptr_t)p - offsetof(t, m))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#define MAX(a,b) ((a) >= (b) ? (a) : (b))

#define ROUND_UP(a, sz)   ((((a) + (sz - 1)) / (sz)) * (sz))
#define ROUND_DOWN(a, sz) (((a) / (sz)) * (sz))
