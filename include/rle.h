/*=============================================================================
                                    rle.h
-------------------------------------------------------------------------------
run length encoding compression

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://filestore.aqa.org.uk/resources/computing/AQA-8525-TG-RLE.PDF

TODO:
- Implement BI_RLE8 and BI_RLE4 compression
-----------------------------------------------------------------------------*/
#ifndef __rle_h
#define __rle_h
#include <stdint.h>

typedef struct rle1_t
{
    uint8_t length : 7;
    uint8_t value : 1;
} rle1;

// typedef struct rle4_t
// {
//     uint8_t length : 8;
//     uint8_t lower : 4;
//     uint8_t upper : 4;
// } rle4;

typedef struct rle4_t
{
    uint8_t length : 8;
    uint8_t value : 8;
} rle4;

uint8_t *rle4_compress(uint8_t *in, uint32_t count, uint32_t *size);
uint8_t *rle4_decompress(uint8_t *in, uint32_t size, uint32_t *count);
uint8_t *rle1_compress(uint8_t *in, uint32_t count, uint32_t *size);
uint8_t *rle1_decompress(uint8_t *in, uint32_t size, uint32_t *count);

#endif