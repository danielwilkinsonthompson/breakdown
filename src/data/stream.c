/*=============================================================================
                                stream.c
-------------------------------------------------------------------------------
data streams, in the style used by DEFLATE and GZIP

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

Known bugs
- FIXME: stream_write_bits() and/or stream_read_bits() seems to be reversing byte order
- FIXME: length reported as one extra byte if for a stream with a partial byte
-----------------------------------------------------------------------------*/
#include <stdbool.h> // bool
#include <stdint.h>  // uint8_t
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy
#include "buffer.h"  // buffer
#include "hexdump.h" // hexdump
#include "error.h"   // error
#include "stream.h"  // stream

// init
stream *stream_init(size_t size)
{
    stream *s = (stream *)malloc(sizeof(stream));
    if (s == NULL)
        return NULL;

    s->data = (uint8_t *)malloc(sizeof(uint8_t) * size);
    if (s->data == NULL)
        goto malloc_error;

    s->length = 0;
    s->capacity = size;
    s->head.byte = s->data;
    s->head.bit = 0;
    s->tail.byte = s->data;
    s->tail.bit = 0;

    return s;

malloc_error:
    free(s);
    return NULL;
}

stream *stream_init_from_bytes(uint8_t *bytes, size_t size, bool reverse_bits)
{
    stream *s = stream_init(size);
    if (s == NULL)
        return NULL;

    s->capacity = size;
    s->head.byte = s->data;
    s->head.bit = 0;
    stream_write_bytes(s, bytes, size, reverse_bits);

    return s;
}

stream *stream_init_from_buffer(buffer *b, bool reverse_bits)
{
    stream *s = stream_init(b->length);
    if (s == NULL)
        return NULL;

    s->capacity = b->length;
    s->head.byte = s->data;
    s->head.bit = 0;
    stream_write_buffer(s, b, reverse_bits);

    return s;
}

// write
size_t stream_write_bits(stream *s, uint8_t *src_byte, size_t size, bool reverse_bits)
{
    // note, bits are packed in reverse order by convention

    size_t n_bytes = (size + s->tail.bit) / 8;
    size_t bits_written = 0;

    if (s->length + n_bytes > s->capacity)
    {
        printf("stream_write_bits: not enough space in stream\n");
        printf("stream_write_bits: s->length: %zu\n", s->length);
        printf("stream_write_bits: n_bytes: %zu\n", n_bytes);
        printf("stream_write_bits: s->capacity: %zu\n", s->capacity);

        return 0;
    }

    if (reverse_bits)
        printf("stream_write_bits: reverse_bits\n");

    for (size_t i = 0; i < size; i++)
    {
        // if ((s->length == 0) && (s->tail.bit == 0))
        //     s->length = 1; // partial byte counts as 1 byte in length

        uint8_t bit;
        if (reverse_bits)
        {
            // TODO: this reverses the order of bytes too, do we want that?
            bit = (src_byte[(size - 1 - i) / 8] >> ((size - 1 - i) % 8)) & 0x01;
        }
        else
        {
            bit = (src_byte[i / 8] >> (i % 8)) & 0x01;
        }
        *s->tail.byte |= bit << s->tail.bit;
        s->tail.bit++;
        if (s->tail.bit == 8)
        {
            s->tail.bit = 0;
            s->tail.byte++;
            s->length++;
        }
        if (s->tail.byte >= s->data + s->capacity)
            break;
        bits_written++;
    }

    return bits_written;
}

size_t stream_write_bytes(stream *s, uint8_t *bytes, size_t size, bool reverse_bits)
{
    return stream_write_bits(s, bytes, size * 8, reverse_bits) / 8;
}

error stream_write_buffer(stream *s, buffer *buf, bool reverse_bits)
{
    if (stream_write_bytes(s, buf->data, buf->length, reverse_bits) != buf->length)
        return buffer_underflow;
    else
        return success;
}

// read
uint8_t *stream_read_bits(stream *s, size_t size, bool reverse_bits)
{
    // note, bits are packed in reverse order by convention
    size_t n_bytes = size / 8;
    uint8_t *bits = (uint8_t *)calloc(n_bytes, sizeof(uint8_t));
    if (bits == NULL)
        return NULL;

    // printf("bits: %p (%zu bytes)\n", bits, n_bytes);

    // hexdump(stderr, (const void *)bits, n_bytes);

    // printf("n_bytes: %zu vs s->length: %zu\n", n_bytes, s->length);

    if (n_bytes > s->capacity)
        return NULL;

    for (size_t i = 0; i < size; i++)
    {
        if (reverse_bits)
        {
            // do we also want to reverse the byte order?
            bits[i / 8] |= ((*s->head.byte & (0x01 << s->head.bit)) >> (s->head.bit)) << (7 - (i % 8));
        }
        else
        {
            bits[i / 8] |= ((*s->head.byte & (0x01 << s->head.bit)) >> (s->head.bit)) << (i % 8);
        }
        s->head.bit++;
        if (s->head.bit == 8)
        {
            s->head.bit = 0;
            s->head.byte++;
            s->length--;
        }
        if (s->length == 0)
            break;
    }

    return bits;
}
uint8_t *stream_read_bytes(stream *s, size_t size, bool reverse_bits)
{
    return stream_read_bits(s, size * 8, reverse_bits);
}
buffer *stream_read_buffer(stream *s, size_t size, bool reverse_bits)
{
    buffer *buf = (buffer *)malloc(sizeof(buffer));
    if (buf == NULL)
        return NULL;

    buf->data = stream_read_bytes(s, size, reverse_bits);
    if (buf->data == NULL)
        goto malloc_error;

    buf->length = size;

    return buf;

malloc_error:
    buffer_free(buf);
    return NULL;
}

// debug
void stream_print(stream *s)
{
    fprintf(stderr, "stream: %p\n", s);
    fprintf(stderr, " - data: %p\n", s->data);
    fprintf(stderr, " - length: %zu\n", s->length);
    fprintf(stderr, " - capacity: %zu\n", s->capacity);
    fprintf(stderr, " - head: %p.%d\n", s->head.byte, s->head.bit);
    fprintf(stderr, " - tail: %p.%d\n", s->tail.byte, s->tail.bit);
    fprintf(stderr, " - contents:\n");
    hexdump(stderr, (const void *)s->data, s->capacity);
}

// free
void stream_free(stream *s)
{
    if (s == NULL)
        return;
    if (s->data != NULL)
        free(s->data);
    free(s);
}