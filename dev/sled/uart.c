// SPDX-License-Identifier: MIT License
// Copyright (c) 2022-2024 Shac Ron and The Sled Project

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <device/sled/sled.h>
#include <device/sled/uart.h>
#include <sled/device.h>
#include <sled/error.h>

#define UART_TYPE 'rxtx'
#define UART_VERSION 0

#define BUFLEN  255

typedef struct {
    sl_dev_t *dev;
    int io_type;
    int fd_in;
    int fd_out;

    // registers
    u4 config;
    u4 status;
    u4 buf_pos;

    u1 buf[BUFLEN + 1];
} sled_uart_t;

static void uart_flush(sled_uart_t *u) {
    u->buf[u->buf_pos] = '\0';
    if (u->fd_out >= 0) dprintf(u->fd_out, "%s", u->buf);
    u->buf_pos = 0;
}

static int uart_read(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    sled_uart_t *u = ctx;
    u4 *val = buf;
    int err = 0;

    sl_device_lock(u->dev);
    switch (addr) {
    case UART_REG_DEV_TYPE:     *val = UART_TYPE;           break;
    case UART_REG_DEV_VERSION:  *val = UART_VERSION;        break;
    case UART_REG_CONFIG:       *val = u->config;           break;
    case UART_REG_STATUS:       *val = u->status;           break;
    case UART_REG_FIFO_READ:    *val = 0;                   break;
    case UART_REG_FIFO_WRITE:   err = SL_ERR_IO_NORD;       break;
    default:                    err = SL_ERR_IO_INVALID;    break;
    }
    sl_device_unlock(u->dev);
    return err;
}

static int uart_write(void *ctx, u8 addr, u4 size, u4 count, void *buf) {
    if (size != 4) return SL_ERR_IO_SIZE;
    if (count != 1) return SL_ERR_IO_COUNT;

    sled_uart_t *u = ctx;
    u4 val = *(u4 *)buf;
    u1 c;
    int err = 0;

    sl_device_lock(u->dev);
    switch (addr) {
    case UART_REG_CONFIG:   u->config = val;        break;

    case UART_REG_FIFO_WRITE:
        c = (u1)val;
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
    sl_device_unlock(u->dev);
    return err;
}

int sled_uart_set_channel(sl_dev_t *dev, int io, int fd_in, int fd_out) {
    sled_uart_t *u = sl_device_get_context(dev);
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

static void sled_uart_destroy(sl_dev_t *d) {
    sled_uart_t *u = sl_device_get_context(d);
    if (u->buf_pos > 0) uart_flush(u);
    free(u);
}

static int sled_uart_create(sl_dev_t *d, sl_dev_config_t *cfg) {
    sled_uart_t *u = calloc(1, sizeof(sled_uart_t));
    if (u == NULL) return SL_ERR_MEM;
    u->dev = d;
    sl_device_set_context(d, u);
    cfg->aperture = UART_APERTURE_LENGTH;
    u->fd_in = STDIN_FILENO;
    u->fd_out = STDOUT_FILENO;
    return 0;
}

static const sl_dev_ops_t uart_ops = {
    .type = SL_DEV_SLED_UART,
    .read = uart_read,
    .write = uart_write,
    .create = sled_uart_create,
    .destroy = sled_uart_destroy,
};

DECLARE_DEVICE(sled_uart, SL_DEV_UART, &uart_ops);
