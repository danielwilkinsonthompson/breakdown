/*=============================================================================
                                  layer.c
-------------------------------------------------------------------------------
graphics layer

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#include <stdint.h> // type definitions
#include <stdio.h>  // printf
#include <stdlib.h> // malloc, free
#include "frame.h"  // frame
#include "layer.h"  // layer

static void layer_clear(layer *this_layer);

// FIXME: this should be called by frame_add_layer, not the other way around
layer *layer_init(frame *parent)
{
    layer *this_layer = malloc(sizeof(layer));
    if (this_layer == NULL)
        goto memory_error;
    this_layer->render = image_init(parent->height, parent->width);
    if (this_layer->render == NULL)
        goto memory_error;
    this_layer->gui_elements = NULL;
    this_layer->parent = parent;
    this_layer->no_elements = 0;
    this_layer->position.z = this_layer->parent->layer_count;
    this_layer->position.x = 0;
    this_layer->position.y = 0;
    this_layer->position.width = parent->width; // tracking twice :(
    this_layer->position.height = parent->height;
    this_layer->draw = layer_draw;
    this_layer->redraw = layer_needs_rendering;
    // frame_add_layer(this_layer->parent, this_layer);

    return this_layer;

memory_error:
#if defined(DEBUG)
    printf("layer_init: memory allocation failed.\n");
#endif // DEBUG
    return NULL;
}

void layer_add_gui_element(layer *this_layer, gui_element *this_element)
{
    this_layer->gui_elements = realloc(this_layer->gui_elements, sizeof(gui_element *) * (this_layer->no_elements + 1));
    if (this_layer->gui_elements == NULL)
        goto memory_error;

    this_layer->gui_elements[this_layer->no_elements] = this_element;
    this_layer->no_elements++;
    this_element->parent = this_layer;
    if (this_layer->redraw != layer_hidden)
        this_layer->redraw = layer_needs_rendering;

    return;

memory_error:
#if defined(DEBUG)
    printf("layer_add_ui_element: memory allocation failed.\n");
#endif // DEBUG
    return;
}

void layer_remove_gui_element(gui_element *this_element)
{
    if (this_element->parent == NULL)
        return;

    layer *this_layer = this_element->parent;
    for (uint32_t el = 0; el < this_layer->no_elements; el++)
    {
        if (this_layer->gui_elements[el] == this_element)
        {
            gui_element_free(this_layer->gui_elements[el]);
            this_layer->no_elements--;
            for (uint32_t i = el; i < this_layer->no_elements; i++)
                this_layer->gui_elements[i] = this_layer->gui_elements[i + 1];
            this_layer->gui_elements = realloc(this_layer->gui_elements, sizeof(gui_element *) * this_layer->no_elements);
            if (this_layer->redraw == layer_hidden)
                this_layer->redraw = layer_needs_rendering;
            return;
        }
    }
}

void layer_draw(layer *this_layer)
{
#if defined(DEBUG)
    printf("layer_draw: drawing layer @ %p\r\n", this_layer);
#endif // DEBUG
    if (this_layer->redraw == layer_hidden)
        return;
    else
    {
        if (this_layer->render != NULL)
            image_free(this_layer->render);
        this_layer->render = image_init(this_layer->parent->height, this_layer->parent->width);
        if (this_layer->render == NULL)
            goto memory_error;
    }

    for (int i = 0; i < this_layer->no_elements; i++)
    {
        this_layer->gui_elements[i]->draw(this_layer->gui_elements[i]);
    }
    this_layer->redraw = layer_done_rendering;

    return;
memory_error:
#if defined(DEBUG)
    printf("layer_draw: memory allocation failed.\n");
#endif // DEBUG
    return;
}

void layer_free(layer *this_layer)
{
#if defined(DEBUG)
    printf("layer_free: freeing layer @ %p\r\n", this_layer);
#endif // DEBUG
    for (uint32_t el = 0; el < this_layer->no_elements; el++)
        gui_element_free(this_layer->gui_elements[el]);

    image_free(this_layer->render);
    free(this_layer);
}

void layer_print(layer *this_layer)
{
    printf("layer @ %p\r\n", this_layer);
    printf("->parent: frame @ %p\r\n", this_layer->parent);
    printf("->redraw: %d\r\n", this_layer->redraw);
    printf("->draw: function @ %p\r\n", this_layer->draw);
    printf("->render: image @ %p\r\n", this_layer->render);
    printf("->position:\r\n");
    printf("{ .x: %u ", this_layer->position.x);
    printf("  .y: %u ", this_layer->position.y);
    printf("  .z: %u ", this_layer->position.z);
    printf("  .width: %u ", this_layer->position.width);
    printf("  .height: %u}\r\n", this_layer->position.height);
    printf("->no_elements: %u\r\n", this_layer->no_elements);
    printf("->gui_elements:\r\n");
    for (uint32_t el = 0; el < this_layer->no_elements; el++)
    {
        printf("  gui_elements[%d]: gui_element @ %p\r\n", el, this_layer->gui_elements[el]);
        gui_element_print(this_layer->gui_elements[el]);
    }
}

static void layer_clear(layer *this_layer)
{
    for (uint32_t y = 0; y < this_layer->render->height; y++)
        for (uint32_t x = 0; x < this_layer->render->width; x++)
            *(this_layer->render->pixel_data + (y * this_layer->render->width) + x) = 0x00000000;
}