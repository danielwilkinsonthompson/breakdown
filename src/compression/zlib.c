/*=============================================================================
                                    zlib.c
-------------------------------------------------------------------------------
read and write zlib compressed data streams

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://datatracker.ietf.org/doc/html/rfc1950
- https://datatracker.ietf.org/doc/html/rfc2083
-----------------------------------------------------------------------------*/
#include <stdio.h>  // fprintf, stderr
#include <stdint.h> // uint8_t, uint16_t, uint32_t
#include "deflate.h"
#include "error.h"
#include "endian.h"
#include "hexdump.h"
#include "stream.h"
#include "zlib.h"

typedef struct zlib_comp_t
{
  uint8_t method : 4;
  uint8_t info : 4;
} zlib_comp;

typedef enum zlib_comp_method_t
{
  zlib_comp_method_deflate = 8
} zlib_comp_method;

typedef enum zlib_comp_info_t
{
  zlib_comp_info_deflate_max_window_size = 7
} zlib_comp_info;

typedef struct zlib_flag_t
{
  uint8_t fcheck : 5;
  uint8_t fdict : 1;
  uint8_t flevel : 2;
} zlib_flag;

// fcheck value must be such that CMF and FLG, when viewed as a 16-bit unsigned integer stored in MSB order (CMF*256 + FLG), is a multiple of 31.

typedef enum zlib_comp_level_t
{
  zlib_comp_level_fastest = 0,
  zlib_comp_level_fast = 1,
  zlib_comp_level_default = 2,
  zlib_comp_level_maximum = 3
} zlib_comp_level;

typedef struct zlib_t
{
  zlib_comp compression;
  zlib_flag flag;
  uint8_t *data;
  uint32_t adler32;
} zlib;

error zlib_decompress(stream *compressed, stream *decompressed)
{
  zlib *zstream;
  uint8_t *io_buffer;
  error err;

  zstream = malloc(sizeof(zlib));
  if (zstream == NULL)
    goto decompression_failure;

  io_buffer = stream_read_bytes(compressed, 2, false);
  if (io_buffer == NULL)
    goto decompression_failure;

  zstream->compression.method = io_buffer[0] & 0x0f;
  zstream->compression.info = (io_buffer[0] & 0xf0) >> 4;

  if (zstream->compression.method != zlib_comp_method_deflate)
  {
    printf("zlib compression method not supported\n");
    printf("compression method: %u\n", zstream->compression.method);
    printf("compression info: %u\n", zstream->compression.info);
    goto decompression_failure;
  }

  if (zstream->compression.info > zlib_comp_info_deflate_max_window_size)
  {
    printf("zlib compression window size not supported\n");
    goto decompression_failure;
  }

  zstream->flag.fcheck = io_buffer[1] & 0x1f;
  zstream->flag.fdict = (io_buffer[1] & 0x20) >> 4;
  zstream->flag.flevel = (io_buffer[1] & 0xC0) >> 5;

  /* The FCHECK value must be such that CMF and FLG, when viewed as
  a 16-bit unsigned integer stored in MSB order (CMF*256 + FLG),
  is a multiple of 31. */
  // FIXME: this is not working, check always fails
  // pretty sure this is because adler32 is calculated on flipped bytes
  uint16_t header_check = io_buffer[0] << 8 | io_buffer[1];
  // header_check =
  if (header_check % 31 != 0)
  {
    printf("zlib header check failed\n");
    printf("CMF: %02x\n", io_buffer[0]);
    printf("FLG: %02x\n", io_buffer[1]);
    printf("header check: %u %% 31 = %u\n", header_check, header_check % 31);
    // goto decompression_failure;
  }

  free(io_buffer);
  if (zstream->flag.fdict != 0)
  {
    io_buffer = stream_read_bytes(compressed, 4, true);
    if (io_buffer == NULL)
      goto decompression_failure;
    // for now, we don't support dictionaries

    free(io_buffer);
  }
  // for now, we don't do anything with the compression level

  // strip last 4 bytes from compressed stream and save as adler32
  // checksum of *uncompressed* data
  // io_buffer = compressed->data + compressed->length - 4; // should this be -1?
  io_buffer = compressed->tail.byte - 4;
  compressed->tail.byte -= 4;
  compressed->length -= 4;
  zstream->adler32 = _big_endian_to_uint32_t(io_buffer);
  printf("adler32: %08x\n", zstream->adler32);

  stream_print(compressed);

  err = inflate(compressed, decompressed);
  if (err != success)
    goto decompression_failure;

  // in theory, now we check the adler32 checksum, but we don't do that yet
  // io_buffer = compressed->data;

  printf("decompressed length: %zu\n", decompressed->length);

  free(zstream);
  // free(io_buffer);

  return success;

decompression_failure:
  free(zstream);
  free(io_buffer);
  return unspecified_error;
}
error zlib_compress(stream *uncompressed, stream *compressed)
{
  return success;
}
