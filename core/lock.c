// MIT License

// Copyright (c) 2022 Shac Ron

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// SPDX-License-Identifier: MIT License

#include <core/lock.h>

void lock_init(lock_t *l) {
    pthread_mutex_init(&l->mu, NULL);
}

void lock_lock(lock_t *l) {
    pthread_mutex_lock(&l->mu);
}

void lock_unlock(lock_t *l) {
    pthread_mutex_unlock(&l->mu);
}

void lock_destroy(lock_t *l) {
    pthread_mutex_destroy(&l->mu);
}

void cond_init(cond_t *c) {
    pthread_cond_init(&c->cond, NULL);
}

void cond_wait(cond_t *c, lock_t *l) {
    pthread_cond_wait(&c->cond, &l->mu);
}

void cond_signal_one(cond_t *c) {
    pthread_cond_signal(&c->cond);
}

void cond_signal_all(cond_t *c) {
    pthread_cond_broadcast(&c->cond);
}

void cond_destroy(cond_t *c) {
    pthread_cond_destroy(&c->cond);
}
