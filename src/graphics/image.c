/*=============================================================================
                                  image.c
-------------------------------------------------------------------------------
read and write images

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:

TODO:
    - png
    - jpg
-----------------------------------------------------------------------------*/
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

static image_pixel image_pixel_interpolate(image_pixel p1, image_pixel p2, float ratio);

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

image *image_resize(image *img, uint32_t height, uint32_t width)
{
    image *new_img;

    if image_invalid (img)
        return NULL;

    new_img = image_init(height, width);
    if image_invalid (new_img)
        return NULL;

    float x_ratio = (float)img->width / (float)width;
    float y_ratio = (float)img->height / (float)height;

    // bilinear interpolation of pixels from img to new_img
    for (uint32_t row = 0; row < height; row++)
    {
        for (uint32_t col = 0; col < width; col++)
        {
            float interp_x = x_ratio * col; // equivalent location in img
            float interp_y = y_ratio * row; // equivalent location in img

            image_pixel x0y0 = img->pixel_data[(uint32_t)interp_y * img->width + (uint32_t)interp_x];
            image_pixel x1y0 = img->pixel_data[(uint32_t)interp_y * img->width + (uint32_t)interp_x + 1];
            image_pixel x0y1 = img->pixel_data[(uint32_t)interp_y * img->width + (uint32_t)interp_x + img->width];
            image_pixel x1y1 = img->pixel_data[(uint32_t)interp_y * img->width + (uint32_t)interp_x + img->width + 1];

            image_pixel interp_x0y0_x1y0 = image_pixel_interpolate(x0y0, x1y0, interp_x - (uint32_t)interp_x);
            image_pixel interp_x0y1_x1y1 = image_pixel_interpolate(x0y1, x1y1, interp_x - (uint32_t)interp_x);

            image_pixel interpolated_pixel = image_pixel_interpolate(interp_x0y0_x1y0, interp_x0y1_x1y1, interp_y - (uint32_t)interp_y);

            new_img->pixel_data[row * width + col] = interpolated_pixel;
            // find the 4 nearest pixels in img
            // calculate the weighted average of the 4 pixels
            // set the pixel in new_img to the weighted average
        }
    }

    return new_img;

malloc_error:
    free(new_img);
    return NULL;
}

static image_pixel image_pixel_interpolate(image_pixel p1, image_pixel p2, float ratio)
{
    uint8_t red, green, blue, alpha;
    image_pixel interpolated_pixel;

    red = (uint8_t)((float)image_r(p1) * (1 - ratio) + (float)image_r(p2) * ratio);
    green = (uint8_t)((float)image_g(p1) * (1 - ratio) + (float)image_g(p2) * ratio);
    blue = (uint8_t)((float)image_b(p1) * (1 - ratio) + (float)image_b(p2) * ratio);
    alpha = (uint8_t)((float)image_a(p1) * (1 - ratio) + (float)image_a(p2) * ratio);

    interpolated_pixel = image_argb(alpha, red, green, blue);

    return interpolated_pixel;
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
