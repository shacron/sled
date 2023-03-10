// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stddef.h>

#include <core/irq.h>
#include <sled/error.h>

uint32_t sl_irq_endpoint_get_enabled(sl_irq_ep_t *ep) { return ep->enabled; }
uint32_t sl_irq_endpoint_get_asserted(sl_irq_ep_t *ep) { return ep->retained; }

uint32_t sl_irq_endpoint_get_active(sl_irq_ep_t *ep) {
    return ep->retained & ep->enabled;
}

static void irq_endpoint_set_active(sl_irq_ep_t *ep) {
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

int sl_irq_endpoint_assert(sl_irq_ep_t *ep, uint32_t num, bool high) {
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

int sl_irq_endpoint_set_enabled(sl_irq_ep_t *ep, uint32_t vec) {
    ep->enabled = vec;
    irq_endpoint_set_active(ep);
    return 0;
}

int sl_irq_endpoint_clear(sl_irq_ep_t *ep, uint32_t vec) {
    ep->retained &= ~vec;           // clear set bits
    ep->retained |= ep->asserted;   // asserted irqs can't be cleared
    irq_endpoint_set_active(ep);
    return 0;
}

int sl_irq_endpoint_set_client(sl_irq_ep_t *ep, sl_irq_ep_t *client, uint32_t num) {
    if (num > 31) return SL_ERR_ARG;
    ep->client = client;
    ep->client_num = num;
    irq_endpoint_set_active(ep);
    return 0;
}
