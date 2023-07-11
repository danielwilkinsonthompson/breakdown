#include <stdlib.h> // malloc, free
#include <stdint.h> // type definitions
#include <stdio.h>  // debugging, file read/write
#include "image.h"
#include "png.h"

int main(int argc, char *argv[])
{
    image *test;
    if (argc > 1)
        test = png_read(argv[1]);
    // test = image_read(argv[1]);
    else
    {
        fprintf(stderr, "No file specified\r\n");
        return EXIT_FAILURE;
    }

    if (test == NULL)
        return EXIT_FAILURE;

    fprintf(stderr, "Image read: %s\r\n", argv[1]);

    return EXIT_SUCCESS;
}