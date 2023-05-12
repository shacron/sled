// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/list.h>
#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SL_EV_EP_CALLBACK 0xffffffff

#define SL_EV_FLAG_FREE       (1u << 0) // free event pointer when done
#define SL_EV_FLAG_SIGNAL     (1u << 1) // wait on call completion before returning

struct sl_event_ep {
    int (*handle)(sl_event_ep_t *ep, sl_event_t *ev);
};

struct sl_event {
    sl_list_node_t node;        // internal, should be zero
    u4 epid;              // endpoint id
    u4 type;              // user-defined
    u4 flags;             // combination of zero or more S_EV_FLAGs
    u4 option;            // user-defined
    u8 arg[4];            // user-defined
    uintptr_t signal;           // internal, should be zero
    int (*callback)(sl_event_t *ev); // NULL unless epid is SL_EV_EP_CALLBACK
    void *cookie;               // user-defined, may be NULL
    int err;                    // returned status from handler. Only valid if SL_EV_FLAG_SIGNAL is set.
};

#ifdef __cplusplus
}
#endif
