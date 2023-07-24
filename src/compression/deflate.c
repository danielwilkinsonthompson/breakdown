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
#include "endian.h"
#include "error.h"
#include "buffer.h"
#include "stream.h"
#include "deflate.h"

#define LITERAL_TREE_SIZE 287
#define LITERAL_SYMBOL_BITS // [0..258]
#define DISTANCE_TREE_SIZE 30
#define DISTANCE_SYMBOL_BITS 15 // [0..32768]

const uint8_t length_extra_bits[30] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, // 257..265
    1, 1, 1, 2, 2, 2, 3, 3, 3, // 266..274
    4, 4, 5, 5, 6, 6, 7, 7, 8, // 275..283
    9, 10, 11                  // 284..285
};

const uint16_t length_base[30] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11,            // 257..265
    13, 15, 17, 19, 23, 27, 31, 35, 43,     // 266..274
    51, 59, 67, 83, 99, 115, 131, 163, 195, // 275..283
    227, 258                                // 284..285
};

const uint8_t distance_extra_bits[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3,      // 0..9
    3, 4, 4, 5, 5, 6, 6, 7, 7,      // 10..19
    8, 8, 9, 9, 10, 10, 11, 11, 12, // 20..29
    12, 13, 13                      // 30..31
};

const uint16_t distance_base[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17,                        // 0..9
    25, 33, 49, 65, 97, 129, 193, 257, 385,             // 10..19
    513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, // 20..29
    12289, 16385, 24577                                 // 30..31
};

// fixed tree literal code ranges
//    Value: Bits:                 Code:                    8-bit code:
//   0..143: 8 bits (0x00..0x8f)   0b00110000 ..0b10111111  0x30..0xbf
// 144..255: 9 bits (0x90..0xff)   0b110010000..0b111111111 0xc8..0xff
// 256..279: 7 bits (0x100..0x11b) 0b0000000  ..0b0010111   0x00..0x2f
// 280..287: 8 bits (0x11c..0x12f) 0b11000000 ..0b11000111  0xc0..0xc7
#define fixed_literal_7bit_maximum 0x2f
#define fixed_literal_8bit_minimum 0x30
#define fixed_literal_8bit_maximum 0xbf
#define fixed_literal_9bit_minimum 0xc8

// fixed tree distance code ranges
// 0..31: 5 bits (0x00..0x1f) 0b00000..0b11111

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

// huffman_trees
// static huffman_trees *_decode_huffman_trees(buffer *compressed);
static huffman_trees *_init_fixed_huffman_trees(void);
static huffman_trees *_decode_dynamic_trees(buffer *compressed);

// static huffman_trees *_decode_dynamic_trees(buffer *compressed);
static void _print_huffman_trees(huffman_trees *trees);
static void _free_huffman_trees(huffman_trees *trees);

// huffman_tree
static huffman_tree *_init_huffman_tree(size_t size);
static uint32_t huffman_decode(huffman_tree *tree, uint32_t code, uint32_t *code_length);
static void _construct_tree_from_code_lengths(huffman_tree *tree);
static void _assign_codes(node *this_node, uint32_t this_code, uint32_t this_code_length);
static void _print_huffman_tree(huffman_tree *tree);
static void _free_huffman_tree(huffman_tree *tree);

// node
static node *_init_node(void);
static void _print_node(node *n);
static void _free_node(node *n);

// block header
typedef struct block_header_t
{
  uint8_t final : 1;
  uint8_t block_type : 2;
  uint8_t reserved : 5;
} block_header;

// block_type = 0: Uncompressed block (starting at next byte)
typedef struct uncompressed_block_header_t
{
  uint16_t length;     // number of bytes in block
  uint16_t not_length; // one's complement of len
} uncompressed_block_header;

// Literal bytes fall within the set [0..255]
// length is  drawn from the set [3..258] (see RFC 1951, section 3.2.5)
// length and literal are merged into a single set [0..285], where
// 0..255 represent the literal bytes
// 256 represents the end of the block
// 257..285 represent the length codes
// extra bits after the symbol?? are drawn from the set [0..5]
// distance is drawn from the set [0..29]
// distance is drawn from the set [1..32768]

// Block_type = 2: Dynamic Huffman codes (starting at next byte)
// typedef struct dynamic_code_block_header_t
// {
//   uint16_t hlit;  // number of literal/length codes - 257 [0..256]
//   uint16_t hdist; // number of distance codes - 1
//   uint16_t hclen; // number of code length codes - 4
// } dynamic_code_block_header;

typedef enum block_type_t
{
  block_type_uncompressed = 0,
  block_type_fixed_huffman = 1,
  block_type_dynamic_huffman = 2,
} block_type;

error deflate(stream *uncompressed, stream *compressed)
{
  if (compressed == NULL || uncompressed == NULL)
    return null_pointer_error;

  block_header header;

  // TODO: Need an algorithm to determine the appropriate block type
  // TODO: Need an algorithm to break up the uncompressed stream into blocks
  header.final = 1;
  header.block_type = block_type_uncompressed;
  header.reserved = 0;

  // write block header
  size_t bytes_written = stream_write_bytes(compressed, (uint8_t *)&header, 1, false);
  if (bytes_written != 1)
    goto io_error;

  uncompressed_block_header uncompressed_header;
  uncompressed_header.length = uncompressed->length;
  uncompressed_header.not_length = ~uncompressed->length;

  bytes_written = stream_write_bytes(compressed, (uint8_t *)&uncompressed_header.length, 4, false);
  if (bytes_written != 4)
    goto io_error;

  // write uncompressed data
  bytes_written = stream_write_bytes(compressed, uncompressed->data, uncompressed->length, false);
  if (bytes_written != uncompressed->length)
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

  if (compressed == NULL || decompressed == NULL)
    return null_pointer_error;

  block_header *header = (block_header *)stream_read_bits(compressed, 3, false);
  if (header == NULL)
    return io_error;

  do
  {
    // check block type
    switch (header->block_type)
    {
    case block_type_uncompressed:
      printf("uncompressed_block\n");

      // TODO: pull remaining 5 bits of block header (actually need to align to next byte)
      uint8_t *unused_bits = stream_read_bits(compressed, 5, true);
      free(unused_bits);

      uint8_t *raw_header = stream_read_bytes(compressed, 4, true);
      uint16_t len = (*raw_header << 8) | *(raw_header + 1);
      uint16_t nlen = (*(raw_header + 2) << 8) | *(raw_header + 3);
      if (len != ~nlen)
        goto io_error;
      free(raw_header);

      buffer *raw_data = stream_read_buffer(compressed, len, true);
      if (raw_data == NULL)
        goto io_error;

      err = stream_write_buffer(decompressed, raw_data, false);
      if (err != success)
        goto io_error;

      free(raw_data);

      break;
    case block_type_fixed_huffman:
      printf("fixed_huffman_block\n");
      trees = _init_fixed_huffman_trees();
      if (trees == NULL)
        goto malloc_error;
      // goto process_stream;
      break;
    case block_type_dynamic_huffman:
      printf("dynamic_huffman_block\n");
      // need to grab the symbol table from the stream
      // trees = _decode_dynamic_trees(compressed);  // FIXME
      if (trees == NULL)
        goto malloc_error;
      break;
    default:
      printf("unknown_block\n");
      break;
    }
    // break;
    while (compressed->length > 0)
    {
      // decode literal/length code to value
      // if literal/length value < 256
      //   copy literal byte to output
      // else if literal/length value == 256
      //   break from loop
      // else (value = [257..285])
      //   decode distance values from input stream
      //   jump back in output stream by 'distance' and copy 'length' bytes to output
      // end if

      uint8_t *raw_data = stream_read_bytes(compressed, 1, true);
      if (raw_data == NULL)
        goto io_error;

      uint32_t literal_code = *raw_data;
      // printf("raw_data: %x\n", *code);
      free(raw_data);

      // Any code <= 0x2f is actually 7 bits, write 8th bit back to stream
      if ((literal_code) <= fixed_literal_7bit_maximum)
      {
        uint8_t code_lsb = (uint8_t)literal_code & 0x01;
        literal_code = literal_code >> 1;
        stream_write_bits(compressed, &code_lsb, 1, false);
      }

      // Any code >= 0xc8 is actually 9 bits, pull 9th bit from stream
      if ((literal_code) >= fixed_literal_9bit_minimum)
      {
        uint8_t *code_lsb = stream_read_bits(compressed, 1, true);
        literal_code = (literal_code << 1) | *code_lsb;
        free(code_lsb);
      }

      uint32_t code_length;
      uint32_t literal_value = huffman_decode(trees->literal, literal_code, &code_length);
      // printf("literal_code: 0x%02x literal_value: 0x%02x code_length: %u\n", literal_code, literal_value, code_length);

      if (literal_value < 256)
      {
        // printf("writing value %c to decompressed stream\n", value);
        uint8_t value_byte = (uint8_t)literal_value;
        size_t bytes_written = stream_write_bytes(decompressed, &value_byte, 1, false);
        if (bytes_written != 1)
        {
          printf("error writing to decompressed stream: %zu bytes written\n", bytes_written);
          stream_print(decompressed);

          goto io_error;
        }
      }
      else if (literal_value == 256)
        break;
      else
      {
        // printf("value: %u\n", value);
        // printf("value: %x\n", value);
        uint32_t length = (literal_value - 257) % 30;
        uint32_t extra_bits = length_extra_bits[length];
        uint32_t extra_length = 0;
        if (extra_bits > 0)
        {
          uint8_t *extra = stream_read_bits(compressed, extra_bits, true);
          extra_length = *extra;
          free(extra);
        }
        length = length_base[length] + extra_length;
        // printf("length: %u\n", length);

        uint8_t *distance_code = stream_read_bits(compressed, 5, true);
        uint32_t distance = huffman_decode(trees->distance, *distance_code, &code_length);
        // printf("distance_code: 0x%02x value: 0x%02x code_length: %u\n", *distance_code, distance, code_length);
        uint32_t extra_distance = 0;
        extra_bits = distance_extra_bits[distance];
        if (extra_bits > 0)
        {
          uint8_t *extra = stream_read_bits(compressed, extra_bits, true);
          extra_distance = *extra;
          free(extra);
        }
        distance = distance_base[distance] + extra_distance;
        // printf("distance: %u\n", distance);
        free(distance_code);

        while (length > 0)
        {
          uint8_t *byte_value = (decompressed->tail.byte - distance);

          size_t bytes_written = stream_write_bytes(decompressed, byte_value, 1, false);
          if (bytes_written != 1)
          {
            printf("error writing to decompressed stream\n");
            goto io_error;
          }
          length--;
        }
      }
    }

  } while ((header->final == 0) && (compressed->length > 0) && (err == success));

close_out:
  // if (trees != NULL)  // FIXME
  //   free_huffman_trees(trees);

  if (header != NULL)
    free(header);
  return err;

io_error:
  return io_error;

malloc_error:
  return memory_error;
}

static huffman_trees *_init_fixed_huffman_trees(void)
{
  if (fixed_trees != NULL)
    return fixed_trees;

  fixed_trees = (huffman_trees *)malloc(sizeof(huffman_trees));
  if (fixed_trees == NULL)
    goto malloc_error;

  // tree for literal/length codes from RFC 1951
  fixed_trees->literal = _init_huffman_tree(LITERAL_TREE_SIZE);
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

  fixed_trees->distance = _init_huffman_tree(32);
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
  _free_huffman_trees(fixed_trees);
  return NULL;
}

static uint32_t huffman_decode(huffman_tree *tree, uint32_t code, uint32_t *code_length)
{
  // node *this_node = tree->root;
  // for (size_t n = 0; n < code_length; n++)
  // {
  //   if (this_node->left == NULL && this_node->right == NULL)
  //     return this_node->value;

  //   if (code & (1 << (code_length - n - 1)))
  //     this_node = this_node->right;
  //   else
  //     this_node = this_node->left;
  // }

  // return this_node->value;

  // brute force search for now -> TODO: optimize with binary search
  for (size_t n = 0; n < tree->size; n++)
  {
    if (tree->nodes[n]->code == code)
    {
      *code_length = tree->nodes[n]->code_length;
      return tree->nodes[n]->value;
    }
  }

  code_length = 0;
  return 0;
}

static void _construct_tree_from_code_lengths(huffman_tree *tree)
{
  node *head, *this_node, *next_node;

  // find maximum code length
  uint32_t max_code_length = 0;
  for (size_t n = 0; n < tree->size; n++)
  {
    if (tree->nodes[n]->code_length > max_code_length)
      max_code_length = tree->nodes[n]->code_length;
  }

  uint32_t *next_code = (uint32_t *)calloc(max_code_length + 1, sizeof(uint32_t));
  uint32_t *length_histogram = (uint32_t *)calloc(max_code_length + 1, sizeof(uint32_t));
  if (next_code == NULL || length_histogram == NULL)
    goto malloc_error;

  for (size_t n = 0; n < tree->size; n++)
    length_histogram[tree->nodes[n]->code_length] += 1;

  // construct the base code for each code length
  uint32_t code = 0;
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

    // first code for a node with a given code length starts at the base code
    tree->nodes[n]->code = next_code[tree->nodes[n]->code_length];

    // increment the code for the next node with the same code length
    next_code[tree->nodes[n]->code_length]++;
  }

  return;
malloc_error:
  printf("malloc error\r\n");
  free(next_code);
  free(length_histogram);
  return;
}

// Assign codes based on tree structure
static void _assign_codes(node *this_node, uint32_t this_code, uint32_t this_code_length)
{
  if (this_node == NULL)
    return; // we have reached the end of the tree/branch

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

  // block_header deflate_header;
  // deflate_header.as_byte = (compressed->data[0]);
  // switch (deflate_header.as_bits.block_type)
  // {
  // case BLOCK_TYPE_UNCOMPRESSED:
  //   // nothing to be done, data is not compressed
  //   trees = NULL;
  // case BLOCK_TYPE_FIXED_HUFFMAN:
  //   trees = _init_fixed_huffman_trees();
  //   break;
  // case BLOCK_TYPE_DYNAMIC_HUFFMAN:
  //   trees = _decode_dynamic_trees(compressed);
  //   break;
  // default:
  //   // invalid block type
  //   trees = NULL;
  // }

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

  // we need to know the longest code length to print the tree
  uint32_t max_code_length = 0;
  for (size_t n = 0; n < tree->size; n++)
  {
    if (tree->nodes[n]->code_length > max_code_length)
      max_code_length = tree->nodes[n]->code_length;
  }

  printf("max code length: %d\r\n", max_code_length);

  // sprintf(str, "%C : %02x", n->value, n->code);

  // for (size_t n = 0; n < tree->size; n++)
  // {
  //   printf("node %zu: %d\n", n, tree->nodes[n]->value);
  // }
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
  fprintf(stderr, "\tvalue -> %C\r\n", n->value);
  fprintf(stderr, "\tfreq -> %zu\r\n", n->weight);
  fprintf(stderr, "\tfreq_children -> %zu\r\n", n->weight_children);
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

static const char *_node_to_string(node *n)
{
  static char str[256];

  sprintf(str, "%C : %02x", n->value, n->code);

  return str;
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