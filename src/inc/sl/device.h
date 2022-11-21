#pragma once

#include <stdint.h>

#include <sled/device.h>

struct device {
    uint32_t type;
    uint32_t id;
    uint64_t base;
    uint32_t length;
    const char *name;

    int (*read)(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    int (*write)(device_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf);
    void (*destroy)(device_t *dev);
};

// device API
int device_create(uint32_t type, const char *name, device_t **dev_out);

// internal device calls
void dev_init(device_t *d, uint32_t type);
