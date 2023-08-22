/*=============================================================================
                                 plot.h
-------------------------------------------------------------------------------
simple x-y line plots

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#ifndef __plot_h
#define __plot_h
#include "frame.h"

typedef struct plot_trace_t
{
    int32_t *x;
    int32_t *y;
    int32_t length;
    const char *label;
    uint32_t colour;
} plot_trace;

typedef struct axis_t
{
    int32_t min;
    int32_t max;
    const char *label;
} axis;

typedef struct plot_t
{
    plot_trace *trace;
    int32_t trace_count;
    axis x_axis;
    axis y_axis;
    const char *title;
    frame *window;
} plot;

/*----------------------------------------------------------------------------
  init
    creates a new plot
-----------------------------------------------------------------------------*/
plot *plot_init(const char *title, const char *x_label, const char *y_label);

/*----------------------------------------------------------------------------
  draw
    renders the plot
-----------------------------------------------------------------------------*/
void plot_redraw(plot *plt);

/*----------------------------------------------------------------------------
  add_trace
    adds a trace to the plot
-----------------------------------------------------------------------------*/
void plot_add_trace(plot *plt, int32_t *x, int32_t *y, int32_t length, const char *label, uint32_t colour);

/*----------------------------------------------------------------------------
  plot_show
    displays the plot in a window
-----------------------------------------------------------------------------*/
void plot_show(plot *plt);

#endif // __plot_h