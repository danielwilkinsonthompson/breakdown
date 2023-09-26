/*=============================================================================
                                  gui.h
-------------------------------------------------------------------------------
graphical user interface elements

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

NOTE: THE GUI FUNCTIONS SHOULD ALMOST NEVER BE CALLED DIRECTLY. INSTEAD, USE
      THE DRAW FUNCTIONS IN DRAW.H
-----------------------------------------------------------------------------*/
#ifndef __gui_h
#define __gui_h
#include <stdint.h> // type definitions
#include "image.h"  // image

typedef struct gui_element_t gui_element;
typedef struct layer_t layer;
typedef void (*gui_element_draw_callback)(gui_element *this_element);

typedef enum gui_element_type_t
{
  gui_pixel,
  gui_line,
  gui_image,
  // gui_text,
  gui_rectangle,
  // gui_ellipse,
  gui_polyline,
  // gui_polygon,
  gui_curve,
  // gui_sprite,
  // gui_textbox,
  // gui_button,
  // gui_slider,
  // gui_checkbox,
  // gui_radiobutton,
  // gui_dropdown,
  // gui_inputbox,
  // gui_scrollbar,
  // gui_scrollbox,
  // gui_listbox,
  // gui_canvas,
  // gui_plot,
  // gui_table,
  // gui_menu,
  // gui_window,
  // gui_dialog,
  // gui_popup,
  // gui_tooltip,
  // gui_cursor,
  // gui_keyboard,
  // gui_unspecified
} gui_element_type;

// FIXME: x and y should be signed, but we're using unsigned for now
typedef struct coordinates_t
{
  uint32_t x;
  uint32_t y;
  uint32_t z; // should z index be included?
  uint32_t width;
  uint32_t height;
  uint32_t angle; // should this be a float?
} coordinates;

typedef struct gui_element_data_t
{
  image_pixel colour; // line colour
  image_pixel fill;
  uint32_t weight; // line weight
  char *text;
  image *img;
  coordinates *position; // should this be a pointer? that would allow coordinate arrays for polyline and polygon
  uint32_t num_points;
} gui_element_data;

typedef struct gui_element_t
{
  gui_element_type type;
  gui_element_data *data;
  gui_element_draw_callback draw;
  layer *parent;
} gui_element;

/*-----------------------------------------------------------------------------
  gui_element_init
    creates a new gui element
-----------------------------------------------------------------------------*/
gui_element *gui_element_init(layer *parent, gui_element_type type, coordinates position);

/*-----------------------------------------------------------------------------
  gui_element_print
    print details of a gui element
-----------------------------------------------------------------------------*/
void gui_element_print(gui_element *this_element);

/*-----------------------------------------------------------------------------
  gui_element_free
    free a gui element
-----------------------------------------------------------------------------*/
void gui_element_free(gui_element *this_element);

#endif // __gui_h