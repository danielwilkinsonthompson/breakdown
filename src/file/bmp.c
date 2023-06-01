/*=============================================================================
                                    bmp.c
-------------------------------------------------------------------------------
read and write bitmap files

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

// #define DEBUG_BMP

static uint16_t _bmp_read_uint16(FILE *image_file);
static uint32_t _bmp_read_uint32(FILE *image_file);
static void _bmp_read_infoheader(FILE *image_file, bmp_header *header);
static void _bmp_read_coreheader(FILE *image_file, bmp_header *header);
static bmp_pixel _bmp_colour_lookup(bmp *image, uint32_t index);
static const uint8_t bmp_file_header_signature[2] = BMP_FILE_HEADER_SIGNATURE;

bmp *bmp_read(const char *filename)
{
    FILE *image_file;
    bmp_file_header *image_file_header;
    bmp *image;

    // check inputs
    if (filename == NULL)
        return NULL;

    // allocate memory for image
    image = (bmp *)malloc(sizeof(bmp));
    if (image == NULL)
        return NULL;
    image->header = (bmp_header *)malloc(sizeof(bmp_header));
    if (image->header == NULL)
    {
        bmp_free(image);
        return NULL;
    }

    // extract file header
    image_file_header = (bmp_file_header *)malloc(sizeof(bmp_file_header));
    if (image_file_header == NULL)
    {
        bmp_free(image);
        return NULL;
    }
    image_file = fopen(filename, "rb+");
    if (image_file == NULL)
    {
        bmp_free(image);
        free(image_file_header);
        return NULL;
    }
    fread(image_file_header, sizeof(bmp_file_header), 1, image_file);
    if ((image_file_header->signature[0] != bmp_file_header_signature[0]) | (image_file_header->signature[1] != bmp_file_header_signature[1]))
    {
        bmp_free(image);
        free(image_file_header);
        fclose(image_file);
        printf("Not a bitmap file\r\n");
        return NULL;
    }
    image->header->header_size = _bmp_read_uint32(image_file);
    switch (image->header->header_size)
    {
    case BMP_BITMAPINFOHEADER_SIZE:
        _bmp_read_infoheader(image_file, image->header);
        break;
    case BMP_BITMAPCOREHEADER_SIZE:
        _bmp_read_coreheader(image_file, image->header);
        break;
    default:
        bmp_free(image);
        free(image_file_header);
        fclose(image_file);
        printf("Not a recognised bitmap format\r\n");
        return NULL;
    }

    if (image->header->compression >= BI_COMPRESSION_SUPPORT)
        printf("Unsupported compression type\r\n");

    // extract colour table, if any
    if (image->header->number_of_colours != BMP_ALL_COLOURS)
    {
        image->colour_table = (bmp_colour *)malloc(sizeof(bmp_colour) * image->header->number_of_colours);
        fread(image->colour_table, sizeof(bmp_colour) * image->header->number_of_colours, 1, image_file);
    }
    else
    {
        image->colour_table = NULL;
    }

    // allocate memory for pixel data
    image->pixel_data = (bmp_pixel **)malloc_2d(image->header->height, image->header->width, sizeof(bmp_pixel));
    if (image->pixel_data == NULL)
    {
        bmp_free(image);
        free(image_file_header);
        fclose(image_file);
        return NULL;
    }

    // read pixel data from file
    int32_t padding = (32 - image->header->width * image->header->bitdepth % 32) % 32;
    size_t padded_width = (image->header->width * image->header->bitdepth + padding) / 8;
    uint8_t *buffer = (uint8_t *)malloc(padded_width);
    uint8_t *buffer_head;
    uint32_t colour_index = 0;
    if (buffer == NULL)
    {
        bmp_free(image);
        free(image_file_header);
        fclose(image_file);
        return NULL;
    }
    for (int32_t row = 0; row < abs(image->header->height); row++)
    {
        buffer_head = buffer;
        fread(buffer, sizeof(uint8_t), padded_width, image_file);
        for (int32_t col = 0; col < image->header->width; col++)
        {
            switch (image->header->bitdepth)
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
            image->pixel_data[row][col] = _bmp_colour_lookup(image, colour_index);
        }
    }
    fclose(image_file);
    free(image_file_header);

    return image;
}

static uint16_t _bmp_read_uint16(FILE *image_file)
{
    uint8_t buffer[2];
    fread(buffer, sizeof(uint8_t), sizeof(buffer), image_file);
    return uint8_x2_uint16(buffer);
}

static uint32_t _bmp_read_uint32(FILE *image_file)
{
    uint8_t buffer[4];
    fread(buffer, sizeof(uint8_t), sizeof(buffer), image_file);
    return uint8_x4_uint32(buffer);
}

static void _bmp_read_infoheader(FILE *image_file, bmp_header *header)
{
    header->width = _bmp_read_uint32(image_file);
    header->height = _bmp_read_uint32(image_file);
    header->planes = _bmp_read_uint16(image_file);
    header->bitdepth = _bmp_read_uint16(image_file);
    header->compression = _bmp_read_uint32(image_file);
    header->image_size = _bmp_read_uint32(image_file);
    header->width_pixel_per_metre = _bmp_read_uint32(image_file);
    header->height_pixel_per_metre = _bmp_read_uint32(image_file);
    header->number_of_colours = _bmp_read_uint32(image_file);
    header->number_of_important_colours = _bmp_read_uint32(image_file);
}

static void _bmp_read_coreheader(FILE *image_file, bmp_header *header)
{
    header->width = _bmp_read_uint16(image_file);
    header->height = _bmp_read_uint16(image_file);
    header->planes = _bmp_read_uint16(image_file);
    header->bitdepth = _bmp_read_uint16(image_file);
}

static bmp_pixel _bmp_colour_lookup(bmp *image, uint32_t index)
{
    bmp_pixel colour;

    if bmp_invalid (image)
    {
        colour.red = 0;
        colour.green = 0;
        colour.blue = 0;

        return colour;
    }

    if (image->colour_table == NULL)
    {
        switch (image->header->bitdepth)
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
        colour.red = image->colour_table[index].red;
        colour.green = image->colour_table[index].blue;
        colour.blue = image->colour_table[index].blue;
    }

    return colour;
}

void bmp_write(bmp *image, const char *filename)
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

void bmp_fprintf(FILE *f, const char *format, bmp *image, uint32_t left_start, uint32_t bottom_start, uint32_t width, uint32_t height)
{
    if bmp_invalid (image)
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
            fprintf(f, format, image->pixel_data[bottom_start + row][left_start + col].red);
            fprintf(f, ".");
            fprintf(f, format, image->pixel_data[bottom_start + row][left_start + col].green);
            fprintf(f, ".");
            fprintf(f, format, image->pixel_data[bottom_start + row][left_start + col].blue);
            fprintf(f, " ");
        }
        fprintf(f, "\r\n");
    }
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

void bmp_free(bmp *image)
{
    if (image->pixel_data != NULL)
    {
        for (int32_t row = 0; row < image->header->width; row++)
            free(image->pixel_data[row]);
        free(image->pixel_data);
    }
    free(image->header);
    free(image);
}

#ifdef DEBUG_BMP
int main(int argc, char *argv[])
{
    bmp *test;
    if (argc > 1)
        test = bmp_read(argv[1]);
    else
        test = bmp_read("lena.bmp");

    printf("bmp.header_size = %d\n\r", test->header->header_size);
    printf("bmp.width = %d\n\r", test->header->width);
    printf("bmp.height = %d\n\r", test->header->height);
    printf("bmp.planes = %d\n\r", test->header->planes);
    printf("bmp.bitdepth = %d\n\r", test->header->bitdepth);
    printf("bmp.compression = %d\n\r", test->header->compression);
    printf("bmp.image_size = %d\n\r", test->header->image_size);
    printf("bmp.number_of_colours = %d\n\r", test->header->number_of_colours);
    printf("bmp.number_of_important_colours = %d\n\r", test->header->number_of_important_colours);
    bmp_colour_table_printf(test->colour_table, test->header->number_of_colours);
    bmp_debug_value(test);
}
#endif
