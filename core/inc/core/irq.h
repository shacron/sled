// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/irq.h>

struct sl_irq_mux {
    sl_irq_ep_t *client;    // client irq link
    u4 client_num;          // interrupt number to use in client invocation
    u4 enabled;             // inverse mask
    u4 active;              // irq bits processed into output
};

struct sl_irq_ep {
    u4 asserted;            // input irq line state
    u4 retained;            // sticky version of asserted bits
    sl_irq_mux_t mux;       // output irq to client

    // the supplied assert function, if any, should do the proper locking to
    // protect this structure from possible concurrency
    int (*assert)(sl_irq_ep_t *ep, u4 num, bool high);
    void *context;
};

int sl_irq_ep_init(sl_irq_ep_t *ep);
void sl_irq_ep_shutdown(sl_irq_ep_t *ep);
