// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <core/types.h>
#include <sled/mapper.h>

typedef struct map_ent map_ent_t;

struct sl_mapper {
    int mode;
    uint32_t num_ents;
    map_ent_t **list;
    sl_mapper_t *next;
};

void mapper_init(sl_mapper_t *m);
void mapper_shutdown(sl_mapper_t *m);

void mapper_set_mode(sl_mapper_t *m, int mode);
int mapper_update(sl_mapper_t *m, sl_event_t *ev);