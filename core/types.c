// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <core/chrono.h>
#include <core/common.h>
#include <core/device.h>
#include <core/engine.h>
#include <core/obj.h>
#include <core/riscv/rv.h>
#include <core/worker.h>

static const sl_obj_class_t obj_class_list[] = {
    [SL_OBJ_TYPE_DEVICE] = {
        .size = sizeof(sl_dev_t),
        .init = device_obj_init,
        .shutdown = device_obj_shutdown,
    },
    [SL_OBJ_TYPE_ENGINE] = {
        .size = sizeof(sl_engine_t),
        .init = engine_obj_init,
        .shutdown = engine_obj_shutdown,
    },
    [SL_OBJ_TYPE_RVCORE] = {
        .size = sizeof(rv_core_t),
        .init = riscv_core_obj_init,
        .shutdown = riscv_core_obj_shutdown,
    },
    [SL_OBJ_TYPE_CHRONO] = {
        .size = sizeof(sl_chrono_t),
        .init = chrono_obj_init,
        .shutdown = chrono_obj_shutdown,
    },
    [SL_OBJ_TYPE_WORKER] = {
        .size = sizeof(sl_worker_t),
        .init = worker_obj_init,
        .shutdown = worker_obj_shutdown,
    },
};

const sl_obj_class_t * sl_obj_class_for_type(u1 type) {
    if (type > countof(obj_class_list)) return NULL;
    return &obj_class_list[type];
}
