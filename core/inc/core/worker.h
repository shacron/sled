// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/worker.h>

#define SL_WORKER_MAX_EPS   64

struct sl_worker {
    sl_obj_t *obj_;

    const char *name;

    lock_t lock;
    cond_t has_event;
    sl_list_t ev_list;

    u4 state;
    sl_engine_t *engine;

    sl_event_ep_t *endpoint[SL_WORKER_MAX_EPS];

    pthread_t thread;
    int thread_status;
    bool thread_running;
};

int worker_obj_init(void *o, const char *name);
void worker_obj_shutdown(void *o);

