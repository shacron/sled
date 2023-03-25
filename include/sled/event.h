// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/list.h>
#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SL_EV_FLAG_FREE       (1u << 0) // free event pointer when done
#define SL_EV_FLAG_CALLBACK   (1u << 1) // invoke callback
#define SL_EV_FLAG_WAIT       (1u << 2) // wait on call completion before returning

struct sl_event {
    sl_list_node_t node;        // internal, should be zero
    uint32_t type;              // user-defined
    uint32_t flags;             // combination of zero or more S_EV_FLAGs
    uint32_t option;            // user-defined
    uint64_t arg[4];            // user-defined
    uintptr_t signal;           // internal, should be zero
    int (*callback)(sl_event_t *ev); // NULL unless SL_EV_FLAG_CALLBACK is set
    void *cookie;               // user-defined, may be NULL
};

int sl_event_send_async(sl_event_queue_t *el, sl_event_t *ev);

#ifdef __cplusplus
}
#endif
