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
#ifndef __draw_h
#define __draw_h
#include "frame.h"
#include "image.h"
#include "draw.h"

static image_pixel *pixel_at(image *img, int32_t x, int32_t y);
static void draw_pixel(layer *l, int32_t x, int32_t y, uint32_t colour);

/*----------------------------------------------------------------------------
  pixel_at
    returns a pointer to the pixel at (x, y)
-----------------------------------------------------------------------------*/
static image_pixel *pixel_at(image *img, int32_t x, int32_t y)
{
  return img->pixel_data + (y * img->width) + x;
}

/*----------------------------------------------------------------------------
  draw_pixel
    draws a pixel at (x, y) with colour
-----------------------------------------------------------------------------*/
static void draw_pixel(layer *l, int32_t x, int32_t y, uint32_t colour)
{
  image_pixel *p = pixel_at(l->img, x, y);
  p->r = (colour >> 16) & 0xff;
  p->g = (colour >> 8) & 0xff;
  p->b = colour & 0xff;
  p->a = (colour >> 24) & 0xff;
}

/*----------------------------------------------------------------------------
  draw_line
    draws a line between two points
-----------------------------------------------------------------------------*/
void draw_line(frame *f, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t colour)
{
  layer *l = frame_add_layer(f, 1);
  l->draw_callback = default_layer_draw_callback;

  // Bresenham's line algorithm
  int32_t dx = abs(x2 - x1);
  int32_t dy = abs(y2 - y1);
  int32_t sx = (x1 < x2) ? 1 : -1;
  int32_t sy = (y1 < y2) ? 1 : -1;
  int32_t err = dx - dy;
  for (;;)
  {
    draw_pixel(l, x1, y1, colour);
    if ((x1 == x2) && (y1 == y2))
      break;
    int32_t e2 = 2 * err;
    if (e2 > -dy)
    {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx)
    {
      err += dx;
      y1 += sy;
    }
  }

  // Wu's line algorithm
}

/*----------------------------------------------------------------------------
  draw_polyline
    draws a polyline between a series of points
-----------------------------------------------------------------------------*/
void draw_polyline(frame *f, int32_t *x, int32_t *y, uint32_t num_points, uint32_t colour)
{
  for (uint32_t i = 0; i < num_points - 1; i++)
  {
    draw_line(f, x[i], y[i], x[i + 1], y[i + 1], colour);
  }
}

/*----------------------------------------------------------------------------
  draw_circle
    draws a circle with top-left at (x, y) with radius r
-----------------------------------------------------------------------------*/
void draw_circle(frame *f, int32_t x, int32_t y, int32_t radius, uint32_t colour);

/*----------------------------------------------------------------------------
  draw_rectangle
    draws a rectangle with top-left corner at (x, y) with width and height
-----------------------------------------------------------------------------*/
void draw_rectangle(frame *f, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t colour)
{
  layer *l = frame_add_layer(f, 1);
  l->draw_callback = default_layer_draw_callback;

  for (int32_t i = x; i < x + width; i++)
  {
    for (int32_t j = y; j < y + height; j++)
    {
      draw_pixel(l, i, j, colour);
    }
  }
}

/*----------------------------------------------------------------------------
  draw_text
    draws text with top-left corner at (x, y)
-----------------------------------------------------------------------------*/
void draw_text(frame *f, int32_t x, int32_t y, const char *text, uint32_t colour)
{
  layer *l = frame_add_layer(f, 1);
  l->draw_callback = default_layer_draw_callback;
  // TODO: draw text
}

/*----------------------------------------------------------------------------
  draw_image
    draws an image with top-left corner at (x, y)
-----------------------------------------------------------------------------*/
void draw_image(frame *f, int32_t x, int32_t y, image *img)
{
  layer *l = frame_add_layer(f, 1);
  l->draw_callback = default_layer_draw_callback;
}

#endif // __draw_h