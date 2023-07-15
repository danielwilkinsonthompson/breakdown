/*=============================================================================
                                deflate.c
-------------------------------------------------------------------------------
deflate stream compression and decompression

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

References
- https://tools.ietf.org/html/rfc1951
- https://www.youtube.com/watch?v=SJPvNi4HrWQ
- zlib:
    - https://zlib.net/manual.html
    - https://datatracker.ietf.org/doc/html/rfc1950
-----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include "bits.h"
#include "huffman.h"
#include "buffer.h"

#define LENGTH_LITERAL_TREE_SIZE 286
#define LENGTH_LITERAL_TREE_BITS 9 //
#define LENGTH_LITERAL_SYMBOL_BITS // [0..258]
#define DISTANCE_TREE_SIZE 30
#define DISTANCE_TREE_BITS 5
#define DISTANCE_SYMBOL_BITS 15 // [0..32768]

// In general, we may want to start with larger trees and truncate them
// to have the above sizes, as this ensures we give the most common
// symbols priority in the tree

// Huffman codes are packed with the most significant bit first
// Data elements are packed with the least significant bit first
// The first bit of the first byte of the stream is the most significant bit of the first bit of the first code

// That is, for data elements:
//  BYTE 2   | BYTE 1   | BYTE 0
//  76543210 | 76543210 | 76543210

// For Huffman codes
//  BYTE 2   | BYTE 1   | BYTE 0
//  01234567 | 01234567 | 01234567

// Block header
// 3 bits: BFINAL
// 2 bits: BTYPE

typedef struct block_header_t
{
    union
    {
        struct
        {
            uint8_t block_final : 1;
            uint8_t block_type : 2;
            uint8_t reserved : 5;
        } as_bits;
        uint8_t as_byte;
    };
} block_header;

typedef enum block_type_t
{
    BLOCK_TYPE_UNCOMPRESSED = 0,
    BLOCK_TYPE_FIXED_HUFFMAN = 1,
    BLOCK_TYPE_DYNAMIC_HUFFMAN = 2,
} block_type;

// block_type = 0: Uncompressed block (starting at next byte)
typedef struct uncompressed_block_header_t
{
    uint16_t length;     // number of bytes in block
    uint16_t not_length; // one's complement of len
} uncompressed_block_header_t;

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
typedef struct fixed_code_block_header_t
{
    uint16_t hlit : 5; // number of literal/length codes - 257
    uint16_t hdist;    // number of distance codes - 1
    uint16_t hclen;    // number of code length codes - 4
} fixed_code_block_header_t;
// the codes of literal values can be constructed from code lengths
// values 0..143 are represented by the codes 00110000 through 10111111 (8-bit codes)
// values 144..255 are represented by the codes 110010000 through 111111111 (9-bit codes)
// values 256..279 are represented by the codes 0000000 through 0010111 (7-bit codes)
// values 280..287 are represented by the codes 11000000 through 11000111 (8-bit codes)
// this could be constructed as a lookup table

// block_type = 2: Dynamic Huffman codes (starting at next byte)
typedef struct dynamic_code_block_header_t
{
    uint16_t hlit;  // number of literal/length codes - 257
    uint16_t hdist; // number of distance codes - 1
    uint16_t hclen; // number of code length codes - 4
} dynamic_code_block_header;

// Huffman block (starting at next byte)
typedef struct huffman_block_header_t
{
    uint16_t hlit;  // number of literal/length codes - 257
    uint16_t hdist; // number of distance codes - 1
    uint16_t hclen; // number of code length codes - 4
} huffman_block_header;

// Huffman block code length codes (starting at next byte)
typedef struct huffman_block_code_length_codes_t
{
    uint8_t code_length_code_lengths[19]; // 3 bits each
} huffman_block_code_length_codes_t;

buffer *deflate(buffer *data)
{
    buffer *compressed_data;
    // = buffer_create();
    return compressed_data;
}

buffer *inflate(buffer *compressed, size_t inflated_size)
{
    buffer *uncompressed;
    block_header deflate_header;

    deflate_header.as_byte = (compressed->data[0]);
    if (deflate_header.as_bits.block_final == 1)
    {
        printf("final_block\n");
    }
    switch (deflate_header.as_bits.block_type)
    {
    case BLOCK_TYPE_UNCOMPRESSED:
        printf("uncompressed_block\n");
        break;
    case BLOCK_TYPE_FIXED_HUFFMAN:
        printf("fixed_huffman_block\n");
        break;
    case BLOCK_TYPE_DYNAMIC_HUFFMAN:
        printf("dynamic_huffman_block\n");
        break;
    default:
        printf("unknown_block\n");
        break;
    }

    uncompressed = buffer_init(inflated_size);

    huffman_decompress(compressed, );

    return uncompressed;
}