/*=============================================================================
                                 draw.h
-------------------------------------------------------------------------------
simple drawing routines

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

Notes:
- Are we better off drawing these primitives into an SVG, and then rendering the
  SVG into a frame? Either way, we need routines to render the frame.
-----------------------------------------------------------------------------*/
#ifndef __draw_h
#define __draw_h
#include "frame.h"
#include "image.h"

/*----------------------------------------------------------------------------
  draw_line
    draws a line between two points
-----------------------------------------------------------------------------*/
void draw_line(frame *f, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_polyline
    draws a polyline between a series of points
-----------------------------------------------------------------------------*/
void draw_polyline(frame *f, int32_t *x, int32_t *y, uint32_t num_points, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_circle
    draws a circle with top-left at (x, y) with radius r
-----------------------------------------------------------------------------*/
void draw_circle(frame *f, int32_t x, int32_t y, int32_t radius, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_rectangle
    draws a rectangle with top-left corner at (x, y) with width and height
-----------------------------------------------------------------------------*/
void draw_rectangle(frame *f, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_text
    draws text with top-left corner at (x, y)
-----------------------------------------------------------------------------*/
void draw_text(frame *f, int32_t x, int32_t y, const char *text, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_image
    draws an image with top-left corner at (x, y)
-----------------------------------------------------------------------------*/
void draw_image(frame *f, int32_t x, int32_t y, image *img);

#endif // __draw_h