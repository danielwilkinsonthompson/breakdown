#include "buffer.h"
#include "stream.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[])
{
    // check inputs
    if (argc != 1)
    {
        printf("Usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Initialise stream
    clock_t t0, now;
    t0 = clock();
    stream *s = stream_init(9);
    now = clock();
    printf("Time taken: %fus\n", (double)(now - t0));
    stream_print(s);

    // Write literal to stream
    t0 = clock();
    stream_write_bytes(s, (uint8_t *)"Hello!", 6, true);
    now = clock();
    printf("Time taken: %fus\n", (double)(now - t0));

    // Write bits to stream
    uint8_t bits_to_write = 0b00010101;
    t0 = clock();
    stream_write_bits(s, &bits_to_write, 5, true);
    stream_write_bits(s, &bits_to_write, 5, true);
    stream_write_bits(s, &bits_to_write, 5, true);
    now = clock();
    stream_print(s);
    printf("Time taken: %fus\n", (double)(now - t0));

    // Read buffer from stream
    t0 = clock();
    buffer *b = stream_read_buffer(s, 9, true);
    now = clock();
    if (b == NULL)
        return EXIT_FAILURE;

    buffer_print(b);
    printf("Time taken: %fus\n", (double)(now - t0));

    // Free memory
    buffer_free(b);
    stream_free(s);

    return EXIT_SUCCESS;
}