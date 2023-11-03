// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SL_OBJ_TYPE_DEVICE      0
#define SL_OBJ_TYPE_ENGINE      1
#define SL_OBJ_TYPE_WORKER      2
#define SL_OBJ_TYPE_CHRONO      3
#define SL_OBJ_TYPE_RVCORE      4
#define SL_OBJ_TYPE_BUS         5
#define SL_OBJ_TYPE_IRQ_EP      6
#define SL_OBJ_TYPE_REGVIEW     7

int sl_obj_alloc_init(u1 type, const char *name, void *cfg, void **o_out);
void sl_obj_retain(void *o);
void sl_obj_release(void *o);

#ifdef __cplusplus
}
#endif
