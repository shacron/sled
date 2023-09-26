// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/event.h>
#include <core/irq.h>
#include <core/obj.h>
#include <core/types.h>
#include <sled/engine.h>

struct sl_engine {
    sl_obj_t obj_;

    const char *name;
    u4 state;         // current running state
    sl_irq_ep_t irq_ep;
    sl_worker_t *worker;
    u4 epid;
    sl_event_ep_t event_ep;
    sl_engine_ops_t ops;
    void *context;
};

int engine_obj_init(void *o, const char *name);
void engine_obj_shutdown(void *o);

int sl_engine_handle_interrupts(sl_engine_t *e);
int sl_engine_wait_for_interrupt(sl_engine_t *e);
