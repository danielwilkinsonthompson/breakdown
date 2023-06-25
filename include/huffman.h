/*=============================================================================
                                huffman.h
-------------------------------------------------------------------------------
compress data using huffman encoding

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
-----------------------------------------------------------------------------*/
#ifndef __huffman_h
#define __huffman_h
#include <stdio.h>
#include <stdlib.h>

typedef uint32_t huffman_data;
typedef uint32_t huffman_length;
typedef struct huffman_buffer_t
{
    huffman_length length;
    huffman_data *data;
} huffman_buffer;

/*----------------------------------------------------------------------------
  buffer_init
  ----------------------------------------------------------------------------
  create a buffer
  length : length of buffer
-----------------------------------------------------------------------------*/
huffman_buffer *huffman_buffer_init(huffman_length length);

/*----------------------------------------------------------------------------
  compress
  ----------------------------------------------------------------------------
  compress a buffer of data using huffman encoding
  uncompressed  : uncompressed data
  returns       : compressed data
-----------------------------------------------------------------------------*/
huffman_buffer *huffman_compress(huffman_buffer *uncompressed);

/*----------------------------------------------------------------------------
  decompress
  ----------------------------------------------------------------------------
  decompress a buffer of huffman encoded data
  compressed  : compressed data
  returns     : decompressed data
-----------------------------------------------------------------------------*/
huffman_buffer *huffman_decompress(huffman_buffer *compressed);
#endif