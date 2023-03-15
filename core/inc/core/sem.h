// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>

#if __APPLE__

#include <dispatch/dispatch.h>

struct sl_sem {
    dispatch_semaphore_t dsem;
};

#else

#include <semaphore.h>

struct sl_sem {
    sem_t psem;
};

#endif

int sl_sem_init(sl_sem_t *sem, unsigned int value);
int sl_sem_post(sl_sem_t *sem);
int sl_sem_wait(sl_sem_t *sem);
int sl_sem_trywait(sl_sem_t *sem);
int sl_sem_timedwait(sl_sem_t *restrict sem, const struct timespec *restrict abs_timeout);
int sl_sem_destroy(sl_sem_t *sem);

