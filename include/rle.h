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

uint8_t *rle1_compress(uint8_t *input_buffer, uint32_t input_length, uint32_t *output_length);
uint8_t *rle1_decompress(uint8_t *input_buffer, uint32_t input_length, uint32_t *output_length);

#endif