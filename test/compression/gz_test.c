/*


*/
#include <stdio.h>  // printf
#include <stdlib.h> // size_t
#include <string.h> // strrchr
#include "gz.h"

int main(int argc, char *argv[])
{
    const char *filename;
    char output_filename[256];
    if (argc > 0)
        filename = argv[1];
    else
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strncpy(output_filename, filename, 256);
    char *file_extension = strrchr(output_filename, '.');
    if (!file_extension || file_extension == output_filename)
        return EXIT_FAILURE;
    *file_extension = '\0';

    buffer *buf = gz_read(filename);
    if (buf == NULL)
    {
        fprintf(stderr, "Error reading file\n");
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(output_filename, "w");

    // buffer_print(buf);
    for (size_t i = 0; i < buf->length; i++)
    {
        fprintf(fp, "%c", buf->data[i]);
    }
    fclose(fp);

    buffer *buf2 = buffer_init(2);
    buffer_write(buf2, (uint8_t *)"a\n", 2);
    gz_write("test.gz", buf2);

    return EXIT_SUCCESS;
}