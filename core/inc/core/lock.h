// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <pthread.h>

#include <core/types.h>

struct sl_lock {
    pthread_mutex_t mu;
};

struct sl_cond {
    pthread_cond_t cond;
};

void sl_lock_init(sl_lock_t *l);
void sl_lock_lock(sl_lock_t *l);
void sl_lock_unlock(sl_lock_t *l);
void sl_lock_destroy(sl_lock_t *l);

void sl_cond_init(sl_cond_t *c);
void sl_cond_wait(sl_cond_t *c, sl_lock_t *l);
int sl_cond_timed_wait_abs(sl_cond_t *c, sl_lock_t *l, u8 utime);
void sl_cond_signal_one(sl_cond_t *c);
void sl_cond_signal_all(sl_cond_t *c);
void sl_cond_destroy(sl_cond_t *c);
