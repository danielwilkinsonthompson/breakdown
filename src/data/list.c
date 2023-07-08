/*=============================================================================
                                    list.c
-------------------------------------------------------------------------------
linked list

  filo (stack) : use push & pop
  fifo (queue) : use append & pop

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "list.h"

static void _printf_list_element(list_element *element);

list *list_init(void)
{
    list *new_list;

    new_list = (list *)malloc(sizeof(list));
    if (new_list == NULL)
        return NULL;

    new_list->size = 0;
    new_list->head = NULL;
    new_list->tail = NULL;

    return new_list;
}

error list_append(list *l, void *value)
{
    list_element *current_tail;
    list_element *new_tail;

    new_tail = (list_element *)malloc(sizeof(list_element));
    if (new_tail == NULL)
        return memory_error;

    if (l->size == 0)
    {
        l->tail = new_tail;
        l->head = new_tail;
        l->tail->prev = NULL;
    }
    else
    {
        current_tail = l->tail;
        current_tail->next = new_tail;
        l->tail = new_tail;
        l->tail->prev = current_tail;
    }
    l->tail->value = value;
    l->tail->next = NULL;
    l->size += 1;

    return success;
}

error list_push(list *l, void *value)
{
    list_element *current_head;
    list_element *new_head;

    new_head = (list_element *)malloc(sizeof(list_element));
    if (new_head == NULL)
        return memory_error;

    if (l->size == 0)
    {
        l->tail = new_head;
        l->head = new_head;
        l->head->next = NULL;
    }
    else
    {
        current_head = l->head;
        current_head->prev = new_head;
        l->head = new_head;
        l->head->next = current_head;
    }
    l->head->value = value;
    l->head->prev = NULL;
    l->size += 1;

    return success;
}

void *list_pop(list *l)
{
    list_element *object;

    if list_invalid (l)
        return NULL;

    object = l->head;
    l->size -= 1;

    if (l->size == 0)
        l->head = NULL;
    else
    {
        l->head = l->head->next;
        l->head->prev = NULL;
    }

    return object->value;
}

static void _printf_list_element(list_element *element)
{
    fprintf(stderr, "element = %p\r\n", element);
    fprintf(stderr, "\t->value = %C\r\n", *((char *)element->value));
    fprintf(stderr, "\t->next = %p\r\n", element->next);
    fprintf(stderr, "\t->prev = %p\r\n", element->prev);
}

void list_print(list *l)
{
    fprintf(stderr, "list = %p\r\n", l);
    fprintf(stderr, "list->head = %p\r\n", l->head);
    fprintf(stderr, "list->tail = %p\r\n", l->tail);
    fprintf(stderr, "list->size = %zu\r\n", l->size);
}

error list_remove(list *l, size_t index)
{
    list_element *this_element;
    size_t this_index = 0;

    if list_invalid (l)
        return value_error;

    this_element = l->head;
    for (size_t i = 0; i < index; i++)
    {
        this_element = this_element->next;
    }

    if (this_element->next != NULL)
        this_element->next->prev = this_element->prev;
    if (this_element->next != NULL)
        this_element->prev->next = this_element->next;

    free(this_element);

    return success;
}

void *list_item(list *l, size_t index)
{
    list_element *this_element;
    size_t this_index = 0;

    if list_invalid (l)
        return NULL;

    this_element = l->head;
    for (size_t i = 0; i < index; i++)
    {
        this_element = this_element->next;
    }

    return this_element->value;
}

void list_free(list *l)
{
    list_element *this_element;
    list_element *next_element;

    if list_invalid (l)
        return;

    this_element = l->head;
    while (this_element != NULL)
    {
        next_element = this_element->next;
        free(this_element);
        this_element = next_element;
    }
    free(l);
}