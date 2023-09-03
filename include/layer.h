/*=============================================================================
                                  layer.h
-------------------------------------------------------------------------------
graphics layer

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
-----------------------------------------------------------------------------*/
#ifndef __layer_h
#define __layer_h
#include <stdint.h> // type definitions
#include "frame.h"  // frame
#include "gui.h"    // ui_element
#include "image.h"  // image

typedef struct layer_t layer;
typedef struct frame_t frame;
typedef void (*_layer_draw_callback)(layer *this_layer);

typedef enum
{
  layer_hidden,
  layer_needs_rendering,
  layer_done_rendering,
} layer_draw_state;

typedef struct layer_t
{
  uint32_t no_elements;
  gui_element **gui_elements;
  coordinates position;
  image *render;
  _layer_draw_callback draw;
  layer_draw_state redraw;
  frame *parent;
} layer;

/*-----------------------------------------------------------------------------
  layer_init
    creates a new layer
-----------------------------------------------------------------------------*/
layer *layer_init(frame *parent);

/*-----------------------------------------------------------------------------
  layer_draw
    draw/redraw a layer
-----------------------------------------------------------------------------*/
void layer_draw(layer *this_layer);

/*-----------------------------------------------------------------------------
  layer_add_ui_element
    add a gui element to the layer
-----------------------------------------------------------------------------*/
void layer_add_gui_element(layer *this_layer, gui_element *this_element);

/*-----------------------------------------------------------------------------
  layer_remove_ui_element
    remove a gui element from the layer
-----------------------------------------------------------------------------*/
void layer_remove_gui_element(gui_element *this_element);

/*-----------------------------------------------------------------------------
  layer_free
    free a layer
-----------------------------------------------------------------------------*/
void layer_free(layer *this_layer);

/*-----------------------------------------------------------------------------
  layer_print
    print a layer
-----------------------------------------------------------------------------*/
void layer_print(layer *this_layer);

#endif // __layer_h