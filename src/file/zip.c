/*=============================================================================
                                    zip.c
-------------------------------------------------------------------------------
read and write zip files

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://en.wikipedia.org/wiki/ZIP_(file_format)
- https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
- https://docs.fileformat.com/compression/zip/
-----------------------------------------------------------------------------*/
#include <stdlib.h> // malloc, calloc, free
#include <stdio.h>  // debugging, file read/write
#include <stdint.h> // type definitions
#include <string.h> // strcmpn
#include "buffer.h" // buffer
#include "crc.h"    // crc32
#include "endian.h" // _little_endian_to_uint32_t, _little_endian_to_uint16_t
#include "hexdump.h"
#include "zip.h"

// FIXME: all file reads/writes should employ platform independent little-endian byte order

typedef struct zip_file_header_t
{
  uint32_t signature;    // 0x04034b50
  uint16_t version;      // version needed to extract
  uint16_t flags;        // general purpose bit flag
  uint16_t compression;  // compression method
  uint16_t mod_time;     // last mod file time
  uint16_t mod_date;     // last mod file date
  uint32_t crc32;        // crc-32 of uncompressed data
  uint32_t compressed;   // compressed size
  uint32_t uncompressed; // uncompressed size
  uint16_t name_length;  // file name length
  uint16_t extra_length; // extra field length
} zip_file_header;
uint32_t zip_file_signature = 0x04034b50;
size_t zip_file_length = 30;
// Note uin32_t are not byte-aligned to addr%4, so we need to read/write them byte-by-byte
// file_header and cdr_header contents should match

typedef struct zip_data_descriptor_t
{
  uint32_t signature;    // 0x08074b50 (if present)
  uint32_t crc32;        // crc-32 of uncompressed data
  uint32_t compressed;   // compressed size
  uint32_t uncompressed; // uncompressed size
} zip_data_descriptor;

typedef struct zip_central_directory_file_header_t
{
  uint32_t signature;      // 0x02014b50
  uint16_t version;        // version made by
  uint16_t version_needed; // version needed to extract
  uint16_t flags;          // general purpose bit flag
  uint16_t compression;    // compression method
  uint16_t mod_time;       // last mod file time
  uint16_t mod_date;       // last mod file date
  uint32_t crc32;          // crc-32
  uint32_t compressed;     // compressed size
  uint32_t uncompressed;   // uncompressed size
  uint16_t name_length;    // file name length
  uint16_t extra_length;   // extra field length
  uint16_t comment_length; // file comment length
  uint16_t disk_number;    // disk number start
  uint16_t internal;       // internal file attributes
  uint32_t external;       // external file attributes
  uint32_t offset;         // relative offset of local header

} zip_cdr_header;
size_t zip_cdr_length = 46;
uint32_t zip_cdr_signature = 0x02014b50;

typedef struct zip_cdr_t
{
  zip_cdr_header *header;
  char *name;    // file name
  char *extra;   // extra field
  char *comment; // file comment
  zip_file_header *file_header;
  zip_data_descriptor *data_descriptor;
  buffer *compressed_data;
  buffer *uncompressed_data;
} zip_cdr;

typedef struct zip_end_of_central_directory_record_t
{
  uint32_t signature;      // 0x06054b50
  uint16_t disk_number;    // number of this disk
  uint16_t disk_start;     // disk where central directory starts
  uint16_t disk_entries;   // number of central directory records on this disk
  uint16_t total_entries;  // total number of central directory records
  uint32_t size;           // size of central directory (bytes)
  uint32_t offset;         // offset of start of central directory, relative to start of archive
  uint16_t comment_length; // .ZIP file comment length
  char *comment;           // .ZIP file comment
} zip_meta;

uint32_t zip_dir_signature = 0x06054b50;
uint16_t zip_dir_length = 22;
uint16_t zip_dir_comment_length = 65535;

typedef struct zip_t
{
  FILE *file;           // file pointer
  zip_cdr **files;      // central directory of zip archive
  zip_meta *metadata;   // end of central directory record
  const char *filename; // zip archive filename
} zip;

/*----------------------------------------------------------------------------
  open
  open zip archive; if file does not exist, create it
-----------------------------------------------------------------------------*/
zip *zip_open(const char *filename)
{
  uint8_t *file_header_bytes;
  size_t cdr_end;

  zip *archive = (zip *)malloc(sizeof(zip));
  if (archive == NULL)
    return NULL;

  archive->file = fopen(filename, "rb+");
  if (archive->file == NULL)
  {
    // printf("file not found, creating a new one\n");
    archive->file = fopen(filename, "wb+");
    if (archive->file == NULL)
    {
      goto file_error;
    }
    // init all other structs
    // return empty zip object
  }
  archive->metadata = (zip_meta *)malloc(sizeof(zip_meta));

  if (fseek(archive->file, -sizeof(zip_meta), SEEK_END) != 0)
    goto file_error;

  // search for end of central directory record
  uint32_t signature;
  for (int i = 0; i < zip_dir_comment_length; i++)
  {
    fread(&signature, sizeof(uint32_t), 1, archive->file);
    if (signature == zip_dir_signature)
    {
      fseek(archive->file, -sizeof(uint32_t), SEEK_CUR);
      fread(archive->metadata, sizeof(zip_meta), 1, archive->file);
      break;
    }
    else
    {
      fseek(archive->file, -(sizeof(uint8_t) + sizeof(uint32_t)), SEEK_CUR);
    }
  }

  if (signature != zip_dir_signature)
  {
    printf("zip.open(): not a valid zip file\n");
    goto file_error;
  }

  // build array of central directory records
  archive->files = (zip_cdr **)malloc(sizeof(zip_cdr) * archive->metadata->total_entries);
  if (archive->files == NULL)
    goto file_error;

  fseek(archive->file, archive->metadata->offset, SEEK_SET);

  for (uint16_t i = 0; i < archive->metadata->total_entries; i++)
  {
    archive->files[i] = (zip_cdr *)malloc(sizeof(zip_cdr));
    if (archive->files[i] == NULL)
    {
      printf("zip.open(): memory allocation error\n");
      goto file_error;
    }

    // header
    archive->files[i]->header = (zip_cdr_header *)malloc(sizeof(zip_cdr_header));
    fread(archive->files[i]->header, zip_cdr_length, 1, archive->file);
    if (archive->files[i]->header->signature != zip_cdr_signature)
    {
      printf("zip.open(): cdr signature mismatch\n");
      goto file_error;
    }

    // name
    archive->files[i]->name = (char *)calloc(archive->files[i]->header->name_length + 1, sizeof(char));
    if (archive->files[i]->name == NULL)
      goto file_error;
    fread(archive->files[i]->name, sizeof(char), archive->files[i]->header->name_length, archive->file);
    printf("file %d: %s\n", i, archive->files[i]->name);

    // extra
    archive->files[i]->extra = (char *)malloc(sizeof(char) * archive->files[i]->header->extra_length);
    if (archive->files[i]->extra == NULL)
    {
      printf("zip.open(): memory allocation error for extra\n");
      goto file_error;
    }
    fread(archive->files[i]->extra, sizeof(char), archive->files[i]->header->extra_length, archive->file);
    if (archive->files[i]->header->extra_length > 0)
      printf("extra: %s\n", archive->files[i]->extra);

    // comment
    archive->files[i]->comment = (char *)calloc(archive->files[i]->header->comment_length + 1, sizeof(char));
    if (archive->files[i]->comment == NULL)
    {
      printf("zip.open(): memory allocation error for comment\n");
      goto file_error;
    }
    fread(archive->files[i]->comment, sizeof(char), archive->files[i]->header->comment_length, archive->file);
    if (archive->files[i]->header->comment_length > 0)
      printf("comment: %s\n", archive->files[i]->comment);

    cdr_end = ftell(archive->file);

    // file header
    // FIXME: this doesn't work on the second run-through for some reason
    fseek(archive->file, archive->files[i]->header->offset, SEEK_SET);
    archive->files[i]->file_header = (zip_file_header *)malloc(sizeof(zip_file_header));
    if (archive->files[i]->file_header == NULL)
    {
      printf("zip.open(): memory allocation error for file header\n");
      goto file_error;
    }

    // pull header data and sort into file_header
    file_header_bytes = (uint8_t *)malloc(zip_file_length);

    fread(file_header_bytes, zip_file_length, 1, archive->file);
    archive->files[i]->file_header->signature = _little_endian_to_uint32_t(file_header_bytes);
    file_header_bytes += sizeof(uint32_t);
    archive->files[i]->file_header->version = _little_endian_to_uint16_t(file_header_bytes);
    file_header_bytes += sizeof(uint16_t);
    archive->files[i]->file_header->flags = _little_endian_to_uint16_t(file_header_bytes);
    file_header_bytes += sizeof(uint16_t);
    archive->files[i]->file_header->compression = _little_endian_to_uint16_t(file_header_bytes);
    file_header_bytes += sizeof(uint16_t);
    archive->files[i]->file_header->mod_time = _little_endian_to_uint16_t(file_header_bytes);
    file_header_bytes += sizeof(uint16_t);
    archive->files[i]->file_header->mod_date = _little_endian_to_uint16_t(file_header_bytes);
    file_header_bytes += sizeof(uint16_t);
    archive->files[i]->file_header->crc32 = _little_endian_to_uint32_t(file_header_bytes);
    file_header_bytes += sizeof(uint32_t);
    archive->files[i]->file_header->compressed = _little_endian_to_uint32_t(file_header_bytes);
    file_header_bytes += sizeof(uint32_t);
    archive->files[i]->file_header->uncompressed = _little_endian_to_uint32_t(file_header_bytes);
    file_header_bytes += sizeof(uint32_t);
    archive->files[i]->file_header->name_length = _little_endian_to_uint16_t(file_header_bytes);
    file_header_bytes += sizeof(uint16_t);
    archive->files[i]->file_header->extra_length = _little_endian_to_uint16_t(file_header_bytes);

    hexdump(stderr, archive->files[i]->header, sizeof(zip_cdr_header));
    hexdump(stderr, archive->files[i]->file_header, sizeof(zip_file_header));

    if (archive->files[i]->file_header->signature != zip_file_signature)
    {
      printf("zip.open(): file header signature mismatch\n");
      goto file_error;
    }

    // printf("file header signature: %d\n", archive->files[i]->file_header->signature);
    // printf("file header version: %d vs header version: %d\n", archive->files[i]->file_header->version, archive->files[i]->header->version);
    // printf("file header flags: %d vs header flags: %d\n", archive->files[i]->file_header->flags, archive->files[i]->header->flags);
    // printf("file header compression: %d vs header compression: %d\n", archive->files[i]->file_header->compression, archive->files[i]->header->compression);
    // printf("file header mod_time: %d vs header mod_time: %d\n", archive->files[i]->file_header->mod_time, archive->files[i]->header->mod_time);
    // printf("file header mod_date: %d vs header mod_date: %d\n", archive->files[i]->file_header->mod_date, archive->files[i]->header->mod_date);
    // printf("file header crc32: %d vs header crc32: %d\n", archive->files[i]->file_header->crc32, archive->files[i]->header->crc32);
    // printf("file header compressed: %d vs header compressed: %d\n", archive->files[i]->file_header->compressed, archive->files[i]->header->compressed);
    // printf("file header uncompressed: %d vs header uncompressed: %d\n", archive->files[i]->file_header->uncompressed, archive->files[i]->header->uncompressed);
    // printf("file header name_length: %d vs header name_length: %d\n", archive->files[i]->file_header->name_length, archive->files[i]->header->name_length);

    printf("name: \n");
    // name (again)
    if (archive->files[i]->file_header->name_length == archive->files[i]->header->name_length)
    {
      char *name_temp = (char *)calloc(archive->files[i]->file_header->name_length + 1, sizeof(char));
      if (name_temp == NULL)
      {
        printf("zip.open(): memory allocation error\n");
        goto file_error;
      }
      fread(name_temp, sizeof(char), archive->files[i]->file_header->name_length, archive->file);
      if (strcmp(name_temp, archive->files[i]->name) != 0)
      {
        printf("zip.open(): file name mismatch\n");
        goto file_error;
      }
    }
    else
    {
      printf("zip.open(): file name length mismatch: %d (cdr) vs %d (file)\n", archive->files[i]->header->name_length, archive->files[i]->file_header->name_length);
      goto file_error;
    }

    printf("extra: \n");
    // extra (again)
    if (archive->files[i]->file_header->extra_length == archive->files[i]->header->extra_length)
    {
      char *extra_temp = (char *)malloc(sizeof(char) * archive->files[i]->file_header->extra_length);
      if (extra_temp == NULL)
      {
        printf("zip.open(): memory allocation error\n");
        goto file_error;
      }
      fread(extra_temp, sizeof(char), archive->files[i]->file_header->extra_length, archive->file);
      if (strcmp(extra_temp, archive->files[i]->extra) != 0)
      {
        printf("zip.open(): extra field mismatch\n");
        goto file_error;
      }
    }
    else
    {
      printf("zip.open(): extra field length mismatch\n");
      goto file_error;
    }

    printf("finished opening\n");

    fseek(archive->file, cdr_end, SEEK_SET);

    // data descriptor

    // compressed

    // uncompressed
  }
  printf("zip.open(): cleaning up\n");

  free(file_header_bytes);

  printf("zip.open(): success\n");
  return archive;

file_error:
  printf("zip.open(): file error\n");
  fclose(archive->file);
  free(archive);
  return NULL;
}

// FILE* zip_fopen(zip z, const char* path, const char* mode)
// {
//   return NULL;
// }

// void zip_fclose(FILE* file)
// {
//   return;
// }

/*----------------------------------------------------------------------------
  add
  add a new file to a zip archive
-----------------------------------------------------------------------------*/
void zip_add(zip z, const char *path, buffer *data)
{

  return;
}

/*----------------------------------------------------------------------------
  remove
  remove a file from a zip archive
-----------------------------------------------------------------------------*/
void zip_remove(zip z, const char *path)
{

  return;
}

/*----------------------------------------------------------------------------
  close
  close zip archive
-----------------------------------------------------------------------------*/
void zip_close(zip *z)
{
  fclose(z->file);
  free(z);
}