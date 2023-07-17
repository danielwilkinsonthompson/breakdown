/*=============================================================================
                                deflate.c
-------------------------------------------------------------------------------
deflate stream compression

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

References
- https://github.com/madler/infgen
- https://tools.ietf.org/html/rfc1951
- https://www.youtube.com/watch?v=SJPvNi4HrWQ
- zlib:
    - https://zlib.net/manual.html
    - https://datatracker.ietf.org/doc/html/rfc1950
-----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "bit_buffer.h"
#include "error.h"
#include "buffer.h"

#define LITERAL_TREE_SIZE 286
#define LITERAL_SYMBOL_BITS // [0..258]
#define DISTANCE_TREE_SIZE 30
#define DISTANCE_SYMBOL_BITS 15 // [0..32768]

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
} huffman_tree;

typedef struct huffman_trees_t
{
  huffman_tree *literal;
  huffman_tree *distance;
} huffman_trees;

static huffman_trees *fixed_trees = NULL;

// blocks
static error _inflate_uncompressed_block(bit_buffer *uncompressed, bit_buffer *compressed);
static error _inflate_fixed_huffman_block(bit_buffer *uncompressed, bit_buffer *compressed);
static error _inflate_dynamic_huffman_block(bit_buffer *uncompressed, bit_buffer *compressed);

// huffman_trees
static huffman_trees *_decode_huffman_trees(buffer *compressed);
static huffman_trees *_init_fixed_trees(buffer *compressed);
static huffman_trees *_decode_dynamic_trees(buffer *compressed);
static void _print_huffman_trees(huffman_trees *trees);
static void _free_huffman_trees(huffman_trees *trees);

// huffman_tree
static huffman_tree *_init_huffman_tree(size_t size);
static void _construct_tree_from_code_lengths(huffman_tree *tree);
static void _assign_codes(node *this_node, uint32_t this_code, uint32_t this_code_length);
static void _print_huffman_tree(huffman_tree *tree);
static void _free_huffman_tree(huffman_tree *tree);

// node
static node *_init_node(void);
static void _print_node(node *n);
static void _free_node(node *n);

// Huffman codes are packed with the most significant bit first
// Data elements are packed with the least significant bit first
// The first bit of the first byte of the stream is the most significant bit of the first bit of the first code

// That is, for data elements:
//  BYTE 2   | BYTE 1   | BYTE 0
//  76543210 | 76543210 | 76543210

// For Huffman codes
//  BYTE 2   | BYTE 1   | BYTE 0
//  01234567 | 01234567 | 01234567

#define BLOCK_FINAL 1
#define BLOCK_TYPE_MASK 6
#define BLOCK_TYPE_UNCOMPRESSED 0
#define BLOCK_TYPE_FIXED_HUFFMAN 2
#define BLOCK_TYPE_DYNAMIC_HUFFMAN 4

// block_type = 0: Uncompressed block (starting at next byte)
typedef struct uncompressed_block_header_t
{
  uint16_t length;     // number of bytes in block
  uint16_t not_length; // one's complement of len
} uncompressed_block_header;

// literal bytes fall within the set [0..255]
// length is  drawn from the set [3..258] (see RFC 1951, section 3.2.5)
// length and literal are merged into a single set [0..285], where
// 0..255 represent the literal bytes
// 256 represents the end of the block
// 257..285 represent the length codes
// extra bits after the symbol?? are drawn from the set [0..5]
// distance is drawn from the set [0..29]
// distance is drawn from the set [1..32768]

// block_type = 1: Fixed Huffman codes (starting at next byte)
// typedef struct fixed_code_block_header_t
// {
//   uint16_t hlit : 5; // number of literal/length codes - 257
//   uint16_t hdist;    // number of distance codes - 1
//   uint16_t hclen;    // number of code length codes - 4
// } fixed_code_block_header_t;

// the codes of literal values can be constructed from code lengths
// values 0..143 are represented by the codes 00110000 through 10111111 (8-bit codes)
// values 144..255 are represented by the codes 110010000 through 111111111 (9-bit codes)
// values 256..279 are represented by the codes 0000000 through 0010111 (7-bit codes)
// values 280..287 are represented by the codes 11000000 through 11000111 (8-bit codes)
// this could be constructed as a lookup table

// block_type = 2: Dynamic Huffman codes (starting at next byte)
// typedef struct dynamic_code_block_header_t
// {
//   uint16_t hlit;  // number of literal/length codes - 257 [0..256]
//   uint16_t hdist; // number of distance codes - 1
//   uint16_t hclen; // number of code length codes - 4
// } dynamic_code_block_header;

// // Huffman block code length codes (starting at next byte)
// typedef struct huffman_block_code_length_codes_t
// {
//   uint8_t code_length_code_lengths[19]; // 3 bits each
// } huffman_block_code_length_codes_t;

// typedef struct deflate_block_t
// {
//   size_t length;
//   uint8_t *data;
// } deflate_block;

// typedef struct deflate_blocks_t
// {
//   deflate_block **block;
//   size_t size;
// } deflate_blocks;

// typedef struct block_header_t
// {
//   uint8_t block_final : 1;
//   uint8_t block_type : 2;
// } block_header;

// typedef enum block_type_t
// {
//   BLOCK_TYPE_UNCOMPRESSED = 0,
//   BLOCK_TYPE_FIXED_HUFFMAN = 1,
//   BLOCK_TYPE_DYNAMIC_HUFFMAN = 2,
// } block_type;

buffer *deflate(buffer *data)
{
  buffer *compressed_data;
  // = buffer_create();
  return compressed_data;
}

// compressed should be composed of all compressed blocks concatenated together
// inflated_size is the size of the uncompressed data
buffer *inflate(buffer *compressed_stream, size_t inflated_size)
{
  uint8_t *block_header = compressed_stream->data;
  buffer *uncompressed_stream = buffer_init(inflated_size);
  if (uncompressed_stream == NULL)
    goto malloc_error;

  bit_buffer *compressed = bit_buffer_from_buffer(compressed_stream);
  bit_buffer *uncompressed = bit_buffer_from_buffer(uncompressed_stream);
  if (compressed == NULL || uncompressed == NULL)
    goto malloc_error;

  // loop through blocks
  while ((block_header < compressed->data + compressed->size))
  {
    if (*block_header & BLOCK_FINAL == 0) // why do we care?
    {
      printf("final_block\n");
    }

    // check block type
    switch (*block_header & BLOCK_TYPE_MASK)
    {
    case BLOCK_TYPE_UNCOMPRESSED:
      uncompressed_block_header *hdr = (uncompressed_block_header *)bit_read(compressed, 3); // skip over block type bits
      uint16_t len = (*block_header << 8) | *(block_header + 1);
      uint16_t nlen = (*(block_header + 2) << 8) | *(block_header + 3);
      block_header += 4;
      if (len != ~nlen)
        goto io_error;

      // // I'm not convinced input and output streams are byte-aligned
      // memcpy(uncompressed_bits->next_byte, compressed_bits->next_byte, len);
      // next_uncompressed_byte += len;
      block_header += len;
      break;
    case BLOCK_TYPE_FIXED_HUFFMAN:
      printf("fixed_huffman_block\n");
      block_header = inflate_fixed_huffman_block(compressed_bits, uncompressed_bits, );
      static error _inflate_fixed_huffman_block(bit_buffer * uncompressed, bit_buffer * compressed);

      // inflate_fixed_huffman_block(block_header);
      break;
    case BLOCK_TYPE_DYNAMIC_HUFFMAN:
      printf("dynamic_huffman_block\n");
      // inflate_dynamic_huffman_block(block_header);
      break;
    default:
      printf("unknown_block\n");
      break;
    }
  }
  // process stream
  // loop until end of block
  //   decode literal/length code to value
  //   if literal/length value < 256
  //     copy literal byte to output
  //   else if literal/length value == 256
  //     break from loop
  //   else (value = [257..285])
  //     decode distance values from input stream
  //     jump back in output stream by 'distance' and copy 'length' bytes to output
  //  end if
  // end loop
  // read next block header

  return uncompressed;

io_error:
malloc_error:
  printf("malloc error\n");
  buffer_free(uncompressed);
  free(compressed_bits);
  free(uncompressed_bits);
  return NULL;
}

static huffman_trees *_init_fixed_huffman_trees(void)
{
  if (fixed_trees != NULL)
    return fixed_trees;

  fixed_trees = (huffman_trees *)malloc(sizeof(huffman_trees));
  if (fixed_trees == NULL)
    goto malloc_error;

  // TODO: replace magic numbers with #defines
  fixed_trees->literal = _init_huffman_tree(288);
  if (fixed_trees->literal == NULL)
    goto malloc_error;

  for (size_t n = 0; n < fixed_trees->literal->size; n++)
  {
    fixed_trees->literal->nodes[n]->value = n;
    if (n < 144)
      fixed_trees->literal->nodes[n]->code_length = 8;
    else if (n < 256)
      fixed_trees->literal->nodes[n]->code_length = 9;
    else
      fixed_trees->literal->nodes[n]->code_length = 7;
  }
  // TODO: figure out how to add 'extra bits' to symbol tree
  // build_huffman_tree_from_code_lengths(fixed_trees->literal);

  fixed_trees->distance = _init_huffman_tree(32);
  if (fixed_trees->distance == NULL)
    goto malloc_error;

  for (size_t n = 0; n < fixed_trees->distance->size; n++)
  {
    fixed_trees->distance->nodes[n]->value = n;
    fixed_trees->distance->nodes[n]->code_length = 5;
  }
  // TODO: figure out how to add 'extra bits' to distance tree
  // build_huffman_tree_from_code_lengths(fixed_trees->distance);

  return fixed_trees;

malloc_error:
  _free_huffman_trees(fixed_trees);
  return NULL;
}

static void _construct_tree_from_code_lengths(huffman_tree *tree)
{
  // we have a tree with a node for every symbol
  // every node has only the code length and symbol populated
  // we need to construct the tree from this information
  // we can do this by sorting the nodes by code length
  // then we can iterate through the nodes and add them to the tree
  // we can use the code length to determine where to add the node
  // if the code length is 0, we add it to the head
  // if the code length is 1, we add it to the head's child0 or child1
  // if the code length is 2, we add it to the head's child0's child0 or child1
  // if the code length is 3, we add it to the head's child0's child0's child0 or child1
  // etc.

  node *this_node, *next_node, *head;
  size_t *length_tally;
  list *stack = list_init();
  size_t n, m;
  uint32_t code;
  uint32_t next_code[tree->size];

  // TODO: this should be one array with an element for all possible code lengths
  length_tally = (size_t *)calloc(tree->size, sizeof(size_t));
  if (length_tally == NULL)
    goto malloc_error;

  // sort nodes by code length - ok
  for (n = 0; n < tree->size; n++)
  {
    length_tally[tree->nodes[n]->code_length] += 1;
    for (m = n; m > 0; m--)
    {
      if (tree->nodes[m]->code_length < tree->nodes[m - 1]->code_length)
      {
        this_node = tree->nodes[m];
        tree->nodes[m] = tree->nodes[m - 1];
        tree->nodes[m - 1] = this_node;
      }
    }
  }

  // calculate next code for each code length
  length_tally[0] = 0;
  code = 0;
  for (uint16_t bit_position = 1; bit_position < sizeof(tree->nodes[0]->code_length); bit_position++)
  {
    code = (code + length_tally[bit_position - 1]) << 1;
    next_code[bit_position] = code;
  }

  // assign codes to each node
  for (n = 0; n < tree->size; n++)
  {
    this_node = tree->nodes[n];
    if (this_node->code_length == 0)
      continue;
    this_node->code = next_code[this_node->code_length];
    next_code[this_node->code_length]++;
  }

  // add nodes to tree - TODO: need to check this, copilot generated
  for (n = 0; n < tree->size; n++)
  {
    this_node = tree->nodes[n];
    if (this_node->code_length == 0)
    {
      tree->head = this_node;
      continue;
    }
    head = tree->head;
    for (m = 0; m < this_node->code_length - 1; m++)
    {
      if (this_node->code & (1 << (this_node->code_length - m - 1)))
      {
        if (head->child1 == NULL)
        {
          next_node = _init_node();
          head->child1 = next_node;
        }
        head = head->child1;
      }
      else
      {
        if (head->child0 == NULL)
        {
          next_node = _init_node();
          head->child0 = next_node;
        }
        head = head->child0;
      }
    }
    if (this_node->code & 1)
      head->child1 = this_node;
    else
      head->child0 = this_node;
  }

  return;
malloc_error:
  printf("malloc error\r\n");
  list_free(stack);
  // _free_huffman_tree(tree);
  return;
}

static void _assign_codes(node *this_node, uint32_t this_code, uint32_t this_code_length)
{
  if (this_node == NULL)
    return;
  this_node->code = this_code;
  this_node->code_length = this_code_length;
  _assign_codes(this_node->child0, (this_code << 1) + 0, this_code_length + 1);
  _assign_codes(this_node->child1, (this_code << 1) + 1, this_code_length + 1);
  return;
}

// huffman trees
static huffman_trees *_decode_huffman_trees(buffer *compressed)
{
  huffman_trees *trees = NULL;

  if (trees == NULL)
    goto malloc_error;

  block_header deflate_header;
  deflate_header.as_byte = (compressed->data[0]);
  switch (deflate_header.as_bits.block_type)
  {
  case BLOCK_TYPE_UNCOMPRESSED:
    // nothing to be done, data is not compressed
    trees = NULL;
  case BLOCK_TYPE_FIXED_HUFFMAN:
    trees = _init_fixed_huffman_trees();
    break;
  case BLOCK_TYPE_DYNAMIC_HUFFMAN:
    trees = _decode_dynamic_trees(compressed);
    break;
  default:
    // invalid block type
    trees = NULL;
  }

  return trees;

malloc_error:
  _free_huffman_trees(trees);
  return NULL;
}

static huffman_trees *_decode_dynamic_trees(buffer *compressed)
{
  huffman_trees *trees = NULL;

  if (trees == NULL)
    goto malloc_error;

  // read code trees (literal/length + distance) from stream
  // process stream as below

  return trees;
malloc_error:
  return NULL;
}

static void _print_huffman_trees(huffman_trees *trees)
{
  if (trees == NULL)
    return;
  printf("literal/length tree:\r\n");
  _print_huffman_tree(trees->literal);
  printf("distance tree:\r\n");
  _print_huffman_tree(trees->distance);
  return;
}

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

// static huffman_tree *_decode_dynamic_tree(buffer *compressed)
// {
//   // TODO: Gotta figure this one out
//   return NULL;
// }

static void _print_huffman_tree(huffman_tree *tree)
{
  // TODO: Must be a way to do this with ASCII art
  if (tree == NULL)
    return;
  for (size_t n = 0; n < tree->size; n++)
  {
    printf("node %d: %d\n", n, tree->nodes[n]->value);
  }
  return;
}

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

static void _print_node(node *n)
{
  fprintf(stderr, "node:\r\n");
  fprintf(stderr, "\tvalue -> %C\r\n", n->value);
  fprintf(stderr, "\tfreq -> %d\r\n", n->weight);
  fprintf(stderr, "\tfreq_children -> %d\r\n", n->weight_children);
  fprintf(stderr, "\tcode -> %d\r\n", n->code);
  fprintf(stderr, "\tcode_length -> %d\r\n", n->code_length);
  if (n->child0 == NULL)
    fprintf(stderr, "\tchild0 -> NULL\r\n");
  else
    fprintf(stderr, "\tchild0 -> %C\r\n", n->child0->value);
  if (n->child1 == NULL)
    fprintf(stderr, "\tchild1 -> NULL\r\n");
  else
    fprintf(stderr, "\tchild1 -> %C\r\n", n->child1->value);
}

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