/* =============================================================================
                                  frame.c
-------------------------------------------------------------------------------
frame compositor

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:
 - https://github.com/danielwilkinsonthompson/breakdown
 - https://github.com/emoon/minifb
 - https://en.wikipedia.org/wiki/Alpha_compositing
-----------------------------------------------------------------------------*/

/*
TODO:
- frame_draw doesn't composite alpha correctly
- Separate frame_width, frame_height from pixel_width, pixel_height (essentially, set dpi)
- create frame(width, height) // fullscreen vs windowed
- close frame
- redraw frame
- resize frame
- set refresh rate
- set/get dpi
- events
-   active window
-   resize window
-   close window
-   mouse button
-   mouse move
-   mouse scroll
-   redraw
- get mouse pos
- get keystroke
-
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "error.h"
#include "image.h"
#include "frame.h"

// Use minifb for these platforms
#if defined(__APPLE__) || defined(_WIN32) || defined(__ANDROID__) || defined(__wasm__) || defined(__linux__)
#include "MiniFB.h"
typedef struct mfb_window window_t;
#define MINIFB_IMPLEMENTATION
#else
#error "Current platform is not supported by frame.c"
#endif

#if defined(__linux_) || defined(__APPLE__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

void default_layer_draw_callback(layer *l);

static void _get_draw_order(frame *f)
{
    for (uint32_t i = 0; i < f->layer_count; i++)
    {
        layer *l = f->layers[i];
        uint32_t z = l->z;
        uint32_t j = i;
        while (j > 0 && f->layers[j - 1]->z > z)
        {
            f->layers[j] = f->layers[j - 1];
            j--;
        }
        f->layers[j] = l;
    }
}

void default_layer_draw_callback(layer *l)
{
    l->needs_redraw = false;
    return;
}

frame *frame_init(uint32_t width, uint32_t height, const char *title)
{
    return frame_init_with_options(width, height, title, false, false, false, false, 1.0);
}

frame *frame_init_with_options(uint32_t width, uint32_t height, const char *title, bool fullscreen, bool vsync, bool resizable, bool borderless, float scaling)
{
    frame *f = malloc(sizeof(frame));
    if (f == NULL)
        return NULL;

    f->scaling = scaling;
    f->width = width * scaling;
    f->height = height * scaling;
    f->buffer = malloc(f->width * f->height * sizeof(uint32_t));
    if (f->buffer == NULL)
    {
        free(f);
        return NULL;
    }
    f->layers = NULL;
    f->layer_count = 0;

#ifdef MINIFB_IMPLEMENTATION
    unsigned int flags = 0;
    if (borderless == true)
    {
        flags |= WF_BORDERLESS;
    }
    if (resizable == true)
    {
        flags |= WF_RESIZABLE;
    }
    if (fullscreen == true)
    {
        flags |= WF_FULLSCREEN;
    }
    f->window = (window *)mfb_open_ex(title, width, height, flags);
    if (f->window == NULL)
    {
        free(f->buffer);
        free(f);
        return NULL;
    }
#endif
    f->needs_redraw = false;

    return f;
}

error frame_resize(frame *f, uint32_t width, uint32_t height)
{
    f->width = width * f->scaling;
    f->height = height * f->scaling;
    f->buffer = realloc(f->buffer, f->width * f->height * sizeof(uint32_t));
    if (f->buffer == NULL)
    {
        return memory_error;
    }

#ifdef DEBUG
    printf("frame resized to %dx%d\n", f->width, f->height);
#endif

    for (uint32_t i = 0; i < f->layer_count; i++)
    {
        layer *l = f->layers[i];
        l->width = f->width;
        l->height = f->height;
        l->buffer = realloc(l->buffer, l->width * l->height * sizeof(uint32_t));
        if (l->buffer == NULL)
            return memory_error;

#ifdef DEBUG
        printf("layer %d resized to %dx%d\r\n", i, l->width, l->height);
#endif

        l->draw(l);
    }
    f->needs_redraw = true;

    return success;
}

// void frame_resize_layer(frame *f)

layer *frame_add_layer(frame *f, uint32_t z)
{
    layer *l = malloc(sizeof(layer));
    if (l == NULL)
    {
        return NULL;
    }

    l->width = f->width;
    l->height = f->height;
    l->buffer = malloc(l->width * l->height * sizeof(uint32_t));
    if (l->buffer == NULL)
    {
        free(l);
        return NULL;
    }
    l->z = z;
    l->needs_redraw = true;

    f->layers = realloc(f->layers, sizeof(layer *) * (f->layer_count + 1));
    if (f->layers == NULL)
    {
        free(l->buffer);
        free(l);
        return NULL;
    }
    f->layers[f->layer_count] = l;
    f->layer_count++;
    _get_draw_order(f);
    l->draw = default_layer_draw_callback;

    f->needs_redraw = true;

    return l;
}

error frame_draw(frame *f)
{
    error status;
    uint8_t red_composite, green_composite, blue_composite;
    float alpha_frame, alpha_layer, alpha_composite;

    // printf("frame_draw: needs_redraw = %s layer_count = %d\r\n", f->needs_redraw ? "true" : "false", f->layer_count);

    if ((f->layer_count == 0) || (f->needs_redraw == false))
    {
        return success;
    }

    _get_draw_order(f);

    for (uint32_t i = 0; i < f->layer_count; i++)
    {
        layer *l = f->layers[i];
        if (l->needs_redraw == true)
            l->draw(l);

        for (uint32_t y = 0; y < l->height; y++)
        {
            for (uint32_t x = 0; x < l->width; x++)
            {
                uint32_t *pixel = l->buffer + (y * l->width) + x;
                uint32_t *frame_pixel = f->buffer + ((y + l->y) * f->width) + (x + l->x);
                // alpha blending should occur here
                alpha_layer = (float)image_a(*pixel) / 255.0;
                alpha_frame = (float)image_a(*frame_pixel) / 255.0;
                alpha_composite = alpha_layer + alpha_frame * (1 - alpha_layer);
                if (alpha_layer == 0)
                {
                    continue;
                }
                if ((alpha_layer == 255) || (i == 0))
                {
                    *frame_pixel = *pixel;
                    continue;
                }

                red_composite = (float)image_r(*pixel) * alpha_layer + (float)image_r(*frame_pixel) * alpha_frame * (1 - alpha_layer) / alpha_composite;
                green_composite = (float)image_g(*pixel) * alpha_layer + (float)image_g(*frame_pixel) * alpha_frame * (1 - alpha_layer) / alpha_composite;
                blue_composite = (float)image_b(*pixel) * alpha_layer + (float)image_b(*frame_pixel) * alpha_frame * (1 - alpha_layer) / alpha_composite;
                *frame_pixel = image_argb((uint8_t)(alpha_composite * 255.0), red_composite, green_composite, blue_composite);
            }
        }
#ifdef DEBUG
        printf("layer[%d] alpha = %0.3f; frame alpha = %0.3f; composite alpha = %0.3f\r\n", i, alpha_layer, alpha_frame, alpha_composite);
#endif
    }
#ifdef MINIFB_IMPLEMENTATION

    mfb_update_state state = mfb_update_ex(f->window, f->buffer, f->width, f->height);
    if (state != STATE_OK)
    {
        mfb_close(f->window);
        free(f->buffer);
        free(f);
        status = io_error;
    }
    else
    {
        status = success;
    }
#endif

    return status;
}

void frame_msleep(uint32_t ms)
{
#if defined(__linux__) || defined(__APPLE__)
    usleep(ms * 1000);
#elif defined(_WIN32)
    Sleep(ms);
#endif
}