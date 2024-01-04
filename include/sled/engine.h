// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sl_engine_ops {
    int (*step)(sl_engine_t *e);
    int (*interrupt)(sl_engine_t *e);
};

int sl_engine_create(const char *name, const sl_engine_ops_t *ops, sl_engine_t **e_out);
void sl_engine_destory(sl_engine_t *e);

void sl_engine_set_context(sl_engine_t *e, void *ctx);
void * sl_engine_get_context(sl_engine_t *e);

int sl_engine_run(sl_engine_t *e);
int sl_engine_step(sl_engine_t *e, u8 num);
void sl_engine_interrupt_set(sl_engine_t *e, bool enable);

int sl_engine_async_command(sl_engine_t *e, u4 cmd, bool wait);

#ifdef __cplusplus
}
#endif
