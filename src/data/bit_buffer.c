#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "buffer.h"
#include "error.h"
#include "hexdump.h"
#include "bit_buffer.h"

static uint8_t _get_bit(uint8_t *byte, uint8_t bit_offset);
static void _set_bit(uint8_t *byte, uint8_t bit_offset, uint8_t bit);

bit_buffer *bb_from_buffer(buffer *buf)
{
    bit_buffer *bit_buf = malloc(sizeof(bit_buffer));
    bit_buf->data = buf->data;
    bit_buf->size = buf->length;
    bit_buf->byte = buf->data;
    bit_buf->bit = 0;
    return bit_buf;
}

buffer *bb_to_buffer(bit_buffer *bit_buf)
{
    buffer *buf = malloc(sizeof(buffer));
    buf->data = bit_buf->data;
    buf->length = bit_buf->size;
    return buf;
}

error *bb_read(bit_buffer *bb, uint8_t *data, size_t n_bits, bool reverse_bits)
{
    buffer *out_buf = init_buffer(n_bits / sizeof(uint8_t));
    bit_buffer *output = bb_from_buffer(out_buf);

    if (n_bits > ((bb->byte - bb->data) * sizeof(uint8_t) + bb->bit))
        n_bits = (bb->byte - bb->data) * sizeof(uint8_t) + bb->bit;

    for (size_t n_bits_read = 0; n_bits_read < n_bits; n_bits_read++)
    {

        uint8_t bit = _get_bit(bb->byte, bb->bit);
        _set_bit(output->byte, output->bit, bit);
        output->bit++;
        bb->bit--;
        if (bb->bit == 0)
        {
            bb->byte--;
            bb->bit = 7;
        }
    }
}

static uint8_t _get_bit(uint8_t *byte, uint8_t bit_offset)
{
    return (*byte >> bit_offset) & 1;
}

void *bb_write(bit_buffer *bit_buf, uint8_t *data, size_t n_bits)
{
    uint8_t *next_byte = bit_buf->data;
    uint8_t bit_offset = bit_buf->bit_index;
    uint8_t byte;
    uint8_t bit;
    uint8_t n_bits_written = 0;

    while (n_bits_written < n_bits)
    {
        byte = *next_byte;
        bit = get_bit(data, n_bits_written);
        set_bit(byte, bit_offset, bit);
        bit_offset++;
        n_bits_written++;
    }
}

static void _set_bit(uint8_t *byte, uint8_t bit_offset, uint8_t bit)
{
    *byte |= (bit << bit_offset);
}

void bb_print(bit_buffer *bit_buf)
{
    hexdump(stderr, bit_buf->data, bit_buf->size);
}

// void bit_buffer_read(bit_buffer *buffer, uint8_t *data, size_t n_bits)
// {
//     uint8_t *next_byte = buffer->data;
//     uint8_t bit_offset = buffer->bit_index;
//     uint8_t byte;
//     uint8_t bit;
//     uint8_t n_bits_read = 0;

//     while (n_bits_read < n_bits)
//     {
//         byte = *next_byte;
//         bit = get_bit(byte, bit_offset);
//         set_bit(data, n_bits_read, bit);
//         bit_offset++;
//         n_bits_read++;
//     }
// }

// void bit_buffer_skip(bit_buffer *buffer, size_t n_bits)
// {
//     buffer->bit_index += n_bits;
// }

// void bit_buffer_seek(bit_buffer *buffer, size_t n_bits)
// {
//     buffer->bit_index = n_bits;
// }

// void bit_buffer_seek_byte(bit_buffer *buffer, size_t n_bytes)
// {
//     buffer->byte_index = n_bytes;
// }
