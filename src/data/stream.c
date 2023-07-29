/*=============================================================================
                                stream.c
-------------------------------------------------------------------------------
data streams, in the style used by DEFLATE and GZIP

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

known bugs:
- cannot read back partial bytes
-----------------------------------------------------------------------------*/
#include <stdbool.h> // bool
#include <stdint.h>  // uint8_t
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc
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
    size_t bits_written = 0;
    if ((s->length + size) > (s->capacity * 8))
    {
        printf("stream_write_bits: not enough space in stream\n");
        printf("stream_write_bits: s->length: %zu.%zu bytes (%zu bits)\n", s->length / 8, s->length % 8, s->length);
        printf("stream_write_bits: s->capacity: %zu.%d bytes (%zu bits)\n", s->capacity, 0, s->capacity * 8);

        return 0;
    }

    for (size_t i = 0; i < size; i++)
    {
        uint8_t bit;
        if (reverse_bits)
        {
            bit = (src_byte[(size - 1 - i) / 8] >> ((size - 1 - i) % 8)) & 0x01;
        }
        else
        {
            bit = (src_byte[i / 8] >> (i % 8)) & 0x01;
        }
        *s->tail.byte |= bit << s->tail.bit;
        s->tail.bit++;
        s->length++;
        bits_written++;
        if (s->tail.bit == 8)
        {
            s->tail.bit = 0;
            s->tail.byte++;
        }
        if (s->length >= (s->capacity * 8))
            break;
        if (s->tail.byte >= s->data + s->capacity)
            s->tail.byte = s->data;
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
    size_t n_bytes = size / 8 + (size % 8 > 0 ? 1 : 0); // ceil(size / 8)
    uint8_t *bits = (uint8_t *)calloc(n_bytes, sizeof(uint8_t));
    if (bits == NULL)
        return NULL;

    // size_t bytes_occupied = s->tail.byte >= s->head.byte ? (s->tail.byte - s->head.byte) : (s->capacity - (s->head.byte - s->tail.byte));
    // size_t bits_occupied = s->tail.bit >= s->head.bit ? (s->tail.bit - s->head.bit) : (8 - (s->head.bit - s->tail.bit));

    size_t n_bits = s->length;
    // bytes_occupied * 8 + bits_occupied;
    // printf("bits_occupied: %zu bytes_occupied: %zu n_bits: %zu\n", bits_occupied, bytes_occupied, n_bits);
    if (size > s->length)
    {
        printf("error: not enough bits in stream\n");
        printf("stream contains %zu.%zu bytes (%zu bits)\n", s->length / 8, s->length % 8, s->length);
        printf("attempting to read %zu bits\n", size);

        free(bits);
        return NULL;
    }

    for (size_t i = 0; i < size; i++)
    {
        if (reverse_bits)
        {
            uint8_t shift_max = size < 8 ? size - 1 : 7;
            bits[(size - 1 - i) / 8] |= ((*s->head.byte & (0x01 << s->head.bit)) >> (s->head.bit)) << (shift_max - (i % 8));
        }
        else
        {
            bits[i / 8] |= ((*s->head.byte & (0x01 << s->head.bit)) >> (s->head.bit)) << (i % 8);
            // printf("bits[%zu / 8]: %02x\n", i, bits[i / 8]);
        }
        s->head.bit++;
        s->length--;
        if (s->head.bit == 8)
        {
            s->head.bit = 0;
            *s->head.byte = 0;
            s->head.byte++;
        }

        if (s->length == 0)
            break;

        if (s->head.byte >= s->data + s->capacity)
            s->head.byte = s->data;
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
    fprintf(stderr, " - length: %zu.%zu bytes (%zu bits)\n", s->length / 8, s->length % 8, s->length);
    fprintf(stderr, " - capacity: %zu.0 bytes (%zu bits)\n", s->capacity, s->capacity * 8);
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