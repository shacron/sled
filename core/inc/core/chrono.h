// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>
#include <sled/list.h>
#include <sled/chrono.h>

struct sl_chrono {
    sl_obj_t *obj_;
    const char *name;
    u8 next_id;
    lock_t lock;
    cond_t cond;
    sl_list_t active_timers;
    sl_list_t unused_timers;
    pthread_t thread;
    u1 state;
};

int sl_chrono_init(sl_chrono_t *c, const char *name, sl_obj_t *o);
void sl_chrono_shutdown(sl_chrono_t *c);
