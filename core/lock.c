// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <core/lock.h>

#define LOCK_DEBUG  1

#if LOCK_DEBUG
#include <assert.h>
#include <stdio.h>
#define LOCK_ASSERT_OK(x) assert((x) == 0)
#else
#define LOCK_ASSERT_OK(x) x
#endif

void lock_init(lock_t *l) {
    LOCK_ASSERT_OK(pthread_mutex_init(&l->mu, NULL));
}

void lock_lock(lock_t *l) {
    LOCK_ASSERT_OK(pthread_mutex_lock(&l->mu));
}

void lock_unlock(lock_t *l) {
    LOCK_ASSERT_OK(pthread_mutex_unlock(&l->mu));
}

void lock_destroy(lock_t *l) {
    LOCK_ASSERT_OK(pthread_mutex_destroy(&l->mu));
}

void cond_init(cond_t *c) {
    LOCK_ASSERT_OK(pthread_cond_init(&c->cond, NULL));
}

void cond_wait(cond_t *c, lock_t *l) {
    LOCK_ASSERT_OK(pthread_cond_wait(&c->cond, &l->mu));
}

void cond_signal_one(cond_t *c) {
    LOCK_ASSERT_OK(pthread_cond_signal(&c->cond));
}

void cond_signal_all(cond_t *c) {
    LOCK_ASSERT_OK(pthread_cond_broadcast(&c->cond));
}

void cond_destroy(cond_t *c) {
    LOCK_ASSERT_OK(pthread_cond_destroy(&c->cond));
}
