/*=============================================================================
                                    rle.c
-------------------------------------------------------------------------------
run length encoding

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-wmf/73b57f24-6d78-4eeb-9c06-8f892d88f1ab

TODO:
- Implement BI_RLE8 and BI_RLE4 compression

-----------------------------------------------------------------------------*/
#include <stdint.h> // uint8_t
#include <stdlib.h> // malloc
#include <stdio.h>  // fprintf
#include <stdbool.h>
#include "rle.h"

#define MAX_RUN 127

#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

uint32_t compression_ratio_guess = 5;

uint8_t *rle1_compress(uint8_t *input_buffer, uint32_t input_length, uint32_t *output_length)
{
    uint8_t *output_buffer;
    rle1 *current_rle = NULL;
    uint8_t run_total = 0;
    uint8_t bit_value;

    // output buffer should be shorter than input buffer, otherwise not compressed
    output_buffer = (uint8_t *)calloc((size_t)input_length, sizeof(uint8_t));
    if (output_buffer == NULL)
        return NULL;

    *output_length = 0;
    current_rle = (rle1 *)output_buffer; // head of output buffer
    current_rle->length = 0;

    for (uint32_t buffer_index = 0; buffer_index < input_length; buffer_index++)
    {
        // if we're starting a new run, set the bit value
        if (current_rle->length == 0)
        {
            current_rle->value = (input_buffer[buffer_index] & 0x80) >> 7; // starting at msb
            current_rle->length = 0;
        }

        for (uint8_t bit_index = 8; bit_index > 0; bit_index--)
        {
            run_total = current_rle->length;
            bit_value = (input_buffer[buffer_index] & (0x01 << (bit_index - 1))) >> (bit_index - 1);

            if (bit_value == current_rle->value)
            {
                run_total++;
            }
            else
            {
                if (*output_length < input_length)
                {
                    *output_length += 1;
                    current_rle++;
                }
                else
                {
                    *output_length = input_length;
                    free(output_buffer);
                    return NULL;
                }
                current_rle->value = bit_value;
                run_total = 1;
            }

            // deal with overflowing the run length counter
            if (run_total > MAX_RUN)
            {
                current_rle->length = MAX_RUN;
                if (*output_length < input_length)
                {
                    *output_length += 1;
                    current_rle++;
                }
                else
                {
                    *output_length = input_length;
                    free(output_buffer);
                    return NULL;
                }
                current_rle->value = bit_value;
                current_rle->length = run_total - MAX_RUN;
            }
            else
            {
                current_rle->length = run_total;
            }
        }
    }
    *output_length += 1; // include the current_rle in output length

    return output_buffer;
}

uint8_t *rle1_decompress(uint8_t *input_buffer, uint32_t input_length, uint32_t *output_length)
{
    uint8_t *output_buffer;
    uint32_t output_buffer_index;
    uint32_t output_length_guess;
    uint8_t partial_byte_bits = 0;
    rle1 *current_rle;
    current_rle = (rle1 *)input_buffer;

    // how do we know how large the output might be?
    output_length_guess = input_length * compression_ratio_guess;
    output_buffer = (uint8_t *)calloc(sizeof(uint8_t), output_length_guess);
    if (output_buffer == NULL)
        return NULL;
    *output_length = 0;

    for (uint32_t byte_ind = 0; byte_ind < input_length; byte_ind++)
    {
        current_rle = (rle1 *)&input_buffer[byte_ind];
        if (current_rle->value == 1)
        {
            // bug here when current_rle->length = 127??
            for (uint8_t bit_ind = 0; bit_ind < current_rle->length; bit_ind++)
            {
                output_buffer_index = *output_length + bit_ind / 8;
                output_buffer[output_buffer_index] |= current_rle->value << (7 - ((bit_ind + partial_byte_bits) % 8));
            }
        }
        partial_byte_bits += (current_rle->length % 8);
        if (partial_byte_bits >= 8)
        {
            *output_length += partial_byte_bits / 8;
            partial_byte_bits -= 8 * (partial_byte_bits / 8);
        }
        *output_length += (current_rle->length / 8);
        if (*output_length >= output_length_guess)
        {
            free(output_buffer);
            // call recursively until buffer is big enough
            compression_ratio_guess *= 1.5;
            output_buffer = rle1_decompress(input_buffer, input_length, output_length);
        }
    }

    return output_buffer;
}

// rle4_compress()

// rle4_decompress()

// rle8_compress()

// rle8_decompress()
