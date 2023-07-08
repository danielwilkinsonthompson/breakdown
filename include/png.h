/*=============================================================================
                                    png.h
-------------------------------------------------------------------------------
read and write portable network graphics (png) files

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://en.wikipedia.org/wiki/PNG
- http://www.libpng.org/pub/png/book/chapter08.html
- http://www.schaik.com/pngsuite/
-----------------------------------------------------------------------------*/
#ifndef __png_h
#define __png_h
#include <stdio.h>  // debugging, file read/write
#include <stdint.h> // type definitions
#include "image.h"

/*----------------------------------------------------------------------------
  typdefs
-----------------------------------------------------------------------------*/
typedef uint8_t png_subpixel; // subpixel data

#define PNG_FILE_HEADER_SIGNATURE                           \
  {                                                         \
    0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x0D, 0x0A, 0x1A, 0x0A \
  }

typedef struct png_chunk_t
{
  uint32_t length; // chunk length big-endian
  uint32_t type;   // size of file in bytes
  uint8_t *data;   // array of pixel data
  uint32_t crc;    // offset to start of pixels_data
} png_chunk;

typedef struct png_chunk_header_t
{
  uint8_t length[4]; // chunk length big-endian
  uint8_t type[4];   // size of file in bytes
} png_chunk_header;

typedef struct png_ihdr_t
{
  uint8_t width[4];
  uint8_t height[4];
  uint8_t bit_depth;
  uint8_t colour_type;
  uint8_t compression;
  uint8_t filter;
  uint8_t interlacing[13];
} png_ihdr;

// bit_depth = {1, 2, 4, 8, 16}
// colour_type = { 0, 2, 3, 4, 6}
// compression = 0
// filter = 0
// interlacing = {"no interlace", "Adam7 interlace"}

typedef struct png_plte_t
{
  uint8_t width[4];
  uint8_t height[4];
  uint8_t bit_depth;
  uint8_t colour_type;
  uint8_t compression;
  uint8_t filter;
  uint8_t interlacing[13];
} png_plte;

// typedef enum png_compression_t
// {
//   BI_RGB = 0, // no compression (default)
//   // BI_RLE8,    // 8-bit run length encoding
//   // BI_RLE4,    // 4-bit run length encoding
//   // BI_BITFIELDS,
//   // BI_JPEG,
//   // BI_PNG,
//   // BI_ALPHABITFIELDS,
//   // BI_CMYK = 11,
//   // BI_CMYKRLE8,
//   // BI_CMYKRLE4,
//   BI_COMPRESSION_SUPPORT
// } png_compression;

/*----------------------------------------------------------------------------
  write
-----------------------------------------------------------------------------*/
void png_write(image *img, const char *filename);

/*----------------------------------------------------------------------------
  read
-----------------------------------------------------------------------------*/
image *png_read(const char *filename);

#endif