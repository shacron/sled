// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#pragma once

#include <core/lock.h>
#include <core/mapper.h>
#include <core/obj.h>
#include <core/types.h>
#include <sled/device.h>
#include <sled/event.h>
#include <sled/io.h>
#include <sled/list.h>
#include <sled/worker.h>

#define DEV_FLAG_EMBEDDED   (1u << 0)

struct sl_dev {
    sl_obj_t obj_;
    sl_list_node_t node;

    u4 magic;
    u2 type;
    u8 base;
    const char *name;

    sl_lock_t lock;
    sl_map_ep_t map_ep;     // incoming io from external mapper
    void *context;          // context of owner object
    u4 aperture;
    sl_dev_ops_t ops;
    sl_event_ep_t event_ep; // async event endpoint
    sl_mapper_t *mapper;    // held for owner object, todo: remove
    sl_worker_t *worker;    // worker thread and event loop
    u4 worker_epid;         // id to use for event loop direct events
};

// device API
static inline void dev_lock(sl_dev_t *d) { sl_lock_lock(&d->lock); }
static inline void dev_unlock(sl_dev_t *d) { sl_lock_unlock(&d->lock); }

// internal device calls
void device_config(sl_dev_t *d, sl_dev_config_t *cfg, u4 aperture, const sl_dev_ops_t *ops);

sl_dev_t * device_get_for_ep(sl_map_ep_t *ep);
