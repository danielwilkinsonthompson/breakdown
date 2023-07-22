#ifndef __stream_h
#define __stream_h

#include <stdint.h>
#include <stdbool.h>
#include "error.h"
#include "buffer.h"

typedef struct stream_bit_t
{
    uint8_t *byte;
    uint8_t bit;
} stream_bit;

typedef struct stream_t
{
    uint8_t *data;
    size_t length;
    size_t capacity;
    stream_bit head;
    stream_bit tail;
} stream;

// typedef struct stream_t stream;

// init
stream *stream_init(size_t size);
stream *stream_init_from_bytes(uint8_t *bytes, size_t size, bool reverse_bits);
stream *stream_init_from_buffer(buffer *b, bool reverse_bits);

// write
size_t stream_write_bits(stream *s, uint8_t *bits, size_t size, bool reverse_bits);
size_t stream_write_bytes(stream *s, uint8_t *bytes, size_t size, bool reverse_bits);
error stream_write_buffer(stream *s, buffer *buf, bool reverse_bits);

// read
uint8_t *stream_read_bits(stream *s, size_t size, bool reverse_bits);
uint8_t *stream_read_bytes(stream *s, size_t size, bool reverse_bits);
buffer *stream_read_buffer(stream *s, size_t size, bool reverse_bits);

// debug
void stream_print(stream *s);

// free
void stream_free(stream *s);

#endif