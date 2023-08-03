/*


*/
#include <stdio.h>  // printf
#include <stdlib.h> // size_t
#include <string.h> // strrchr
#include <time.h>
#include "gz.h"

// type2 file contents from https://www.youtube.com/watch?v=SJPvNi4HrWQ
const uint8_t type2[42] = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x03, 0x3d, 0xc6, 0x39, 0x11, 0x00, 0x00,
                           0x0c, 0x02, 0x30, 0x2b, 0xb5, 0x52, 0x1e, 0xff,
                           0x96, 0x38, 0x16, 0x96, 0x5c, 0x1e, 0x94, 0xcb,
                           0x6d, 0x01, 0x17, 0x1c, 0x39, 0xb4, 0x13, 0x00,
                           0x00, 0x00};

int main(int argc, char *argv[])
{
    const char *filename;
    char output_filename[256];
    if (argc > 1)
        filename = argv[1];
    else
    {
        printf("No filename provided, using default\n");
        FILE *fp = fopen("block_type2.txt.gz", "wb");
        if (fp == NULL)
        {
            fprintf(stderr, "Error opening file\n");
            return EXIT_FAILURE;
        }
        fwrite(type2, sizeof(uint8_t), sizeof(type2), fp);
        fclose(fp);
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strncpy(output_filename, filename, 256);
    char *file_extension = strrchr(output_filename, '.');
    if (!file_extension || file_extension == output_filename)
        return EXIT_FAILURE;
    *file_extension = '\0';

    clock_t start = clock();
    buffer *buf = gz_read(filename);
    clock_t end = clock();
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time taken: %0.3fs\r\n", time_taken);
    if (buf == NULL)
    {
        fprintf(stderr, "Error reading file\n");
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(output_filename, "w");
    for (size_t i = 0; i < buf->length; i++)
    {
        fprintf(fp, "%c", buf->data[i]);
    }
    fclose(fp);

    // buffer *buf2 = buffer_init(2);
    // buffer_write(buf2, (uint8_t *)"a\n", 2);
    // gz_write("test.gz", buf2);

    return EXIT_SUCCESS;
}