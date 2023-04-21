// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#if __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

#include <core/types.h>

struct sl_sem {
#if __APPLE__
    dispatch_semaphore_t dsem;
#else
    sem_t psem;
#endif
};

int sl_sem_init(sl_sem_t *sem, unsigned int value);
int sl_sem_post(sl_sem_t *sem);
int sl_sem_wait(sl_sem_t *sem);
int sl_sem_trywait(sl_sem_t *sem);
int sl_sem_timedwait(sl_sem_t *restrict sem, const struct timespec *restrict abs_timeout);
int sl_sem_destroy(sl_sem_t *sem);

