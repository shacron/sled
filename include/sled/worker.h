// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Worker is a container that executes one or more engines, as well as handling async events for them.
// A worker is intended as a context for a workloop thread.
// The worker calls the engine's step() function repeatedly as long as that engine is runnable.
// In between steps it checks for events. If no engines are runnable, the thread blocks until more events
// are enqueued.


// Synchronous functions
// These functions must only be called when the work loop is not running.
// -------------------------------------------------

int sl_worker_create(const char *name, sl_worker_t **w_out);

void sl_worker_retain(sl_worker_t *w);
void sl_worker_release(sl_worker_t *w);

int sl_worker_add_engine(sl_worker_t *w, sl_engine_t *e, u4 *id_out);

// Add a event handler that will receive events in the context of the work queue thread.
// This is guaranteed to be between engine steps.
int sl_worker_add_event_endpoint(sl_worker_t *w, sl_event_ep_t *ep, u4 *id_out);

// Runs 'num' iterations of the work loop on current thread
int sl_worker_step(sl_worker_t *w, u8 num);

// Run work loop on current thread
// This will not return until the worker has exited.
int sl_worker_run(sl_worker_t *w);

// Run work loop on a new thread. This call returns immediately.
int sl_worker_thread_run(sl_worker_t *w);

// Sets the current state of the engine to runnable. Runnable engines are stepped by the work loop.
// Note: special use case:
// This function is safe to call from the context of the work loop thread. For example, an engine may
// set its run state in response to an event.
void sl_worker_set_engine_runnable(sl_worker_t *w, bool runnable);


// Synchronizing functions
// The following must only be called when the workloop thread is running.
// -------------------------------------------------

// Join the thread created by sl_worker_thread_run().
// This call does not command the work loop to exit. The worker must be notified that it needs
// to exit via an async event if it does not exit on its own.
int sl_worker_thread_join(sl_worker_t *w);


// Async functions
// -------------------------------------------------

// Send an aynchronous event to the work queue. The call returns after the event has been enqueued.
// The event will only be processed if the work loop thread is running.
int sl_worker_event_enqueue_async(sl_worker_t *w, sl_event_t *ev);



#ifdef __cplusplus
}
#endif
