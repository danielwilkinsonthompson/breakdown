#include "buffer.h"
#include "stream.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "hexdump.h"

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
    stream *s = stream_init(10);
    now = clock();
    printf("stream_init: time taken: %fus\n", (double)(now - t0));
    // stream_print(s);

    // // Write literal to stream
    // t0 = clock();
    // stream_write_bytes(s, (uint8_t *)"Hello ", 7, false);
    // now = clock();
    // printf("stream_write_bytes Hello time taken: %fus\n", (double)(now - t0));
    // // stream_print(s);

    // // Read literal back from stream
    // t0 = clock();
    // uint8_t *b2 = stream_read_bytes(s, 7, false);
    // if (b2 == NULL)
    //     return EXIT_FAILURE;
    // now = clock();
    // if (b2 == NULL)
    //     return EXIT_FAILURE;
    // printf("Time taken: %fus\n", (double)(now - t0));
    // printf("Read: %s\n", b2);
    // // stream_print(s);

    // // Write literal to stream
    // t0 = clock();
    // stream_write_bytes(s, (uint8_t *)"World!", 7, false);
    // now = clock();
    // printf("stream_write_bytes World! time taken: %fus\n", (double)(now - t0));
    // // stream_print(s);

    // // Read literal back from stream
    // // t0 = clock();
    // b2 = stream_read_bytes(s, 7, false);
    // if (b2 == NULL)
    //     return EXIT_FAILURE;
    // // now = clock();
    // if (b2 == NULL)
    //     return EXIT_FAILURE;
    // // printf("Time taken: %fus\n", (double)(now - t0));
    // printf("Read: %s\n", b2);
    // // stream_print(s);

    // Write bits to stream
    uint8_t bits_to_write = 0b00010101;
    t0 = clock();
    stream_write_bits(s, &bits_to_write, 5, false);
    stream_write_bits(s, &bits_to_write, 5, false);
    stream_write_bits(s, &bits_to_write, 5, false);
    now = clock();
    stream_print(s);
    printf("Time taken: %fus\n", (double)(now - t0));

    // Read buffer from stream
    t0 = clock();
    uint8_t *b = stream_read_bits(s, 15, false);
    now = clock();
    if (b == NULL)
        return EXIT_FAILURE;

    hexdump(stderr, b, 2);
    printf("Time taken: %fus\n", (double)(now - t0));

    // Free memory
    free(b);
    // free(b2);
    stream_free(s);

    return EXIT_SUCCESS;
}