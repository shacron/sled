// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

#define SL_IRQ_VEC_ALL 0xffffffff

// interrupt mux
// Interrupt multiplexer is a simple array of interrupt bits and a target interrupt endpoint.
// When one or more bits are active, the multiplexer's output signal is set to high
// Otherwise it is set to low.
// One use for this is a device that can issue multiple interrupt events, all of which should
// trigger the same interrupt request number on the system's interrupt controller.
// Interrupt mux is present in the sl_dev_t structure for this purpose.

void sl_irq_mux_set_active(sl_irq_mux_t *m, u4 vec);
int sl_irq_mux_set_active_bit(sl_irq_mux_t *m, u4 index, bool high);
u4 sl_irq_mux_get_active(sl_irq_mux_t *m);
void sl_irq_mux_set_enabled(sl_irq_mux_t *m, u4 vec);
u4 sl_irq_mux_get_enabled(sl_irq_mux_t *m);

// irq endpoint
// Interrupt request endpoint provides a framework for receiving interrupts. This is
// intended for implementing interrupt controller devices.

// input irq functions
int sl_irq_endpoint_assert(sl_irq_ep_t *ep, u4 num, bool high);

// user functions
int sl_irq_endpoint_set_enabled(sl_irq_ep_t *ep, u4 vec);
int sl_irq_endpoint_clear(sl_irq_ep_t *ep, u4 vec);
u4 sl_irq_endpoint_get_enabled(sl_irq_ep_t *ep);
u4 sl_irq_endpoint_get_asserted(sl_irq_ep_t *ep);
u4 sl_irq_endpoint_get_active(sl_irq_ep_t *ep);

// setup functions
int sl_irq_ep_create(sl_irq_ep_t **ep_out);
void sl_irq_ep_destroy(sl_irq_ep_t *ep);

int sl_irq_mux_set_client(sl_irq_mux_t *m, sl_irq_ep_t *ep, u4 num);
int sl_irq_endpoint_set_client(sl_irq_ep_t *ep, sl_irq_ep_t *client, u4 num);
void sl_irq_endpoint_set_handler(sl_irq_ep_t *ep, int (*assert)(sl_irq_ep_t *ep, u4 num, bool high));

void sl_irq_endpoint_set_context(sl_irq_ep_t *ep, void *context);
void * sl_irq_endpoint_get_context(sl_irq_ep_t *ep);

