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

typedef struct png_plte_t
{
  uint8_t *colour_table; // 3 bytes per entry, r, g, b
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
  image_pixel *colour_table;
  size_t colour_table_size;
  // deflate_blocks *compressed_blocks;
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

  // for (uint8_t i = 0; i < img->compressed_blocks->size; i++)
  // {
  //   printf("img->compressed_blocks->block[%d]->length: %zu\n", i, img->compressed_blocks->block[i]->length);
  //   free(img->compressed_blocks->block[i]);
  // }
  // if ((img->compressed_blocks->block != NULL) && (img->compressed_blocks->size == 1))
  // {
  // }
  // free(img->compressed_blocks->block);

  return img->image;

io_error:
memory_error:
  return NULL;
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
  stream *decompressed;
  stream *compressed;
  error status;

  idat = (png_idat *)malloc(sizeof(png_idat));
  if (chunk->type == IDAT) // IDAT
  {
    idat->data = (uint8_t *)chunk->data;
    idat->length = chunk->length;
  }
  else
    goto io_error;

  // decompress idat chunk
  /* Deflate-compressed datastreams within PNG are stored in the "zlib"
   format, which has the structure:

      Compression method/flags code: 1 byte
      Additional flags/check bits:   1 byte
      Compressed data blocks:        n bytes
      Check value:                   4 bytes

   Further details on this format are given in the zlib specification
   [RFC1950](https://datatracker.ietf.org/doc/html/rfc1950)
   */

  compressed = stream_init_from_buffer(idat, false);
  decompressed = stream_init(32768);
  status = zlib_decompress(compressed, decompressed);

  // from RFC2083
  // "Note that with filter method 0, the only one currently defined, this implies prepending a filter type byte to each scanline"
  // The zlib compression method used in the "deflate" compression method is described in [RFC-1950].

  // note that a single filter-type byte is prepended to each scanline
  // this seems like it is 0x78, which is the zlib compression method
  printf("idat->length: %zu\n", idat->length);
  // add idat block to blocks, decompress all at the end
  // printf("img->compressed_blocks->size: %zu\n", img->compressed_blocks->size);
  // printf("img->compressed_blocks->block[img->compressed_blocks->size]->length: %zu\n", img->compressed_blocks->block[img->compressed_blocks->size]->length);
  // img->compressed_blocks->block[img->compressed_blocks->size]->data = chunk->data;
  // img->compressed_blocks->block[img->compressed_blocks->size]->length = chunk->length;
  // img->compressed_blocks->size++;

  hexdump(stderr, idat->data, idat->length);
  hexdump(stderr, decompressed->data, decompressed->length);

  free(idat);

  return success;

io_error:
  printf("png.c _process_idat():  io_error\n");
  free(idat);
  return io_error;
}

static error _process_plte(png *img, png_chunk *chunk)
{
  png_plte *plte;

  if (chunk->type == PLTE) // PLTE
    plte = (png_plte *)chunk->data;
  else
    goto io_error;

  // img->colour_table = (image_pixel *)malloc(sizeof(image_pixel) * chunk->length / sizeof(image_pixel));
  img->colour_table = (image_pixel *)chunk->data;
  img->colour_table_size = chunk->length / sizeof(image_pixel);
  printf("img->colour_table_size: %zu\n", img->colour_table_size);

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
