/*=============================================================================
                                dictionary.h
-------------------------------------------------------------------------------
dictionary data structure (hashtables)

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://www.digitalocean.com/community/tutorials/hash-table-in-c-plus-plus#
-----------------------------------------------------------------------------*/
#ifndef __dictionary_h
#define __dictionary_h
#include <stdlib.h>

typedef struct dictionary_item_t
{
    char *key;
    char *value;
    struct dictionary_item_t *next;
} dictionary_item;

typedef struct dictionary_t
{
    size_t size;
    size_t count;
    dictionary_item **items;
} dictionary;

dictionary *dictionary_new(size_t size);
void dictionary_delete(dictionary *d);
void dictionary_insert(dictionary *d, char *key, char *value);
char *dictionary_find(dictionary *d, char *key);
void dictionary_remove(dictionary *d, char *key);
void dictionary_print(dictionary *d);

#endif