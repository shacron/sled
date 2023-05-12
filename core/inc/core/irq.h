// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/irq.h>

struct sl_irq_ep {
    u4 asserted;           // line level state
    u4 retained;           // sticky version of asserted bits
    u4 enabled;            // inverse mask

    sl_irq_ep_t *client;    // client irq link
    u4 client_num;         // interrupt number to use in client invocation
    bool high;              // is the client currently asserted

    // the supplied assert function, if any, should do the proper locking to
    // protect this structure from possible concurrency
    int (*assert)(sl_irq_ep_t *ep, u4 num, bool high);
};
