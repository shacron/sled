// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2023 Shac Ron and The Sled Project

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <core/common.h>
#include <core/device.h>
#include <device/sled/uart.h>
#include <sled/error.h>

#define UART_TYPE 'rxtx'
#define UART_VERSION 0

#define BUFLEN  255

#define FROMDEV(d) containerof((d), uart_t, dev)

typedef struct {
    sl_dev_t dev;
    int io_type;
    int fd_in;
    int fd_out;

    // registers
    uint32_t config;
    uint32_t status;
    uint32_t buf_pos;

    uint8_t buf[BUFLEN + 1];
} uart_t;

static void uart_flush(uart_t *u) {
    u->buf[u->buf_pos] = '\0';
    if (u->fd_out >= 0) dprintf(u->fd_out, "%s", u->buf);
    u->buf_pos = 0;
}

static int uart_read(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    uart_t *u = FROMDEV(d);
    uint32_t *val = buf;
    int err = 0;

    lock_lock(&d->lock);
    switch (addr) {
    case UART_REG_DEV_TYPE:     *val = UART_TYPE;           break;
    case UART_REG_DEV_VERSION:  *val = UART_VERSION;        break;
    case UART_REG_CONFIG:       *val = u->config;           break;
    case UART_REG_STATUS:       *val = u->status;           break;
    case UART_REG_FIFO_READ:    *val = 0;                   break;
    case UART_REG_FIFO_WRITE:   err = SL_ERR_IO_NORD;       break;
    default:                    err = SL_ERR_IO_INVALID;    break;
    }
    lock_unlock(&d->lock);
    return err;
}

static int uart_write(sl_dev_t *d, uint64_t addr, uint32_t size, uint32_t count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    uart_t *u = FROMDEV(d);
    uint32_t val = *(uint32_t *)buf;
    uint8_t c;
    int err = 0;

    lock_lock(&d->lock);
    switch (addr) {
    case UART_REG_CONFIG:   u->config = val;        break;

    case UART_REG_FIFO_WRITE:
        c = (uint8_t)val;
        u->buf[u->buf_pos++] = c;
        if ((c == '\n') || (u->buf_pos == BUFLEN)) uart_flush(u);
        break;

    case UART_REG_DEV_TYPE:
    case UART_REG_DEV_VERSION:
    case UART_REG_STATUS:
    case UART_REG_FIFO_READ:
        err = SL_ERR_IO_NOWR;
        break;

    default:    err = SL_ERR_IO_INVALID;        break;
    }
    lock_unlock(&d->lock);
    return err;
}

int sled_uart_set_channel(sl_dev_t *dev, int io, int fd_in, int fd_out) {
    uart_t *u = FROMDEV(dev);
    switch (io) {
    case UART_IO_NULL:
        u->fd_in = u->fd_out = -1;
        break;

    case UART_IO_FILE:
        u->fd_in = -1;
        u->fd_out = fd_out;
        break;

    case UART_IO_CONS:
    case UART_IO_PORT:
        u->fd_in = fd_in;
        u->fd_out = fd_out;
        break;

    default:
        return -1;
    }
    u->io_type = io;
    return 0;
}

void uart_destroy(sl_dev_t *dev) {
    if (dev == NULL) return;
    uart_t *u = FROMDEV(dev);
    if (u->buf_pos > 0) uart_flush(u);
    dev_shutdown(dev);
    free(u);
}

int uart_create(sl_dev_t **dev_out) {
    uart_t *u = calloc(1, sizeof(uart_t));
    if (u == NULL) return SL_ERR_MEM;
    *dev_out = &u->dev;

    dev_init(&u->dev, SL_DEV_UART);
    u->fd_in = STDIN_FILENO;
    u->fd_out = STDOUT_FILENO;
    u->dev.length = UART_APERTURE_LENGTH;
    u->dev.read = uart_read;
    u->dev.write = uart_write;
    u->dev.destroy = uart_destroy;
    return 0;
}


