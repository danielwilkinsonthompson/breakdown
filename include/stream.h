#ifndef __stream_h
#define __stream_h

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "error.h"
#include "buffer.h"

typedef struct stream_bit_t
{
    uint8_t *byte;
    uint8_t bit;
} stream_bit;

typedef struct stream_t stream;

typedef void (*stream_full_function)(stream *s, size_t size);
typedef void (*stream_empty_function)(stream *s, size_t size);

typedef struct stream_t
{
    uint8_t *data;
    size_t length; // stream length in bits
    size_t capacity;
    stream_bit head;
    stream_bit tail;
    stream_empty_function empty;
    stream_full_function full;
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

// file
// error stream_fwrite(FILE *file, stream *s, bool reverse_bits);
// error stream_fread(FILE *file, stream *s, bool reverse_bits);

// debug
void stream_print(stream *s);

// free
void stream_free(stream *s);

#endif