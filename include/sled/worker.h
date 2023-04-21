// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int sl_worker_create(const char *name, sl_worker_t **w_out);

void sl_worker_retain(sl_worker_t *w);
void sl_worker_release(sl_worker_t *w);

int sl_worker_add_engine(sl_worker_t *w, sl_engine_t *e, uint32_t *id_out);
int sl_worker_add_event_endpoint(sl_worker_t *w, sl_event_ep_t *ep, uint32_t *id_out);

void sl_worker_set_engine_runnable(sl_worker_t *w, bool runnable);

// Runs 'num' iterations of the work loop on current thread
int sl_worker_step(sl_worker_t *w, uint64_t num);

// Run work loop on current thread
int sl_worker_run(sl_worker_t *w);


// int sl_worker_thread_run(sl_worker_t *w);

// async functions

// int sl_worker_thread_join(sl_worker_t *w);

int sl_worker_event_enqueue_async(sl_worker_t *w, sl_event_t *ev);

#ifdef __cplusplus
}
#endif