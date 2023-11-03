// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <core/bus.h>
#include <core/chrono.h>
#include <core/common.h>
#include <core/device.h>
#include <core/engine.h>
#include <core/irq.h>
#include <core/obj.h>
#include <core/regview.h>
#include <core/riscv/rv.h>
#include <core/worker.h>

#define CLASS_FUNC(name) \
int name ## _obj_init(void *o, const char *name, void *cfg); \
void name ## _obj_shutdown(void *o);

#define CLASS_INST(type, s, name) \
    [type] = { .size = sizeof(s), .init = name ## _obj_init, .shutdown = name ## _obj_shutdown, },

CLASS_FUNC(device)
CLASS_FUNC(engine)
CLASS_FUNC(riscv_core)
CLASS_FUNC(chrono)
CLASS_FUNC(worker)
CLASS_FUNC(bus)
CLASS_FUNC(irq_endpoint)
CLASS_FUNC(reg_view)

static const sl_obj_class_t obj_class_list[] = {
    CLASS_INST(SL_OBJ_TYPE_DEVICE, sl_dev_t, device)
    CLASS_INST(SL_OBJ_TYPE_ENGINE, sl_engine_t, engine)
    CLASS_INST(SL_OBJ_TYPE_RVCORE, rv_core_t, riscv_core)
    CLASS_INST(SL_OBJ_TYPE_CHRONO, sl_chrono_t, chrono)
    CLASS_INST(SL_OBJ_TYPE_WORKER, sl_worker_t, worker)
    CLASS_INST(SL_OBJ_TYPE_BUS, sl_bus_t, bus)
    CLASS_INST(SL_OBJ_TYPE_IRQ_EP, sl_irq_ep_t, irq_ep)
    CLASS_INST(SL_OBJ_TYPE_REGVIEW, sl_reg_view_t, reg_view)
};

const sl_obj_class_t * sl_obj_class_for_type(u1 type) {
    if (type > countof(obj_class_list)) return NULL;
    return &obj_class_list[type];
}
