/*=============================================================================
                                    gz.c
-------------------------------------------------------------------------------
read and write gzip files

references
- https://tools.ietf.org/html/rfc1952
- https://github.com/billbird/gzstat/blob/master/gzstat.py

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#include <stdint.h>  // uint8_t
#include <stdio.h>   // printf
#include <string.h>  // strnlen
#include "buffer.h"  // buffer
#include "crc.h"     // crc32
#include "deflate.h" // inflate
#include "gz.h"      // gz

typedef struct gz_header_t
{
  // +---+---+---+---+---+---+---+---+---+---+
  // |ID1|ID2|CM |FLG|     MTIME     |XFL|OS | (more-->)
  // +---+---+---+---+---+---+---+---+---+---+
  uint8_t magic[2];           // ID 1, ID 2
  uint8_t compression_method; // CM
  uint8_t flags;              // FLAGS
  uint8_t timestamp[4];       // MTIME
  uint8_t extra_flags;        // XFL
  uint8_t os;                 // OS
} gz_header;

const uint8_t gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

typedef enum gz_compression_method_t
{
  gz_deflate_compression = 8,
} gz_compression_method;

typedef enum gz_flags_t
{
  gz_flag_text = 1,
  gz_flag_hcrc = 2,
  gz_flag_extra = 4,
  gz_flag_name = 8,
  gz_flag_comment = 16,
} gz_flags;

typedef struct gz_header_crc_t
{
  // +---+---+
  // | CRC16 |
  // +---+---+
  uint8_t crc16[2]; // CRC16
} gz_header_crc;

typedef struct gz_extra_subfield_t
{
  // +---+---+---+---+==================================+
  // |SI1|SI2|  LEN  |... LEN bytes of subfield data ...|
  // +---+---+---+---+==================================+
  uint8_t id[2];     // SI1, SI2
  uint8_t length[2]; // SLEN
} gz_extra_subfield;

typedef struct gz_extra_t
{
  // +---+---+=================================+
  // | XLEN  |...XLEN bytes of "extra field"...| (more-->)
  // +---+---+=================================+
  uint8_t length[2]; // XLEN
} gz_extra;

typedef struct gz_footer_t
{
  // +---+---+---+---+---+---+---+---+
  // |     CRC32     |     ISIZE     |
  // +---+---+---+---+---+---+---+---+
  uint32_t crc32; // CRC-32
  uint32_t size;  // ISIZE
} gz_footer;

typedef struct gz_t
{
  FILE *file;
  gz_header header;
  uint16_t crc16;
  gz_footer footer;
  const char *filename;
} gz;

void gz_free(gz *zip);

buffer *gz_read(const char *filename)
{
  gz *zip;
  stream *compressed, *decompressed;
  buffer *out = NULL, *io_buffer;
  size_t bytes_read = 0, data_start, data_end;
  error status;

  // check inputs
  if (filename == NULL)
    return NULL;

  // allocate memory
  zip = (gz *)malloc(sizeof(gz));
  if (zip == NULL)
    return NULL;

  compressed = stream_init(GZ_MAX_BUFFER_SIZE * 1024);
  if (compressed == NULL)
    goto memory_error;

  io_buffer = buffer_init(GZ_MAX_BUFFER_SIZE * 1024);
  if (io_buffer == NULL)
    goto memory_error;

  // read file header
  zip->file = fopen(filename, "rb");
  if (zip->file == NULL)
    goto file_error;

  bytes_read = fread(&zip->header, sizeof(uint8_t), sizeof(gz_header), zip->file);
  if (bytes_read != sizeof(gz_header))
    goto file_error;

  data_start = ftell(zip->file);
  if (data_start == -1)
    goto file_error;

  // check file header
  if ((zip->header.magic[0] != gz_magic[0]) || (zip->header.magic[1] != gz_magic[1]))
    goto file_error;

  if (zip->header.compression_method != gz_deflate_compression)
    goto file_error;

  // extra
  if (zip->header.flags & gz_flag_extra)
  {
    gz_extra extra;
    bytes_read = fread(extra.length, sizeof(uint8_t), sizeof(extra.length), zip->file);
    if (bytes_read != sizeof(extra.length))
      goto file_error;
    data_start += bytes_read;
  }

  // filename
  if (zip->header.flags & gz_flag_name)
  { // no upper bound is set on the length of the filename, so we have to read until we find a null terminator
    bytes_read = fread(io_buffer->data, sizeof(uint8_t), io_buffer->length, zip->file);
    size_t filename_length = strnlen((char *)io_buffer->data, io_buffer->length);
    data_start += filename_length + 1; // include null terminator
    zip->filename = strncpy((char *)malloc(filename_length), (char *)io_buffer->data, filename_length);
    if (fseek(zip->file, data_start, SEEK_SET) != 0)
      goto file_error;
  }

  // comment
  if (zip->header.flags & gz_flag_comment)
  { // no upper bound is set on the comment length, so we have to read until we find the null terminator
    bytes_read = fread(io_buffer->data, sizeof(uint8_t), io_buffer->length, zip->file);
    size_t comment_length = strnlen((char *)io_buffer->data, io_buffer->length);
    data_start += comment_length + 1; // include null terminator
    if (fseek(zip->file, data_start, SEEK_SET) != 0)
      goto file_error;
  }

  // crc16
  if (zip->header.flags & gz_flag_hcrc)
  {
    gz_header_crc header_crc;
    bytes_read = fread(header_crc.crc16, sizeof(uint8_t), sizeof(header_crc.crc16), zip->file);
    if (bytes_read != sizeof(header_crc.crc16))
      goto file_error;

    zip->crc16 = header_crc.crc16[0] | (header_crc.crc16[1] << 8);

    // read the entire header again up to the crc16, but this time into the io_buffer
    rewind(zip->file);
    io_buffer->length = data_start - 1; // -1 to exclude the crc16 first byte
    bytes_read = fread(io_buffer->data, sizeof(uint8_t), io_buffer->length, zip->file);
    if (bytes_read != io_buffer->length)
      goto file_error;

    // note that crc16 in gzip is actually a crc32, but only the first two bytes are used
    uint32_t crc16 = calculate_crc32(io_buffer);
    io_buffer->length = GZ_MAX_BUFFER_SIZE;

    if (crc16 && 0x0000FFFF != zip->crc16)
      goto file_error;

    data_start += 2;
  }

  // read file footer
  if (fseek(zip->file, -sizeof(gz_footer), SEEK_END) == 0)
  {
    data_end = ftell(zip->file);
    bytes_read = fread(&zip->footer, sizeof(uint8_t), sizeof(gz_footer), zip->file);
    if (bytes_read != sizeof(gz_footer))
      goto file_error;
  }
  else
    goto file_error;

  // now that we know the size of the uncompressed data, we can allocate the decompression buffer
  decompressed = stream_init(zip->footer.size + 1);
  if (decompressed == NULL)
    goto memory_error;

  // decompress the file starting at the first byte after the file header
  fseek(zip->file, data_start, SEEK_SET);
  bytes_read = 0;
  uint8_t buffer_counts = 0;
  status = buffer_underflow;

  while ((bytes_read < data_end - data_start) && (status == buffer_underflow))
  {
    io_buffer->length = fread(io_buffer->data, sizeof(uint8_t), GZ_MAX_BUFFER_SIZE * 1024, zip->file);
    bytes_read += io_buffer->length;
    stream_write_buffer(compressed, io_buffer, false);
    status = inflate(compressed, decompressed);
    buffer_counts++;
    // FIXME: multi-buffer processing doesn't work
  }

  // printf("buffer_counts: %d\r\n", buffer_counts);

  if (status != success)
    goto file_error;

  out = stream_read_buffer(decompressed, zip->footer.size, false);
  if (out == NULL)
    goto memory_error;

  uint32_t crc32 = calculate_crc32(out);
  if (crc32 != zip->footer.crc32)
  {
    printf("crc32 mismatch: %08x != %08x\n", crc32, zip->footer.crc32);
    goto file_error;
  }

#ifdef DEBUG
  printf("compressed: %zu uncompressed: %zu ratio: %0.1f%% uncompressed_name: %s\n", data_end + sizeof(gz_footer), out->length, (float)(data_end + sizeof(gz_footer)) / (float)out->length * 100.0, zip->filename);
#endif
  gz_free(zip);
  stream_free(compressed);
  stream_free(decompressed);
  buffer_free(io_buffer);

  return out;

memory_error:
file_error:
  gz_free(zip);
  stream_free(compressed);
  stream_free(decompressed);
  buffer_free(io_buffer);
  buffer_free(out);

  return NULL;
}

void gz_write(const char *filename, buffer *buf)
{
  gz *zip;
  stream *uncompressed, *compressed;
  error status;
  uint32_t timestamp;

  zip = (gz *)malloc(sizeof(gz));
  if (zip == NULL)
    return;

  // write the header to file
  zip->header.magic[0] = gz_magic[0];
  zip->header.magic[1] = gz_magic[1];
  zip->header.compression_method = gz_deflate_compression;
  zip->header.flags = 0;
  zip->header.timestamp[0] = timestamp & 0xFF;
  zip->header.timestamp[1] = (timestamp >> 8) & 0xFF;
  zip->header.timestamp[2] = (timestamp >> 16) & 0xFF;
  zip->header.timestamp[3] = (timestamp >> 24) & 0xFF;
  zip->header.extra_flags = 0;
#if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__linux) || defined(__APPLE__) || defined(__MACH__)
  zip->header.os = 3; // unix
#elif defined(_WIN32) || defined(_WIN64)
  zip->header.os = 0; // fat
#else
  zip->header.os = 255; // unknown
#endif
  zip->filename = filename;
  zip->file = fopen(filename, "wb");
  fwrite(&zip->header, sizeof(gz_header), 1, zip->file);

  // compress the buffer and write to file
  uncompressed = stream_init_from_buffer(buf, false);
  if (uncompressed == NULL)
  {
    printf("error: could not allocate memory for uncompressed stream\n");
    goto memory_error;
  }
  compressed = stream_init(GZ_MAX_BUFFER_SIZE);
  if (compressed == NULL)
  {
    printf("error: could not allocate memory for compressed stream\n");
    goto memory_error;
  }
  printf("got here: %zu\n", uncompressed->length);
  deflate(uncompressed, compressed, 0);
  fwrite(compressed->data, sizeof(uint8_t), compressed->length / 8, zip->file);

  // write the footer to file
  zip->footer.crc32 = calculate_crc32(buf);
  fwrite(&zip->footer.crc32, sizeof(uint32_t), 1, zip->file);
  fwrite(&buf->length, sizeof(uint32_t), 1, zip->file);

  // clean up
memory_error:
  stream_free(uncompressed);
  stream_free(compressed);
  gz_free(zip);
}

void gz_free(gz *zip)
{
  if (zip == NULL)
    return;

  if (zip->file != NULL)
    fclose(zip->file);

  free(zip);
}