/*=============================================================================
                                  deflate.c
-------------------------------------------------------------------------------
deflate stream compression

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references
- https://github.com/madler/infgen
- https://tools.ietf.org/html/rfc1951
- https://www.youtube.com/watch?v=SJPvNi4HrWQ
- https://go-compression.github.io/algorithms/lzss/

known bugs
- deflate() does not handle static huffman trees
- deflate() does not handle dynamic huffman trees
- deflate() does not have an algorithm to determine the appropriate block type
- huffman_decode() currently based on brute-force search, should be optimized

todo
- build static huffman tree object that could be decoded in the same way as dynamic trees
- write a function to construct length/distance pairs from a stream of bytes
- TODO: optimise data types to match RFC 1951
-----------------------------------------------------------------------------*/
#include <stdint.h>  // uint8_t, uint16_t, uint32_t
#include <stdlib.h>  // malloc, free
#include <stdio.h>   // printf
#include "endian.h"  // bytes to int
#include "error.h"   // error
#include "buffer.h"  // buffer
#include "stream.h"  // stream
#include "deflate.h" // deflate

#define literals 257
#define distances 32
#define lengths 30
#define cl_size 19
#define cl_code_length 3

// fixed tree literals/lengths
//    value:   bits                code                     8-bit code
// -------------------------------------------------------------------
//   0..143: 8 bits (0x00..0x8f)   0b00110000 ..0b10111111  0x30..0xbf
// 144..255: 9 bits (0x90..0xff)   0b110010000..0b111111111 0xc8..0xff
// 256..279: 7 bits (0x100..0x11b) 0b0000000  ..0b0010111   0x00..0x2f
// 280..287: 8 bits (0x11c..0x12f) 0b11000000 ..0b11000111  0xc0..0xc7
#define fixed_literal_7bit_maximum 0x2f
#define fixed_literal_9bit_minimum 0xc8

#define max_lzss_pairs_per_block 4096
// research RangeCoder

// see RFC 1951, section 3.2.5
const uint8_t length_extra_bits[lengths] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, // 257..266
    1, 1, 2, 2, 2, 2, 3, 3, 3, 3, // 267..276
    4, 4, 4, 4, 5, 5, 5, 5, 0     // 277..285
};

// see RFC 1951, section 3.2.5
const uint16_t length_base[lengths] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13,         // 257..266
    15, 17, 19, 23, 27, 31, 35, 43, 51, 59,  // 267..276
    67, 83, 99, 115, 131, 163, 195, 227, 258 // 277..285
};

// see RFC 1951, section 3.2.5
const uint8_t distance_extra_bits[distances] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3,        // 0..9
    4, 4, 5, 5, 6, 6, 7, 7, 8, 8,        // 10..19
    9, 9, 10, 10, 11, 11, 12, 12, 13, 13 // 20..29
};

// see RFC 1951, section 3.2.5
const uint16_t distance_base[distances] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25,                              // 0..9
    33, 49, 65, 97, 129, 193, 257, 385, 513, 769,                 // 10..19
    1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 // 20..29
};

// see RFC 1951, section 3.2.7
const uint8_t dynamic_cl_symbol_order[cl_size] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

typedef struct lzss_t
{
  uint32_t length;
  uint32_t distance;
} lzss;

typedef struct node_t
{
  uint32_t value;
  size_t weight;
  size_t weight_children;
  uint32_t code;
  uint32_t code_length;
  struct node_t *child0;
  struct node_t *child1;
} node;

typedef struct huffman_tree_t
{
  size_t size;
  node *head;
  node **nodes;
  size_t min_code_length;
  size_t max_code_length;
} huffman_tree;

typedef struct huffman_trees_t
{
  huffman_tree *literal;
  huffman_tree *distance;
} huffman_trees;

// see RFC 1951, section 3.2.3
typedef struct block_header_t
{
  uint8_t final : 1;
  uint8_t block_type : 2;
  uint8_t reserved : 5;
} block_header;

// see RFC 1951, section 3.2.3
typedef enum block_type_t
{
  block_type_uncompressed = 0,
  block_type_fixed_huffman = 1,
  block_type_dynamic_huffman = 2,
} block_type;

// see RFC 1951, section 3.2.4
typedef struct uncompressed_block_header_t
{
  uint16_t length;     // number of bytes in block
  uint16_t not_length; // one's complement of length
} uncompressed_block_header;

// see RFC 1951, section 3.2.7
typedef struct dynamic_block_header_t
{
  uint16_t cl_literals;  // number of literal/length codes - 257 [0..256]
  uint16_t cl_distances; // number of distance codes - 1
  uint16_t cl_lengths;   // number of code length codes - 4
} dynamic_block_header;

static huffman_trees *fixed_trees = NULL;

// huffman_trees (literal/length + distance)
static huffman_trees *_init_fixed_huffman_trees(void);
static huffman_trees *_decode_dynamic_trees(stream *compressed);
// static void _print_huffman_trees(huffman_trees *trees);
static void _free_huffman_trees(huffman_trees *trees);

// huffman_tree
static huffman_tree *_init_huffman_tree(size_t size);
static error huffman_decode(stream *compressed, huffman_tree *tree, uint32_t *symbol);
static void _construct_tree_from_code_lengths(huffman_tree *tree);
// static void _assign_codes(node *this_node, uint32_t this_code, uint32_t this_code_length);
// static void _print_huffman_tree(huffman_tree *tree);
static void _free_huffman_tree(huffman_tree *tree);

// node
static node *_init_node(void);
// static void _print_node(node *n);
static void _free_node(node *n);

error deflate(stream *uncompressed, stream *compressed)
{
  if (compressed == NULL || uncompressed == NULL)
    return null_pointer_error;

  block_header header;

  // Recommended flow for deflate()
  // Capture a manageable block of input data (e.g. 100kB)
  // - Note that the distance codes are limited to 32kB, so perhaps limit the block size to 32kB
  // Append EOB marker to block
  // Apply LZSS to compress repeated symbols
  // Construct dynamically-generated Huffman trees for literal/length and distance codes
  // - All codes must be used by symbols in the tree, even if they are not used in the block (Kraft-McMillan inequality)
  // - literal/length and distance codewords are limited to 15 bits (code length <= 15)
  // - could also use Package-Merge algorithm to construct the tree
  // Encode the dynamic trees and write to compressed stream
  // Use the dynamic trees to encode the block

  // TODO: Need an algorithm to determine the appropriate block type
  // TODO: Need an algorithm to break up the uncompressed stream into blocks
  header.final = 1;
  header.block_type = block_type_uncompressed;
  header.reserved = 0;

  // write block header
  size_t bits_written = stream_write_bits(compressed, (uint8_t *)&header, 8, false);
  if (bits_written != sizeof(block_header) * 8)
    goto io_error;

  uncompressed_block_header uncompressed_header;
  uncompressed_header.length = uncompressed->length / 8;
  uncompressed_header.not_length = ~(uncompressed->length / 8);

  bits_written = stream_write_bits(compressed, (uint8_t *)&uncompressed_header.length, sizeof(uncompressed_block_header) * 8, false);
  if (bits_written != sizeof(uncompressed_block_header) * 8)
    goto io_error;

  // write uncompressed data
  bits_written = stream_write_bits(compressed, uncompressed->head.byte, uncompressed->length, false);
  if (bits_written != uncompressed->length)
    goto io_error;

  return success;

io_error:
  printf("deflate.c - deflate() : io_error\r\n");

  return io_error;
}

error inflate(stream *compressed, stream *decompressed)
{
  error err = success;
  huffman_trees *trees = NULL;
  uint8_t *unused;

  // check inputs
  if (compressed == NULL || decompressed == NULL)
    return null_pointer_error;
  if (compressed->data == NULL || decompressed->data == NULL)
    return null_pointer_error;

  do
  {
    // read block header
    block_header *header = (block_header *)stream_read_bits(compressed, 3, false);
    if (header == NULL)
      return io_error;

    switch (header->block_type)
    {
    case block_type_uncompressed:
      unused = stream_read_bits(compressed, 5, false);
      free(unused);

      // pull uncompressed block header
      uint8_t *raw_header = stream_read_bytes(compressed, 4, false);

      uint16_t len = 0xffff & ((0x00ff & raw_header[1]) << 8) | (raw_header[0]);
      uint16_t nlen = 0xffff & ((0x00ff & raw_header[3]) << 8) | (raw_header[2]);

      free(raw_header);
      if (len != (~nlen & 0xffff)) // trying to avoid weird uint16_t errors
      {
        printf("deflate.c - inflate() : invalid uncompressed block header: len = 0x%02x, ~nlen = 0x%02x\r\n", len, ~nlen);
        goto io_error;
      }
      if (len * 8 != (compressed->length))
      {
        printf("deflate.c - inflate() : uncompressed block length (%d) is not equal to compressed block length (%zu) \r\n", len, compressed->length / 8);
      }

      buffer *raw_data = stream_read_buffer(compressed, len, false);
      if (raw_data == NULL)
        goto io_error;
      err = stream_write_buffer(decompressed, raw_data, false);
      free(raw_data);
      if (err != success)
        goto decompressed_overflow;
      break;

    case block_type_fixed_huffman:
      trees = _init_fixed_huffman_trees();
      if (trees == NULL)
        goto malloc_error;
      break;

    case block_type_dynamic_huffman:
      trees = _decode_dynamic_trees(compressed);
      if (trees == NULL)
        goto malloc_error;
      break;

    default:
      printf("unknown_block\n");
      goto io_error;
    }

    while ((compressed->length > 0) && (header->block_type != block_type_uncompressed))
    {
      // decode literal/length code
      uint32_t literal_value;
      err = huffman_decode(compressed, trees->literal, &literal_value);
      if (err == buffer_underflow)
        goto compressed_underflow;

      // literal codes [0..255] are just transferred to decompressed stream
      if (literal_value < 256)
      {
        uint8_t value_byte = (uint8_t)literal_value;
        size_t bytes_written = stream_write_bytes(decompressed, &value_byte, 1, false);
        if (bytes_written != 1)
          goto decompressed_overflow;
      }

      // literal code 256 signifies the end of a block
      else if (literal_value == 256)
        break;

      // literal codes [257..285] are length codes, we must decode length and distance
      else
      {
        // decode length
        uint32_t length = (literal_value - literals);
        uint32_t extra_bits = length_extra_bits[length];
        uint32_t extra_length = 0;
        if (extra_bits > 0)
        {
          uint8_t *extra = stream_read_bits(compressed, extra_bits, false);
          if (extra == NULL)
            goto compressed_underflow;
          extra_length = *extra;
          free(extra);
        }
        length = length_base[length] + extra_length;

        // decode distance
        uint32_t distance_index;
        err = huffman_decode(compressed, trees->distance, &distance_index);
        if (err == buffer_underflow)
          goto compressed_underflow;
        uint32_t extra_distance = 0;
        extra_bits = distance_extra_bits[distance_index];
        if (extra_bits > 0)
        {
          uint8_t *extra = stream_read_bits(compressed, extra_bits, false);
          if (extra == NULL)
            goto compressed_underflow;
          extra_distance = extra[0];
          if (extra_bits > 8)
            extra_distance += (extra[1] << 8);

          free(extra);
        }
        uint32_t distance = distance_base[distance_index] + extra_distance;
        // copy length bytes from distance bytes back in the stream
        while (length > 0)
        {
          // FIXME: need to make this wrap as it is a circular buffer
          uint8_t *byte_value = (decompressed->tail.byte - distance);
          if (byte_value < decompressed->data)
            printf("panic! panic! panic!\r\n"); // for now, just warn when reading invalid data
          size_t bytes_written = stream_write_bytes(decompressed, byte_value, 1, false);
          if (bytes_written != 1)
            goto decompressed_overflow;
          length--;
        }
      }
    }

    // if not up to final block, and remaining data <32768, we might be about to underflow
    if (header->final == 1)
    {
      free(header);
      break;
    }
    else
      free(header);

  } while ((compressed->length > 0) && (err == success));

  return err;

io_error:
  printf("deflate.c - inflate() : io_error\r\n");
  return io_error;

malloc_error:
  printf("deflate.c - inflate() : malloc_error\r\n");
  return memory_error;

  // handle compressed stream underflow
compressed_underflow:
  printf("compressed = %zu bytes\r\n", compressed->length / 8);
  return buffer_underflow;

decompressed_overflow:
  return buffer_overflow;

  // handle decompressed stream overflow
}

// TODO: replace this with a binary sequence that inflates to the static trees
static huffman_trees *_init_fixed_huffman_trees(void)
{
  if (fixed_trees != NULL)
    return fixed_trees;

  fixed_trees = (huffman_trees *)malloc(sizeof(huffman_trees));
  if (fixed_trees == NULL)
    goto malloc_error;

  // tree for literal/length codes from RFC 1951
  fixed_trees->literal = _init_huffman_tree(literals + lengths);
  if (fixed_trees->literal == NULL)
    goto malloc_error;

  for (size_t n = 0; n < fixed_trees->literal->size; n++)
  {
    fixed_trees->literal->nodes[n]->value = n;
    if (n < 144)
      fixed_trees->literal->nodes[n]->code_length = 8;
    else if (n < 256)
      fixed_trees->literal->nodes[n]->code_length = 9;
    else if (n < 280)
      fixed_trees->literal->nodes[n]->code_length = 7;
    else
      fixed_trees->literal->nodes[n]->code_length = 8;
  }

  _construct_tree_from_code_lengths(fixed_trees->literal);

  fixed_trees->distance = _init_huffman_tree(distances);
  if (fixed_trees->distance == NULL)
    goto malloc_error;

  for (size_t n = 0; n < fixed_trees->distance->size; n++)
  {
    fixed_trees->distance->nodes[n]->value = n;
    fixed_trees->distance->nodes[n]->code_length = 5;
  }
  _construct_tree_from_code_lengths(fixed_trees->distance);

  return fixed_trees;

malloc_error:
  printf("deflate: init_fixed_huffman_trees: malloc error\n");
  _free_huffman_trees(fixed_trees);
  return NULL;
}

static error huffman_decode(stream *compressed, huffman_tree *tree, uint32_t *symbol)
{
  uint32_t code_buffer = 0, flip_buffer = 0;

  // read shortest possible code from stream
  // for (uint8_t bits = 0; bits < tree->min_code_length; bits++)
  // {
  //   uint8_t *stream_bits = stream_read_bits(compressed, 1, false);
  //   if (stream_bits == NULL)
  //     return buffer_underflow;

  //   code_buffer <<= 1;
  //   code_buffer |= *stream_bits;
  //   free(stream_bits);
  // }
  uint8_t *stream_bits = stream_read_bits(compressed, tree->min_code_length, true);
  if (stream_bits == NULL)
    return buffer_underflow;

  code_buffer = *stream_bits;
  free(stream_bits);

  // loop through possible code lengths
  for (size_t code_length = tree->min_code_length; code_length <= tree->max_code_length; code_length++)
  {
    // search for code in tree
    for (size_t n = 0; n < tree->size; n++)
    {
      // if code is found, return symbol
      if ((tree->nodes[n]->code == code_buffer) && (tree->nodes[n]->code_length == code_length))
      {
        *symbol = tree->nodes[n]->value;
        return success;
      }
    }

    // TODO: implement tree traversal instead of brute force search

    // if code is not found, read another bit from the stream
    uint8_t *extra_bits = stream_read_bits(compressed, 1, false);
    if (extra_bits == NULL)
      return buffer_underflow;

    code_buffer <<= 1;
    code_buffer |= *extra_bits;
    free(extra_bits);
  }

  printf("deflate: huffman_decode: code not found\n");
  return io_error;
}

static void _construct_tree_from_code_lengths(huffman_tree *tree)
{
  node *head, *this_node, *next_node;

  // find minimum & maximum code length
  uint32_t max_code_length = 0;
  uint32_t min_code_length = 0xffffffff;
  for (size_t n = 0; n < tree->size; n++)
  {
    if (tree->nodes[n]->code_length > max_code_length)
      max_code_length = tree->nodes[n]->code_length;
    if ((tree->nodes[n]->code_length < min_code_length) && (tree->nodes[n]->code_length > 0))
      min_code_length = tree->nodes[n]->code_length;
  }

  uint32_t *next_code = (uint32_t *)calloc(max_code_length + 1, sizeof(uint32_t));
  uint32_t *length_histogram = (uint32_t *)calloc(max_code_length + 1, sizeof(uint32_t));
  if (next_code == NULL || length_histogram == NULL)
    goto malloc_error;

  for (size_t n = 0; n < tree->size; n++)
    length_histogram[tree->nodes[n]->code_length] += 1;

  // construct the base code for each code length, ignoring length = 0
  uint32_t code = 0;
  length_histogram[0] = 0;
  for (uint16_t bit_position = 1; bit_position <= max_code_length; bit_position++)
  {
    code = (code + length_histogram[bit_position - 1]) << 1;
    next_code[bit_position] = code;
  }

  // assign codes to each node
  for (size_t n = 0; n < tree->size; n++)
  {
    if (tree->nodes[n]->code_length == 0)
      continue; // don't assign codes to branch nodes

    // first code for a node of a given code length starts at the base code
    tree->nodes[n]->code = next_code[tree->nodes[n]->code_length];

    // increment the code for the next node with the same code length
    next_code[tree->nodes[n]->code_length]++;
  }

  tree->max_code_length = max_code_length;
  tree->min_code_length = min_code_length;
  return;

malloc_error:
  printf("deflate: construct_tree_from_code_lengths: malloc error\r\n");
  free(next_code);
  free(length_histogram);
  return;
}

// construct huffman tree in the style of RFC 1951

// Assign codes based on tree structure
// static void _assign_codes(node *this_node, uint32_t this_code, uint32_t this_code_length)
// {
//   if (this_node == NULL)
//     return; // we have reached the end of the tree/branch

//   this_node->code = this_code;
//   this_node->code_length = this_code_length;
//   _assign_codes(this_node->child0, (this_code << 1) + 0, this_code_length + 1);
//   _assign_codes(this_node->child1, (this_code << 1) + 1, this_code_length + 1);
//   return;
// }

static lzss *lzss_encode(stream *uncompressed)
{
  // the size of this array should be based on statistics of the uncompressed data
  lzss *pairs = (lzss *)malloc(sizeof(lzss) * max_lzss_pairs_per_block);
  if (pairs == NULL)
    goto malloc_error;
  uint32_t pair_count = 0;

  // data storage:
  // output array of length,distance pairs
  // sliding window of 32kB generated from uncompressed stream
  // table of known symbols and their locations in the sliding window

  // brute-force search of sliding window for matches with current character
  // of these matches, which also match the next character?
  // of these matches, which also match the next character?
  // end with longest match, or for repeats, closest match (fewer bits to encode distance)
  // if no matches, write literal byte to output array -- interesting, should we already return a stream with matches inline?
  // if match, write length,distance pair to output array

  // stream circular buffer location:
  // index = (head + distance) % capacity + buffer_start
  // unfortunately, distance is negative, so modulo operator doesn't work

malloc_error:
  return NULL;
}

static huffman_tree *_decode_cl_tree(stream *compressed, size_t size)
{
  error err;

  // read code-length table from stream
  uint8_t *cl_lengths = (uint8_t *)calloc(cl_size, sizeof(uint8_t));
  for (size_t n = 0; n < size; n++)
  {
    uint8_t *cl_length = stream_read_bits(compressed, cl_code_length, false);
    if (cl_length == NULL)
      return NULL;

    cl_lengths[dynamic_cl_symbol_order[n]] = *cl_length;
    free(cl_length);
  }

  // construct huffman tree from code-lengths
  huffman_tree *cl_tree = _init_huffman_tree(cl_size);
  if (cl_tree == NULL)
    return NULL;

  for (size_t n = 0; n < cl_size; n++)
  {
    cl_tree->nodes[n]->value = n;
    cl_tree->nodes[n]->code_length = cl_lengths[n];
  }
  _construct_tree_from_code_lengths(cl_tree);

  free(cl_lengths);

  return cl_tree;
}

static huffman_tree *_decode_cl_lengths(stream *compressed, huffman_tree *cl_tree, size_t cl_count, size_t tree_size)
{
  uint32_t symbol;
  error status;

  // read code-length table from stream
  huffman_tree *tree = _init_huffman_tree(tree_size);
  if (tree == NULL)
    return NULL;

  // decode code lengths from stream
  for (size_t n = 0; n < tree->size; n++)
  {
    if (n < cl_count)
    {
      status = huffman_decode(compressed, cl_tree, &symbol);
      if (status != success)
        break;
    }
    else
    {
      symbol = 0;
    }
    tree->nodes[n]->value = n;

    // deal with special cases of length = 16, 17, 18
    if (symbol == 16)
    {
      uint8_t *repeat_count = stream_read_bits(compressed, 2, false);
      if (repeat_count == NULL)
        goto io_error;

      for (size_t m = 0; m < *repeat_count + 3; m++)
      {
        tree->nodes[n + m]->value = n + m;
        tree->nodes[n + m]->code_length = tree->nodes[n - 1]->code_length;
      }

      n += *repeat_count + 2;
      free(repeat_count);
    }
    else if (symbol == 17)
    {
      uint8_t *repeat_count = stream_read_bits(compressed, 3, false);
      if (repeat_count == NULL)
        goto io_error;

      for (size_t m = 0; m < *repeat_count + 3; m++)
      {
        tree->nodes[n + m]->value = n + m;
        tree->nodes[n + m]->code_length = 0;
      }

      n += *repeat_count + 2;
      free(repeat_count);
    }
    else if (symbol == 18)
    {
      uint8_t *repeat_count = stream_read_bits(compressed, 7, false);
      if (repeat_count == NULL)
        goto io_error;

      for (size_t m = 0; m < *repeat_count + 11; m++)
      {
        tree->nodes[n + m]->value = n + m;
        tree->nodes[n + m]->code_length = 0;
      }

      n += *repeat_count + 10;
      free(repeat_count);
    }
    else
    {
      tree->nodes[n]->code_length = symbol;
    }
  }
  _construct_tree_from_code_lengths(tree);

  return tree;

malloc_error:
io_error:
  _free_huffman_tree(tree);
  return NULL;
}

static huffman_trees *_decode_dynamic_trees(stream *compressed)
{
  dynamic_block_header dynamic;
  error err;

  huffman_trees *trees = (huffman_trees *)malloc(sizeof(huffman_trees));
  if (trees == NULL)
    goto malloc_error;

  // read dynamic block header:
  // TODO: replace magic numbers with constants
  uint8_t *raw_header = stream_read_bits(compressed, 14, false);
  if (raw_header == NULL)
    goto io_error;

  uint16_t dynamic_header = _little_endian_to_uint16_t(raw_header);
  free(raw_header);

  dynamic.cl_literals = (dynamic_header & 0x001f) + 257;       // FIXME: Changed to 258 to debug
  dynamic.cl_distances = ((dynamic_header >> 5) & 0x001f) + 1; // FIXME: Changed to 2 to debug
  dynamic.cl_lengths = ((dynamic_header >> 10) & 0x000f) + 4;

  // construct cl tree
  huffman_tree *cl_tree = _decode_cl_tree(compressed, dynamic.cl_lengths);
  if (cl_tree == NULL)
    goto malloc_error;

  // construct literal/length tree
  trees->literal = _decode_cl_lengths(compressed, cl_tree, dynamic.cl_literals, literals + lengths);
  if (trees->literal == NULL)
    goto malloc_error;

  // construct distance tree
  trees->distance = _decode_cl_lengths(compressed, cl_tree, dynamic.cl_distances, distances);
  if (trees->distance == NULL)
    goto malloc_error;

  return trees;

io_error:
  return NULL;

malloc_error:
  return NULL;
}

// encode dynamic huffman trees
error _encode_dynamic_trees(huffman_trees *trees, buffer *compressed)
{
  error err = success;

  // note that the trees are not actually encoded, only the code lengths
  // trees->literal should still only be 286 long -- or is that 287?
  // trees->literal lengths limited to 16 -- should be 15?
  // trees->distance should still only be 30 long

  if (trees == NULL || compressed == NULL)
    return null_pointer_error;

  // go through the raw data, count the number of times each code length is used
  uint32_t literal_lengths[286] = {0};

  for (size_t n = 0; n < trees->literal->size; n++)
  {
    literal_lengths[trees->literal->nodes[n]->code_length] += 1;
  }

  // CL symbols are limited to 7 bits
  // Go through huffman tree and assign code lengths to each node
  // Then go through the tree again and assign codes to each node
  //

  // for code_length [0.. 15], lengths are literal lengths
  // for code_length [16..18], lengths are repeat lengths:
  // - 16 = repeat previous length 3..6 times (2-bit repeat count)
  // - 17 = repeat 0 for 3..10 times (3-bit repeat count)
  // - 18 = repeat 0 for 11..138 times (7-bit repeat count)

  // CL symbols (code lengths) are encoded using the following symbols:
  // 0..15: Represent code lengths of 0..15
  // 16: Copy the previous code length 3..6 times.
  //    The next 2 bits indicate repeat length (0 = 3, ... , 3 = 6)
  //    Example:  Codes 8, 16 (+2 bits 11), 16 (+2 bits 10) will expand to
  //    12 code lengths of 8 (1 + 6 + 5): [8 8 8 8 8 8 8 8 16 16 16 16 16 16]
  // use the code lengths to construct the code trees

  return success;
}

// static void _print_huffman_trees(huffman_trees *trees)
// {
//   if (trees == NULL)
//     return;
//   printf("literal/length tree:\r\n");
//   _print_huffman_tree(trees->literal);
//   printf("distance tree:\r\n");
//   _print_huffman_tree(trees->distance);
//   return;
// }

static void _free_huffman_trees(huffman_trees *trees)
{
  if (trees == NULL)
    return;
  if (trees->literal != NULL)
    _free_huffman_tree(trees->literal);
  if (trees->distance != NULL)
    _free_huffman_tree(trees->distance);
  free(trees);
  return;
}

// huffman tree
static huffman_tree *_init_huffman_tree(size_t size)
{
  huffman_tree *tree;

  tree = (huffman_tree *)malloc(sizeof(huffman_tree));
  if (tree == NULL)
    goto malloc_error;
  tree->size = size;
  tree->nodes = (node **)malloc(size * sizeof(node *));
  if (tree->nodes == NULL)
    goto malloc_error;
  for (size_t n = 0; n < size; n++)
  {
    tree->nodes[n] = _init_node();
    if (tree->nodes[n] == NULL)
      goto malloc_error;
  }
  return tree;

malloc_error:
  _free_huffman_tree(tree);
  return NULL;
}

// static void _print_huffman_tree(huffman_tree *tree)
// {
//   // TODO: Must be a way to do this with ASCII art
//   if (tree == NULL)
//     return;

//   // we need to know the longest code length to print the tree
//   uint32_t max_code_length = 0;
//   for (size_t n = 0; n < tree->size; n++)
//   {
//     if (tree->nodes[n]->code_length > max_code_length)
//       max_code_length = tree->nodes[n]->code_length;
//   }

//   printf("max code length: %d\r\n", max_code_length);

//   // sprintf(str, "%C : %02x", n->value, n->code);

//   // for (size_t n = 0; n < tree->size; n++)
//   // {
//   //   printf("node %zu: %d\n", n, tree->nodes[n]->value);
//   // }
//   return;
// }

static void _free_huffman_tree(huffman_tree *tree)
{
  if (tree == NULL)
    return;
  if (tree->nodes != NULL)
  {
    for (size_t n = 0; n < tree->size; n++)
    {
      _free_node(tree->nodes[n]);
    }
    free(tree->nodes);
  }
  free(tree);
  return;
}

// node
static node *_init_node(void)
{
  node *new_node;

  new_node = (node *)malloc(sizeof(node));
  if (new_node == NULL)
    goto malloc_error;

  new_node->value = 0;
  new_node->child0 = NULL;
  new_node->child1 = NULL;
  new_node->code = 0;
  new_node->code_length = 0;

  return new_node;

malloc_error:
  free(new_node);
  return NULL;
}

// static void _print_node(node *n)
// {
//   fprintf(stderr, "\tvalue -> %C\r\n", n->value);
//   fprintf(stderr, "\tfreq -> %zu\r\n", n->weight);
//   fprintf(stderr, "\tfreq_children -> %zu\r\n", n->weight_children);
//   fprintf(stderr, "\tcode -> %d\r\n", n->code);
//   fprintf(stderr, "\tcode_length -> %d\r\n", n->code_length);
//   if (n->child0 == NULL)
//     fprintf(stderr, "\tchild0 -> NULL\r\n");
//   else
//     fprintf(stderr, "\tchild0 -> %C\r\n", n->child0->value);
//   if (n->child1 == NULL)
//     fprintf(stderr, "\tchild1 -> NULL\r\n");
//   else
//     fprintf(stderr, "\tchild1 -> %C\r\n", n->child1->value);
// }

// static const char *_node_to_string(node *n)
// {
//   static char str[256];

//   sprintf(str, "%C : %02x", n->value, n->code);

//   return str;
// }

static void _free_node(node *n)
{
  if (n == NULL)
    return;
  if (n->child0 != NULL)
    _free_node(n->child0);
  if (n->child1 != NULL)
    _free_node(n->child1);
  free(n);
  return;
}