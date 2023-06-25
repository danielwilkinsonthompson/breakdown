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

#define MAX_RLE1_RUN 127

static uint8_t _nibble_at_index(uint8_t *buffer, uint32_t index);

uint8_t *rle1_compress(uint8_t *in, uint32_t count, uint32_t *size)
{
    uint8_t *out;
    rle1 *rle = NULL;
    uint8_t run_total = 0;
    uint8_t bit_value;

    // output buffer should be shorter than input buffer, otherwise not compressed!
    out = (uint8_t *)calloc((size_t)count * 2, sizeof(uint8_t));
    if (out == NULL)
        return NULL;

    *size = 0;
    rle = (rle1 *)out; // head of output buffer
    rle->length = 0;

    for (uint32_t byte_ind = 0; byte_ind < count; byte_ind++)
    {
        // if we're starting a new run, set the bit value
        if (rle->length == 0)
        {
            rle->value = (in[byte_ind] & 0x80) >> 7; // starting at msb
            rle->length = 0;
        }

        for (uint8_t bit_ind = 8; bit_ind > 0; bit_ind--)
        {
            run_total = rle->length;
            bit_value = (in[byte_ind] & (0x01 << (bit_ind - 1))) >> (bit_ind - 1);

            if (bit_value == rle->value)
            {
                run_total++;
            }
            else
            {
                *size += 1;
                rle++;
                rle->value = bit_value;
                run_total = 1;
            }

            // deal with overflowing the run length counter
            if (run_total > MAX_RLE1_RUN)
            {
                rle->length = MAX_RLE1_RUN;
                *size += 1;
                rle++;
                rle->value = bit_value;
                rle->length = run_total - MAX_RLE1_RUN;
            }
            else
            {
                rle->length = run_total;
            }
        }
    }
    *size += 1; // include the rle in output length

    return out;
}

uint8_t *rle1_decompress(uint8_t *in, uint32_t count, uint32_t *size)
{
    uint8_t *out;
    uint32_t out_index;
    uint32_t output_length_guess;
    static uint32_t compression_ratio_guess = 5;

    uint8_t partial_byte_bits = 0;

    rle1 *rle;
    rle = (rle1 *)in;

    // recursive search for correct output buffer length
    output_length_guess = count * compression_ratio_guess;
    out = (uint8_t *)calloc(sizeof(uint8_t), output_length_guess);
    if (out == NULL)
        return NULL;
    *size = 0;

    for (uint32_t byte_ind = 0; byte_ind < count; byte_ind++)
    {
        rle = (rle1 *)&in[byte_ind];
        if (rle->value == 1)
        {
            for (uint8_t bit_ind = 0; bit_ind < rle->length; bit_ind++)
            {
                out_index = *size + bit_ind / 8;
                out[out_index] |= rle->value << (7 - ((bit_ind + partial_byte_bits) % 8));
            }
        }
        partial_byte_bits += (rle->length % 8);
        if (partial_byte_bits >= 8)
        {
            *size += partial_byte_bits / 8;
            partial_byte_bits -= 8 * (partial_byte_bits / 8);
        }
        *size += (rle->length / 8);
        if (*size >= output_length_guess)
        {
            free(out);
            // call recursively until buffer is big enough
            compression_ratio_guess *= 1.5;
            out = rle1_decompress(in, count, size);
        }
    }

    return out;
}

// uint8_t *rle4_compress(uint8_t *in, uint32_t count, uint32_t *size)
// {
//     uint8_t *out;
//     rle4 *rle;
//     uint32_t run_count = 0;
//     uint32_t last_index = 0;
//     uint8_t upper;
//     uint8_t lower;
//     bool match = false;

//     out = (uint8_t *)calloc(count, sizeof(uint8_t));
//     if (out == NULL)
//         return NULL;
//     rle = (rle4 *)out;

//     *size = 0;
//     upper = _nibble_at_index(in, 0);
//     lower = _nibble_at_index(in, 1);
//     for (uint32_t index = 0; index < (count * 2); index++)
//     {
//         // if (index < (count - 1))
//         // {
//         //     fprintf(stderr, "in[%d] = %1x, in[%d] = %1x,\r\n", index, _nibble_at_index(in, index), index + 1, _nibble_at_index(in, index + 1));
//         //     fprintf(stderr, "upper = %1x, lower = %1x\r\n", upper, lower);
//         // }
//         // fprintf(stderr, "nibble = %d\r\n", index);
//         if (upper == _nibble_at_index(in, index))
//         {
//             match = true;
//             run_count++;
//             index++;
//             // fprintf(stderr, "match at nibble %d\r\n", index);
//             if (lower == _nibble_at_index(in, index))
//             {
//                 run_count++;
//                 index++;
//                 // fprintf(stderr, "match at nibble %d\r\n", index);
//             }
//             else
//             {
//                 match = false;
//             }
//         }
//         // fprintf(stderr, "run_count = %d\r\n", run_count);

//         if (!match || (run_count == 0) || (index >= (count - 1)))
//         {
//             fprintf(stderr, "match = %c\r\n", match ? '1' : '0');
//             fprintf(stderr, "run_count = %d\r\n", run_count);
//             fprintf(stderr, "index = %d vs count = %d\r\n", index, count);
//             // finish out existing rle
//             do
//             {
//                 rle->upper = upper;
//                 rle->lower = lower;
//                 if (run_count >= 256)
//                 {
//                     rle->length = 255;
//                     run_count -= 255;
//                 }
//                 else
//                     rle->length = run_count;
//                 fprintf(stderr, "out[%d] = %02x %02x\r\n", *size, out[index / 2], out[index / 2 + 1]);

//                 rle++;
//                 *size += 2;
//             } while (run_count >= 256);

//             upper = _nibble_at_index(in, index);
//             lower = _nibble_at_index(in, index + 1);
//             run_count = 0;
//         }
//         match = false;
//     }

//     return out;
// }

uint8_t *rle4_compress(uint8_t *in, uint32_t count, uint32_t *size)
{
    uint8_t *out;
    uint32_t run_total;
    bool odd = false;

    *size = 0;
    out = (uint8_t *)calloc(count * 2, sizeof(uint8_t));
    if (out == NULL)
        return NULL;

    // need to deal with overflowing run length counter
    // out[*size] = 2;
    run_total = 2;
    out[*size + 1] = in[0];
    for (uint32_t i = 1; i < count; i++)
    {
        if (odd == false)
        {
            if (in[i] == in[i - 1])
            {
                run_total += 2;
                // out[*size] += 2;
                continue;
            }
            if ((in[i] & 0xF0) == (in[i - 1] & 0xF0))
            {
                run_total += 1;
                // out[*size] += 1;
                odd = true;
            }
            out[*size + 1] = in[i - 1];

            *size += 2;
            out[*size] = 2;
        }
        else
        {
            if (i < (count - 1))
            {
                if (((in[i] & 0x0F) == (in[i - 1] & 0x0F)) && ((in[i] & 0xF0) == (in[i + 1] & 0xF0)))
                {
                    out[*size] += 2;
                    continue;
                }
            }
            if ((in[i] & 0x0F) == (in[i - 1] & 0x0F))
            {
                out[*size] += 1;
                odd = false;
            }
            out[*size + 1] = ((in[i - 1] & 0x0F) << 4) + ((in[i] & 0xF0) >> 4);
            *size += 2;
            out[*size] = 2;
            i++;
        }
    }

    *size += 2;

    return out;
}

uint8_t *rle4_decompress(uint8_t *in, uint32_t size, uint32_t *count)
{
    uint8_t *out;
    uint32_t out_index;
    uint32_t output_length_guess;
    static uint32_t compression_ratio_guess = 5;

    uint8_t partial_byte = 0;
    rle4 *rle;
    rle = (rle4 *)in;

    // recursive search for correct output buffer length
    output_length_guess = size * compression_ratio_guess;
    out = (uint8_t *)calloc(sizeof(uint8_t), output_length_guess);
    if (out == NULL)
        return NULL;
    *count = 0;

    // for (uint32_t byte_ind = 0; byte_ind < count; byte_ind++)
    // {

    //     rle = (rle4 *)&in[byte_ind];
    //     for (uint8_t nibble_ind = 0; nibble_ind < rle->length; nibble_ind++)
    //     {
    //         out_index = *count + nibble_ind / 2;
    //         out[out_index] |= rle->value & (0xF0 >> 4 * (nibble_ind) % 2) << (7 - 4 * ((nibble_ind + partial_byte) % 2));
    //     }
    //     partial_byte += (rle->length % 2);
    //     if (partial_byte >= 2)
    //     {
    //         *count += 2;
    //         partial_byte -= 2 * (partial_byte_bits / 8);
    //     }
    //     *size += (rle->length / 8);
    //     if (*size >= output_length_guess)
    //     {
    //         free(out);
    //         // call recursively until buffer is big enough
    //         compression_ratio_guess *= 1.5;
    //         out = rle1_decompress(in, count, size);
    //     }
    // }

    return out;
}

static uint8_t _nibble_at_index(uint8_t *buffer, uint32_t index)
{
    return ((index % 2) == 0) ? ((buffer[index / 2] & 0xF0) >> 4) : ((buffer[index / 2]) & 0x0F);
}

// rle8_compress()

// rle8_decompress()
