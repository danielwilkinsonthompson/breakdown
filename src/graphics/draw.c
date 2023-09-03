/*=============================================================================
                                 draw.c
-------------------------------------------------------------------------------
simple drawing routines

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

Notes:
- Are we better off drawing these primitives into an SVG, and then rendering the
  SVG into a frame? Either way, we need routines render the SVG into a frame.
-----------------------------------------------------------------------------*/
#include <stdlib.h> // abs
#include <stdint.h> // type definitions
#include "draw.h"   // draw
#include "gui.h"    // gui_element
#include "frame.h"  // frame
#include "image.h"  // image
#include "layer.h"  // layer

static image_pixel *pixel_at(layer *this_layer, int32_t x, int32_t y);

/*----------------------------------------------------------------------------
  pixel_at
    returns a pointer to the pixel at (x, y)
-----------------------------------------------------------------------------*/
static image_pixel *pixel_at(layer *this_layer, int32_t x, int32_t y)
{
  return this_layer->render->pixel_data + (y * this_layer->render->width) + x;
}

/*----------------------------------------------------------------------------
  draw_pixel
    draws a pixel at (x, y) with colour
-----------------------------------------------------------------------------*/
void draw_pixel(layer *this_layer, int32_t x, int32_t y, image_pixel colour)
{
  gui_element *this_element = gui_element_init(this_layer, gui_pixel, (coordinates){x, y, 0, 1, 1});
  this_element->data->colour = colour;
}
/*----------------------------------------------------------------------------
  draw_line
    draws a line between two points
-----------------------------------------------------------------------------*/
void draw_line(layer *this_layer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, image_pixel colour)
{
  gui_element *this_element = gui_element_init(this_layer, gui_line, (coordinates){x1, y1, 0, x2, y2});
  this_element->data->colour = colour;
}

/*----------------------------------------------------------------------------
  draw_polyline
    draws a polyline between a series of points
-----------------------------------------------------------------------------*/
void draw_polyline(layer *this_layer, int32_t *x, int32_t *y, uint32_t num_points, uint32_t colour)
{
  gui_element *this_element = gui_element_init(this_layer, gui_polyline, (coordinates){x[0], y[0], 0, x[num_points - 1], y[num_points - 1]});
  for (uint32_t i = 0; i < num_points - 1; i++)
  {
    draw_line(this_layer, x[i], y[i], x[i + 1], y[i + 1], colour);
  }
}

/*----------------------------------------------------------------------------
  draw_circle
    draws a circle with top-left at (x, y) with radius r
-----------------------------------------------------------------------------*/
void draw_circle(layer *this_layer, int32_t x, int32_t y, int32_t radius, uint32_t colour)
{
  // generalise to ellipse
}

/*----------------------------------------------------------------------------
  draw_rectangle
    draws a rectangle with top-left corner at (x, y) with width and height
-----------------------------------------------------------------------------*/
void draw_rectangle(layer *this_layer, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t colour)
{
  // for (int32_t i = x; i < x + width; i++)
  // {
  //   for (int32_t j = y; j < y + height; j++)
  //   {
  //     draw_pixel(this_layer, i, j, colour);
  //   }
  // }
  gui_element *this_element = gui_element_init(this_layer, gui_rectangle, (coordinates){x, y, 0, width, height});
  this_element->data->colour = colour;
}

/*----------------------------------------------------------------------------
  draw_text
    draws text with top-left corner at (x, y)
-----------------------------------------------------------------------------*/
void draw_text(layer *this_layer, int32_t x, int32_t y, const char *text, uint32_t colour)
{
  // TODO: draw text
}

/*----------------------------------------------------------------------------
  draw_image
    draws a gui_element of type gui_image
-----------------------------------------------------------------------------*/
void draw_image(layer *this_layer, int32_t x, int32_t y, uint32_t width, uint32_t height, image *img)
{
  // we have frame width and height, layer width and height, gui_image width and height, and image width and height
  // can we consolidate this somehow? seems excessive
  gui_element *this_element = gui_element_init(this_layer, gui_image, (coordinates){x, y, 0, width, height});
  this_element->data->img = img;
}