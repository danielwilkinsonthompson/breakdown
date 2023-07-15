#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "gz.h"

int main(int argc, char *argv[])
{
    const char *filename;
    if (argc > 0)
        filename = argv[1];
    else
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    buffer *buf = gz_read(filename);
    if (buf == NULL)
    {
        fprintf(stderr, "Error reading file\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}