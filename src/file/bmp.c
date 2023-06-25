/*=============================================================================
                                    bmp.c
-------------------------------------------------------------------------------
read and write bitmap images

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://github.com/marc-q/libbmp/blob/master/libbmp.h
- https://en.wikipedia.org/wiki/BMP_file_format
- http://entropymine.com/jason/bmpsuite/bmpsuite/html/bmpsuite.html

TODO:
- Implement BI_RLE8 and BI_RLE4 compression
- Support for V5 bitmap headers: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header

-----------------------------------------------------------------------------*/
#include <stdio.h>   // file read/write
#include <stdint.h>  // types
#include <stdbool.h> // boolean
#include <stdlib.h>  // memory allocation
#include "image.h"   // base image code
#include "bmp.h"

/*----------------------------------------------------------------------------
  private macros & defines
-----------------------------------------------------------------------------*/
typedef enum _compression_t
{
    BI_RGB = 0, // no compression (default)
    // BI_RLE8,           // unsupported
    // BI_RLE4,           // unsupported
    // BI_BITFIELDS,      // unsupported
    // BI_JPEG,           // unsupported
    // BI_PNG,            // unsupported
    // BI_ALPHABITFIELDS, // unsupported
    // BI_CMYK = 11,      // unsupported
    // BI_CMYKRLE8,       // unsupported
    // BI_CMYKRLE4,       // unsupported
    BI_COMPRESSION_SUPPORT
} _compression;

typedef enum _header_format_t
{
    _BITMAPCOREHEADER = 12,
    _BITMAPINFOHEADER = 40
} _header_format;
#define _BMP_ALL_COLOURS 0
#define _BMP_SIGNATURE_LENGTH 14
#define MAX_COLOUR_TABLE_SIZE 65536

// validation
#define _bmp_invalid(bitmap) ((bitmap == NULL) || (bitmap->file == NULL) || (bitmap->header == NULL))

// bmp file multi-byte reads (bmp is little-endian)
#define _bytes_to_uint32(u8) ((u8[3] << 24) | (u8[2] << 16) | (u8[1] << 8) | (u8[0]))
#define _bytes_to_uint24(u8) ((u8[2] << 16) | (u8[1] << 8) | (u8[0]))
#define _bytes_to_uint16(u8) ((u8[1] << 8) | (u8[0]))

/*----------------------------------------------------------------------------
  private typdefs
-----------------------------------------------------------------------------*/
typedef struct _bmp_header_t
{
    uint32_t header_size;                 // header size defines version
    int32_t width;                        //
    int32_t height;                       // negative height indicates origin in top-left, positive indicates bottom left
    uint16_t planes;                      // always 1
    uint16_t bitdepth;                    // {1, 4, 8, 16, 24, 32}, default 24
    uint32_t compression;                 // see compression_format
    uint32_t image_size;                  // compressed bitmap in bytes (set to 0 if compression = BI_RGB)
    uint32_t width_pixel_per_metre;       // dpi equivalent
    uint32_t height_pixel_per_metre;      // dpi equivalent
    uint32_t number_of_colours;           // length of valid colours in bmp_colour_table
    uint32_t number_of_important_colours; // rarely used, 0 = all colours
} _bmp_header;

typedef struct _bmp_t
{
    FILE *file;
    uint8_t signature[_BMP_SIGNATURE_LENGTH]; // signature[2], file_size[4], reserved[4], data_offset[4]
    _bmp_header *header;
    image_pixel *colour_table;
} _bmp;

typedef struct _image_info_t
{
    uint32_t number_of_colours;
    bool uses_alpha_channel;
    bool greyscale;
    bool colour_fills;
    float mean_run_length;
    image_pixel *colour_table;
    uint32_t max_run_length;
} _image_info;

/*----------------------------------------------------------------------------
    private functions
-----------------------------------------------------------------------------*/
static _bmp *
_bmp_init();
static uint16_t _bmp_read_uint16(_bmp *bitmap);
static uint32_t _bmp_read_uint32(_bmp *bitmap);
static size_t _bmp_write_uint16(_bmp *bitmap, uint16_t data);
static size_t _bmp_write_uint32(_bmp *bitmap, uint32_t data);
static void _bmp_read_infoheader(_bmp *bitmap);
static size_t _bmp_write_infoheader(_bmp *bitmap);
static void _bmp_read_coreheader(_bmp *bitmap);
static size_t _bmp_write_coreheader(_bmp *bitmap);
static image_pixel _bmp_colour_lookup(_bmp *bitmap, uint32_t index);
static void _bmp_colour_printf(image_pixel colour);
static void _bmp_printf(_bmp *bitmap);
static void _bmp_fprintf(FILE *f, _bmp *bitmap);
static void _bmp_colour_table_printf(_bmp *bitmap);
static void _bmp_free(_bmp *bitmap);
static const uint8_t bitmap_signature[2] = {'B', 'M'};
static void _analyse_image(image *img, _image_info *info);

static _bmp *_bmp_init()
{
    _bmp *bitmap;

    bitmap = (_bmp *)malloc(sizeof(_bmp));
    if (bitmap == NULL)
        return NULL;

    bitmap->header = (_bmp_header *)malloc(sizeof(_bmp_header));
    if (bitmap->header == NULL)
    {
        free(bitmap);
        return NULL;
    }

    // no colour table by default
    bitmap->colour_table = NULL;

    return bitmap;
}

image *bmp_read(const char *filename)
{
    _bmp *bitmap = NULL;
    image *img = NULL;

    // set up bitmap
    bitmap = _bmp_init();
    if ((bitmap == NULL) || (filename == NULL))
        goto cleanup;

    // open bitmap file
    bitmap->file = fopen(filename, "rb+");
    if (bitmap->file == NULL)
        goto cleanup;

    // check bitmap signature
    fread(bitmap->signature, sizeof(uint8_t), sizeof(bitmap->signature), bitmap->file);
    if ((bitmap->signature[0] != bitmap_signature[0]) || (bitmap->signature[1] != bitmap_signature[1]))
        goto cleanup;

    // read bitmap header
    bitmap->header->header_size = _bmp_read_uint32(bitmap);
    switch (bitmap->header->header_size)
    {
    case _BITMAPINFOHEADER:
        _bmp_read_infoheader(bitmap);
        break;
    case _BITMAPCOREHEADER:
        _bmp_read_coreheader(bitmap);
        break;
    default:
        goto cleanup;
    }
    if (bitmap->header->compression >= BI_COMPRESSION_SUPPORT)
    {
        fprintf(stderr, "Unsupported compression type\r\n");
        goto cleanup;
    }

    // extract colour table, if any
    if (bitmap->header->number_of_colours != _BMP_ALL_COLOURS)
    {
        bitmap->colour_table = (image_pixel *)malloc(sizeof(image_pixel) * bitmap->header->number_of_colours);
        fread(bitmap->colour_table, sizeof(image_pixel) * bitmap->header->number_of_colours, 1, bitmap->file);
    }
    else
        bitmap->colour_table = NULL;

    // read pixel data from file
    img = image_init(abs(bitmap->header->height), bitmap->header->width);
    if (img == NULL)
        goto cleanup;
    int32_t padding = (32 - bitmap->header->width * bitmap->header->bitdepth % 32) % 32;
    size_t padded_width = (bitmap->header->width * bitmap->header->bitdepth + padding) / 8;
    uint8_t *buffer = (uint8_t *)malloc(padded_width);
    uint8_t *buffer_head;
    uint32_t colour_index = 0;
    if (buffer == NULL)
        goto cleanup;

    for (int32_t row = 0; row < abs(bitmap->header->height); row++)
    {
        buffer_head = buffer;
        fread(buffer, sizeof(uint8_t), padded_width, bitmap->file);
        for (int32_t col = 0; col < bitmap->header->width; col++)
        {
            switch (bitmap->header->bitdepth)
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
                colour_index = _bytes_to_uint16(buffer_head);
                buffer_head += 2;
                break;
            case 24:
                colour_index = _bytes_to_uint24(buffer_head);
                buffer_head += 3;
                break;
            case 32:
                colour_index = _bytes_to_uint32(buffer_head);
                buffer_head += 4;
                break;
            default:
                colour_index = 0;
                break;
            }
            if (bitmap->header->height > 0)
                img->pixel_data[(img->height - 1 - row) * img->width + col] = _bmp_colour_lookup(bitmap, colour_index);
            else
                img->pixel_data[(row)*img->width + col] = _bmp_colour_lookup(bitmap, colour_index);
        }
    }
    free(buffer);
    return img;

cleanup:
    _bmp_free(bitmap);
    image_free(img);
    return NULL;
}

static uint16_t _bmp_read_uint16(_bmp *bitmap)
{
    uint8_t buffer[2];
    fread(buffer, sizeof(uint8_t), sizeof(buffer), bitmap->file);

    return _bytes_to_uint16(buffer);
}

static uint32_t _bmp_read_uint32(_bmp *bitmap)
{
    uint8_t buffer[4];
    if _bmp_invalid (bitmap)
        return 0;

    fread(buffer, sizeof(uint8_t), sizeof(buffer), bitmap->file);

    return _bytes_to_uint32(buffer);
}

static size_t _bmp_write_uint16(_bmp *bitmap, uint16_t data)
{
    uint8_t buffer[2];
    size_t bytes_written;

    if _bmp_invalid (bitmap)
        return 0;

    buffer[0] = (data & 0x00FF);
    buffer[1] = (data & 0xFF00) >> 8;
    bytes_written = fwrite(buffer, sizeof(uint8_t), sizeof(buffer), bitmap->file);

    return bytes_written;
}

static size_t _bmp_write_uint32(_bmp *bitmap, uint32_t data)
{
    uint8_t buffer[4];
    size_t bytes_written;

    if _bmp_invalid (bitmap)
        return 0;

    buffer[0] = (data & 0x000000FF);
    buffer[1] = (data & 0x0000FF00) >> 8;
    buffer[2] = (data & 0x00FF0000) >> 16;
    buffer[3] = (data & 0xFF000000) >> 24;
    bytes_written = fwrite(buffer, sizeof(uint8_t), sizeof(buffer), bitmap->file);

    return bytes_written;
}

static void _bmp_read_infoheader(_bmp *bitmap)
{
    bitmap->header->width = _bmp_read_uint32(bitmap);
    bitmap->header->height = _bmp_read_uint32(bitmap);
    bitmap->header->planes = _bmp_read_uint16(bitmap);
    bitmap->header->bitdepth = _bmp_read_uint16(bitmap);
    bitmap->header->compression = _bmp_read_uint32(bitmap);
    bitmap->header->image_size = _bmp_read_uint32(bitmap);
    bitmap->header->width_pixel_per_metre = _bmp_read_uint32(bitmap);
    bitmap->header->height_pixel_per_metre = _bmp_read_uint32(bitmap);
    bitmap->header->number_of_colours = _bmp_read_uint32(bitmap);
    bitmap->header->number_of_important_colours = _bmp_read_uint32(bitmap);
}

static size_t _bmp_write_infoheader(_bmp *bitmap)
{
    size_t bytes_written = 0;

    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->header_size);
    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->width);
    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->height);
    bytes_written += _bmp_write_uint16(bitmap, bitmap->header->planes);
    bytes_written += _bmp_write_uint16(bitmap, bitmap->header->bitdepth);
    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->compression);
    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->image_size);
    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->width_pixel_per_metre);
    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->height_pixel_per_metre);
    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->number_of_colours);
    bytes_written += _bmp_write_uint32(bitmap, bitmap->header->number_of_important_colours);

    return bytes_written;
}

static void _bmp_read_coreheader(_bmp *bitmap)
{
    bitmap->header->width = _bmp_read_uint16(bitmap);
    bitmap->header->height = _bmp_read_uint16(bitmap);
    bitmap->header->planes = _bmp_read_uint16(bitmap);
    bitmap->header->bitdepth = _bmp_read_uint16(bitmap);
}

static size_t _bmp_write_coreheader(_bmp *bitmap)
{
    size_t bytes_written = 0;

    bytes_written += _bmp_write_uint16(bitmap, bitmap->header->width);
    bytes_written += _bmp_write_uint16(bitmap, bitmap->header->height);
    bytes_written += _bmp_write_uint16(bitmap, bitmap->header->planes);
    bytes_written += _bmp_write_uint16(bitmap, bitmap->header->bitdepth);

    return bytes_written;
}

static image_pixel _bmp_colour_lookup(_bmp *bitmap, uint32_t index)
{
    image_pixel pixel;

    if _bmp_invalid (bitmap)
        return image_argb(0xFF, 0x00, 0x00, 0x00);

    if (bitmap->colour_table == NULL)
    {
        switch (bitmap->header->bitdepth)
        {
        case 1:
        case 4:
        case 8:
            // Unsupported, 1 to 8-bit BMP requires colour table
            pixel = image_argb(0xFF, 0x00, 0x00, 0x00);
            fprintf(stderr, "Error: %d-bit image with no colour table\r\n", bitmap->header->bitdepth);
        case 16:
            // Should r and b be swapped? Should they actually be 5-bits wide?
            pixel = image_argb(0xFF, ((0x0F00 & index) >> 8), ((0x00F0 & index) >> 4), (0x000F & index));
        case 24:
        case 32:
            pixel = image_argb(0xFF, ((0xFF0000 & index) >> 16), ((0x00FF00 & index) >> 8), (0x0000FF & index));
        default:
            break;
        }
    }
    else
    {
        pixel = 0xFF000000 | bitmap->colour_table[index];
    }
    return pixel;
}

void bmp_write(image *img, const char *filename)
{
    _bmp *bitmap;
    uint32_t file_size;
    size_t bytes_written = 0;

    bitmap = _bmp_init();
    if (bitmap == NULL)
        return;

    bitmap->file = fopen(filename, "wb+");
    if (bitmap->file == NULL)
        return;

    bitmap->header->header_size = _BITMAPINFOHEADER;
    bitmap->header->width = img->width;
    bitmap->header->height = -img->height;
    bitmap->header->planes = 1;
    bitmap->header->bitdepth = 32;
    bitmap->header->compression = BI_RGB;
    bitmap->header->image_size = img->width * img->height * bitmap->header->bitdepth / 8;
    bitmap->header->number_of_colours = 0;
    bitmap->header->number_of_important_colours = 0;
    bitmap->colour_table = NULL;

    file_size = _BMP_SIGNATURE_LENGTH + bitmap->header->header_size + bitmap->header->image_size + bitmap->header->number_of_colours * sizeof(image_pixel);
    bytes_written += fwrite(bitmap_signature, sizeof(char), sizeof(bitmap_signature), bitmap->file);
    bytes_written += _bmp_write_uint32(bitmap, file_size);
    bytes_written += _bmp_write_uint32(bitmap, 0);
    bytes_written += _bmp_write_uint32(bitmap, (uint32_t)(_BMP_SIGNATURE_LENGTH + 40 + bitmap->header->number_of_colours * sizeof(image_pixel)));
    if (bytes_written != _BMP_SIGNATURE_LENGTH)
        goto cleanup;

    bytes_written += _bmp_write_infoheader(bitmap);
    if (bytes_written != _BMP_SIGNATURE_LENGTH + bitmap->header->header_size)
        goto cleanup;

    if ((bitmap->header->number_of_colours != _BMP_ALL_COLOURS) && (bitmap->colour_table != NULL))
    {
        bytes_written += fwrite(bitmap->colour_table, sizeof(image_pixel) * bitmap->header->number_of_colours, 1, bitmap->file);
        if (bytes_written != (_BMP_SIGNATURE_LENGTH + sizeof(bitmap->header) + bitmap->header->number_of_colours * sizeof(image_pixel)))
            goto cleanup;
    }

    bytes_written += fwrite(img->pixel_data, sizeof(image_pixel), (size_t)bitmap->header->image_size, bitmap->file);
    if (bytes_written != file_size)
        goto cleanup;

    _image_info info;
    _analyse_image(img, &info);

    free(info.colour_table);
    fclose(bitmap->file);
    return;
cleanup:
    fprintf(stderr, "Could not write bitmap to file\r\n");
    fclose(bitmap->file);
}

static void _bmp_printf(_bmp *bitmap)
{
    _bmp_fprintf(stderr, bitmap);
}

static void _bmp_fprintf(FILE *f, _bmp *bitmap)
{
    if (bitmap == NULL)
    {
        fprintf(f, "bmp = NULL\r\n");
        return;
    }
    if (bitmap->header == NULL)
    {
        fprintf(f, "bmp->header = NULL\r\n");
        return;
    }

    fprintf(f, "bmp.header_size = %d\n\r", bitmap->header->header_size);
    fprintf(f, "bmp.width = %d\n\r", bitmap->header->width);
    fprintf(f, "bmp.height = %d\n\r", bitmap->header->height);
    fprintf(f, "bmp.planes = %d\n\r", bitmap->header->planes);
    fprintf(f, "bmp.bitdepth = %d\n\r", bitmap->header->bitdepth);
    fprintf(f, "bmp.compression = %d\n\r", bitmap->header->compression);
    fprintf(f, "bmp.image_size = %d\n\r", bitmap->header->image_size);
    fprintf(f, "bmp.number_of_colours = %d\n\r", bitmap->header->number_of_colours);
    fprintf(f, "bmp.number_of_important_colours = %d\n\r", bitmap->header->number_of_important_colours);
    _bmp_colour_table_printf(bitmap);
}

static void _bmp_colour_printf(image_pixel colour)
{
    fprintf(stderr, "%02x.%02x.%02x ", image_r(colour), image_g(colour), image_b(colour));
}

static void _bmp_colour_table_printf(_bmp *bitmap)
{
    uint32_t total_rows = bitmap->header->number_of_colours / 16;
    if ((bitmap->colour_table == NULL) || (bitmap->header->number_of_colours == 0))
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
            _bmp_colour_printf(bitmap->colour_table[row * 16 + col]);
        }
        fprintf(stderr, "\r\n");
    }
}

static void _bmp_free(_bmp *bitmap)
{
    fclose(bitmap->file);
    free(bitmap->header);
    free(bitmap->colour_table);
    free(bitmap);
}

// static image_pixel *_construct_colour_table(image *img)
// {
//     image_pixel *colour_table;
//     uint8_t number_of_colours = 0;
//     bool known_colour = false;
// }

static void _analyse_image(image *img, _image_info *info)
{
    // let's check the most appropriate bitdepth, encoding, number of colours
    image_pixel colour;
    image_pixel previous_colour;
    uint32_t run_length = 0;
    uint32_t run_counter = 0;
    image_pixel *colour_table;

    bool known_colour = false;

    info->greyscale = true;
    info->number_of_colours = 0;
    info->max_run_length = 0;
    info->mean_run_length = 0;
    info->uses_alpha_channel = false;
    info->colour_table = (image_pixel *)calloc(MAX_COLOUR_TABLE_SIZE, sizeof(image_pixel));

    if (info->colour_table == NULL)
    {
        fprintf(stderr, "could not allocate colour table\r\n");
        return;
    }

    for (uint32_t row = 0; row < img->height; row++)
    {
        for (uint32_t col = 0; col < img->width; col++)
        {
            known_colour = false;
            colour = img->pixel_data[row * img->width + col];
            if (info->number_of_colours < MAX_COLOUR_TABLE_SIZE)
            {
                for (uint32_t c = 0; c < info->number_of_colours; c++)
                {
                    if (info->number_of_colours < MAX_COLOUR_TABLE_SIZE)
                    {
                        if (colour == info->colour_table[c])
                        {
                            known_colour = true;
                            break;
                        }
                    }
                }

                // if this is the first encounter of this colour, write it to the colour table
                if (known_colour == false)
                {
                    info->number_of_colours++;
                    if (info->number_of_colours < (MAX_COLOUR_TABLE_SIZE))
                        info->colour_table[info->number_of_colours] = colour;
                }
            }

            if ((image_r(colour) != image_g(colour)) || (image_r(colour) != image_b(colour)))
                info->greyscale = false;
            if ((image_a(colour) != 255) && (image_a(colour) != 0))
            {
                fprintf(stderr, "alpha = %d", image_a(colour));
                info->uses_alpha_channel = true;
            }

            // if we're on a run of the same colour, increment run length
            if (colour == previous_colour)
            {
                run_length++;
                if (run_length > info->max_run_length)
                    info->max_run_length = run_length;
            }
            else
            {
                if (run_length > 0)
                {
                    info->mean_run_length = (info->mean_run_length * run_counter + run_length) / (run_counter + 1);
                    run_counter++;
                }
                run_length = 0;
            }
            previous_colour = colour;
        }
    }
    if (info->number_of_colours >= MAX_COLOUR_TABLE_SIZE)
        info->number_of_colours = 0;

    fprintf(stderr, "info->greyscale = %s\r\n", info->greyscale ? "true" : "false");
    fprintf(stderr, "info->number_of_colours = %d\r\n", info->number_of_colours);
    fprintf(stderr, "info->max_run_length = %d\r\n", info->max_run_length);
    fprintf(stderr, "info->mean_run_length = %f\r\n", info->mean_run_length);
    fprintf(stderr, "info->uses_alpha_channel = %s\r\n", info->uses_alpha_channel ? "true" : "false");
}