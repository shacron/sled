// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <core/common.h>
#include <core/lock.h>
#include <core/obj.h>
#include <core/chrono.h>
#include <sled/error.h>

#define FROM_NODE(n) containerof((n), sl_timer_t, node)

#define SL_CHRONO_STATE_NULL           0
#define SL_CHRONO_STATE_STOPPED        1
#define SL_CHRONO_STATE_RUNNING        2
#define SL_CHRONO_STATE_PAUSED         3
#define SL_CHRONO_STATE_EXITING        4

typedef struct {
    sl_list_node_t node;
    u8 id;
    u8 expiry;      // us
    u8 reset_value; // us
    u4 flags;
    int (*callback)(void *context, int err);
    void *context;
} sl_timer_t;

static inline void chrono_lock(sl_chrono_t *c) { lock_lock(&c->lock); }
static inline void chrono_unlock(sl_chrono_t *c) { lock_unlock(&c->lock); }

static u8 get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

static int timer_compare(const sl_list_node_t *na, const sl_list_node_t *nb) {
    const sl_timer_t *a = FROM_NODE(na);
    const sl_timer_t *b = FROM_NODE(nb);
    if (a->expiry > b->expiry) return 1;
    if (a->expiry == b->expiry) return 0;
    return -1;
}

void sl_chrono_retain(sl_chrono_t *c) { sl_obj_retain(c->obj_); }
void sl_chrono_release(sl_chrono_t *c)  { sl_obj_release(c->obj_); }


int sl_chrono_timer_set(sl_chrono_t *c, u8 us, int (*callback)(void *context, int err), void *context, u8 *id_out) {
    sl_timer_t *t;
    int err = 0;

    u8 now = get_time_us();

    chrono_lock(c);

    sl_list_node_t *n = sl_list_remove_first(&c->unused_timers);
    if (n != NULL) {
        t = FROM_NODE(n);
    } else {
        t = malloc(sizeof(*t));
        if (t == NULL) {
            err = SL_ERR_MEM;
            goto out;
        }
        t->id = c->next_id;
        c->next_id++;
    }
    t->expiry = now + us;
    t->reset_value = us;
    t->callback = callback;
    t->context = context;
    *id_out = t->id;
    sl_list_insert_sorted(&c->active_timers, timer_compare, &t->node);
    cond_signal_one(&c->cond);

out:
    chrono_unlock(c);
    return err;
}

int sl_chrono_timer_get_remaining(sl_chrono_t *c, u8 id, u8 *remain_out) {
    int err = SL_ERR_NOT_FOUND;
    u8 remain;

    u8 now = get_time_us();

    chrono_lock(c);

    for (sl_list_node_t *n = sl_list_peek_first(&c->active_timers); n != NULL; n = n->next) {
        sl_timer_t *t = FROM_NODE(n);
        if (t->id == id) {
            if (t->expiry <= now) remain = 0;
            else remain = t->expiry - now;
            err = 0;
            break;
        }
    }

    chrono_unlock(c);
    if (err == 0) *remain_out = remain;
    return err;
}

int sl_chrono_timer_cancel(sl_chrono_t *c, u8 id) {
    int err = SL_ERR_NOT_FOUND;

    chrono_lock(c);

    sl_list_node_t *p = NULL; 
    for (sl_list_node_t *n = sl_list_peek_first(&c->active_timers); n != NULL; n = n->next) {
        sl_timer_t *t = FROM_NODE(n);
        if (t->id == id) {
            if (p == NULL) sl_list_remove_first(&c->active_timers);
            else p->next = n->next;
            sl_list_add_last(&c->unused_timers, n);
            cond_signal_one(&c->cond);
            err = 0;
            break;
        }
        p = n;
    }

    chrono_unlock(c);
    return err;
}

static void chrono_thread_running_locked(sl_chrono_t *c) {
    int err = 0;
    sl_list_t expired_list, restart_list, unused_list;
    sl_list_init(&expired_list);
    sl_list_init(&restart_list);
    sl_list_init(&unused_list);

    while (c->state == SL_CHRONO_STATE_RUNNING) {
        u8 now = get_time_us();
        sl_list_node_t *n = NULL;
        u8 next_exp = 0;

        for ( ; ; ) {
            n = sl_list_peek_first(&c->active_timers);
            if (n == NULL) break;
            sl_timer_t *t = FROM_NODE(n);
            if (t->expiry > now) {
                next_exp = t->expiry;
                break;
            }
            sl_list_remove_first(&c->active_timers);
            sl_list_add_last(&expired_list, n);
        }

        n = sl_list_remove_first(&expired_list);
        if (n == NULL) {
            if (next_exp == 0) cond_wait(&c->cond, &c->lock);
            else cond_timed_wait_abs(&c->cond, &c->lock, next_exp);
            continue;
        }

        if (n != NULL) {
            chrono_unlock(c);

            for ( ; n != NULL; n = sl_list_remove_first(&expired_list)) {
                sl_timer_t *t = FROM_NODE(n);
                err = t->callback(t->context, 0);
                if (err == SL_ERR_RESTART) {
                    t->expiry += t->reset_value;
                    sl_list_add_last(&restart_list, n);
                } else {
                    sl_list_add_last(&unused_list, n);
                }
            }

            // bug: attempting to cancel a fired, restarting timer at this point will fail to find it

            chrono_lock(c);

            // todo: implement list merge-insert to reduce these loops
            for (n = sl_list_remove_first(&restart_list); n != NULL; n = sl_list_remove_first(&restart_list)) {
                sl_list_insert_sorted(&c->active_timers, timer_compare, n);
            }
            for (n = sl_list_remove_first(&unused_list); n != NULL; n = sl_list_remove_first(&unused_list)) {
                sl_list_add_last(&c->unused_timers, n);
            }
        }
    }
}

static void * chrono_thread(void *arg) {
    sl_chrono_t *c = arg;

    chrono_lock(c);

    for ( ; ; ) {
        switch (c->state) {
        case SL_CHRONO_STATE_RUNNING:
            chrono_thread_running_locked(c);
            break;

        case SL_CHRONO_STATE_PAUSED:
            cond_wait(&c->cond, &c->lock);
            break;

        case SL_CHRONO_STATE_EXITING:
            c->state = SL_CHRONO_STATE_STOPPED;
            goto out;

        default:
            assert(false);
            goto out;
        }
    }

out:
    chrono_unlock(c);

    sl_list_t active_list;
    sl_list_init(&active_list);
    sl_list_node_t *n = sl_list_peek_first(&active_list);
    for ( ; n != NULL; n = n->next) {
        sl_timer_t *t = FROM_NODE(n);
        t->callback(t->context, SL_ERR_EXITED);
    }

    chrono_lock(c);
    while ((n = sl_list_remove_first(&active_list))) {
        sl_list_add_last(&c->unused_timers, n);
    }
    chrono_unlock(c);
    return NULL;
}

static void chrono_obj_shutdown(void *o) {
    sl_chrono_shutdown(o);
}

int sl_chrono_run(sl_chrono_t *c) {
    int err = 0;

    chrono_lock(c);

    if (c->state == SL_CHRONO_STATE_PAUSED) {
        c->state = SL_CHRONO_STATE_RUNNING;
        cond_signal_one(&c->cond);
        goto out;
    }

    if (c->state != SL_CHRONO_STATE_STOPPED) {
        err = SL_ERR_STATE;
        goto out;
    }

    c->state = SL_CHRONO_STATE_RUNNING;
    if (pthread_create(&c->thread, NULL, chrono_thread, c)) {
        perror("pthread_create");
        err = SL_ERR_SYSTEM;
        c->state = SL_CHRONO_STATE_STOPPED;
    }

out:
    chrono_unlock(c);
    return err;
}

int sl_chrono_pause(sl_chrono_t *c) {
    int err = 0;

    chrono_lock(c);

    if (c->state != SL_CHRONO_STATE_RUNNING) {
        err = SL_ERR_STATE;
        goto out;
    }

    c->state = SL_CHRONO_STATE_PAUSED;
    cond_signal_one(&c->cond);

out:
    chrono_unlock(c);
    return err;
}

int sl_chrono_stop(sl_chrono_t *c) {
    int err = 0;

    chrono_lock(c);

    switch (c->state) {
    case SL_CHRONO_STATE_RUNNING:
    case SL_CHRONO_STATE_PAUSED:
    case SL_CHRONO_STATE_EXITING:
        c->state = SL_CHRONO_STATE_EXITING;
        cond_signal_one(&c->cond);
        break;

    default:
        err = SL_ERR_STATE;
        break;
    }

    chrono_unlock(c);

    if (err) return err;
    pthread_join(c->thread, NULL);
    c->state = SL_CHRONO_STATE_STOPPED;
    return 0;
}

void sl_chrono_shutdown(sl_chrono_t *c) {
    sl_list_node_t *n;

    sl_chrono_stop(c);

    assert(c->state == SL_CHRONO_STATE_STOPPED);
    while ((n = sl_list_remove_first(&c->unused_timers))) {
        sl_timer_t *t = FROM_NODE(n);
        free(t);
    }
    cond_destroy(&c->cond);
    lock_destroy(&c->lock);
}

int sl_chrono_init(sl_chrono_t *c, const char *name, sl_obj_t *o) {
    c->obj_ = o;
    c->name = name;
    sl_list_init(&c->active_timers);
    sl_list_init(&c->unused_timers);
    lock_init(&c->lock);
    cond_init(&c->cond);
    c->state = SL_CHRONO_STATE_STOPPED;
    return 0;
}

int sl_chrono_create(const char *name, sl_chrono_t **c_out) {
    sl_obj_t *o = sl_allocate_as_obj(sizeof(sl_chrono_t), chrono_obj_shutdown);
    if (o == NULL) return SL_ERR_MEM;
    sl_chrono_t *c = sl_obj_get_item(o);
    *c_out = c;
    sl_chrono_init(c, name, o);
    return 0;
}

