#include <stdio.h>
#include <stdlib.h>
#include "draw.h"
#include "gui.h"
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
    p->x_axis.min = 999;
    p->x_axis.max = -999;
    p->y_axis.min = 999;
    p->y_axis.max = -999;

    p->window = frame_init_with_options(600, 400, title, false, false, true, false, 2.0);
    layer *axis = frame_add_layer(p->window);
    axis->position.z = plot_axis_layer;

    gui_element *background = draw_rectangle(axis, 0, 0, axis->position.width, axis->position.height, 0xFFFFFFFF);
    background->data->fill = image_argb(255, 255, 255, 255);

    gui_element *box = draw_rectangle(axis, 100, 75, axis->position.width - 150, axis->position.height - 175, 0xFF000000);

    layer *trace = frame_add_layer(p->window);
    trace->position.z = plot_trace_layer;

    return p;

memory_error:
    fprintf(stderr, "Error: could not allocate memory for plot\n");
    return NULL;
}

void plot_redraw(plot *plt)
{
    axis x, y;

    // compute xmin, xmax, ymin, ymax
    if (plt->x_axis.min > plt->x_axis.max)
    {
        // no explicit min/max set, need to compute
        for (uint32_t trace_counter = 0; trace_counter < plt->trace_count; trace_counter++)
        {
            for (uint32_t pt = 0; pt < plt->trace[trace_counter].length; pt++)
            {
                if (plt->trace[trace_counter].x[pt] < x.min)
                    x.min = plt->trace[trace_counter].x[pt];
                if (plt->trace[trace_counter].x[pt] > x.max)
                    x.max = plt->trace[trace_counter].x[pt];
            }
        }
    }

memory_error:
    fprintf(stderr, "Error: could not allocate memory for plot\n");
}

void plot_show(plot *plt)
{
}

plot_trace *plot_add_trace(plot *plt, int32_t *x, int32_t *y, int32_t length, const char *label, uint32_t colour)
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
    gui_element *polyline = draw_polyline(this_layer, trace->x, trace->y, trace->length, trace->colour);
    if (polyline == NULL)
        goto memory_error;
    printf("Added polyline @ %p\r\n", polyline);

    return trace;

memory_error:
#if defined(DEBUG)
    fprintf(stderr, "plot_add_trace: could not allocate memory for plot trace\n");
#endif // DEBUG
    return NULL;
}
