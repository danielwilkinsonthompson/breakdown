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
#include "deflate.h" // compression
#include "crc.h"     // crc
#include "buffer.h"  // compression
#include "hexdump.h" // debug
#include "png.h"     // png
#include "stream.h"  // stream
#include "zlib.h"    // compression
// #include "deflate.h" // compression

/*----------------------------------------------------------------------------
  private macros
-----------------------------------------------------------------------------*/
#define DEBUG

#define PNG_SIGNATURE_LENGTH 8
#define png_invalid(img) ((img == NULL) || (img->header == NULL))

/*----------------------------------------------------------------------------
  typdefs
-----------------------------------------------------------------------------*/
// typedef uint8_t png_subpixel; // subpixel data

// pixel data is one of the following
// - greyscale (1, 2, 4, 8, 16 bits per pixel), with optional trailing alpha for 8 and 16 bits
// - truecolour  (8, 16 bits per pixel), red, green, blue, with optional trailing alpha
// - indexed-colour (1, 2, 4, 8 bits per pixel)
// If no alpha channel nor tRNS chunk is present, all pixels in the image are to be treated as fully opaque.
// The alpha channel is not gamma corrected

// a filter type byte is applied to each scanline; no byte is present for empty scanlines

// With interlace method 0, pixels are stored sequentially from left to right, and scanlines sequentially from top to bottom (no interlacing).
// Interlace method 1, known as Adam7 after its author, Adam M. Costello, consists of seven distinct passes over the image. Each pass transmits a subset of the pixels in the image. The pass in which each pixel is transmitted is defined by replicating the following 8-by-8 pattern over the entire image, starting at the upper left corner:
// 16462646
// 77777777
// 56565656
// 77777777
// 36463646
// 77777777
// 56565656
// 77777777

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
// IHDR chunk must appear first

typedef enum bit_depth_t
{
  one_bit = 1,
  two_bits = 2,
  four_bits = 4,
  eight_bits = 8,
  sixteen_bits = 16
} bit_depth;

// bit_depth = {1, 2, 4, 8, 16}
// colour_type = { 0, 2, 3, 4, 6}
// compression = 0
// filter = 0
// interlacing = {"no interlace", "Adam7 interlace"}

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

typedef struct png_plte_t
{
  png_plte_entry *colour_table; // 3 bytes per entry, r, g, b
} png_plte;
// there must not be more than one PLTE chunk

typedef buffer png_idat;
// multiple IDAT chunks may appear, but they must appear contiguously

typedef struct png_iend_t
{
  // no data
} png_iend;

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
// bool _crc_table_generated = false;
// uint32_t crc_table[256];
// const uint32_t crc_polynomial = 0xEDB88320;
const uint32_t crc_initial = 0xFFFFFFFF;
const uint8_t png_signature[PNG_SIGNATURE_LENGTH] = {137, 80, 78, 71, 13, 10, 26, 10};

/*----------------------------------------------------------------------------
  private funcs
-----------------------------------------------------------------------------*/
// static bool _check_crc(png_chunk *chunk);
// static void _generate_crc_table(void);
// uint32_t update_crc(uint32_t crc, uint8_t *buf, size_t len);
static png_chunk *_get_chunk(png *img);
static error _process_ihdr(png *img, png_chunk *chunk);
static error _process_idat(png *img, png_chunk *chunk);
static error _process_plte(png *img, png_chunk *chunk);
// static error _process_iend(png *img, png_chunk *chunk);
static error _process_trns(png *img, png_chunk *chunk);
static image_pixel _colour_lookup(png *img, uint32_t index);
// static error _process_gama(png *img, png_chunk *chunk);
// static error _process_bkgd(png *img, png_chunk *chunk);
// static uint8_t _reflect_uint8(uint8_t b);
// static uint32_t _reflect_uint32(uint32_t u32);

static png_chunk *_get_chunk(png *img)
{
  png_chunk_header *chunk_header = NULL;
  png_chunk *chunk = NULL;
  // uint8_t crc[4];
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
  // printf("chunk->length: %d\n", chunk->length);
  // printf("chunk->type: %C%C%C%C\n", (chunk->type & 0xFF000000) >> 24, (chunk->type & 0x00FF0000) >> 16, (chunk->type & 0x0000FF00) >> 8, (chunk->type & 0x000000FF) >> 0);

  chunk->data = (uint8_t *)malloc(sizeof(uint8_t) * chunk->length);
  if (chunk->data == NULL)
    goto memory_error;
  count = fread(chunk->data, sizeof(uint8_t), chunk->length, img->file);
  if (count != chunk->length)
    goto io_error;
  // else
  //   printf("chunk->data read %zu bytes\n", count);

  // read crc and cross-check against calculated crc
  count = fread(&crc, sizeof(uint8_t), sizeof(crc), img->file);
  if (count != (sizeof(crc)))
    goto io_error;

  chunk->crc = byte_swap_32(crc);
  // if (_check_crc(chunk) == false)
  //   printf("crc check internal\n");
  // goto io_error;
  // else
  //   printf("crc check passed\n");

  // #if 0
  // chunk->type = byte_swap_32(chunk->type);
  // (uint8_t *)&chunk_type
  buf->data = chunk->data;
  buf->length = chunk->length;
  //+sizeof(chunk->type);
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
  // chunk->type = byte_swap_32(chunk->type);
  // #endif

  // #ifdef DEBUG
  //   else printf("crc check passed\n");
  // #endif

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

  // how big should we make the compressed data stream?
  img->compressed_idat = stream_init(5000000);
  if (img->compressed_idat == NULL)
    goto memory_error;

  img->image = image_init(img->header->height, img->header->width);
  if (img->image == NULL)
    goto memory_error;
  // img->compressed_blocks = (deflate_blocks *)malloc(sizeof(deflate_blocks));
  // if (img->compressed_blocks == NULL)
  //   goto memory_error;

  // also need to allocate img->compressed_blocks->block[] array

  // img->compressed_blocks->block = (deflate_block **)malloc(sizeof(deflate_block *) * 100);
  // if (img->compressed_blocks->block == NULL)
  //   goto memory_error;

  for (uint8_t i = 0; i < 100; i++)
  {
    // img->compressed_blocks->block[i] = (deflate_block *)malloc(sizeof(deflate_block));
    // if (img->compressed_blocks->block[i] == NULL)
    //   goto memory_error;
  }

  // img->compressed_blocks->size = 0;

  img->image = image_init(height, width);

  while ((chunk != NULL) && (chunk->type != IEND))
  {
    chunk = _get_chunk(img);

    switch (chunk->type)
    {
    case PLTE:
      status = _process_plte(img, chunk);
      break;
    case IDAT:
      status = _process_idat(img, chunk);
      // TODO: I believe we should aggregate data into a single IDAT stream and decompress via zlib at the end
      break;
    case IEND:
      break;
    default:
      break;
    }

    if (status != success)
      goto io_error;
  }

  printf("finished reading file\n");
  printf("compressed image data size: %zu\r\n", img->compressed_idat->length / 8);

  stream *decompressed = stream_init((img->header->width * img->header->bit_depth / 8 * 4 + 1) * img->header->height);
  printf("decompressed->capacity = %zu\r\n", decompressed->capacity);

  status = zlib_decompress(img->compressed_idat, decompressed);

  printf("decompressed->length = %zu\r\n", decompressed->length);
  if (status != success)
    goto io_error;

  printf("status == success\r\n");

  uint32_t colour_index = 0;
  uint8_t *buffer_head = decompressed->data;
  for (int32_t row = 0; row < img->header->height; row++)
  {
    buffer_head++; // skip filter byte
    for (int32_t col = 0; col < img->header->width; col++)
    {
      switch (img->header->bit_depth)
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
        colour_index = (buffer_head[0] << 8) | buffer_head[1];
        buffer_head += 2;
        break;
      default:
        printf("png_read: invalid bit depth: %d\r\n", img->header->bit_depth);
        goto io_error;
      }

      // image_pixel pixel = _colour_lookup(img, colour_index);
      // printf("0x%06X ", pixel & 0x00FFFFFF);
      img->image->pixel_data[row * (img->header->width) + col] = _colour_lookup(img, colour_index);
    }
    // printf("\r\n");
  }
  img->image->width = img->header->width;
  img->image->height = img->header->height;

  return img->image;

io_error:
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
      // Unsupported, 1 to 8-bit BMP requires colour table
      pixel = image_argb(0xFF, 0x00, 0x00, 0x00);
      // fprintf(stderr, "Error: %d-bit image with no colour table\r\n", bitmap->header->bitdepth);
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
  return io_error;
}

static error _process_idat(png *img, png_chunk *chunk)
{
  buffer *idat;
  error status;

  // check inputs
  if ((img == NULL) || (chunk == NULL) || (chunk->data == NULL) || (chunk->length == 0))
    goto io_error;

  if (chunk->type != IDAT)
    goto io_error;

  idat = (png_idat *)malloc(sizeof(png_idat));
  if (chunk->type == IDAT) // IDAT
  {
    idat->data = (uint8_t *)chunk->data;
    idat->length = chunk->length;
  }
  else
    goto io_error;

  status = stream_write_buffer(img->compressed_idat, idat, false);
  if (status != success)
    goto io_error;
  free(idat);

  return status;

io_error:
  printf("png.c _process_idat():  io_error\n");
  free(idat);
  return status;
}

static error _process_plte(png *img, png_chunk *chunk)
{
  png_plte *plte;

  if (chunk->type == PLTE) // PLTE
    plte = (png_plte *)chunk->data;
  else
    goto io_error;

  // img->colour_table = (image_pixel *)malloc(sizeof(image_pixel) * chunk->length / sizeof(image_pixel));
  img->colour_table = (png_plte_entry *)chunk->data;
  img->colour_table_size = chunk->length / sizeof(png_plte_entry);
  printf("img->colour_table_size: %zu\n", img->colour_table_size);
  hexdump(stderr, img->colour_table, img->colour_table_size * 3);

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

// static void _generate_crc_table(void)
// {
//   uint32_t crc;

//   for (uint16_t byte_value = 0; byte_value < 256; byte_value++)
//   {
//     crc = (uint32_t)byte_value;
//     for (uint8_t bit_index = 0; bit_index < 8; bit_index++)
//       crc = (crc & 1) ? (crc_polynomial ^ (crc >> 1)) : (crc >> 1);

//     crc_table[byte_value] = crc;
//   }
//   _crc_table_generated = true;
// }

// uint32_t update_crc(uint32_t crc, uint8_t *buf, size_t len)
// {
//   if (!_crc_table_generated)
//     _generate_crc_table();

//   for (uint16_t n = 0; n < len; n++)
//     crc = crc_table[(crc ^ buf[n]) & 0xff] ^ (crc >> 8);

//   return crc;
// }

// static bool _check_crc(png_chunk *chunk)
// {
//   // uint8_t chunk_type[4];
//   uint32_t chunk_type;
//   buffer *buf;

//   // buf = (buffer *)malloc(sizeof(buffer));
//   // chunk_type[0] = (chunk->type >> 24);
//   // chunk_type[1] = (chunk->type >> 16);
//   // chunk_type[2] = (chunk->type >> 8);
//   // chunk_type[3] = (chunk->type);
//   chunk_type = byte_swap_32(chunk->type);

//   // uint32_t crc = update_crc(crc_initial, (uint8_t *)&chunk_type, sizeof(chunk->type));
//   // crc = update_crc(crc, chunk->data, chunk->length);
//   uint32_t crc = update_crc(crc_initial, chunk->data, chunk->length);
//   crc ^= 0xffffffffL;

//   printf("chunk->crc = %08x crc = %08x\n", chunk->crc, crc);

//   return crc == chunk->crc;
// }

void png_write(image *img, const char *filename)
{
}
