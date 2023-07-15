#ifndef _crc_h
#define _crc_h

#include <stdint.h>
#include "buffer.h"

#define byte_swap_32(x) (((x & 0x000000FF) << 24) | (x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24)

uint16_t crc16(buffer *buf);
uint32_t crc32(buffer *buf);

uint32_t crc32_update(uint32_t crc, buffer *buf);
uint32_t crc32_finalize(uint32_t crc);

#endif