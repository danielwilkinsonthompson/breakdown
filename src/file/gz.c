
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "buffer.h"
#include "crc.h"
#include "deflate.h"
#include "gz.h"

typedef struct gz_header_t
{
    uint8_t magic[2];
    uint8_t compression_method;
    uint8_t flags;
    uint8_t timestamp[4];
    uint8_t compression_flags;
    uint8_t os;
} gz_header;

typedef enum gz_compression_method_t
{
    GZ_COMPRESSION_METHOD_DEFLATE = 8,
} gz_compression_method;

typedef enum gz_flags_t
{
    GZ_FLAG_FTEXT = 1,
    GZ_FLAG_FHCRC = 2,
    GZ_FLAG_FEXTRA = 4,
    GZ_FLAG_FNAME = 8,
    GZ_FLAG_FCOMMENT = 16,
} gz_flags;

typedef struct gz_footer_t
{
    uint32_t crc32;
    uint32_t size;
} gz_footer;

typedef struct gz_t
{
    FILE *file;
    gz_header header;
    uint16_t crc16;
    buffer *data;
    long data_start;
    long data_end;
    gz_footer footer;
    const char *filename;
} gz;

#define GZ_MAX_BUFFER_SIZE 32768
const uint8_t gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */
#define _byte_swap_32(x) (((x & 0x000000FF) << 24) | (x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24)

buffer *gz_read(const char *filename)
{
    gz *zip;
    buffer *out = NULL;
    size_t bytes_read = 0;

    zip = (gz *)malloc(sizeof(gz));
    if (zip == NULL)
        return NULL;

    zip->file = fopen(filename, "rb");
    if (zip->file == NULL)
        return NULL;

    zip->data = buffer_init(GZ_MAX_BUFFER_SIZE);
    if buffer_invalid (zip->data)
        goto memory_error;

    bytes_read = fread(&zip->header, sizeof(uint8_t), sizeof(gz_header), zip->file);
    if (bytes_read != sizeof(gz_header))
        goto file_error;

    if ((zip->header.magic[0] != gz_magic[0]) || (zip->header.magic[1] != gz_magic[1]))
        goto file_error;

    if (zip->header.compression_method != GZ_COMPRESSION_METHOD_DEFLATE)
        goto file_error;

    if (zip->header.flags & GZ_FLAG_FNAME)
    {
        zip->data_start = ftell(zip->file);
        if (zip->data_start == -1)
            goto file_error;

        bytes_read = fread(zip->data->data, sizeof(uint8_t), zip->data->length, zip->file);
        size_t filename_length = strnlen((char *)zip->data->data, zip->data->length);
        zip->data_start += filename_length + 1;
        zip->filename = strncpy((char *)malloc(filename_length), (char *)zip->data->data, filename_length);
    }

    if (zip->header.flags & GZ_FLAG_FHCRC)
    {
        bytes_read = fread(zip->data->data, sizeof(uint8_t), 2, zip->file);
        if (bytes_read != 2)
            goto file_error;
        zip->crc16 = zip->data->data[0] | (zip->data->data[1] << 8);
        // printf("crc16: %02x%02x\n", zip->data[0], zip->data[1]);
        // TODO: handle crc16
        zip->data_start += 2;
        zip->data->length = zip->data_start - 1;
        uint32_t crc16 = crc32(zip->data);
        zip->data->length = GZ_MAX_BUFFER_SIZE;

        if (crc16 && 0x0000FFFF != zip->crc16)
        {
            printf("crc16 mismatch\n");
            goto file_error;
        }
    }

    if (fseek(zip->file, -sizeof(gz_footer), SEEK_END) == 0)
    {
        zip->data_end = ftell(zip->file);
        bytes_read = fread(&zip->footer, sizeof(uint8_t), sizeof(gz_footer), zip->file);
        if (bytes_read != sizeof(gz_footer))
            goto file_error;
        zip->footer.crc32 = _byte_swap_32(zip->footer.crc32);
    }
    else
        goto file_error;

    // out = buffer_init(zip->footer.size);
    // if buffer_invalid (out)
    //     goto memory_error;

    fseek(zip->file, zip->data_start, SEEK_SET);
    bytes_read = fread(zip->data->data, sizeof(uint8_t), zip->data_end - zip->data_start, zip->file);

    out = inflate(zip->data, zip->footer.size);
    // printf("compressed: %zu uncompressed: %zu ratio: %0.1f%% uncompressed_name: %s\n", zip->data_end + sizeof(gz_footer), out->length, (float)(zip->data_end + sizeof(gz_footer)) / (float)out->length * 100.0, zip->filename);

    fclose(zip->file);
    buffer_free(zip->data);

    return out;

memory_error:
io_error:
file_error:
    fclose(zip->file);
    buffer_free(zip->data);

    return NULL;
}

void gz_write(const char *filename, buffer *buf)
{
    gz *zip;
    FILE *file = fopen(filename, "wb");

    uint16_t crc16;
    uint32_t crc32;
    uint32_t size;

    zip->header.magic[0] = gz_magic[0];
    zip->header.magic[1] = gz_magic[1];
    zip->header.compression_method = GZ_COMPRESSION_METHOD_DEFLATE;
    // header.flags = 0;
    // header.modification_time = 0;
    // header.extra_flags = 0;
    // header.operating_system = 0;

    // crc16 = 0;
    // crc32 = 0;
    // isize = 0;

    // fwrite(&header, sizeof(gz_header), 1, file);
    // fwrite(buf->data, buf->length, 1, file);
    // fwrite(&crc32, sizeof(uint32_t), 1, file);
    // fwrite(&size, sizeof(uint32_t), 1, file);

    fclose(file);
}

void gz_free(gz *zip)
{
    if (zip == NULL)
        return;

    if (zip->file != NULL)
        fclose(zip->file);

    if (zip->data != NULL)
        buffer_free(zip->data);

    free(zip);
}