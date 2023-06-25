// Huffman Coding C
// Source: https://www.programiz.com/dsa/huffman-coding

/*
Status:
- FIXME: Compressed data doesn't seem to cross word boundaries correctly


*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "huffman.h"

typedef struct node_t
{
    huffman_data value;
    huffman_length freq;
    huffman_length freq_children;
    uint32_t code;
    uint32_t code_length;
    struct node_t *child;
} node;

typedef struct huffman_tree_t
{
    huffman_length size;
    node *head;
    node **nodes;
} huffman_tree;

#define debug_node_value(n)      \
    fprintf(stderr, #n " = \n"); \
    _huffman_printf_node(n);     \
    fprintf(stderr, "\n");

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c\n"
#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

#define tree_invalid(tree) ((tree == NULL) || (tree->nodes == NULL))

static void _huffman_printf_node(node *n);
// static void _huffman_printf_huffman_tree(huffman_tree *tree);
static void _free_huffman_tree(huffman_tree *tree);
static huffman_buffer *_huffman_build_compressed_buffer(huffman_buffer *uncompressed, huffman_tree *tree);
static huffman_tree *_build_huffman_tree(huffman_buffer *uncompressed);

huffman_buffer *huffman_buffer_init(huffman_length length)
{
    huffman_buffer *buffer = (huffman_buffer *)malloc(sizeof(huffman_buffer));
    if (buffer == NULL)
        return NULL;

    buffer->length = length;
    buffer->data = (uint32_t *)malloc(sizeof(uint32_t) * buffer->length);
    if (buffer->data == NULL)
        return NULL;

    return buffer;
}

/*
    build_huffman_tree
    traverse uncompressed data and build a corresponding huffman tree
*/
static huffman_tree *_build_huffman_tree(huffman_buffer *uncompressed)
{
    huffman_tree *tree;
    huffman_length ordered_nodes[uncompressed->length];
    uint32_t code_child = 0;
    uint8_t code_length = 0;
    node *this_node;

    tree = (huffman_tree *)malloc(sizeof(huffman_tree));
    if (tree == NULL)
        goto malloc_error;

    tree->nodes = (node **)malloc(sizeof(node) * uncompressed->length);
    if (tree->nodes == NULL)
        goto malloc_error;

    tree->size = 0;
    for (huffman_length n = 0; n < uncompressed->length; n++)
    {
        tree->nodes[n] = (node *)malloc(sizeof(node));
        if (tree->nodes[n] == NULL)
            goto malloc_error;
    }

    // create nodes for all unique data values and track their occurrences
    for (huffman_length i = 0; i < uncompressed->length; i++)
    {
        for (huffman_length n = 0; n < uncompressed->length; n++)
        {
            if (uncompressed->data[i] == tree->nodes[n]->value)
            {
                tree->nodes[n]->freq++;
                break;
            }
            if (n == tree->size)
            { // should only get here after searching whole tree and a match wasn't found
                tree->nodes[tree->size]->value = uncompressed->data[i];
                tree->nodes[tree->size]->freq = 1;
                tree->nodes[tree->size]->freq_children = 0;
                tree->size++;
                break;
            }
        }
    }

    // list nodes in order of occurrence
    for (huffman_length n = 0; n < tree->size; n++)
    {
        ordered_nodes[n] = n;
        for (huffman_length m = n; m > 0; m--)
        {
            if (tree->nodes[ordered_nodes[m]]->freq < tree->nodes[ordered_nodes[m - 1]]->freq)
            {
                huffman_length temp = ordered_nodes[m];
                ordered_nodes[m] = ordered_nodes[m - 1];
                ordered_nodes[m - 1] = temp;
            }
        }
    }

    tree->nodes[ordered_nodes[0]]->freq_children = 0;
    for (huffman_length n = 1; n < tree->size; n++)
    {
        tree->nodes[ordered_nodes[n]]->freq_children = tree->nodes[ordered_nodes[n - 1]]->freq + tree->nodes[ordered_nodes[n - 1]]->freq_children;
    }

    for (huffman_length n = tree->size - 1; n >= 0; n--)
    {
        this_node = tree->nodes[ordered_nodes[n]];
        if (this_node->freq_children == 0)
        {
            this_node->code = code_child;
            this_node->child = NULL;
            this_node->code_length = code_length;
            break;
        }
        if ((this_node->freq < this_node->freq_children))
        {
            this_node->code = (code_child << 1) + 0;
            code_child = (code_child << 1) + 1;
        }
        else
        {
            this_node->code = (code_child << 1) + 1;
            code_child = (code_child << 1) + 0;
        }
        code_length++;
        this_node->code_length = code_length;
        this_node->child = tree->nodes[ordered_nodes[n - 1]];
        // this_node->child->code_length = this_node->code_length + 1;
    }
    tree->head = tree->nodes[ordered_nodes[tree->size - 1]];

    _huffman_printf_node(tree->head);

    return tree;

malloc_error:
    _free_huffman_tree(tree);
    return NULL;
}

static huffman_buffer *_huffman_build_compressed_buffer(huffman_buffer *uncompressed, huffman_tree *tree)
{
    huffman_buffer *compressed;
    uint32_t temp;
    uint8_t bit_index = 0;
    uint8_t bit_shift = 0;
    node *first_node;
    node *head;

    // we don't know how long it needs to be, but it should be less than the uncompressed uncompressed->length
    compressed = (huffman_buffer *)malloc(sizeof(huffman_buffer));
    if (compressed == NULL)
        return NULL;

    compressed->data = (uint32_t *)malloc(sizeof(uint32_t) * uncompressed->length);
    if (compressed->data == NULL)
        return NULL;

    compressed->length = 0;
    first_node = tree->head;
    head = tree->head;

    for (uint32_t i = 0; i < uncompressed->length; i++)
    {
        for (uint32_t n = 0; n < tree->size; n++)
        {
            if (head->value == uncompressed->data[i])
            {
                fprintf(stderr, "uncompressed->data[%d] == %C -> %02x\r\n", i, head->value, head->code);
                // fprintf(stderr, "%C -> code = 0b%C%C%C%C_%C%C%C%C\r\n", head->value, BYTE_TO_BINARY(head->code));
                _huffman_printf_node(head);

                // FIXME: Compressed data is not crossing boundaries correctly
                if ((bit_index + head->code_length) >= 32)
                {
                    if (compressed->length < uncompressed->length)
                    {
                        temp = compressed->data[compressed->length];
                        compressed->data[compressed->length] = (compressed->data[compressed->length] << (head->code_length)) + head->code; // need to figure this out later
                        compressed->length += 1;
                        bit_index = (bit_index + head->code_length) % 32;
                        compressed->data[compressed->length] = temp >> (32 - (bit_index));
                    }
                    else
                    {
                        fprintf(stderr, "panic!!\r\n");
                        break;
                    }
                }
                else
                {
                    compressed->data[compressed->length] = (compressed->data[compressed->length] << head->code_length) + head->code; // need to figure this out later
                    bit_index += head->code_length;
                }

                fprintf(stderr, "compressed->data[%d] = 0b%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C___%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C\r\n", compressed->length, BYTE_TO_BINARY((compressed->data[compressed->length] & 0xFF000000) >> 24), BYTE_TO_BINARY((compressed->data[compressed->length] & 0x00FF0000) >> 16), BYTE_TO_BINARY((compressed->data[compressed->length] & 0x0000FF00) >> 8), BYTE_TO_BINARY(compressed->data[compressed->length] & 0x000000FF));

                head = first_node;
                break;
            }
            else
            {
                // fprintf(stderr, "%C value != %C compressed->data[%d]\r\n", head->value, compressed->data[i], i);

                if (head->child != NULL)
                {
                    // _huffman_printf_node(head);
                    head = head->child;
                }
                else
                {
                    head = first_node;
                    break;
                }
            }
        }
    }
    compressed->length += 1;

    for (uint32_t out_ind = 0; out_ind < compressed->length; out_ind++)
    {
        fprintf(stderr, "compressed->data[%d] = 0b%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C___%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C\r\n", out_ind, BYTE_TO_BINARY((compressed->data[out_ind] & 0xFF000000) >> 24), BYTE_TO_BINARY((compressed->data[out_ind] & 0x00FF0000) >> 16), BYTE_TO_BINARY((compressed->data[out_ind] & 0x0000FF00) >> 8), BYTE_TO_BINARY(compressed->data[out_ind] & 0x000000FF));
    }
    fprintf(stderr, "bit_index = %d, compressed->length = %d\r\n", bit_index, compressed->length);

    return compressed;
}

huffman_buffer *huffman_compress(huffman_buffer *uncompressed)
{
    huffman_tree *tree = _build_huffman_tree(uncompressed);
    huffman_buffer *compressed = _huffman_build_compressed_buffer(uncompressed, tree);

    return compressed;
}

huffman_buffer *huffman_decompress(huffman_buffer *compressed)
{
    huffman_buffer *decompressed = (huffman_buffer *)malloc(sizeof(huffman_buffer));
    if (decompressed == NULL)
        return NULL;

    decompressed->data = (huffman_data *)malloc(sizeof(huffman_data) * compressed->length);

    return decompressed;
}

static huffman_buffer *_huffman_build_decompressed_buffer(huffman_buffer *uncompressed)
{
}

static void _huffman_printf_node(node *n)
{
    fprintf(stderr, "\tvalue -> %C\r\n", n->value);
    fprintf(stderr, "\tfreq -> %d\r\n", n->freq);
    fprintf(stderr, "\tfreq_children -> %d\r\n", n->freq_children);
    fprintf(stderr, "\tcode -> %d\r\n", n->code);
    fprintf(stderr, "\tcode_length -> %d\r\n", n->code_length);
    if (n->child == NULL)
        fprintf(stderr, "\tchild -> NULL\r\n");
    else
        fprintf(stderr, "\tchild -> %C\r\n", n->child->value);
}

static void _free_huffman_tree(huffman_tree *tree)
{
    if (tree == NULL)
        return;

    if (tree->nodes != NULL)
    {
        for (uint32_t n = 0; n < tree->size; n++)
            free(tree->nodes[n]);
    }
    free(tree->nodes);
    free(tree);
}

static void _free_huffman_data(huffman_buffer *buffer)
{
    if (buffer == NULL)
        return;

    free(buffer->data);
    free(buffer);
}