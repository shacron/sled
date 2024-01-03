// SPDX-License-Identifier: MIT License
// Copyright (c) 2023-2024 Shac Ron and The Sled Project

#include <core/sem.h>
#include <sled/error.h>

#if __APPLE__

int sl_sem_init(sl_sem_t *sem, unsigned int value) {
    sem->dsem = dispatch_semaphore_create(value);
    if (sem->dsem == NULL) return SL_ERR_MEM;
    return 0;
}

int sl_sem_post(sl_sem_t *sem) {
    dispatch_semaphore_signal(sem->dsem);
    return 0;
}

int sl_sem_wait(sl_sem_t *sem) {
    dispatch_semaphore_wait(sem->dsem, DISPATCH_TIME_FOREVER);
    return 0;
}

int sl_sem_trywait(sl_sem_t *sem) {
    if (dispatch_semaphore_wait(sem->dsem, DISPATCH_TIME_NOW) < 0)
        return SL_ERR_BUSY;
    return 0;
}

int sl_sem_timedwait(sl_sem_t *restrict sem, const struct timespec *restrict abs_timeout) {
    dispatch_time_t dt = dispatch_walltime(abs_timeout, 0);
    if (dispatch_semaphore_wait(sem->dsem, dt) < 0)
        return SL_ERR_TIMEOUT;
    return 0;
}

int sl_sem_destroy(sl_sem_t *sem) {
    dispatch_release(sem->dsem);
    return 0;
}

#else

#include <errno.h>

int sl_sem_init(sl_sem_t *sem, unsigned int value) {
    if (sem_init(&sem->psem, 0, value)) return SL_ERR_ARG; // errno = EINVAL
    return 0;
}

int sl_sem_post(sl_sem_t *sem) {
    if (sem_post(&sem->psem)) {
        if (errno == EINVAL) return SL_ERR_ARG;
        if (errno == EOVERFLOW) return SL_ERR_FULL;
        return SL_ERR;
    }
    return 0;
}

int sl_sem_wait(sl_sem_t *sem) {
retry:
    if (sem_wait(&sem->psem)) {
        if (errno == EINTR) goto retry;
        if (errno == EINVAL) return SL_ERR_ARG;
        return SL_ERR;
    }
    return 0;
}

int sl_sem_trywait(sl_sem_t *sem) {
retry:
    if (sem_trywait(&sem->psem)) {
        switch (errno) {
        case EINTR:     goto retry;
        case EINVAL:    return SL_ERR_ARG;
        case EAGAIN:    return SL_ERR_BUSY;
        default:        return SL_ERR;
        }
    }
    return 0;
}

int sl_sem_timedwait(sl_sem_t *restrict sem, const struct timespec *restrict abs_timeout) {
retry:
    if (sem_timedwait(&sem->psem, abs_timeout)) {
        switch (errno) {
        case EINTR:     goto retry;
        case EINVAL:    return SL_ERR_ARG;
        case ETIMEDOUT: return SL_ERR_TIMEOUT;
        default:        return SL_ERR;
        }
    }
    return 0;
}

int sl_sem_destroy(sl_sem_t *sem) {
    if (sem_destroy(&sem->psem)) {
        if (errno == EINVAL) return SL_ERR_ARG;
        return SL_ERR;
    }
    return 0;
}

#endif // __APPLE__

