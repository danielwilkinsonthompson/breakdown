
/*=============================================================================
                                dictionary.h
-------------------------------------------------------------------------------
dictionary data structure (hashtables)

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://www.digitalocean.com/community/tutorials/hash-table-in-c-plus-plus#
-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"
#include "dictionary.h"

static uint32_t _hash_function(char *key, size_t size);

// function for hashing strings
// returns a hash value
static uint32_t _hash_function(char *key, size_t size)
{
    uint32_t hash = 0;
    for (int i = 0; key[i] != '\0'; i++)
    {
        hash += key[i];
    }
    return hash % size;
}

// function for creating hashtables
// returns a pointer to the hashtable
// or NULL if error
dictionary *dictionary_new(size_t size)
{
    dictionary *d = malloc(sizeof(dictionary));
    if (d == NULL)
    {
        return NULL;
    }
    d->size = size;
    d->count = 0;
    d->items = calloc((size_t)d->size, sizeof(dictionary_item *));
    if (d->items == NULL)
    {
        return NULL;
    }
    return d;
}

void dictionary_insert(dictionary *d, char *key, char *value)
{
    dictionary_item *item = malloc(sizeof(dictionary_item));
    if (item == NULL)
    {
        return;
    }
    item->key = key;
    item->value = value;

    int index = _hash_function(key, d->size);
    dictionary_item *current_item = d->items[index];

    if (current_item == NULL)
    {
        if (d->count == d->size)
        {
            printf("Insert Error: Hash Table is full\n");
            return;
        }
        d->items[index] = item;
        d->count++;
    }
    else
    {
        if (strcmp(current_item->key, key) == 0)
        {
            printf("Insert Error: Duplicate key\n");
            return;
        }
        while (current_item->next != NULL)
        {
            current_item = current_item->next;
            if (strcmp(current_item->key, key) == 0)
            {
                printf("Insert Error: Duplicate key\n");
                return;
            }
        }
        current_item->next = item;
    }
}

char *dictionary_find(dictionary *d, char *key)
{
    int index = _hash_function(key, d->size);
    dictionary_item *item = d->items[index];
    if (item == NULL)
    {
        return NULL;
    }
    while (item != NULL)
    {
        if (strcmp(item->key, key) == 0)
        {
            return item->value;
        }
        item = item->next;
    }
    return NULL;
}

void dictionary_remove(dictionary *d, char *key)
{
    int index = _hash_function(key, d->size);
    dictionary_item *item = d->items[index];
    if (item == NULL)
    {
        return;
    }
    dictionary_item *prev_item = NULL;
    while (item->next != NULL && strcmp(item->key, key) != 0)
    {
        prev_item = item;
        item = item->next;
    }
    if (prev_item == NULL)
    {
        d->items[index] = item->next;
    }
    else
    {
        prev_item->next = item->next;
    }
    free(item);
    d->count--;
}

// function for deleting hashtables
// returns nothing
void dictionary_delete(dictionary *d)
{
    for (int i = 0; i < d->size; i++)
    {
        dictionary_item *item = d->items[i];
        if (item != NULL)
        {
            free(item->key);
            free(item->value);
            free(item);
        }
    }
    free(d->items);
    free(d);
}

void dictionary_print(dictionary *d)
{
    for (int i = 0; i < d->size; i++)
    {
        dictionary_item *item = d->items[i];
        if (item != NULL)
        {
            printf("%s: %s\n", item->key, item->value);
        }
    }
}