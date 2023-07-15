/*=============================================================================
                                    deflate.h
-------------------------------------------------------------------------------
deflate stream compression and decompression

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

References
- https://tools.ietf.org/html/rfc1951
-----------------------------------------------------------------------------*/
#ifndef __deflate_h
#define __deflate_h
#include <stdint.h>
#include "buffer.h"

typedef struct deflate_block_t
{
  size_t length;
  uint8_t *data;
} deflate_block;

typedef struct deflate_blocks_t
{
  deflate_block **block;
  size_t size;
} deflate_blocks;

typedef struct block_header_t
{
  uint8_t block_final : 1;
  uint8_t block_type : 2;
} block_header;

typedef enum block_type_t
{
  BLOCK_TYPE_UNCOMPRESSED = 0,
  BLOCK_TYPE_FIXED_HUFFMAN = 1,
  BLOCK_TYPE_DYNAMIC_HUFFMAN = 2,
} block_type;

/*----------------------------------------------------------------------------
  deflate
  ----------------------------------------------------------------------------
  compresses a buffer of data using the deflate algorithm
  data     :  uncompressed data
  returns  :  compressed data
-----------------------------------------------------------------------------*/
buffer *deflate(buffer *data);

/*----------------------------------------------------------------------------
  inflate
  ----------------------------------------------------------------------------
  decompresses a buffer of data using the deflate algorithm
  zdata    :  compressed data
  inflated_size :  size of uncompressed data
  returns  :  uncompressed data
-----------------------------------------------------------------------------*/
buffer *inflate(buffer *zdata, size_t inflated_size);

#endif
