// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/irq.h>

struct sl_irq_ep {
    uint32_t asserted;      // line level state
    uint32_t retained;      // sticky version of asserted bits
    uint32_t enabled;       // inverse mask

    sl_irq_ep_t *client;    // client irq link
    uint32_t client_num;    // interrupt number to use in client invocation
    bool high;              // is the client currently asserted

    // the supplied assert function, if any, should do the proper locking to
    // protect this structure from possible concurrency
    int (*assert)(sl_irq_ep_t *ep, uint32_t num, bool high);
};
