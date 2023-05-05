// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>
#include <sled/mapper.h>

typedef struct map_ent map_ent_t;

struct sl_mapper {
    int mode;
    u32 num_ents;
    map_ent_t **list;
    sl_mapper_t *next;
    sl_map_ep_t ep;
};

void mapper_init(sl_mapper_t *m);
void mapper_shutdown(sl_mapper_t *m);

int mapper_update(sl_mapper_t *m, sl_event_t *ev);
