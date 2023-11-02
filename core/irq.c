// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stddef.h>

#include <core/irq.h>
#include <sled/error.h>

u4 sl_irq_endpoint_get_enabled(sl_irq_ep_t *ep) { return ep->mux.enabled; }
u4 sl_irq_endpoint_get_asserted(sl_irq_ep_t *ep) { return ep->retained; }
u4 sl_irq_endpoint_get_active(sl_irq_ep_t *ep) { return ep->mux.active; }
void * sl_irq_endpoint_get_context(sl_irq_ep_t *ep) { return ep->context; }
void sl_irq_endpoint_set_context(sl_irq_ep_t *ep, void *context) { ep->context = context; }
void sl_irq_endpoint_set_handler(sl_irq_ep_t *ep, int (*assert)(sl_irq_ep_t *ep, u4 num, bool high)) { ep->assert = assert; }

u4 sl_irq_mux_get_active(sl_irq_mux_t *m) { return m->active; }
u4 sl_irq_mux_get_enabled(sl_irq_mux_t *m) { return m->enabled; }

void sl_irq_mux_set_active(sl_irq_mux_t *o, u4 vec) {
    const bool was_high = (o->active & o->enabled) > 0;
    o->active = vec;
    if (o->client != NULL) {
        const bool is_high = (vec & o->enabled) > 0;
        if (is_high != was_high)
            o->client->assert(o->client, o->client_num, is_high);
    }
}

int sl_irq_mux_set_active_bit(sl_irq_mux_t *o, u4 index, bool high) {
    if (index > 31) return SL_ERR_ARG;
    const u4 bit = 1u << index;
    u4 act = o->active;
    if (high) act |= bit;
    else act &= ~bit;
    if (act != o->active) sl_irq_mux_set_active(o, act);
    return 0;
}

void sl_irq_mux_set_enabled(sl_irq_mux_t *m, u4 vec) {
    m->enabled = vec;
    sl_irq_mux_set_active(m, m->active);
}

static inline void irq_endpoint_set_active(sl_irq_ep_t *ep) {
    sl_irq_mux_set_active(&ep->mux, ep->retained & ep->mux.enabled);
}

int sl_irq_endpoint_assert(sl_irq_ep_t *ep, u4 num, bool high) {
    if (num > 31) return SL_ERR_ARG;
    const u4 bit = 1u << num;
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

int sl_irq_endpoint_set_enabled(sl_irq_ep_t *ep, u4 vec) {
    ep->mux.enabled = vec;
    irq_endpoint_set_active(ep);
    return 0;
}

int sl_irq_endpoint_clear(sl_irq_ep_t *ep, u4 vec) {
    ep->retained &= ~vec;           // clear set bits
    ep->retained |= ep->asserted;   // asserted irqs can't be cleared
    irq_endpoint_set_active(ep);
    return 0;
}

int sl_irq_mux_set_client(sl_irq_mux_t *m, sl_irq_ep_t *ep, u4 num) {
    if (num > 31) return SL_ERR_ARG;
    m->client = ep;
    m->client_num = num;
    return 0;
}

int sl_irq_endpoint_set_client(sl_irq_ep_t *ep, sl_irq_ep_t *client, u4 num) {
    if (num > 31) return SL_ERR_ARG;
    ep->mux.client = client;
    ep->mux.client_num = num;
    irq_endpoint_set_active(ep);
    return 0;
}

int irq_ep_obj_init(void *o, const char *name, void *cfg) {
    return 0;
}

void irq_ep_obj_shutdown(void *o) { }

