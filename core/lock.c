// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <errno.h>

#include <core/lock.h>
#include <sled/error.h>

#define LOCK_DEBUG  1

#if LOCK_DEBUG
#include <assert.h>
#include <stdio.h>
#define LOCK_ASSERT_OK(x) assert((x) == 0)
#else
#define LOCK_ASSERT_OK(x) x
#endif

void sl_lock_init(sl_lock_t *l) {
    LOCK_ASSERT_OK(pthread_mutex_init(&l->mu, NULL));
}

void sl_lock_lock(sl_lock_t *l) {
    LOCK_ASSERT_OK(pthread_mutex_lock(&l->mu));
}

void sl_lock_unlock(sl_lock_t *l) {
    LOCK_ASSERT_OK(pthread_mutex_unlock(&l->mu));
}

void sl_lock_destroy(sl_lock_t *l) {
    LOCK_ASSERT_OK(pthread_mutex_destroy(&l->mu));
}

void sl_cond_init(sl_cond_t *c) {
    LOCK_ASSERT_OK(pthread_cond_init(&c->cond, NULL));
}

void sl_cond_wait(sl_cond_t *c, sl_lock_t *l) {
    LOCK_ASSERT_OK(pthread_cond_wait(&c->cond, &l->mu));
}

int sl_cond_timed_wait_abs(sl_cond_t *c, sl_lock_t *l, u8 utime) {
    struct timespec ts;
    ts.tv_sec  = utime / 1000000;
    ts.tv_nsec = (utime % 1000000) * 1000;
    int err = pthread_cond_timedwait(&c->cond, &l->mu, &ts);
    if (err == 0) return 0;
#if LOCK_DEBUG
    assert(err == ETIMEDOUT);
#endif
    return SL_ERR_TIMEOUT;
}

void sl_cond_signal_one(sl_cond_t *c) {
    LOCK_ASSERT_OK(pthread_cond_signal(&c->cond));
}

void sl_cond_signal_all(sl_cond_t *c) {
    LOCK_ASSERT_OK(pthread_cond_broadcast(&c->cond));
}

void sl_cond_destroy(sl_cond_t *c) {
    LOCK_ASSERT_OK(pthread_cond_destroy(&c->cond));
}
