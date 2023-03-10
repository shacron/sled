// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SL_IRQ_VEC_ALL 0xffffffff

typedef struct sl_irq_ep sl_irq_ep_t;

// input irq functions
int sl_irq_endpoint_assert(sl_irq_ep_t *ep, uint32_t num, bool high);

// user functions
int sl_irq_endpoint_set_enabled(sl_irq_ep_t *ep, uint32_t vec);
int sl_irq_endpoint_clear(sl_irq_ep_t *ep, uint32_t vec);
uint32_t sl_irq_endpoint_get_enabled(sl_irq_ep_t *ep);
uint32_t sl_irq_endpoint_get_asserted(sl_irq_ep_t *ep);
uint32_t sl_irq_endpoint_get_active(sl_irq_ep_t *ep);

// setup functions
int sl_irq_endpoint_set_client(sl_irq_ep_t *ep, sl_irq_ep_t *client, uint32_t num);

