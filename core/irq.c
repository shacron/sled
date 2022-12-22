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

#include <stddef.h>

#include <core/irq.h>
#include <sled/error.h>

uint32_t irq_endpoint_get_active(irq_endpoint_t *ep) {
    return ep->retained & ep->enabled;
}

static void irq_endpoint_set_active(irq_endpoint_t *ep) {
    const uint32_t active = ep->retained & ep->enabled;
    if (active == 0) {
        if (ep->high) {
            if (ep->client != NULL) ep->client->assert(ep->client, ep->client_num, false);
            ep->high = false;
        }
    } else {
        if (!ep->high) {
            if (ep->client != NULL) ep->client->assert(ep->client, ep->client_num, true);
            ep->high = true;
        }
    }
}

int irq_endpoint_assert(irq_endpoint_t *ep, uint32_t num, bool high) {
    if (num > 31) return SL_ERR_ARG;
    const uint32_t bit = 1u << num;
    if (high) {
        if (ep->asserted & bit) return 0;
        ep->asserted |= bit;
        ep->retained |= bit;
    } else {
        if ((ep->asserted & bit) == 0) return 0;
        ep->asserted &= ~bit;
    }
    irq_endpoint_set_active(ep);
    return 0;
}

int irq_endpoint_set_enabled(irq_endpoint_t *ep, uint32_t vec) {
    ep->enabled = vec;
    irq_endpoint_set_active(ep);
    return 0;
}

int irq_endpoint_clear(irq_endpoint_t *ep, uint32_t vec) {
    ep->retained &= ~vec;           // clear set bits
    ep->retained |= ep->asserted;   // asserted irqs can't be cleared
    irq_endpoint_set_active(ep);
    return 0;
}

int irq_endpoint_set_client(irq_endpoint_t *ep, irq_endpoint_t *client, uint32_t num) {
    if (num > 31) return SL_ERR_ARG;
    ep->client = client;
    ep->client_num = num;
    irq_endpoint_set_active(ep);
    return 0;
}
