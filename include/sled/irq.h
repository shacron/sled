// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#define SL_IRQ_VEC_ALL 0xffffffff

// input irq functions
int sl_irq_endpoint_assert(sl_irq_ep_t *ep, u4 num, bool high);

// user functions
int sl_irq_endpoint_set_enabled(sl_irq_ep_t *ep, u4 vec);
int sl_irq_endpoint_clear(sl_irq_ep_t *ep, u4 vec);
u4 sl_irq_endpoint_get_enabled(sl_irq_ep_t *ep);
u4 sl_irq_endpoint_get_asserted(sl_irq_ep_t *ep);
u4 sl_irq_endpoint_get_active(sl_irq_ep_t *ep);

// setup functions
int sl_irq_endpoint_set_client(sl_irq_ep_t *ep, sl_irq_ep_t *client, u4 num);

