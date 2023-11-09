// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#if __APPLE__
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <core/host.h>

u8 host_get_clock_ns(void) {
#if __APPLE__
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#else
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (tp.tv_usec * 1000) + (tp.tv_sec * 1000000000);
#endif
}
