/*=============================================================================
                                    bmp.c
-------------------------------------------------------------------------------
read and write bitmap images

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://github.com/marc-q/libbmp/blob/master/libbmp.h
- https://en.wikipedia.org/wiki/BMP_file_format
-----------------------------------------------------------------------------*/
#include <stdio.h>  // debugging, file read/write
#include <stdlib.h> // malloc/free
#include <math.h>   // absolute for image dimensions
#include "array2d.h"
#include "bmp.h"
#include "image.h"

// #define DEBUG_BMP

static uint16_t _bmp_read_uint16(FILE *bitmap_file);
static uint32_t _bmp_read_uint32(FILE *bitmap_file);
static void _bmp_read_infoheader(FILE *bitmap_file, bmp_header *header);
static void _bmp_read_coreheader(FILE *bitmap_file, bmp_header *header);
static image_pixel _bmp_colour_lookup(bmp *image, uint32_t index);
static const uint8_t bmp_file_header_signature[2] = BMP_FILE_HEADER_SIGNATURE;

image *bmp_read(const char *filename)
{
    FILE *bitmap_file;
    bmp_file_header *bitmap_file_header;
    bmp *bitmap_data;
    image *return_image;

    // check inputs
    if (filename == NULL)
        return NULL;

    // allocate memory for image
    bitmap_data = (bmp *)malloc(sizeof(bmp));
    if (bitmap_data == NULL)
        return NULL;
    bitmap_data->header = (bmp_header *)malloc(sizeof(bmp_header));
    if (bitmap_data->header == NULL)
    {
        bmp_free(bitmap_data);
        return NULL;
    }

    // extract file header
    bitmap_file_header = (bmp_file_header *)malloc(sizeof(bmp_file_header));
    if (bitmap_file_header == NULL)
    {
        bmp_free(bitmap_data);
        return NULL;
    }
    bitmap_file = fopen(filename, "rb+");
    if (bitmap_file == NULL)
    {
        bmp_free(bitmap_data);
        free(bitmap_file_header);
        return NULL;
    }
    fread(bitmap_file_header, sizeof(bmp_file_header), 1, bitmap_file);
    if ((bitmap_file_header->signature[0] != bmp_file_header_signature[0]) | (bitmap_file_header->signature[1] != bmp_file_header_signature[1]))
    {
        bmp_free(bitmap_data);
        free(bitmap_file_header);
        fclose(bitmap_file);
        printf("Not a bitmap file\r\n");
        return NULL;
    }
    bitmap_data->header->header_size = _bmp_read_uint32(bitmap_file);
    switch (bitmap_data->header->header_size)
    {
    case BMP_BITMAPINFOHEADER_SIZE:
        _bmp_read_infoheader(bitmap_file, bitmap_data->header);
        break;
    case BMP_BITMAPCOREHEADER_SIZE:
        _bmp_read_coreheader(bitmap_file, bitmap_data->header);
        break;
    default:
        bmp_free(bitmap_data);
        free(bitmap_file_header);
        fclose(bitmap_file);
        printf("Not a recognised bitmap format\r\n");
        return NULL;
    }

    if (bitmap_data->header->compression >= BI_COMPRESSION_SUPPORT)
        printf("Unsupported compression type\r\n");

    // extract colour table, if any
    if (bitmap_data->header->number_of_colours != BMP_ALL_COLOURS)
    {
        bitmap_data->colour_table = (bmp_colour *)malloc(sizeof(bmp_colour) * bitmap_data->header->number_of_colours);
        fread(bitmap_data->colour_table, sizeof(bmp_colour) * bitmap_data->header->number_of_colours, 1, bitmap_file);
    }
    else
    {
        bitmap_data->colour_table = NULL;
    }

    // allocate memory for pixel data
    return_image = (image *)malloc(sizeof(image));
    return_image->width = bitmap_data->header->width;
    return_image->height = bitmap_data->header->height;
    return_image->pixel_data = (image_pixel **)malloc_2d(return_image->height, return_image->width, sizeof(image_pixel));
    if (return_image->pixel_data == NULL)
    {
        bmp_free(bitmap_data);
        free(bitmap_file_header);
        fclose(bitmap_file);
        return NULL;
    }

    // read pixel data from file
    int32_t padding = (32 - bitmap_data->header->width * bitmap_data->header->bitdepth % 32) % 32;
    size_t padded_width = (bitmap_data->header->width * bitmap_data->header->bitdepth + padding) / 8;
    uint8_t *buffer = (uint8_t *)malloc(padded_width);
    uint8_t *buffer_head;
    uint32_t colour_index = 0;
    if (buffer == NULL)
    {
        bmp_free(bitmap_data);
        free(bitmap_file_header);
        fclose(bitmap_file);
        return NULL;
    }
    for (int32_t row = 0; row < abs(bitmap_data->header->height); row++)
    {
        buffer_head = buffer;
        fread(buffer, sizeof(uint8_t), padded_width, bitmap_file);
        for (int32_t col = 0; col < bitmap_data->header->width; col++)
        {
            switch (bitmap_data->header->bitdepth)
            {
            case 1:
                colour_index = 0x01 & (buffer_head[0] >> (col % 8));
                buffer_head += (col % 8 + 1) / 8;
                break;
            case 4:
                colour_index = 0x0F & (buffer_head[0] >> ((col % 2) * 4));
                buffer_head += col % 2;
                break;
            case 8:
                colour_index = buffer_head[0];
                buffer_head += 1;
                break;
            case 16:
                colour_index = uint8_x2_uint16(buffer_head);
                buffer_head += 2;
                break;
            case 24:
                colour_index = uint8_x3_uint24(buffer_head);
                buffer_head += 3;
                break;
            case 32:
                colour_index = uint8_x4_uint32(buffer_head);
                buffer_head += 4;
                break;
            default:
                colour_index = 0;
                break;
            }
            return_image->pixel_data[row][col] = _bmp_colour_lookup(bitmap_data, colour_index);
        }
    }
    fclose(bitmap_file);
    free(bitmap_file_header);

    return return_image;
}

static uint16_t _bmp_read_uint16(FILE *bitmap_file)
{
    uint8_t buffer[2];
    fread(buffer, sizeof(uint8_t), sizeof(buffer), bitmap_file);
    return uint8_x2_uint16(buffer);
}

static uint32_t _bmp_read_uint32(FILE *bitmap_file)
{
    uint8_t buffer[4];
    fread(buffer, sizeof(uint8_t), sizeof(buffer), bitmap_file);
    return uint8_x4_uint32(buffer);
}

static void _bmp_read_infoheader(FILE *bitmap_file, bmp_header *header)
{
    header->width = _bmp_read_uint32(bitmap_file);
    header->height = _bmp_read_uint32(bitmap_file);
    header->planes = _bmp_read_uint16(bitmap_file);
    header->bitdepth = _bmp_read_uint16(bitmap_file);
    header->compression = _bmp_read_uint32(bitmap_file);
    header->image_size = _bmp_read_uint32(bitmap_file);
    header->width_pixel_per_metre = _bmp_read_uint32(bitmap_file);
    header->height_pixel_per_metre = _bmp_read_uint32(bitmap_file);
    header->number_of_colours = _bmp_read_uint32(bitmap_file);
    header->number_of_important_colours = _bmp_read_uint32(bitmap_file);
}

static void _bmp_read_coreheader(FILE *bitmap_file, bmp_header *header)
{
    header->width = _bmp_read_uint16(bitmap_file);
    header->height = _bmp_read_uint16(bitmap_file);
    header->planes = _bmp_read_uint16(bitmap_file);
    header->bitdepth = _bmp_read_uint16(bitmap_file);
}

static image_pixel _bmp_colour_lookup(bmp *bitmap_data, uint32_t index)
{
    image_pixel colour;

    if bmp_invalid (bitmap_data)
    {
        colour.red = 0;
        colour.green = 0;
        colour.blue = 0;
        colour.alpha = 0xFF;

        return colour;
    }
    colour.alpha = 0xFF;

    if (bitmap_data->colour_table == NULL)
    {
        switch (bitmap_data->header->bitdepth)
        {
        case 1:
        case 4:
        case 8:
            // Unsupported, colour table needed
            colour.red = 0;
            colour.green = 0;
            colour.blue = 0;
        case 16:
            colour.red = 0x000F & index;
            colour.green = (0x00F0 & index) >> 4;
            colour.blue = (0x0F00 & index) >> 8;
        case 24:
        case 32:
            colour.red = (0xFF0000 & index) >> 16;
            colour.green = (0x00FF00 & index) >> 8;
            colour.blue = 0x0000FF & index;
        default:
            break;
        }
    }
    else
    {
        colour.red = bitmap_data->colour_table[index].red;
        colour.green = bitmap_data->colour_table[index].blue;
        colour.blue = bitmap_data->colour_table[index].blue;
    }

    return colour;
}

void bmp_write(image *bitmap_data, const char *filename)
{
    FILE *bmp_file;
    bmp_file_header *file_header;

    bmp_file = fopen(filename, "wb+");
    fclose(bmp_file);
}

void bmp_printf(const char *format, bmp *image)
{
    bmp_fprintf(stderr, format, image, 0, 0, 16, 16);
}

void bmp_fprintf(FILE *f, const char *format, bmp *bitmap_data, uint32_t left_start, uint32_t bottom_start, uint32_t width, uint32_t height)
{
    if bmp_invalid (bitmap_data)
        return;

    printf("bmp.header_size = %d\n\r", bitmap_data->header->header_size);
    printf("bmp.width = %d\n\r", bitmap_data->header->width);
    printf("bmp.height = %d\n\r", bitmap_data->header->height);
    printf("bmp.planes = %d\n\r", bitmap_data->header->planes);
    printf("bmp.bitdepth = %d\n\r", bitmap_data->header->bitdepth);
    printf("bmp.compression = %d\n\r", bitmap_data->header->compression);
    printf("bmp.image_size = %d\n\r", bitmap_data->header->image_size);
    printf("bmp.number_of_colours = %d\n\r", bitmap_data->header->number_of_colours);
    printf("bmp.number_of_important_colours = %d\n\r", bitmap_data->header->number_of_important_colours);
    bmp_colour_table_printf(bitmap_data->colour_table, bitmap_data->header->number_of_colours);
}

void bmp_colour_printf(bmp_colour colour)
{
    fprintf(stderr, "%02x.%02x.%02x ", colour.red, colour.green, colour.blue);
}

void bmp_colour_table_printf(bmp_colour *colour_table, uint32_t number_of_colours)
{
    uint32_t total_rows = number_of_colours / 16;
    if ((colour_table == NULL) || (number_of_colours == 0))
        return;

    fprintf(stderr, "    ");
    for (uint8_t col_no = 0; col_no < 16; col_no++)
    {
        fprintf(stderr, "   %02x    ", col_no);
    }
    fprintf(stderr, "\r\n");

    for (uint32_t row = 0; row < total_rows; row++)
    {
        fprintf(stderr, "%02x: ", row * 16);
        for (uint8_t col = 0; col < 16; col++)
        {
            bmp_colour_printf(colour_table[row * 16 + col]);
        }
        fprintf(stderr, "\r\n");
    }
}

void bmp_free(bmp *bitmap_data)
{
    free(bitmap_data->header);
    free(bitmap_data->colour_table);
    free(bitmap_data);
}