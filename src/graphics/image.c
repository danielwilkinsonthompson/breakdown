/*=============================================================================
                                  image.h
-------------------------------------------------------------------------------
read and write images

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:
 - https://github.com/danielwilkinsonthompson/breakdown

TODO:
    - read
    - write
    - scale/interpolate
    - bmp
    - png
    - jpg
    -
*/

#include <stdio.h>  // debugging, file read/write
#include <stdint.h> // type definitions
#include <stdlib.h> // malloc/free
#include <string.h>
#include "bmp.h"
#include "image.h"
#include "array2d.h"

static image_type _supported_types[] = {
    [BMP] = {.extension = "bmp", .read = bmp_read, .write = bmp_write}
    // [PNG] = {.extension = "png", .read = png_read, .write = png_write}
};

#define NOF_IMAGE_TYPES (sizeof(_supported_types) / sizeof(image_type))
image *image_read(const char *filename)
{
    FILE *image_file;
    image *img;

    const char *file_type = strrchr(filename, '.');
    if (!file_type || file_type == filename)
        return NULL;
    file_type = file_type + 1;

    for (int type = 0; type < NOF_IMAGE_TYPES; type++)
    {
        if (strstr(file_type, _supported_types[type].extension) != 0)
        {
            img = _supported_types[type].read(filename);
            break;
        }
        if (type == NOF_IMAGE_TYPES)
        {
            printf("Not a recognised image file\r\n");
            return NULL;
        }
    }

    return img;
}

void image_write(image *img, const char *filename)
{
}

void image_printf(const char *format, image *img)
{
}

void image_fprintf(FILE *f, const char *format, image *img, uint32_t left_start, uint32_t bottom_start, uint32_t width, uint32_t height)
{
    if image_invalid (img)
        return;

    fprintf(f, "    ");
    for (uint8_t col_no = 0; col_no < 16; col_no++)
    {
        fprintf(f, "   ");
        fprintf(f, format, col_no);
        fprintf(f, "    ");
    }
    fprintf(f, "\r\n");
    for (uint8_t row = 0; row < height; row++)
    {
        fprintf(f, format, row * 16);
        fprintf(f, ": ");
        for (uint8_t col = 0; col < width; col++)
        {
            fprintf(f, format, img->pixel_data[bottom_start + row][left_start + col].red);
            fprintf(f, ".");
            fprintf(f, format, img->pixel_data[bottom_start + row][left_start + col].green);
            fprintf(f, ".");
            fprintf(f, format, img->pixel_data[bottom_start + row][left_start + col].blue);
            fprintf(f, " ");
        }
        fprintf(f, "\r\n");
    }
}

void image_free(image *img)
{
    free_2d((void **)img->pixel_data, img->height);
    free(img);
}
