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

typedef enum png_colour_bpp_t
{
  greyscale_bit_depth = 1,
  truecolour_bit_depth = 3,
  indexed_colour_bit_depth = 1,
  greyscale_with_alpha_bit_depth = 2,
  truecolour_with_alpha_bit_depth = 4
} png_colour_bpp;

typedef enum compression_method_t
{
  deflate_compression = 0
} compression_method;

typedef enum filter_method_t
{
  adaptive_filtering = 0
} filter_method;

typedef enum png_filter_t
{
  no_filter = 0,
  sub_filter = 1,
  up_filter = 2,
  average_filter = 3,
  paeth_filter = 4
} png_filter;

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

typedef struct png_pixel_t
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha;
} png_pixel;

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
  png_header *header;
  png_plte_entry *palette;
  size_t palette_size;
  stream *compressed_idat;
  stream *decompressed_idat;
  image *image;
  size_t row;
  size_t col;
} png;

/*----------------------------------------------------------------------------
  private globals
-----------------------------------------------------------------------------*/
const uint32_t crc_initial = 0xFFFFFFFF;
const uint8_t png_signature[PNG_SIGNATURE_LENGTH] = {137, 80, 78, 71, 13, 10, 26, 10};

/*----------------------------------------------------------------------------
  private funcs
-----------------------------------------------------------------------------*/
static png_chunk *read_chunk(png *img);
static error _process_ihdr(png *img, png_chunk *chunk);
static error _process_idat(png *img, png_chunk *chunk);
static error _process_plte(png *img, png_chunk *chunk);
static error _process_trns(png *img, png_chunk *chunk);
// static error read_row(png *img);
static image_pixel read_pixel(png *img);
static uint8_t read_subpixel(png *img);
static int32_t paeth_predictor(uint8_t left, uint8_t above, uint8_t above_left);
static image *unpack_image(png *img, uint32_t height, uint32_t width);
// static error _process_gama(png *img, png_chunk *chunk);
// static error _process_bkgd(png *img, png_chunk *chunk);
// static uint8_t _reflect_uint8(uint8_t b);
// static uint32_t _reflect_uint32(uint32_t u32);

static png_chunk *read_chunk(png *img)
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
  image *out = NULL;
  png_chunk *chunk;
  uint32_t width, height;
  size_t count;
  error status;

  // open file
  img = (png *)malloc(sizeof(png));
  if (img == NULL)
    goto memory_error;
  img->file = fopen(filename, "rb");
  if (img->file == NULL)
    goto io_error;

  // check signature
  uint8_t signature[PNG_SIGNATURE_LENGTH];
  count = fread(signature, sizeof(uint8_t), PNG_SIGNATURE_LENGTH, img->file);
  if (count == 0)
    goto io_error;
  for (int i = 0; i < PNG_SIGNATURE_LENGTH; i++)
    if (signature[i] != png_signature[i])
      goto file_format_error;

  // read header
  img->header = (png_header *)malloc(sizeof(png_header));
  if (img == NULL)
    goto memory_error;
  chunk = read_chunk(img);
  if (chunk == NULL)
    goto io_error;
  status = _process_ihdr(img, chunk);
  if (status != success)
    goto io_error;

  // FIXME: how big should we make the compressed data stream?
  // I don't think this is necessary, can we just make a compressed stream per chunk?
  img->compressed_idat = stream_init(5000000);
  if (img->compressed_idat == NULL)
    goto memory_error;

  img->image = image_init(height, width);
  bool have_iend = false;
  bool have_plte = false;
  bool have_idat = false;

  // process chunks
  while ((chunk != NULL) && (chunk->type != IEND))
  {
    chunk = read_chunk(img);
    switch (chunk->type)
    {
    case IHDR:
      printf("png_read: multiple IHDR chunks not allowed\n");
      goto file_format_error;
    case PLTE:
      if (have_plte == true)
        goto file_format_error;
      if (have_idat == true)
        goto file_format_error;
      status = _process_plte(img, chunk);
      have_plte = true;
      break;
    case IDAT:
      status = _process_idat(img, chunk);
      have_idat = true;
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

  // check that we have all the required chunks
  if ((have_iend == false) || (have_idat == false))
    goto io_error;

  // decompress idat chunks
  img->decompressed_idat = stream_init((img->header->width * 4 + 1) * img->header->bit_depth / 2 * img->header->height);
  status = zlib_decompress(img->compressed_idat, img->decompressed_idat);
  if (status != success)
    goto io_error;

  // FIXME: we need to decode interlaced images sequentially
  // e.g. for (int i = 0; i < 7; i++) { decode_pass(i); }
  // hexdump(stderr, img->decompressed_idat->data, img->decompressed_idat->length / 8);

  // unpack image from decompressed idat chunks
  if (img->header->interlacing == no_interlace)
    img->image = unpack_image(img, img->header->height, img->header->width);
  // else if (img->header->interlacing == adam7_interlace)
  // {
  //   // status = unpack_image(img);
  // }
  // else
  //   goto file_format_error;

  free(img->header);
  free(img->compressed_idat);
  free(img->decompressed_idat);
  out = img->image;
  free(img);

  return out;

// FIXME: we need to free all the things here
io_error:
  printf("png_read: i/o error\n");
  return NULL;

file_format_error:
  printf("png_read: file is incorrectly formatted\n");
  return NULL;

memory_error:
  printf("png_read: memory error\n");
  return NULL;
}

static image *unpack_image(png *img, uint32_t height, uint32_t width)
{
  image *px = image_init(height, width);
  if (px == NULL)
    goto memory_error;

  uint8_t bits_per_pixel = img->header->bit_depth * ((img->header->colour_type & 0x02 ? 3 : 1) + (img->header->colour_type & 0x04 ? 1 : 0));
#ifdef DEBUG
  printf("bits_per_pixel: %d\n", bits_per_pixel);
#endif
  uint16_t average;
  uint16_t left, above, above_left;

  uint8_t bytes_per_row = (bits_per_pixel * width) / 8;
  uint8_t bytes_per_pixel = bits_per_pixel / 8;
  if (bytes_per_pixel == 0)
    bytes_per_pixel = 1;

  buffer *above_row = buffer_init(bytes_per_row);
  if (above_row == NULL)
    goto memory_error;

  for (img->row = 0; img->row < height; img->row++)
  {
    png_filter filter = stream_read_byte(img->decompressed_idat);

    switch (filter)
    {
    case no_filter:
      break;

    case sub_filter:
      for (uint32_t byte_pos = 0; byte_pos < bytes_per_row; byte_pos++)
        if (byte_pos >= bytes_per_pixel)
          *(img->decompressed_idat->head.byte + byte_pos) += *(img->decompressed_idat->head.byte + byte_pos - bytes_per_pixel);
      break;

    case up_filter:
      if (img->row == 0)
        break;
      for (uint32_t byte_pos = 0; byte_pos < bytes_per_row; byte_pos++)
      { // previous row is getting zeroed as we read it from the stream
        *(img->decompressed_idat->head.byte + byte_pos) += *(above_row->data + byte_pos);
      }
      break;

    case average_filter:
      for (uint32_t byte_pos = 0; byte_pos < bytes_per_row; byte_pos++)
      {
        if (byte_pos < bytes_per_pixel)
          left = 0;
        else
          left = *(img->decompressed_idat->head.byte + byte_pos - bytes_per_pixel);

        if (img->row == 0)
          average = left / 2;
        else
          average = (left + *(above_row->data + byte_pos)) / 2;

        *(img->decompressed_idat->head.byte + byte_pos) += average;
      }
      break;

    case paeth_filter:
      for (uint32_t byte_pos = 0; byte_pos < bytes_per_row; byte_pos++)
      {
        if (byte_pos < bytes_per_pixel)
          left = 0;
        else
          left = *(img->decompressed_idat->head.byte + byte_pos - bytes_per_pixel);
        if (img->row == 0)
        {
          above = 0;
          above_left = 0;
        }
        else
        {
          above = *(above_row->data + byte_pos);
          if (byte_pos < bytes_per_pixel)
            above_left = 0;
          else
            above_left = *(above_row->data + byte_pos - bytes_per_pixel);
        }
        *(img->decompressed_idat->head.byte + byte_pos) += paeth_predictor(left, above, above_left);
      }
      break;

    default:
      printf("unpack_image: invalid filter type\n");
      goto io_error;
    }
    above_row = buffer_write(above_row, img->decompressed_idat->head.byte, bytes_per_row);

    // img->image->pixel_data[img->row * img->header->width + img->col] =
    for (img->col = 0; img->col < width; img->col++)
    {
      px->pixel_data[img->row * width + img->col] = read_pixel(img);
    }
  }
  buffer_free(above_row);

  return px;

io_error:
  image_free(px);
  printf("png_read: i/o error\n");
  return NULL;
memory_error:
  image_free(px);
  printf("png_read: memory error\n");
  return NULL;
}

static int32_t paeth_predictor(uint8_t left, uint8_t above, uint8_t above_left)
{
  int32_t p, pa, pb, pc;

  p = left + above - above_left;
  pa = abs(p - left);
  pb = abs(p - above);
  pc = abs(p - above_left);

  if (pa <= pb && pa <= pc)
    return left;
  else if (pb <= pc)
    return above;
  else
    return above_left;
}

static uint8_t read_subpixel(png *img)
{
  // stream *byte_stream = stream_init(1);
  uint8_t output;
  uint8_t *index;
  uint32_t value = 0;

  if (png_invalid(img))
    goto io_error;

  if (img->header->bit_depth < 8)
  {
    value = img->decompressed_idat->head.byte[0];
    img->decompressed_idat->head.byte[0] = img->decompressed_idat->head.byte[0] << img->header->bit_depth;
    stream_read_bits(img->decompressed_idat, img->header->bit_depth, false);
    value = value >> (8 - img->header->bit_depth);
  }
  else
  {
    index = stream_read_bits(img->decompressed_idat, img->header->bit_depth, false);
    if (index == NULL)
      goto io_error;

    if (img->header->bit_depth == 16)
      value = (index[1] << 8) | index[0];
    else
      value = index[0];
    free(index);
  }

  if (img->header->colour_type == indexed_colour)
    return value;

  switch (img->header->bit_depth)
  {
  case 1:
    output = (0x01 & value) * 0xFF;
    break;
  case 2:
    output = (0x03 & value) * 0x55;
    break;
  case 4:
    output = (0x0F & value) * 0x11;
    break;
  case 8:
    output = (0xFF & value);
    break;
  case 16:
    output = (0xFFFF & value) >> 8;
    break;
  default:
    printf("read_subpixel: invalid bit depth: %d\n", img->header->bit_depth);
    return 0;
    break;
  }

  return output;
io_error:
  printf("read_subpixel: could not read pixel data from stream\n");
  return 0;
}

static image_pixel read_pixel(png *img)
{
  error status;
  png_pixel pixel = {.red = 0x00, .green = 0x00, .blue = 0x00, .alpha = 0xFF};
  uint8_t index;
  uint8_t *stream_bits;

  if png_invalid (img)
    return invalid_object;

  switch (img->header->colour_type)
  {
  case greyscale:
    index = read_subpixel(img);
    pixel.red = index;
    pixel.green = index;
    pixel.blue = index;
    break;

  case truecolour:
    pixel.red = read_subpixel(img);
    pixel.green = read_subpixel(img);
    pixel.blue = read_subpixel(img);
    break;

  case indexed_colour:
    index = read_subpixel(img);
    pixel.red = img->palette[index].red;
    pixel.blue = img->palette[index].blue;
    pixel.green = img->palette[index].green;
    break;

  case greyscale_with_alpha:
    index = read_subpixel(img);
    pixel.red = index;
    pixel.green = index;
    pixel.blue = index;
    pixel.alpha = read_subpixel(img);
    break;

  case truecolour_with_alpha:
    pixel.red = read_subpixel(img);
    pixel.green = read_subpixel(img);
    pixel.blue = read_subpixel(img);
    pixel.alpha = read_subpixel(img);
    break;

  default:
    printf("png_read: unknown colour type: %d\n", img->header->colour_type);
    goto io_error;
    break;
  }

  return image_argb(pixel.alpha, pixel.red, pixel.green, pixel.blue);

io_error:
  printf("colour_lookup: i/o error\n");
  return image_argb(0, 0, 0, 0);
}

static error _process_ihdr(png *img, png_chunk *chunk)
{
  png_ihdr *ihdr;

  if (chunk->type == IHDR)
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
  img->row = 0;
  img->col = 0;
  if (img->header->filter != adaptive_filtering)
  {
    printf("png.c _process_ihdr():  filter not supported\n");
    return io_error;
  }
  if (img->header->compression != deflate_compression)
  {
    printf("png.c _process_ihdr():  compression not supported\n");
    return io_error;
  }

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

  // can we just do decompression here? I think we need the whole set of IDAT chunks first

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

  img->palette = (png_plte_entry *)chunk->data;
  img->palette_size = chunk->length / sizeof(png_plte_entry);
#ifdef DEBUG
  printf("img->palette_size: %zu\n", img->palette_size);
  hexdump(stderr, img->palette, img->palette_size * 3);
#endif
  return success;

io_error:
  return io_error;
}

static error _process_trns(png *img, png_chunk *chunk)
{
  png_trns *trns;

  if (chunk->type == tRNS)
  {
    trns = (png_trns *)chunk->data;
    printf("trns block\n");
  }
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
