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
#include <stdlib.h> // malloc, free
#include <stdio.h>  // debugging, file read/write
#include <stdint.h> // type definitions
#include "hexdump.h"
#include "zip.h"

#define ZIP_MAX_EOCDR_COMMENT_LENGTH 65535

typedef struct zip_local_file_header_t
{
  uint32_t signature;    // 0x04034b50
  uint16_t version;      // version needed to extract
  uint16_t flags;        // general purpose bit flag
  uint16_t compression;  // compression method
  uint16_t mod_time;     // last mod file time
  uint16_t mod_date;     // last mod file date
  uint32_t crc32;        // crc-32
  uint32_t compressed;   // compressed size
  uint32_t uncompressed; // uncompressed size
  uint16_t name_length;  // file name length
  uint16_t extra_length; // extra field length
  char *name;            // file name
  char *extra;           // extra field
} zip_local_file_header;
uint32_t zip_local_file_signature = 0x04034b50;

typedef struct zip_data_descriptor_t
{
  uint32_t crc32;        // crc-32
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

} zip_central_directory_file_header;
// FIXME: sizeof() is rounding this to the nearest multiple of 4
// consider reading as a byte array and transferring byte-wise
// variable length fields are stored after the fixed length fields
// we should only read out fixed length fields
// name needs to malloced to name_length + 1
//

typedef struct zip_cdr_t
{
  zip_central_directory_file_header *header;
  char *name;    // file name
  char *extra;   // extra field
  char *comment; // file comment
} zip_cdr;

size_t zip_cdr_length = 46;
uint32_t zip_cdr_signature = 0x02014b50;

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
} zip_end_of_central_directory_record;

uint32_t zip_eocdr_signature = 0x06054b50;

typedef struct zip_t
{
  FILE *file;                                 // file pointer
  uint32_t size;                              // size of file
  zip_end_of_central_directory_record *eocdr; // end of central directory record
  zip_cdr **cdr;                              // central directory records
} zip;

/*----------------------------------------------------------------------------
  open
  open zip archive; if file does not exist, create it
-----------------------------------------------------------------------------*/
zip *zip_open(const char *path)
{
  // char buffer[256];
  // zip_central_directory_file_header list_of_files[10];
  zip *z = (zip *)malloc(sizeof(zip));
  if (z == NULL)
    return NULL;

  z->file = fopen(path, "rb+");
  if (z->file == NULL)
  {
    // file not found, create a new one
    printf("file not found, creating a new one\n");
    z->file = fopen(path, "wb+");
    if (z->file == NULL)
    {
      goto file_error;
    }
  }
  z->eocdr = (zip_end_of_central_directory_record *)malloc(sizeof(zip_end_of_central_directory_record));

  // by definition, the end of central directory record is located at the end of the file
  if (fseek(z->file, -sizeof(zip_end_of_central_directory_record), SEEK_END) == 0)
  {
    // read signature at this location
    uint32_t signature;
    for (int i = 0; i < ZIP_MAX_EOCDR_COMMENT_LENGTH; i++)
    {
      fread(&signature, sizeof(uint32_t), 1, z->file);
      if (signature == zip_eocdr_signature)
      {
        fseek(z->file, -sizeof(uint32_t), SEEK_CUR);
        fread(z->eocdr, sizeof(zip_end_of_central_directory_record), 1, z->file);
        // printf("found eocdr at %ld\n", ftell(z->file));
        break;
      }
      else
      {
        // printf("seeking back %d bytes\n", sizeof(uint32_t));
        fseek(z->file, -(sizeof(uint8_t) + sizeof(uint32_t)), SEEK_CUR);
      }
    }

    printf("comment length: %d\n", z->eocdr->comment_length);

    // printf("start of central directory: %ld\n", z->eocdr->offset);

    printf("number of files: %d\n", z->eocdr->total_entries);

    // Need to build array of central directory records
    z->cdr = (zip_cdr **)malloc(sizeof(zip_cdr) * z->eocdr->total_entries);
    if (z->cdr == NULL)
      goto file_error;

    // printf("reading from file\n");
    fseek(z->file, z->eocdr->offset, SEEK_SET);

    for (uint16_t i = 0; i < z->eocdr->total_entries; i++)
    {
      // read fixed length header first
      z->cdr[i] = (zip_cdr *)malloc(sizeof(zip_cdr));
      if (z->cdr[i] == NULL)
        goto file_error;

      z->cdr[i]->header = (zip_central_directory_file_header *)malloc(sizeof(zip_central_directory_file_header));
      // can't reliably read 46 bytes without doing it element-wise
      fread(z->cdr[i]->header, zip_cdr_length, 1, z->file);
      if (z->cdr[i]->header->signature != zip_cdr_signature)
        goto file_error;

      // hexdump(stderr, z->cdr[0]->header, sizeof(zip_central_directory_file_header));

      // name
      z->cdr[i]->name = (char *)malloc(sizeof(char) * z->cdr[i]->header->name_length + 1);
      if (z->cdr[i]->name == NULL)
        goto file_error;
      fread(z->cdr[i]->name, sizeof(char), z->cdr[i]->header->name_length, z->file);
      printf("file %d: %s\n", i, z->cdr[i]->name);

      // extra
      z->cdr[i]->extra = (char *)malloc(sizeof(char) * z->cdr[i]->header->extra_length);
      if (z->cdr[i]->extra == NULL)
        goto file_error;
      fread(z->cdr[i]->extra, sizeof(char), z->cdr[i]->header->extra_length, z->file);

      // comment
      z->cdr[i]->comment = (char *)malloc(sizeof(char) * z->cdr[i]->header->comment_length);
      if (z->cdr[i]->comment == NULL)
        goto file_error;
      fread(z->cdr[i]->comment, sizeof(char), z->cdr[i]->header->comment_length, z->file);
    }
    // loop until we find the signature
    // data_end = ftell(zip->file);
    // bytes_read = fread(&zip->footer, sizeof(uint8_t), sizeof(gz_footer), zip->file);
    // if (bytes_read != sizeof(gz_footer))
    //   goto file_error;
  }
  else
    goto file_error;

  return z;

file_error:
  printf("zip.open(): file error\n");
  fclose(z->file);
  free(z);
  return NULL;
}