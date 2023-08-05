/*=============================================================================
                                    png.c
-------------------------------------------------------------------------------
read and write portable network graphics (png) files

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://datatracker.ietf.org/doc/html/rfc2083
- https://en.wikipedia.org/wiki/PNG
- http://www.libpng.org/pub/png/book/chapter08.html
- http://www.schaik.com/pngsuite/
- https://www.w3.org/TR/PNG-CRCAppendix.html
- https://libspng.org
- http://www.libpng.org/pub/png/spec/1.1/png-1.1.pdf
- http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
- [1] http://www.libpng.org/pub/png/spec/1.2/png-1.2.pdf
- [2] http://pnrsolution.org/Datacenter/Vol4/Issue1/58.pdf

-----------------------------------------------------------------------------*/
#include <stdio.h>   // fprintf, stderr
#include <stdint.h>  // uint8_t, uint16_t, uint32_t
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc, free
#include "image.h"   // image
#include "endian.h"  // endian
#include "error.h"   // error
#include "crc.h"     // crc
#include "buffer.h"  // compression
#include "hexdump.h" // debug
#include "png.h"     // png
#include "stream.h"  // stream
#include "zlib.h"    // compression

/*----------------------------------------------------------------------------
  private macros
-----------------------------------------------------------------------------*/
#define DEBUG

#define PNG_SIGNATURE_LENGTH 8
#define png_invalid(img) ((img == NULL) || (img->header == NULL))

/*----------------------------------------------------------------------------
  typdefs
-----------------------------------------------------------------------------*/
typedef enum chunk_types_t
{
  IHDR = 0x49484452, // must be implemented
  PLTE = 0x504C5445, // must be implemented
  IDAT = 0x49444154, // must be implemented
  IEND = 0x49454E44, // must be implemented
  cHRM = 0x6348524D,
  gAMA = 0x67414D41,
  iCCP = 0x69434350,
  sBIT = 0x73424954,
  sRGB = 0x73524742,
  bKGD = 0x624B4744,
  hIST = 0x68495354,
  tRNS = 0x74524E53,
  pHYs = 0x70485973,
  sPLT = 0x73504C54,
  tIME = 0x74494D45,
  iTXt = 0x69545874,
  tEXt = 0x74455874,
  zTXt = 0x7A545874
} chunk_types;
// 4-byte chunk type code
// first letter is capital if it is critical
// second letter is capital if it is public
// third letter is always a capital
// fourth letter is capital if it is safe to copy (does not depend on other chunks)

typedef struct png_chunk_t
{
  uint32_t length; // chunk length, must not exceed 2**31-1 bytes
  uint32_t type;   // chunk type
  uint8_t *data;   // bytes of chunk data
  uint32_t crc;    // crc of chunk data including type and data fields, but not length field
} png_chunk;

typedef struct png_chunk_header_t
{
  uint8_t length[4]; // chunk length big-endian
  uint8_t type[4];   // chunk type
} png_chunk_header;

typedef struct png_ihdr_t
{
  uint8_t width[4];
  uint8_t height[4];
  uint8_t bit_depth;
  uint8_t colour_type;
  uint8_t compression;
  uint8_t filter;
  uint8_t interlacing;
} png_ihdr;
// bit_depth = {1, 2, 4, 8, 16}
// colour_type = { greyscale, truecolour, indexed_colour, greyscale_with_alpha, truecolour_with_alpha}
// compression = deflate_compression
// filter = adaptive_filtering
// interlacing = {no_interlace, adam7_interlace}

typedef enum png_colour_type_t
{
  greyscale = 0,
  truecolour = 2,
  indexed_colour = 3,
  greyscale_with_alpha = 4,
  truecolour_with_alpha = 6
} png_colour_type;

typedef enum compression_method_t
{
  deflate_compression = 0
} compression_method;

typedef enum filter_method_t
{
  adaptive_filtering = 0
} filter_method;

typedef enum png_scanline_filter_method_t
{
  no_filter = 0,
  sub_filter = 1,
  up_filter = 2,
  average_filter = 3,
  paeth_filter = 4
} png_scanline_filter_method;

typedef struct png_deflate_t
{
  uint8_t cmf;   // compression method and flags
  uint8_t flg;   // flags
  uint8_t *data; // compressed data
} png_deflate;

typedef enum interlace_method_t
{
  no_interlace = 0,
  adam7_interlace = 1
} interlace_method;

typedef struct png_header_t
{
  uint32_t width; // (2**31 - 1) maximum
  uint32_t height;
  uint8_t bit_depth;
  uint8_t colour_type;
  uint8_t compression;
  uint8_t filter;
  uint8_t interlacing;
} png_header;

typedef struct png_plte_entry_t
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} png_plte_entry;

typedef struct png_trns_t
{
  uint8_t *data; // 1 byte per entry
} png_trns;
// in the common case where only index 0 need be made transparent, only a single byte is necessary
// if colour type is greyscale, then two bytes are necessary to specify greyscale values with alpha=0, which is stored in the first byte
// if colour type is truecolour, then six bytes are necessary to specify RGB values with alpha=0, which is stored in the first byte
// tRNS is prohibited for colour_type 4 and 6
// tRNS must precede the first IDAT chunk, and must follow the PLTE chunk if present

typedef struct png_gama_t
{
  uint32_t gamma; // 4-byte unsigned integer
} png_gama;
// specifies gamma value of the image encoding, in units of 1/100000
// gamma is not applied to alpha channel
// if gAMA is not present, a gamma of 1.0 is assumed
// gAMA chunk must precede the first IDAT chunk and PLTE chunk if present

typedef struct png_bkgd_t
{
  uint8_t *data; // 1 byte per entry
} png_bkgd;
// specifies default background colour to display image against
// if present, must precede the first IDAT chunk

typedef struct png_t
{
  FILE *file;
  uint8_t signature[PNG_SIGNATURE_LENGTH]; // signature[2], file_size[4], reserved[4], data_offset[4]
  png_header *header;
  png_plte_entry *colour_table;
  size_t colour_table_size;
  stream *compressed_idat;
  image *image;
} png;

/*----------------------------------------------------------------------------
  private globals
-----------------------------------------------------------------------------*/
const uint32_t crc_initial = 0xFFFFFFFF;
const uint8_t png_signature[PNG_SIGNATURE_LENGTH] = {137, 80, 78, 71, 13, 10, 26, 10};

/*----------------------------------------------------------------------------
  private funcs
-----------------------------------------------------------------------------*/
static png_chunk *_get_chunk(png *img);
static error _process_ihdr(png *img, png_chunk *chunk);
static error _process_idat(png *img, png_chunk *chunk);
static error _process_plte(png *img, png_chunk *chunk);
static error _process_trns(png *img, png_chunk *chunk);
static image_pixel _colour_lookup(png *img, uint32_t index);
// static uint32_t _next_subpixel(png *img, uint8_t *buffer_head, uint32_t col);

// static error _process_gama(png *img, png_chunk *chunk);
// static error _process_bkgd(png *img, png_chunk *chunk);
// static uint8_t _reflect_uint8(uint8_t b);
// static uint32_t _reflect_uint32(uint32_t u32);

static png_chunk *_get_chunk(png *img)
{
  png_chunk_header *chunk_header = NULL;
  png_chunk *chunk = NULL;
  buffer *buf = (buffer *)malloc(sizeof(buffer));
  uint32_t crc;

  // check whether image is valid
  if (png_invalid(img) || img->file == NULL)
    goto png_invalid_error;

  // read chunk header
  chunk_header = (png_chunk_header *)malloc(sizeof(png_chunk_header));
  if (chunk_header == NULL)
    goto memory_error;
  size_t count = fread(chunk_header, sizeof(png_chunk_header), 1, img->file);
  if (count != 1)
    goto io_error;

  // read chunk data
  chunk = (png_chunk *)malloc(sizeof(png_chunk));
  if (chunk == NULL)
    goto memory_error;
  chunk->length = _big_endian_to_uint32_t(chunk_header->length);
  chunk->type = _big_endian_to_uint32_t(chunk_header->type);
  chunk->data = (uint8_t *)malloc(sizeof(uint8_t) * chunk->length);
  if (chunk->data == NULL)
    goto memory_error;
  count = fread(chunk->data, sizeof(uint8_t), chunk->length, img->file);
  if (count != chunk->length)
    goto io_error;

  // read crc and cross-check against calculated crc
  count = fread(&crc, sizeof(uint8_t), sizeof(crc), img->file);
  if (count != (sizeof(crc)))
    goto io_error;
  chunk->crc = byte_swap_32(crc);
  buf->data = chunk->data;
  buf->length = chunk->length;
  buffer *type_header = buffer_init(4);
  type_header->data[0] = chunk_header->type[0];
  type_header->data[1] = chunk_header->type[1];
  type_header->data[2] = chunk_header->type[2];
  type_header->data[3] = chunk_header->type[3];
  crc = crc32_update(crc_initial, type_header);
  crc = crc32_update(crc, buf);
  crc = crc32_finalize(crc);
  if (chunk->crc != crc)
    goto io_error;

  return chunk;

  // handle errors
io_error:
memory_error:
  free(chunk_header);
  free(chunk->data);
  free(chunk);
png_invalid_error:
  return NULL;
}

image *png_read(const char *filename)
{
  png *img = NULL;
  png_chunk *chunk;
  uint32_t width, height;
  size_t count;
  error status;

  img = (png *)malloc(sizeof(png));
  if (img == NULL)
    goto memory_error;

  img->header = (png_header *)malloc(sizeof(png_header));
  if (img == NULL)
    goto memory_error;

  img->file = fopen(filename, "rb");
  if (img->file == NULL)
    goto io_error;

  count = fread(img->signature, sizeof(uint8_t), PNG_SIGNATURE_LENGTH, img->file);
  if (count == 0)
    goto io_error;
  for (int i = 0; i < PNG_SIGNATURE_LENGTH; i++)
    if (img->signature[i] != png_signature[i])
      goto io_error;

  chunk = _get_chunk(img);
  if (chunk == NULL)
    goto io_error;

  status = _process_ihdr(img, chunk);
  if (status != success)
    goto io_error;

  // FIXME: how big should we make the compressed data stream?
  img->compressed_idat = stream_init(5000000);
  if (img->compressed_idat == NULL)
    goto memory_error;

  img->image = image_init(img->header->height, img->header->width);
  if (img->image == NULL)
    goto memory_error;

  img->image = image_init(height, width);
  bool have_iend = false;
  bool have_plte = false;
  bool have_idat = false;

  while ((chunk != NULL) && (chunk->type != IEND))
  {
    chunk = _get_chunk(img);

    switch (chunk->type)
    {
    case PLTE:
      if (have_plte == true)
      {
        printf("Cannot have multiple PLTE chunks\n");
        goto io_error;
      }

      if (have_idat == true)
      {
        printf("PLTE chunk must come before IDAT chunk\n");
        goto io_error;
      }
      status = _process_plte(img, chunk);
      have_plte = true;
      break;
    case IDAT:
      status = _process_idat(img, chunk);
      break;
    case IEND:
      have_iend = true;
      break;
    default:
      break;
    }

    if (status != success)
      goto io_error;
  }

  if (have_iend == false)
  {
    printf("IEND chunk not found\n");
    goto io_error;
  }

  stream *decompressed = stream_init((img->header->width * 4 + 1) * img->header->height);
  status = zlib_decompress(img->compressed_idat, decompressed);
  if (status != success)
    goto io_error;

  // hexdump(stderr, decompressed->data, decompressed->length / 8);

  // Decode pixel value from binary stream based on bit_depth and colour_type
  uint32_t colour_index = 0;
  image_pixel pixel_value;
  uint8_t *buffer_head = decompressed->data;
  png_scanline_filter_method filter = 0;
  uint8_t intensity;
  uint32_t alpha_value = 0;
  for (int32_t row = 0; row < img->header->height; row++)
  {
    filter = *buffer_head;
    buffer_head++;
    switch (filter)
    {
    case no_filter:
      break;
    case sub_filter:
      break;
    case up_filter:
      break;
    case average_filter:
      break;
    case paeth_filter:
      break;
    default:
      printf("png_read: unknown filter type: %d\n", filter);
      goto io_error;
      break;
    }

    for (int32_t col = 0; col < img->header->width; col++)
    {

      // if (img->header->colour_type == indexed_colour)
      // {
      //   colour_index = buffer_head[0];
      //   buffer_head += 1;
      // }
      // else
      // {

      switch (img->header->bit_depth)
      {
      case 1:
        colour_index = 0x01 & (buffer_head[0] >> (col % 8));
        buffer_head += (col % 8 + 1) / 8;
        break;
      case 2:
        colour_index = 0x03 & (buffer_head[0] >> ((col % 4) * 2));
        buffer_head += (col % 4 + 1) / 4;
        printf("%x", colour_index);
        break;
      case 4:
        colour_index = 0x0F & (buffer_head[0] >> ((col % 2) * 4));
        buffer_head += col % 2;
        printf("%x", colour_index);
        break;
      case 8:
        colour_index = buffer_head[0];
        buffer_head += 1;
        break;
      case 16:
        colour_index = (buffer_head[0] << 8) | buffer_head[1];
        buffer_head += 2;
        break;
      default:
        printf("invalid bit depth: %d\r\n", img->header->bit_depth);
        goto io_error;
      }
      // }

      switch (img->header->colour_type)
      {
      case greyscale:
        intensity = colour_index * 0xFF / ((1 << (img->header->bit_depth)) - 1);
        pixel_value = image_argb(0xFF, intensity, intensity, intensity);
        break;

      case truecolour:
        // truecolour
        break;

      case indexed_colour:
        pixel_value = _colour_lookup(img, colour_index);
        break;

      case greyscale_with_alpha:
        intensity = colour_index * 0xFF / ((1 << (img->header->bit_depth)) - 1);

        if (img->header->bit_depth == 16)
          alpha_value = (buffer_head[0] << 8) | (buffer_head[1]);
        else
          alpha_value = buffer_head[0];
        pixel_value = image_argb(alpha_value, intensity, intensity, intensity);
        break;

      case truecolour_with_alpha:
        break;
      }

      img->image->pixel_data[row * (img->header->width) + col] = pixel_value;
    }
  }
  img->image->width = img->header->width;
  img->image->height = img->header->height;

  return img->image;

io_error:
  printf("png_read: io_error\n");
  return NULL;

memory_error:
  return NULL;
}

static image_pixel _colour_lookup(png *img, uint32_t index)
{
  image_pixel pixel;

  if png_invalid (img)
    return image_argb(0xFF, 0x00, 0x00, 0x00);

  if (img->colour_table == NULL)
  {
    switch (img->header->bit_depth)
    {
    case 1:
    case 2:
    case 4:
    case 8:
      pixel = image_argb(0xFF, 0x00, 0x00, 0x00);
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
    pixel = image_argb(0xFF, img->colour_table[index].red, img->colour_table[index].green, img->colour_table[index].blue);
    // printf("%02x %02x %02x", img->colour_table[index].red, img->colour_table[index].green, img->colour_table[index].blue);
    // printf("%02x", index);
  }
  return pixel;
}

static error _process_ihdr(png *img, png_chunk *chunk)
{
  png_ihdr *ihdr;

  if (chunk->type == IHDR) // IHDR
    ihdr = (png_ihdr *)chunk->data;
  else
    goto io_error;

  img->header->width = _big_endian_to_uint32_t(ihdr->width);
  img->header->height = _big_endian_to_uint32_t(ihdr->height);
  img->header->bit_depth = ihdr->bit_depth;
  img->header->colour_type = ihdr->colour_type;
  img->header->compression = ihdr->compression;
  img->header->filter = ihdr->filter;
  img->header->interlacing = ihdr->interlacing;

#ifdef DEBUG
  printf("width: %d\n", img->header->width);
  printf("height: %d\n", img->header->height);
  printf("bit_depth: %d\n", img->header->bit_depth);
  printf("colour_type: %d\n", img->header->colour_type);
  printf("compression: %d\n", img->header->compression);
  printf("filter: %d\n", img->header->filter);
  printf("interlacing: %d\n", img->header->interlacing);
#endif

  return success;

io_error:
  printf("png.c _process_ihdr():  io_error\n");
  return io_error;
}

static error _process_idat(png *img, png_chunk *chunk)
{
  if (chunk->type != IDAT)
    goto io_error;

  buffer *idat = (buffer *)malloc(sizeof(buffer));
  if (idat == NULL)
    goto memory_error;
  idat->data = (uint8_t *)chunk->data;
  idat->length = chunk->length;

  error status = stream_write_buffer(img->compressed_idat, idat, false);
  if (status != success)
    goto io_error;
  free(idat);

  return status;

memory_error:
io_error:
  printf("png.c _process_idat():  io_error\n");
  free(idat);
  return status;
}

static error _process_plte(png *img, png_chunk *chunk)
{
  if (chunk->type != PLTE)
    goto io_error;

  img->colour_table = (png_plte_entry *)chunk->data;
  img->colour_table_size = chunk->length / sizeof(png_plte_entry);
#ifdef DEBUG
  printf("img->colour_table_size: %zu\n", img->colour_table_size);
  hexdump(stderr, img->colour_table, img->colour_table_size * 3);
#endif
  return success;

io_error:
  return io_error;
}

static error _process_trns(png *img, png_chunk *chunk)
{
  png_trns *trns;

  if (chunk->type == tRNS)
    trns = (png_trns *)chunk->data;
  else
    goto io_error;

  // in the common case where only index 0 need be made transparent, only a single byte is necessary
  // if colour type is greyscale, then two bytes are necessary to specify greyscale values with alpha=0, which is stored in the first byte
  // if colour type is truecolour, then six bytes are necessary to specify RGB values with alpha=0, which is stored in the first byte
  // tRNS is prohibited for colour_type 4 and 6
  // tRNS must precede the first IDAT chunk, and must follow the PLTE chunk if present

  if (img->header->colour_type == 3)
  {
    // TODO:
  }
  else if (img->header->colour_type == 0)
  {
    // TODO:
  }
  else if (img->header->colour_type == 2)
  {
    // TODO:
  }
  else
  {
    fprintf(stderr, "colour type is not 0, 2 or 3\n");
    goto io_error;
  }

io_error:
  return io_error;
}

void png_write(image *img, const char *filename)
{
}
