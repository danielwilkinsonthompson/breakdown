/*=============================================================================
                                    bmp.h
-------------------------------------------------------------------------------
read and write bitmap files

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://github.com/marc-q/libbmp/blob/master/libbmp.h
- https://en.wikipedia.org/wiki/BMP_file_format
-----------------------------------------------------------------------------*/
#ifndef __bmp_h
#define __bmp_h
#include <stdio.h>  // debugging, file read/write
#include <stdint.h> // type definitions

/*----------------------------------------------------------------------------
  typdefs
-----------------------------------------------------------------------------*/
typedef uint8_t bmp_subpixel; // subpixel data
typedef struct bmp_pixel_t
{
  bmp_subpixel blue; // reverse rgb order by convention
  bmp_subpixel green;
  bmp_subpixel red;
} bmp_pixel;

typedef struct bmp_colour_t
{
  bmp_subpixel blue;
  bmp_subpixel green;
  bmp_subpixel red;
  bmp_subpixel alpha; // pad to 32-bit by convention
} bmp_colour;

#define BMP_FILE_HEADER_SIGNATURE \
  {                               \
    'B', 'M'                      \
  }
typedef struct bmp_file_header_t
{
  uint8_t signature[2];   // valid signature = {'B', 'M'}
  uint8_t file_size[4];   // size of file in bytes
  uint8_t reserved[4];    // unused
  uint8_t data_offset[4]; // offset to start of pixels_data
} bmp_file_header;

typedef enum bmp_compression_t
{
  BI_RGB = 0, // no compression (default)
  BI_RLE8,    // 8-bit run length encoding
  BI_RLE4,    // 4-bit run length encoding
  // BI_BITFIELDS,
  // BI_JPEG,
  // BI_PNG,
  // BI_ALPHABITFIELDS,
  // BI_CMYK = 11,
  // BI_CMYKRLE8,
  // BI_CMYKRLE4,
  BI_COMPRESSION_SUPPORT
} bmp_compression_format;

#define BMP_BITMAPCOREHEADER_SIZE 12
#define BMP_BITMAPINFOHEADER_SIZE 40
#define BMP_ALL_COLOURS 0
typedef struct bmp_header_t
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
} bmp_header;

typedef struct bmp_t
{
  bmp_header *header;
  bmp_colour *colour_table;
  bmp_pixel **pixel_data;
} bmp;

/*----------------------------------------------------------------------------
  function macros
-----------------------------------------------------------------------------*/
// debugging
#ifndef BMP_DEBUG_FORMAT
#define BMP_DEBUG_FORMAT "%02x"
#endif
#define bmp_debug_value(pix)         \
  fprintf(stderr, #pix " = \n");     \
  bmp_printf(BMP_DEBUG_FORMAT, pix); \
  fprintf(stderr, "\n\n");
#define bmp_debug_command(f)        \
  fprintf(stderr, ">> " #f "\n\n"); \
  f;

// validation
#define bmp_invalid(img) ((img == NULL) || (img->header == NULL) || (img->pixel_data == NULL))

// bmp files are little-endian by convention
#define uint8_x4_uint32(u8) ((u8[3] << 24) | (u8[2] << 16) | (u8[1] << 8) | (u8[0]))
#define uint8_x3_uint24(u8) ((u8[2] << 16) | (u8[1] << 8) | (u8[0]))
#define uint8_x2_uint16(u8) ((u8[1] << 8) | (u8[0]))

/*----------------------------------------------------------------------------
  prototypes
-----------------------------------------------------------------------------*/
// standard
// bmp* bmp_init(bmp_dim width, bmp_dim height);
void bmp_write(bmp *image, const char *filename);
bmp *bmp_read(const char *filename);
void bmp_printf(const char *format, bmp *image);
void bmp_fprintf(FILE *f, const char *format, bmp *image, uint32_t left_start, uint32_t bottom_start, uint32_t width, uint32_t height);
void bmp_colour_printf(bmp_colour colour);
void bmp_colour_table_printf(bmp_colour *colour_table, uint32_t number_of_colours);
void bmp_free(bmp *image);

#endif