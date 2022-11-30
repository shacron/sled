// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define IRQ_VEC_ALL 0xffffffff

typedef struct irq_endpoint irq_endpoint_t;

struct irq_endpoint {
    uint32_t asserted;      // line level state
    uint32_t retained;      // sticky version of asserted bits
    uint32_t enabled;       // inverse mask

    irq_endpoint_t *client; // client irq link
    uint32_t client_num;    // interrupt number to use in client invocation
    bool high;              // is the client currently asserted

    // the supplied assert function, if any, should do the proper locking to
    // protect this structure from possible concurrency
    int (*assert)(irq_endpoint_t *ep, uint32_t num, bool high);
};

// input irq functions
int irq_endpoint_assert(irq_endpoint_t *ep, uint32_t num, bool high);

// user functions
int irq_endpoint_set_enabled(irq_endpoint_t *ep, uint32_t vec);
int irq_endpoint_clear(irq_endpoint_t *ep, uint32_t vec);
uint32_t irq_endpoint_get_active(irq_endpoint_t *ep);

// setup functions
int irq_endpoint_set_client(irq_endpoint_t *ep, irq_endpoint_t *client, uint32_t num);

