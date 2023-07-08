#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "huffman.h"

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
    huffman_buffer *uncompressed;
    huffman_buffer *compressed;

    uint32_t data[] = {'C', 'A', 'C', 'B', 'I', 'I', 'C', 'D', 'A', 'B', 'C', 'A', 'B', 'A', 'Z', 'Z', 'Z', 'D', 'Q'};
    uint32_t length = sizeof(data) / sizeof(data[0]);

    uncompressed = huffman_buffer_init(length);
    uncompressed->data = data;

    fprintf(stderr, "Starting list: ");
    for (uint8_t i = 0; i < uncompressed->length; i++)
    {
        fprintf(stderr, "%C", uncompressed->data[i]);
    }
    fprintf(stderr, "\r\n");

    compressed = huffman_compress(uncompressed);

    return 0;
}