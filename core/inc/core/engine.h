// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/event.h>
#include <core/obj.h>
#include <core/types.h>
#include <sled/engine.h>

struct sl_engine {
    sl_obj_t obj_;

    uint32_t state;         // current running state
    sl_bus_t *bus;
    sl_irq_ep_t irq_ep;
    sl_worker_t *worker;
    uint32_t epid;
    sl_event_ep_t event_ep;
    sl_engine_ops_t ops;
    void *context;
};

int sl_engine_init(sl_engine_t *e, sl_bus_t *b);
void sl_engine_shutdown(sl_engine_t *e);
int sl_engine_handle_interrupts(sl_engine_t *e);
int sl_engine_wait_for_interrupt(sl_engine_t *e);
