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
#include "buffer.h"

// typedef uint32_t huffman_data;
// typedef uint32_t huffman_length;
// typedef struct huffman_buffer_t
// {
//   huffman_length length;
//   huffman_data *data;
// } huffman_buffer;

typedef enum huffman_type_t
{
  BLOCK_TYPE_UNCOMPRESSED = 0,
  BLOCK_TYPE_FIXED_HUFFMAN = 1,
  BLOCK_TYPE_DYNAMIC_HUFFMAN = 2,
} huffman_type;

/*----------------------------------------------------------------------------
  huffman_tree
  ----------------------------------------------------------------------------
  generate huffman trees
-----------------------------------------------------------------------------*/
// typedef struct node_t node;
// typedef struct huffman_tree_t
// {
//   size_t size;
//   node *head;
//   node **nodes;
// } huffman_tree;

// typedef struct huffman_trees_t
// {
//   huffman_tree *literal;
//   huffman_tree *distance;
// } huffman_trees;

// huffman_trees *huffman_trees_fixed();
// huffman_trees *huffman_trees_dynamic(buffer *compressed);

/*----------------------------------------------------------------------------
  compress
  ----------------------------------------------------------------------------
  compress a buffer using either fixed or dynamic huffman encoding
  uncompressed  : uncompressed data
  type          : type of huffman encoding to use
  returns       : compressed data
-----------------------------------------------------------------------------*/
buffer *huffman_compress(buffer *uncompressed, huffman_type type);

/*----------------------------------------------------------------------------
  decompress
  ----------------------------------------------------------------------------
  decompress a buffer using either fixed or dynamic huffman encoding
  compressed  : compressed data
  type        : type of huffman encoding used
  returns     : decompressed data
-----------------------------------------------------------------------------*/
buffer *huffman_decompress(buffer *compressed, huffman_type type);
#endif