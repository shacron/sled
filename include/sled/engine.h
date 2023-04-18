// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

struct sl_engine_ops {
    int (*step)(sl_engine_t *e);
    int (*interrupt)(sl_engine_t *e);
};

void sl_engine_retain(sl_engine_t *e);
void sl_engine_release(sl_engine_t *e);

int sl_engine_run(sl_engine_t *e);
int sl_engine_step(sl_engine_t *e, uint64_t num);
void sl_engine_interrupt_set(sl_engine_t *e, bool enable);


int sl_engine_async_command(sl_engine_t *e, uint32_t cmd, bool wait);


