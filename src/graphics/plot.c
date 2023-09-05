#include <stdio.h>
#include <stdlib.h>
#include "draw.h"
#include "plot.h"

typedef enum plot_layer_t
{
    plot_axis_layer = 0,
    plot_trace_layer = 1
} plot_layer;

plot *plot_init(const char *title, const char *x_label, const char *y_label)
{
    plot *p = (plot *)malloc(sizeof(plot));
    if (p == NULL)
        goto memory_error;
    p->title = title;
    p->x_axis.label = x_label;
    p->y_axis.label = y_label;
    p->trace_count = 0;
    p->trace = NULL;

    p->window = frame_init_with_options(600, 400, title, false, false, false, false, 2.0);
    layer *axis = frame_add_layer(p->window);
    axis->position.z = plot_axis_layer;

    layer *trace = frame_add_layer(p->window);
    trace->position.z = plot_trace_layer;

    return p;

memory_error:
    fprintf(stderr, "Error: could not allocate memory for plot\n");
    return NULL;
}

// void plot_redraw(plot *plt)
// {
//     // for now, just add a new layer to the frame
//     layer *l = frame_add_layer(plt->window, 1);
//     if (l == NULL)
//         goto memory_error;

//     // draw the axes
//     // FIXME: draw_line takes a frame, we are sending a layer
//     draw_line(l, 0, 0, 0, l->position.height, 0x00000000);
//     draw_line(l, 0, l->position.height, l->position.width, l->position.height, 0x00000000);

// memory_error:
//     fprintf(stderr, "Error: could not allocate memory for plot layer\n");
// }

void plot_add_trace(plot *plt, int32_t *x, int32_t *y, int32_t length, const char *label, uint32_t colour)
{
    plot_trace *trace = (plot_trace *)malloc(sizeof(plot_trace));
    if (trace == NULL)
        goto memory_error;

    trace->x = x;
    trace->y = y;
    trace->length = length;
    trace->label = label;
    trace->colour = colour;

    plt->trace_count++;
    plt->trace = (plot_trace *)realloc(plt->trace, sizeof(plot_trace) * plt->trace_count);
    if (plt->trace == NULL)
        goto memory_error;
    plt->trace[plt->trace_count - 1] = *trace;

    layer *this_layer = plt->window->layers[plot_trace_layer];
    draw_polyline(this_layer, trace->x, trace->y, trace->length, trace->colour);

    return;

memory_error:
#if defined(DEBUG)
    fprintf(stderr, "plot_add_trace: could not allocate memory for plot trace\n");
#endif // DEBUG
}
