/*=============================================================================
                                    huffman.c
-------------------------------------------------------------------------------
huffman coding

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references
- https://www.programiz.com/dsa/huffman-coding

status
- compressed data doesn't seem to cross word boundaries correctly
- need to insert EOF node into tree
- need to ensure that equal length codes are sorted by value
- need to implement tree traversal
- need to implement tree freeing
- need to implement tree printing
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "huffman.h"
#include "list.h"

typedef struct node_t
{
    huffman_data value;
    huffman_length weight;
    huffman_length weight_children;
    uint32_t code;
    uint32_t code_length;
    struct node_t *child0;
    struct node_t *child1;
} node;

typedef struct huffman_tree_t
{
    huffman_length size;
    node *head;
    node **nodes;
} huffman_tree;

#define debug_node_value(n)      \
    fprintf(stderr, #n " = \n"); \
    _print_node(n);              \
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

static node *_init_node(void);
static void _print_node(node *n);
static void _free_huffman_tree(huffman_tree *tree);
static huffman_buffer *_huffman_build_compressed_buffer(huffman_buffer *uncompressed, huffman_tree *tree);
static huffman_tree *_build_huffman_tree(huffman_buffer *uncompressed);
static void _assign_codes(node *this_node, uint32_t this_code, uint32_t this_code_length);

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

static huffman_tree *_build_huffman_tree(huffman_buffer *uncompressed)
{
    huffman_tree *tree;
    uint32_t code_child = 0;
    uint8_t code_length = 0;
    node *this_node, *next_node, *head;
    list *stack = list_init();
    huffman_length i, n, m;

    tree = (huffman_tree *)malloc(sizeof(huffman_tree));
    if (tree == NULL)
        goto malloc_error;

    tree->nodes = (node **)malloc(sizeof(node *) * uncompressed->length);
    if (tree->nodes == NULL)
        goto malloc_error;

    tree->size = 0;
    for (n = 0; n < uncompressed->length; n++)
    {
        tree->nodes[n] = _init_node();
        if (tree->nodes[n] == NULL)
            goto malloc_error;
    }

    // create nodes for all unique data values and track their occurrences
    for (i = 0; i < uncompressed->length; i++)
    {
        for (n = 0; n < uncompressed->length; n++)
        {
            if (uncompressed->data[i] == tree->nodes[n]->value)
            {
                tree->nodes[n]->weight++;
                break;
            }
            if (n == tree->size)
            { // should only get here after searching whole tree and a match wasn't found
                tree->nodes[tree->size]->value = uncompressed->data[i];
                tree->nodes[tree->size]->weight = 1;
                tree->nodes[tree->size]->weight_children = 0;
                tree->size++;
                break;
            }
        }
    }

    // sort nodes by weight
    for (n = 0; n < tree->size; n++)
    {
        for (m = n; m > 0; m--)
        {
            if (tree->nodes[m]->weight < tree->nodes[m - 1]->weight)
            {
                this_node = tree->nodes[m];
                tree->nodes[m] = tree->nodes[m - 1];
                tree->nodes[m - 1] = this_node;
            }
        }
    }

    // create tree
    for (n = 0; n < tree->size; n++)
        list_append(stack, tree->nodes[n]);

    while (true)
    {
        for (n = 0; n < stack->size; n++)
        {
            this_node = list_item(stack, n);
        }
        this_node = _init_node();
        this_node->value = 0;
        this_node->child0 = list_pop(stack);
        if (this_node->child0 == NULL)
            break;
        this_node->child1 = list_pop(stack);
        if (this_node->child1 == NULL)
            break;
        tree->size++;
        this_node->weight_children = this_node->child0->weight + this_node->child1->weight;
        this_node->weight = this_node->weight_children;
        this_node->code = 0;
        this_node->code_length = 0;
        next_node = list_pop(stack);
        if (next_node == NULL)
        {
            list_push(stack, this_node);
            break;
        }
        if (next_node->weight > this_node->weight)
        {
            this_node->code = (this_node->code << 1) + 0;
            this_node->code_length++;
            list_push(stack, next_node);
            list_push(stack, this_node);
        }
        else
        {
            this_node->code = (this_node->code << 1) + 1;
            this_node->code_length++;
            list_push(stack, this_node);
            list_push(stack, next_node);
        }
    }

    if (stack->size > 1)
    {
        printf("something went wrong: stack size: %zu\n", stack->size);
        goto malloc_error;
    }
    else
    {
        this_node = list_pop(stack);
        tree->head = this_node;
    }

    // assign codes to nodes
    _assign_codes(tree->head, 0, 0);

    list_free(stack);

    return tree;

malloc_error:
    printf("malloc error\r\n");
    list_free(stack);
    _free_huffman_tree(tree);
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
    huffman_length *length_tally;
    list *stack = list_init();
    huffman_length n, m;
    uint32_t code;
    uint32_t next_code[tree->size];

    // TODO: this should be one array with an element for all possible code lengths
    length_tally = (huffman_length *)calloc(tree->size, sizeof(huffman_length));
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

static huffman_buffer *_huffman_build_compressed_buffer(huffman_buffer *uncompressed, huffman_tree *tree)
{
    huffman_buffer *compressed;
    uint32_t temp;
    uint8_t bit_index = 0;
    uint8_t bit_shift = 0;
    node *first_node;
    node *head, *parent;

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
    parent = tree->head;

    for (uint32_t i = 0; i < uncompressed->length; i++)
    {
        for (uint32_t n = 0; n < tree->size; n++)
        {
            if ((head->child0 == NULL) && (head->value == uncompressed->data[i]))
            {
                printf("%C -> %C%C%C%C_%C%C%C%C\r\n", uncompressed->data[i], BYTE_TO_BINARY(head->code));
                // FIXME: Compressed data is not crossing boundaries correctly
                if ((bit_index + head->code_length) >= 32)
                {
                    if (compressed->length < uncompressed->length)
                    {
                        temp = head->code;
                        compressed->data[compressed->length] = (compressed->data[compressed->length] << (32 - bit_index)) + (head->code << (head->code_length - (32 - bit_index))); // need to figure this out later
                        fprintf(stderr, "compressed->data[%d] = 0b%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C___%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C\r\n", compressed->length, BYTE_TO_BINARY((compressed->data[compressed->length] & 0xFF000000) >> 24), BYTE_TO_BINARY((compressed->data[compressed->length] & 0x00FF0000) >> 16), BYTE_TO_BINARY((compressed->data[compressed->length] & 0x0000FF00) >> 8), BYTE_TO_BINARY(compressed->data[compressed->length] & 0x000000FF));
                        compressed->length += 1;
                        bit_index = (bit_index + head->code_length) % 32;
                        compressed->data[compressed->length] = head->code >> (head->code_length - bit_index);
                        fprintf(stderr, "compressed->data[%d] = 0b%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C___%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C\r\n", compressed->length, BYTE_TO_BINARY((compressed->data[compressed->length] & 0xFF000000) >> 24), BYTE_TO_BINARY((compressed->data[compressed->length] & 0x00FF0000) >> 16), BYTE_TO_BINARY((compressed->data[compressed->length] & 0x0000FF00) >> 8), BYTE_TO_BINARY(compressed->data[compressed->length] & 0x000000FF));
                    }
                    else
                    {
                        // call again with longer output buffer
                        fprintf(stderr, "panic!!\r\n");
                        break;
                    }
                }
                else
                {
                    compressed->data[compressed->length] = (compressed->data[compressed->length] << head->code_length) + head->code; // need to figure this out later
                    bit_index += head->code_length;
                    fprintf(stderr, "compressed->data[%d] = 0b%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C___%C%C%C%C_%C%C%C%C__%C%C%C%C_%C%C%C%C\r\n", compressed->length, BYTE_TO_BINARY((compressed->data[compressed->length] & 0xFF000000) >> 24), BYTE_TO_BINARY((compressed->data[compressed->length] & 0x00FF0000) >> 16), BYTE_TO_BINARY((compressed->data[compressed->length] & 0x0000FF00) >> 8), BYTE_TO_BINARY(compressed->data[compressed->length] & 0x000000FF));
                }
                head = first_node;
                break;
            }
            else
            {
                if (head->child0 != NULL)
                {
                    parent = head;
                    head = head->child0;
                }
                else
                {
                    if (parent->child1 != NULL)
                        head = parent->child1;
                    else
                    {
                        head = first_node;
                        fprintf(stderr, "no_match_found!!\r\n");
                        break;
                    }
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

huffman_buffer *huffman_decompress(huffman_buffer *compressed, huffman_tree *tree)
{
    huffman_buffer *decompressed = (huffman_buffer *)malloc(sizeof(huffman_buffer));
    if (decompressed == NULL)
        return NULL;

    decompressed->data = (huffman_data *)malloc(sizeof(huffman_data) * compressed->length);

    return decompressed;
}

static huffman_buffer *_huffman_build_decompressed_buffer(huffman_buffer *uncompressed)
{
    return uncompressed; // FIXME: need to complete
}

static void _huffman_print_tree(huffman_tree *tree)
{
    for (huffman_length n = 0; n < tree->size; n++)
    {
        fprintf(stderr, "tree->nodes[%d] = %C\r\n", n, tree->nodes[n]->value);
    }
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

static void _print_huffman_tree(huffman_tree *tree)
{
    // for (uint32_t n = 0; n < tree->size; n++)
    // {
    //     fprintf(stderr, "tree->nodes[%d] = %C\r\n", n, tree->nodes[n]->value);
    //     _print_node(tree->nodes[n]);
    // }
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