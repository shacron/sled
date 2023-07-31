// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#include <stdatomic.h>
#include <string.h>

#include <sled/error.h>
#include <sled/ringbuffer.h>
#include <sled/types.h>

#define IS_WRITER(c) (c->flags & SL_RB_FLAG_WRITER)
#define IS_READER(c) ((c->flags & SL_RB_FLAG_WRITER) == 0)

typedef struct {
    u4 read_index;
    u4 write_index;
    u4 length; // in bytes
    u4 flags;
    u1 data[];
} rb_header_t;

static inline u4 load_explicit32(const u4 *p, memory_order order) {
    return atomic_load_explicit((const _Atomic u4 *)p, order);
}

static inline void store_explicit32(u4 *p, u4 value, memory_order order) {
    atomic_store_explicit((_Atomic u4 *)p, value, order);
}

usize sl_ringbuf_get_header_size(void) { return sizeof(rb_header_t); }

int sl_ringbuf_init(void *base, usize len) {
    if (base == NULL) return SL_ERR_ARG;
    if (len <= (sizeof(rb_header_t) + 4)) return SL_ERR_ARG;
    rb_header_t *q = base;
    q->read_index = 0;
    q->write_index = 0;
    q->length = len - sizeof(rb_header_t);
    q->flags = 0;
    return 0;
}

int sl_ringbuf_client_init(void *base, sl_ringbuf_client_t *c, u4 flags) {
    rb_header_t *q = base;
    c->flags = flags;
    c->base = base;
    c->read_index = load_explicit32(&q->read_index, memory_order_acquire);
    c->write_index = load_explicit32(&q->write_index, memory_order_relaxed);
    c->length = load_explicit32(&q->length, memory_order_relaxed);
    return 0;
}

u4 sl_ringbuf_num_bytes(sl_ringbuf_client_t *c) {
    rb_header_t *q = c->base;
    if (IS_WRITER(c)) {
        c->read_index = load_explicit32(&q->read_index, memory_order_relaxed);
    } else {
        c->write_index = load_explicit32(&q->write_index, memory_order_relaxed);
    }
    u4 write_index = c->write_index;
    if (write_index < c->read_index) write_index += c->length;
    return write_index - c->read_index;
}

u4 sl_ringbuf_num_free(sl_ringbuf_client_t *c) {
    return c->length - sl_ringbuf_num_bytes(c) - 1;
}

ssize_t sl_ringbuf_read(sl_ringbuf_client_t *c, void *buf, usize len) {
    if (!IS_READER(c)) return SL_ERR_ARG;
    if (len == 0) return 0;

    rb_header_t *q = c->base;
    c->write_index = load_explicit32(&q->write_index, memory_order_acquire);
    if (c->read_index == c->write_index) return 0; // nothing to read

    ssize_t num_read = 0;
    if (c->read_index > c->write_index) {
        // read from [read_index to end)
        u4 bytes = c->length - c->read_index;
        if (bytes > len) bytes = len;
        if (buf != NULL) memcpy(buf, q->data + c->read_index, bytes);
        num_read += bytes;
        c->read_index += bytes;
        if (c->read_index == c->length) c->read_index = 0;
        len -= bytes;
        if (buf != NULL) buf += bytes;
    }

    if ((c->read_index < c->write_index) && (len > 0)) {
        // read from [read_index to write_index)
        u4 bytes = c->write_index - c->read_index;
        if (bytes > len) bytes = len;
        if (buf != NULL) memcpy(buf, q->data + c->read_index, bytes);
        num_read += bytes;
        c->read_index += bytes;
    }
    store_explicit32(&q->read_index, c->read_index, memory_order_relaxed);
    return num_read;
}

ssize_t sl_ringbuf_write(sl_ringbuf_client_t *c, const void *buf, usize len) {
    if (!IS_WRITER(c)) return SL_ERR_ARG;
    if (len == 0) return 0;

    rb_header_t *q = c->base;
    c->read_index = load_explicit32(&q->read_index, memory_order_relaxed);

    if (c->write_index + 1 == c->read_index) return 0;
    if ((c->read_index == 0) && (c->write_index == c->length - 1)) return 0;

    ssize_t num_written = 0;
    if (c->read_index <= c->write_index) {
        // write [write_index to end)
        u4 bytes = c->length - c->write_index;
        if (c->read_index == 0) bytes--;
        if (bytes > len) bytes = len;
        memcpy(q->data + c->write_index, buf, bytes);
        num_written = bytes;
        len -= bytes;
        buf += bytes;
        c->write_index += bytes;
        if (c->write_index == c->length) c->write_index = 0;
    }

    if ((c->write_index < c->read_index) && (len > 0)) {
        u4 bytes = c->read_index - c->write_index - 1;
        if (bytes > len) bytes = len;
        if (bytes > 0) {
            memcpy(q->data + c->write_index, buf, bytes);
            num_written += bytes;
            c->write_index += bytes;
        }
    }

    store_explicit32(&q->write_index, c->write_index, memory_order_release);
    return num_written;
}

