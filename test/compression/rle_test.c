#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "rle.h"

// uint8_t input_buffer[] = {0b00001111, 0b11110000};
// uint8_t input_buffer[] = {0b11001111, 0b11110000};
// uint8_t input_buffer[] = {0b11111111, 0b11111111};
// uint8_t input_buffer[] = {0b11111111, 0b11111111, 0b11110000};
// uint8_t input_buffer[] = {0b11111111, 0b11111111, 0b11110000, 0b00000000, 000000000};

#define INPUT_BUF_LEN
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c\n"
#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

int main(int argc, char *argv[])
{
    uint8_t *compressed_buffer;
    uint32_t compressed_length;
    uint8_t *decompressed_buffer;
    uint32_t decompressed_length;
    // uint8_t input_buffer[100] = {0};
    uint8_t input_buffer[25] = {0b11111111, 0b11111111, 0b11110000, 0b00110000, 0b00101010};

    compressed_buffer = rle1_compress(input_buffer, sizeof(input_buffer), &compressed_length);

    fprintf(stderr, "compressed_length %d\r\n", compressed_length);
    if (compressed_buffer == NULL)
    {
        fprintf(stderr, "could not compress buffer\r\n");
        return -1;
    }
    else
        fprintf(stderr, "compression ratio %lu%%\r\n", 100 * sizeof(input_buffer) / compressed_length);

    for (uint32_t out_ind = 0; out_ind < compressed_length; out_ind++)
    {
        fprintf(stderr, "out[%d] = 0b%c%c%c%c_%c%c%c%c\r\n", out_ind, BYTE_TO_BINARY(compressed_buffer[out_ind]));
    }

    decompressed_buffer = rle1_decompress(compressed_buffer, compressed_length, &decompressed_length);

    if (decompressed_buffer == NULL)
    {
        fprintf(stderr, "could not decompress buffer\r\n");
        return -1;
    }
    fprintf(stderr, "decompressed length = %d\r\n", decompressed_length);

    // for (uint32_t out_ind = 0; out_ind < decompressed_length; out_ind++)
    // {
    //     fprintf(stderr, "out[%d] = 0b%c%c%c%c_%c%c%c%c\r\n", out_ind, BYTE_TO_BINARY(decompressed_buffer[out_ind]));
    // }

    uint32_t mismatched_bytes = 0;
    if (decompressed_length != sizeof(input_buffer))
        fprintf(stderr, "decompressed length does not match original buffer length\r\n");

    else
    {
        for (uint32_t ind = 0; ind < decompressed_length; ind++)
        {
            mismatched_bytes += (input_buffer[ind] == decompressed_buffer[ind] ? 0 : 1);
            if (input_buffer[ind] != decompressed_buffer[ind])
                fprintf(stderr, "data[%d]: 0b%c%c%c%c_%c%c%c%c%c= 0b%c%c%c%c_%c%c%c%c\r\n", ind, BYTE_TO_BINARY(input_buffer[ind]), (input_buffer[ind] == decompressed_buffer[ind] ? ' ' : '!'), BYTE_TO_BINARY(decompressed_buffer[ind]));
        }
    }
    if (mismatched_bytes > 0)
    {
        fprintf(stderr, "decompressed data does not match original data: %d mismatched bytes\r\n", mismatched_bytes);
    }
    else
    {
        fprintf(stderr, "decompressed data matches original data\r\n");
    }

    free(compressed_buffer);
    return 0;
}