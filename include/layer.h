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
#include <stdbool.h>
#include "image.h"
#include "error.h"

typedef struct layer_t layer;
typedef void (*_layer_draw_callback)(layer *l);
typedef void (*_ui_element_draw_callback)(layer *l, ui_object *obj);

typedef enum ui_element_type_t
{
    ui_image,
    ui_text,
    ui_rectangle,
    ui_ellipse,
    ui_line,
    ui_polyline,
    ui_polygon,
    ui_sprite,
    ui_textbox,
    ui_button,
    ui_slider,
    ui_checkbox,
    ui_radiobutton,
    ui_dropdown,
    ui_inputbox,
    ui_scrollbar,
    ui_scrollbox,
    ui_listbox,
    ui_canvas,
    ui_plot,
    ui_table,
    ui_menu,
    ui_window,
    ui_dialog,
    ui_popup,
    ui_tooltip,
    ui_cursor,
    ui_keyboard,
    ui_unspecified
} ui_element_type;

typedef struct ui_object_t
{
    ui_element_type type;
    void *object_data;
    uint32_t x;  // left edge relative to layer
    uint32_t y;  // top edge relative to layer
    uint32_t z;  // object depth within layer
    image *raster;
    _ui_element_draw_callback draw;
    bool needs_redraw;
} ui_object;

typedef struct layer_t
{
    image *render;
    uint32_t nobjects;
    ui_object **objects;
    uint32_t x; // left edge relative to frame
    uint32_t y; // top edge relative to frame
    uint32_t z; // layer depth within frame
    _layer_draw_callback draw;
    bool needs_redraw;
} layer;

#endif // __layer_h