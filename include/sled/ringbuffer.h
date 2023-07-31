// SPDX-License-Identifier: MIT License
// Copyright (c) 2023 Shac Ron and The Sled Project

#pragma once

#include <unistd.h>

#include <sled/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Thread-safe ring-buffer queue

#define SL_RB_FLAG_WRITER (1u << 0)

typedef struct {
    u4 read_index;
    u4 write_index;
    u4 length; // in bytes
    u4 flags;
    void *base;
} sl_ringbuf_client_t;

// Returns a constant value of the header size that will be written to the ringbuf
// The caller of sl_ringbuf_init should add this to their expected usable space.
usize sl_ringbuf_get_header_size(void);

// Initialize a ringbuf in a memory region. This should be called once,
// before clients are initialized.
int sl_ringbuf_init(void *base, usize len);

// Create a ringbuf client for ringbuf in previously initialized memory region.
// Flags should either be Q_FLAG_WRITER for a writer client, or 0 for a reader.
int sl_ringbuf_client_init(void *base, sl_ringbuf_client_t *c, u4 flags);

// Read or write bytes from the ringbuf through client.
// The call will return fewer bytes than requested if more bytes are not available,
// and zero if there are no bytes available to read or space to write.
// On some platforms a negative value may be returned in case of an IO error.
// If a NULL 'buf' pointer is passed to read(), 'len' bytes will be consumed.
ssize_t sl_ringbuf_read(sl_ringbuf_client_t *c, void *buf, usize len);
ssize_t sl_ringbuf_write(sl_ringbuf_client_t *c, const void *buf, usize len);

// Return the number of bytes currently in the ringbuf for consumption.
u4 sl_ringbuf_num_bytes(sl_ringbuf_client_t *c);

// Returns the number of bytes that can be added to the ringbuf before it is full.
u4 sl_ringbuf_num_free(sl_ringbuf_client_t *c);

#ifdef __cplusplus
}
#endif
