/*=============================================================================
                                  image.h
-------------------------------------------------------------------------------
read and write images

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:
 - https://github.com/danielwilkinsonthompson/breakdown

TODO:
    - write
    - scale/interpolate
    - png
    - jpg
*/

#include <stdio.h>  // debugging, file read/write
#include <stdint.h> // type definitions
#include <stdlib.h> // malloc/free
#include <string.h>
#include "bmp.h"
#include "image.h"
#include "array2d.h"

typedef image *(*_image_read_function)(const char *filename);
typedef void (*_image_write_function)(image *img, const char *filename);

typedef struct _image_type_t
{
    const char *extension;
    _image_read_function read;
    _image_write_function write;
} _image_type;

static _image_type _supported_types[] = {
    {.extension = "bmp", .read = bmp_read, .write = bmp_write}
    // [PNG] = {.extension = "png", .read = png_read, .write = png_write}
};

static _image_type *_image_get_type(const char *filename);
#define NOF_IMAGE_TYPES (sizeof(_supported_types) / sizeof(_image_type))

static _image_type *_image_get_type(const char *filename)
{
    _image_type *matching_type;
    const char *file_extension = strrchr(filename, '.');

    if (!file_extension || file_extension == filename)
        return NULL;
    file_extension = file_extension + 1; // start at extension, not '.'

    for (int type = 0; type < NOF_IMAGE_TYPES; type++)
    {
        if (strstr(file_extension, _supported_types[type].extension) != 0)
        {
            matching_type = &_supported_types[type];
            break;
        }
        if (type == NOF_IMAGE_TYPES)
        {
            printf("Not a recognised image file\r\n");
            return NULL;
        }
    }

    return matching_type;
}

image *image_init(uint32_t height, uint32_t width)
{
    image *img;

    img = (image *)malloc(sizeof(image));
    if (img == NULL)
        return NULL;

    img->width = width;
    img->height = height;

    img->pixel_data = (image_pixel *)malloc(height * width * sizeof(image_pixel));
    if (img->pixel_data == NULL)
    {
        free(img);
        return NULL;
    }

    return img;
}

image *image_read(const char *filename)
{
    FILE *image_file;
    image *img;
    _image_type *handler;

    handler = _image_get_type(filename);
    if (handler == NULL)
        return NULL;

    img = handler->read(filename);
    if image_invalid (img)
        return NULL;

    return img;
}

void image_write(image *img, const char *filename)
{
    FILE *image_file;
    _image_type *handler;

    if image_invalid (img)
        return;

    handler = _image_get_type(filename);
    if (handler == NULL)
        return;

    handler->write(img, filename);
}

void image_printf(const char *format, image *img)
{
}

void image_fprintf(FILE *f, const char *format, image *img, uint32_t left_start, uint32_t bottom_start, uint32_t width, uint32_t height)
{
    if image_invalid (img)
        return;

    fprintf(f, "    ");
    for (uint8_t col_no = 0; col_no < width; col_no++)
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
            fprintf(f, format, image_r(img->pixel_data[(bottom_start + row) * img->width + left_start + col]));
            fprintf(f, ".");
            fprintf(f, format, image_g(img->pixel_data[(bottom_start + row) * img->width + left_start + col]));
            fprintf(f, ".");
            fprintf(f, format, image_b(img->pixel_data[(bottom_start + row) * img->width + left_start + col]));
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
