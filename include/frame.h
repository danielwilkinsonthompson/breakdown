/* =============================================================================
                                  frame.h
-------------------------------------------------------------------------------
frame compositor

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
 - https://github.com/danielwilkinsonthompson/breakdown
 - https://github.com/emoon/minifb
-----------------------------------------------------------------------------*/
#ifndef __frame_h
#define __frame_h
#include <stdint.h> // type definitions
#include <stdbool.h>
#include "layer.h"
#include "error.h"

typedef struct layer_t layer;
typedef void (*_layer_draw_callback)(layer *l);
typedef struct window_t window;

typedef struct frame_t
{
  uint32_t width;
  uint32_t height;
  uint32_t *buffer;
  layer **layers;
  uint32_t layer_count;
  bool needs_redraw;
  void *window;
  float scaling;
} frame;

/*----------------------------------------------------------------------------
  frame_init
  ----------------------------------------------------------------------------
  initialise frame compositor
  width  : width of frame
  height : height of frame
-----------------------------------------------------------------------------*/
frame *frame_init(uint32_t width, uint32_t height, const char *title);
frame *frame_init_with_options(uint32_t width, uint32_t height, const char *title, bool fullscreen, bool vsync, bool resizable, bool borderless, float scaling);

/*----------------------------------------------------------------------------
  frame_add_layer
  ----------------------------------------------------------------------------
  add a layer to the frame
  frame  : frame to add layer to
  z      : z position of layer
-----------------------------------------------------------------------------*/
layer *frame_add_layer(frame *frame, uint32_t z);

/*----------------------------------------------------------------------------
  frame_clear
  ----------------------------------------------------------------------------
  clear frame
  frame  : frame to clear
-----------------------------------------------------------------------------*/
void frame_clear(frame *frame);

/*----------------------------------------------------------------------------
  frame_draw
  ----------------------------------------------------------------------------
  draw frame
  frame  : frame to draw
-----------------------------------------------------------------------------*/
error frame_draw(frame *frame);

/*----------------------------------------------------------------------------
  frame_resize
  ----------------------------------------------------------------------------
  resize frame
  frame  : frame to destroy
-----------------------------------------------------------------------------*/
error frame_resize(frame *f, uint32_t width, uint32_t height);

/*----------------------------------------------------------------------------
  frame_destroy
  ----------------------------------------------------------------------------
  destroy frame
  frame  : frame to destroy
-----------------------------------------------------------------------------*/
void frame_destroy(frame *frame);

/*----------------------------------------------------------------------------
  frame_msleep
  ----------------------------------------------------------------------------
  sleep current thread
  ms :  number of milliseconds to put thread to sleep
-----------------------------------------------------------------------------*/
void frame_msleep(uint32_t ms);
#endif