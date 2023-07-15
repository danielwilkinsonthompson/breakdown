#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "buffer.h"
#include "crc.h"

/*---------------------------------------------------------------------------
  crc16
-----------------------------------------------------------------------------*/
static uint16_t crc16_table[256];
static bool crc16_table_generated = false;

static void generate_crc16_table(void)
{
    uint16_t crc;
    const uint16_t crc16_polynomial = 0xa001;

    for (uint16_t byte_value = 0; byte_value < 256; byte_value++)
    {
        crc = byte_value;
        for (uint8_t bit_index = 0; bit_index < 8; bit_index++)
            crc = (crc & 1) ? ((crc >> 1) ^ crc16_polynomial) : (crc >> 1);

        crc16_table[byte_value] = crc;
    }
    crc16_table_generated = true;
}

uint16_t crc16_update(uint16_t crc, buffer *buf)
{
    if (!crc16_table_generated)
        generate_crc16_table();

    for (uint16_t n = 0; n < buf->length; n++)
        crc = (crc >> 8) ^ crc16_table[(crc ^ buf->data[n]) & 0xff];

    return crc;
}

uint16_t crc16_finalize(uint16_t crc)
{
    return crc ^ 0xffff;
}

uint16_t crc16(buffer *buf)
{
    const uint16_t crc16_initial = 0xffff;

    return crc16_finalize(crc16_update(crc16_initial, buf));
}

/*---------------------------------------------------------------------------
  crc32
-----------------------------------------------------------------------------*/
static uint32_t crc32_table[256];
static bool crc32_table_generated = false;

static void generate_crc32_table(void)
{
    uint32_t crc;
    const uint32_t crc32_polynomial = 0xedb88320L;

    for (uint16_t byte_value = 0; byte_value < 256; byte_value++)
    {
        crc = (uint32_t)byte_value;
        for (uint8_t bit_index = 0; bit_index < 8; bit_index++)
            crc = (crc & 1) ? (crc32_polynomial ^ (crc >> 1)) : (crc >> 1);

        crc32_table[byte_value] = crc;
    }
    crc32_table_generated = true;
}

uint32_t crc32_update(uint32_t crc, buffer *buf)
{
    if (!crc32_table_generated)
        generate_crc32_table();

    for (uint16_t n = 0; n < buf->length; n++)
        crc = crc32_table[(crc ^ buf->data[n]) & 0xff] ^ (crc >> 8);

    return crc;
}

uint32_t crc32_finalize(uint32_t crc)
{
    return crc ^ 0xffffffffL;
}

uint32_t crc32(buffer *buf)
{
    const uint32_t crc32_initial = 0xffffffffL;

    return crc32_finalize(crc32_update(crc32_initial, buf));
}