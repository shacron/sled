// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <sled/types.h>

// sl_chrono_t provides a core timer implementation that can be used to implement asynchronous timer devices

#ifdef __cplusplus
extern "C" {
#endif

// setup functions
int sl_chrono_create(const char *name, sl_chrono_t **c_out);
void sl_chrono_destroy(sl_chrono_t *c);

int sl_chrono_run(sl_chrono_t *c);
int sl_chrono_pause(sl_chrono_t *c);
int sl_chrono_stop(sl_chrono_t *c);

int sl_chrono_timer_set(sl_chrono_t *c, u8 us, int (*callback)(void *context, int err), void *context, u8 *id_out);
int sl_chrono_timer_get_remaining(sl_chrono_t *c, u8 id, u8 *remain_out);
int sl_chrono_timer_cancel(sl_chrono_t *c, u8 id);

#ifdef __cplusplus
}
#endif
