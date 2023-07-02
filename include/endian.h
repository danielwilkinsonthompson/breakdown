/*=============================================================================
                                    endian.h
-------------------------------------------------------------------------------
explicit conversion from little-endian/big-endian data buffers to stdint

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#ifndef __endian_h
#define __endian_h

// Little endian
#define _little_endian_to_uint32_t(u32) ((u32[3] << 24) | (u32[2] << 16) | (u32[1] << 8) | (u32[0]))
#define _little_endian_to_uint16_t(u16) ((u16[1] << 8) | (u16[0]))

// Big endian
#define _big_endian_to_uint32_t(u32) ((u32[0] << 24) | (u32[1] << 16) | (u32[2] << 8) | (u32[3]))
#define _big_endian_to_uint16_t(u16) ((u16[0] << 8) | (u16[1]))

#endif