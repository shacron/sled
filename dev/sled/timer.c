// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <device/sled/timer.h>
#include <sled/device.h>
#include <sled/error.h>
#include <sled/machine.h>
#include <sled/chrono.h>
#include <sled/irq.h>

#define TIMER_TYPE 'timr'
#define TIMER_VERSION       0
#define TIMER_MAX_UNITS     8

typedef struct sled_timer sled_timer_t;

typedef struct {
    sled_timer_t *timer;
    u4 config;
    u4 status;
    u8 reset_val;
    u8 tid;     // timer id for chrono
    u8 count;
} sled_timer_unit_t;

struct sled_timer {
    sl_dev_t *dev;
    sl_chrono_t *chrono;

    // registers
    u4 config;
    u4 status;
    u4 scalar;
    u4 num_units;
    sled_timer_unit_t unit[TIMER_MAX_UNITS];
};

int sled_timer_create(const char *name, sl_dev_config_t *cfg, sl_dev_t **dev_out);

static u4 index_for_pointer(sled_timer_t *t, sled_timer_unit_t *u) {
    return ((uptr)u - (uptr)&t->unit[0]) / sizeof(sled_timer_unit_t);
}

static sled_timer_unit_t * addr_to_unit(sled_timer_t *t, u8 addr, u4 *reg) {
    addr -= 0x20;
    u4 u = addr / 0x20;
    *reg = (addr & (0x20 - 1)) >> 2;
    return &t->unit[u];
}

static int timer_read(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;
    if (addr & 3) return SL_ERR_IO_ALIGN;

    sled_timer_t *t = ctx;
    u4 *val = buf;
    int err = 0;
    sl_irq_mux_t *m;

    sl_device_lock(t->dev);
    switch (addr) {
    case TIMER_REG_DEV_TYPE:     *val = TIMER_TYPE;          goto out;
    case TIMER_REG_DEV_VERSION:  *val = TIMER_VERSION;       goto out;
    case TIMER_REG_CONFIG:       *val = t->config;           goto out;
    case TIMER_REG_STATUS:       *val = t->status;           goto out;
    case TIMER_REG_RT_SCALER_US: *val = t->scalar;           goto out;
    case TIMER_REG_NUM_UNITS:    *val = t->num_units;        goto out;

    case TIMER_IRQ_MASK:
        m = sl_device_get_irq_mux(t->dev);
        *val = ~sl_irq_mux_get_enabled(m);
        goto out;

    case TIMER_IRQ_STATUS:
        m = sl_device_get_irq_mux(t->dev);
        *val = sl_irq_mux_get_active(m);
        goto out;

    default:    break;
    }
    if ((addr < 0x20) || (addr >= TIMER_APERTURE_LENGTH(t->num_units))) {
        err = SL_ERR_IO_INVALID;
        goto out;
    }

    u4 reg;
    sled_timer_unit_t *u = addr_to_unit(t, addr, &reg);
    switch (reg) {
    case 0:     *val = u->config;                break;
    case 1:     *val = (u4)u->reset_val;         break;
    case 2:     *val = (u4)(u->reset_val >> 32); break;
    // case 3:
    // case 4:     *val = (u4)unit->reset_val;         break;
    }

out:
    sl_device_unlock(t->dev);
    return err;
}

static int timer_callback(void *context, int err) {
    if (err) {
        printf("timer stopped\n");
        return 0;
    }

    sled_timer_unit_t *u = context;
    sled_timer_t *t = u->timer;
    int ret = 0;
    u8 count;
    u4 index = index_for_pointer(t, u);
    sl_irq_mux_t *m = sl_device_get_irq_mux(t->dev);

    sl_device_lock(t->dev);

    if (u->config & TIMER_UNIT_CONFIG_CONTINUOUS) ret = SL_ERR_RESTART;
    else u->config &= ~TIMER_UNIT_CONFIG_RUN;
    u->config |= TIMER_UNIT_CONFIG_LOOPED;
    u->count++;
    count = u->count;
    sl_irq_mux_set_active_bit(m, index, true);
    sl_device_unlock(t->dev);

    printf("timer fired %" PRIu64 "\n", count);
    return ret;
}

static void timer_set_unit_config_locked(sled_timer_t *t, sled_timer_unit_t *u, u4 val) {
    u4 config = u->config;

    config &= ~TIMER_UNIT_CONFIG_CONTINUOUS;
    config |= (val & TIMER_UNIT_CONFIG_CONTINUOUS);
    // clear looped on write
    if (val & TIMER_UNIT_CONFIG_LOOPED) config &= ~TIMER_UNIT_CONFIG_LOOPED;

    if (config & TIMER_UNIT_CONFIG_RUN) {
        // currently running
        if ((val & TIMER_UNIT_CONFIG_RUN) == 0) {
            // stop timer
            config &= ~TIMER_UNIT_CONFIG_RUN;
            sl_chrono_timer_cancel(t->chrono, u->tid);
        }
    } else {
        if (val & TIMER_UNIT_CONFIG_RUN) {
            // start timer
            config &= ~TIMER_UNIT_CONFIG_LOOPED;
            config |= TIMER_UNIT_CONFIG_RUN;
            int err = sl_chrono_timer_set(t->chrono, u->reset_val, timer_callback, u, &u->tid);
            if (err) {
                fprintf(stderr, "timer_set_unit_config_locked: failed to allocate memory for system timer, device is in broken state\n");
            }
        }
    }
    u->config = config;
}

static int timer_write(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;
    if (addr & 3) return SL_ERR_IO_ALIGN;

    sled_timer_t *t = ctx;
    u4 val = *(u4 *)buf;
    int err = 0;
    sl_irq_mux_t *m;

    sl_device_lock(t->dev);
    switch (addr) {
    case TIMER_REG_CONFIG:          t->config = val;    goto out;
    case TIMER_REG_RT_SCALER_US:    t->scalar = val;    goto out;

    case TIMER_IRQ_MASK:
        m = sl_device_get_irq_mux(t->dev);
        sl_irq_mux_set_enabled(m, ~val);
        goto out;

    case TIMER_IRQ_STATUS: {
        m = sl_device_get_irq_mux(t->dev);
        u4 vec = sl_irq_mux_get_active(m);
        vec &= ~val;
        sl_irq_mux_set_active(m, vec);
        goto out;
    }

    case TIMER_REG_DEV_TYPE:
    case TIMER_REG_DEV_VERSION:
    case TIMER_REG_STATUS:
    case TIMER_REG_NUM_UNITS:
        err = SL_ERR_IO_NOWR;
        goto out;

    // default:    err = SL_ERR_IO_INVALID;            goto out;
    default:    break;
    }
    if (addr >= TIMER_APERTURE_LENGTH(t->num_units)) {
        err = SL_ERR_IO_INVALID;
        goto out;
    }
    u4 reg;
    sled_timer_unit_t *u = addr_to_unit(t, addr, &reg);
    switch (reg) {
    case 0: // TIMER_REG_UNIT_CONFIG
        timer_set_unit_config_locked(t, u, val);                   break;

    case 1: // TIMER_REG_UNIT_RESET_VAL_LO
        u->reset_val = (u->reset_val & 0xffffffff00000000) | val;       break;

    case 2: // TIMER_REG_UNIT_RESET_VAL_HI
        u->reset_val = (u->reset_val & 0xffffffff) | ((u8)val << 32);   break;

    case 3: // TIMER_REG_UNIT_CURRENT_VAL_LO
    case 4: // TIMER_REG_UNIT_CURRENT_VAL_HI
        err = SL_ERR_IO_NOWR;                               break;

    default:
        err = SL_ERR_IO_INVALID;                            break;
    }

out:
    sl_device_unlock(t->dev);
    return err;
}

static void timer_release(void *ctx) {
    if (ctx == NULL) return;
    sled_timer_t *t = ctx;
    sl_obj_release(t->chrono);
    free(t);
}

static const sl_dev_ops_t timer_ops = {
    .type = SL_DEV_SLED_TIMER,
    .read = timer_read,
    .write = timer_write,
    .create = sled_timer_create,
    .release = timer_release,
};

int sled_timer_create(const char *name, sl_dev_config_t *cfg, sl_dev_t **dev_out) {
    sled_timer_t *t = calloc(1, sizeof(sled_timer_t));
    if (t == NULL) return SL_ERR_MEM;

    int err = sl_device_allocate(name, cfg, TIMER_APERTURE_LENGTH(TIMER_MAX_UNITS), &timer_ops, dev_out);
    if (err) {
        free(t);
        return err;
    }
    t->dev = *dev_out;
    sl_device_set_context(t->dev, t);
    t->chrono = sl_machine_get_chrono(cfg->machine);
    t->num_units = 1;
    t->scalar = 1;
    for (int i = 0; i < TIMER_MAX_UNITS; i++) t->unit[i].timer = t;
    return 0;
}

DECLARE_DEVICE(sled_timer, SL_DEV_TIMER, &timer_ops);
