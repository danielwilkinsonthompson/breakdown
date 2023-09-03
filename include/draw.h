/*=============================================================================
                                 draw.h
-------------------------------------------------------------------------------
simple drawing routines

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#ifndef __draw_h
#define __draw_h
#include "frame.h"
#include "image.h"

/*----------------------------------------------------------------------------
  draw_pixel
    draws a pixel at a point
-----------------------------------------------------------------------------*/
void draw_pixel(layer *this_layer, int32_t x, int32_t y, image_pixel colour);

/*----------------------------------------------------------------------------
  draw_line
    draws a line between two points
-----------------------------------------------------------------------------*/
void draw_line(layer *this_layer, int32_t x1, int32_t y1, int32_t x2, int32_t y2, image_pixel colour);

/*----------------------------------------------------------------------------
  draw_polyline
    draws a polyline between a series of points
-----------------------------------------------------------------------------*/
void draw_polyline(layer *this_layer, int32_t *x, int32_t *y, uint32_t num_points, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_circle
    draws a circle with top-left at (x, y) with radius r
-----------------------------------------------------------------------------*/
void draw_circle(layer *this_layer, int32_t x, int32_t y, int32_t radius, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_rectangle
    draws a rectangle with top-left corner at (x, y) with width and height
-----------------------------------------------------------------------------*/
void draw_rectangle(layer *this_layer, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_text
    draws text with top-left corner at (x, y)
-----------------------------------------------------------------------------*/
void draw_text(layer *this_layer, int32_t x, int32_t y, const char *text, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_image
    draws an image img with top-left corner at (x, y) with width and height
-----------------------------------------------------------------------------*/
void draw_image(layer *this_layer, int32_t x, int32_t y, uint32_t width, uint32_t height, image *img);

#endif // __draw_h