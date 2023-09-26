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

/*----------------------------------------------------------------------------
  draw_pixel
    draws a pixel at (x, y) with colour
-----------------------------------------------------------------------------*/
gui_element *draw_pixel(layer *this_layer, int32_t x, int32_t y, image_pixel colour)
{
  gui_element *this_element = gui_element_init(this_layer, gui_pixel, (coordinates){x, y, 0, 1, 1});
  if (this_element == NULL)
    return NULL;
  this_element->data->colour = colour;

  return this_element;

  return NULL;
}
/*----------------------------------------------------------------------------
  draw_line
    draws a line between two points
-----------------------------------------------------------------------------*/
gui_element *draw_line(layer *this_layer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, image_pixel colour)
{
  gui_element *this_element = gui_element_init(this_layer, gui_line, (coordinates){x1, y1, 0, x2, y2});
  if (this_element == NULL)
    goto memory_error;
  this_element->data->colour = colour;

  return this_element;

memory_error:
#if defined(DEBUG)
  printf("draw_line: could not initialise line\r\n");
#endif // DEBUG
  return NULL;
}

/*----------------------------------------------------------------------------
  draw_polyline
    draws a polyline between a series of points
-----------------------------------------------------------------------------*/
gui_element *draw_polyline(layer *this_layer, int32_t *x, int32_t *y, uint32_t num_points, uint32_t colour)
{
  // need to figure out how to allocate an array of coordinates
  coordinates *positions = (coordinates *)malloc(num_points * sizeof(coordinates));
  for (uint32_t i = 0; i < num_points; i++)
  {
    // draw_line(this_layer, x[i], y[i], x[i + 1], y[i + 1], colour);
    positions[i].x = x[i];
    positions[i].y = y[i];
  }
  gui_element *this_element = gui_element_init(this_layer, gui_polyline, *positions);
  if (this_element == NULL)
    goto memory_error;
  free(this_element->data->position);
  this_element->data->position = positions;
  this_element->data->num_points = num_points;
  this_element->data->colour = colour;

  return this_element;

memory_error:
#if defined(DEBUG)
  printf("draw_polyline: could not initialise polyline\r\n");
#endif // DEBUG
  return NULL;
}

/*----------------------------------------------------------------------------
  draw_circle
    draws a circle with top-left at (x, y) with radius r
-----------------------------------------------------------------------------*/
gui_element *draw_circle(layer *this_layer, int32_t x, int32_t y, int32_t radius, uint32_t colour)
{
  // generalise to ellipse

  return NULL;
}

/*----------------------------------------------------------------------------
  draw_rectangle
    draws a rectangle with top-left corner at (x, y) with width and height
-----------------------------------------------------------------------------*/
gui_element *draw_rectangle(layer *this_layer, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t colour)
{
  gui_element *this_element = gui_element_init(this_layer, gui_rectangle, (coordinates){x, y, 0, width, height});
  if (this_element == NULL)
    goto memory_error;
  this_element->data->colour = colour;

  return this_element;

memory_error:
#if defined(DEBUG)
  printf("draw_rectangle: could not initialise rectangle\r\n");
#endif // DEBUG
  return NULL;
}

/*----------------------------------------------------------------------------
  draw_curve
    draws a bezier curve (quadratic) between three points
-----------------------------------------------------------------------------*/
gui_element *draw_curve(layer *this_layer, int32_t *x, int32_t *y, uint32_t colour)
{
  // FIXME: this interface is a little dangerous -- assumes x and y are both int32_t[3] arrays
  coordinates *positions = (coordinates *)malloc(3 * sizeof(coordinates));
  for (uint32_t i = 0; i < 3; i++)
  {
    positions[i].x = x[i];
    positions[i].y = y[i];
  }
  gui_element *this_element = gui_element_init(this_layer, gui_curve, *positions);
  if (this_element == NULL)
    goto memory_error;
  free(this_element->data->position);
  this_element->data->position = positions;
  this_element->data->num_points = 3;
  this_element->data->colour = colour;

  return this_element;

memory_error:
#if defined(DEBUG)
  printf("draw_curve: could not initialise curve\r\n");
#endif // DEBUG
  return NULL;
}

/*----------------------------------------------------------------------------
  draw_text
    draws text with top-left corner at (x, y)
-----------------------------------------------------------------------------*/
gui_element *draw_text(layer *this_layer, int32_t x, int32_t y, const char *text, uint32_t colour)
{
  // TODO: draw text

  return NULL;
}

/*----------------------------------------------------------------------------
  draw_image
    draws a gui_element of type gui_image
-----------------------------------------------------------------------------*/
gui_element *draw_image(layer *this_layer, int32_t x, int32_t y, uint32_t width, uint32_t height, image *img)
{
  // we have frame width and height, layer width and height, gui_image width and height, and image width and height
  // can we consolidate this somehow? seems excessive
  gui_element *this_element = gui_element_init(this_layer, gui_image, (coordinates){x, y, 0, width, height});
  if (this_element == NULL)
    goto memory_error;
  this_element->data->img = img;

  return this_element;

memory_error:
#if defined(DEBUG)
  printf("draw_image: could not initialise image\r\n");
#endif // DEBUG
  return NULL;
}