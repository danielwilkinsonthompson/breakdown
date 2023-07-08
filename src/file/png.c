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
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "image.h"
#include "endian.h"
#include "png.h"

/*----------------------------------------------------------------------------
  private macros
-----------------------------------------------------------------------------*/
#ifndef PNG_DEBUG_FORMAT
#define PNG_DEBUG_FORMAT "%02x"
#endif
#define png_debug_value(pix)         \
  fprintf(stderr, #pix " = \n");     \
  png_printf(PNG_DEBUG_FORMAT, pix); \
  fprintf(stderr, "\n\n");
#define png_debug_command(f)        \
  fprintf(stderr, ">> " #f "\n\n"); \
  f;

#define png_invalid(img) ((img == NULL) || (img->header == NULL))

/*----------------------------------------------------------------------------
  private globals
-----------------------------------------------------------------------------*/
bool crc_table_generated = false;
uint32_t crc_table[256];

/*----------------------------------------------------------------------------
  private funcs
-----------------------------------------------------------------------------*/
static bool _check_crc(png_chunk *chunk);
static void _generate_crc_table(void);
static png_chunk *_get_chunk(FILE *fp);

static bool _check_crc(png_chunk *chunk)
{
  return true;
}

static void _generate_crc_table(void)
{
  uint32_t c;
  int n, k;

  for (n = 0; n < 256; n++)
  {
    c = (uint32_t)n;
    for (k = 0; k < 8; k++)
    {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
  crc_table_generated = true;
}

static png_chunk *_get_chunk(FILE *fp)
{
  png_chunk_header *chunk_header = NULL;
  png_chunk *chunk = NULL;
  uint8_t crc[4];

  chunk_header = (png_chunk_header *)malloc(sizeof(png_chunk_header));
  if (chunk_header == NULL)
    goto malloc_error;

  size_t count = fread(chunk_header, sizeof(png_chunk_header), 1, fp);
  if (count != sizeof(png_chunk_header))
    goto malloc_error;

  chunk = (png_chunk *)malloc(sizeof(png_chunk));
  if (chunk == NULL)
    goto malloc_error;

  chunk->length = _big_endian_to_uint32_t(chunk_header->length);
  chunk->type = _big_endian_to_uint32_t(chunk_header->type);
  chunk->data = (uint8_t *)malloc(sizeof(uint8_t) * chunk->length);
  if (chunk->data == NULL)
    goto malloc_error;

  count = fread(chunk->data, sizeof(uint8_t), chunk->length, fp);
  if (count != chunk->length)
    goto malloc_error;

  count = fread(crc, sizeof(uint8_t), sizeof(crc) / sizeof(uint8_t), fp);
  if (count != (sizeof(crc) / sizeof(uint8_t)))
    goto malloc_error;

  chunk->crc = _big_endian_to_uint32_t(crc);
  if (_check_crc(chunk) == false)
    goto malloc_error;

  return chunk;

malloc_error:
  free(chunk_header);
  // free(chunk->data);
  free(chunk);
  return NULL;
}

image *png_read(const char *filename)
{
  image *new_image;

  return new_image;
}

void png_write(image *img, const char *filename)
{
}
