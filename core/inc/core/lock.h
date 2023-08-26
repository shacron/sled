// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <pthread.h>

#include <core/types.h>

struct lock {
    pthread_mutex_t mu;
};

struct cond {
    pthread_cond_t cond;
};

void lock_init(lock_t *l);
void lock_lock(lock_t *l);
void lock_unlock(lock_t *l);
void lock_destroy(lock_t *l);

void cond_init(cond_t *c);
void cond_wait(cond_t *c, lock_t *l);
int cond_timed_wait_abs(cond_t *c, lock_t *l, u8 utime);
void cond_signal_one(cond_t *c);
void cond_signal_all(cond_t *c);
void cond_destroy(cond_t *c);
