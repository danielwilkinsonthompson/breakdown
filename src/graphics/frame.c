/* =============================================================================
                                  frame.c
-------------------------------------------------------------------------------
frame compositor

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
 - https://github.com/emoon/minifb
 - https://en.wikipedia.org/wiki/Alpha_compositing
-----------------------------------------------------------------------------*/

/*
TODO:
- make base layer distinctive for alpha compositing
- frame_draw doesn't composite alpha correctly
- Separate frame_width, frame_height from pixel_width, pixel_height (essentially, set dpi)
- create frame(width, height) // fullscreen vs windowed
- close frame
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
*/

#include <stdlib.h> // malloc/realloc/free
#include <stdio.h>  // printf
#include <stdbool.h>
#include <string.h> // memset
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

static void _get_draw_order(frame *this_frame)
{
    for (uint32_t i = 0; i < this_frame->layer_count; i++)
    {
        layer *this_layer = this_frame->layers[i];
        uint32_t z = this_layer->position.z;
        uint32_t j = i;
        // FIXME: This should be a for loop
        // for (uint32_t j = i, j > 0; j--)
        while (j > 0 && this_frame->layers[j - 1]->position.z > z)
        {
            // if(this_frame->layers[j - 1]->position.z > this_layer->position.z)
            this_frame->layers[j] = this_frame->layers[j - 1];
            j--;
        }
        this_frame->layers[j] = this_layer;
    }
}

frame *frame_init(uint32_t width, uint32_t height, const char *title)
{
    return frame_init_with_options(width, height, title, false, false, false, false, 1.0);
}

frame *frame_init_with_options(uint32_t width, uint32_t height, const char *title, bool fullscreen, bool vsync, bool resizable, bool borderless, float scaling)
{
    frame *f = (frame *)malloc(sizeof(frame));
    if (f == NULL)
        return NULL;

    f->scaling = scaling;
    f->width = width * scaling;
    f->height = height * scaling;
    f->buffer = (uint32_t *)malloc(f->width * f->height * sizeof(uint32_t));
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

error frame_resize(frame *this_frame, uint32_t width, uint32_t height)
{
    this_frame->width = width * this_frame->scaling;
    this_frame->height = height * this_frame->scaling;
    this_frame->buffer = (uint32_t *)realloc(this_frame->buffer, this_frame->width * this_frame->height * sizeof(uint32_t));
    if (this_frame->buffer == NULL)
        return memory_error;

    // FIXME: don't generally need to set pixels to 0
    memset(this_frame->buffer, 0, this_frame->width * this_frame->height * sizeof(uint32_t));

    // FIXME: resizing layers within a frame needs concept of scaling/bounding box
    for (uint32_t i = 0; i < this_frame->layer_count; i++)
    {
        layer *l = this_frame->layers[i];
        if (l->redraw != layer_hidden)
            l->redraw = layer_needs_rendering;
    }
    this_frame->needs_redraw = true;

    return success;
}

layer *frame_add_layer(frame *this_frame)
{
    if (this_frame == NULL)
        return NULL;
    if (this_frame->layers == NULL)
        this_frame->layers = (layer **)malloc(sizeof(layer *));
    else
        this_frame->layers = (layer **)realloc(this_frame->layers, sizeof(layer *) * (this_frame->layer_count + 1));
    if (this_frame->layers == NULL)
        goto memory_error;

    layer *this_layer = layer_init(this_frame);
    this_frame->layers[this_frame->layer_count] = this_layer;
    this_frame->layer_count++;
    _get_draw_order(this_frame);

    this_frame->needs_redraw = true;

    return this_layer;

memory_error:
#if defined(DEBUG)
    printf("frame_add_layer: memory allocation failed.\n");
#endif // DEBUG
    return NULL;
}

error frame_draw(frame *f)
{
    error status;
    uint8_t red_composite, green_composite, blue_composite;
    float alpha_frame, alpha_layer, alpha_composite;

    if ((f->layer_count == 0) || (f->needs_redraw == false))
        return success;

    _get_draw_order(f);

    for (uint32_t i = 0; i < f->layer_count; i++)
    {
        layer *l = f->layers[i];
        if (l->redraw == layer_hidden)
            continue;
        l->draw(l);

        for (uint32_t y = 0; y < l->position.height; y++) // FIXME: should not exceed frame->height
        {
            for (uint32_t x = 0; x < l->position.width; x++) // FIXME: should not exceed frame->width
            {
                uint32_t *pixel = l->render->pixel_data + (y * l->render->width) + x;
                uint32_t *frame_pixel = f->buffer + ((y + l->position.y) * f->width) + (x + l->position.x);

                if (image_a(*pixel) == 0)
                {
                    continue;
                }
                if ((image_a(*pixel) == 255) || (i == 0))
                {
                    *frame_pixel = *pixel;
                    continue;
                }
                alpha_layer = (float)image_a(*pixel) / 255.0;
                alpha_frame = (float)image_a(*frame_pixel) / 255.0;
                alpha_composite = alpha_layer + alpha_frame * (1 - alpha_layer);

                red_composite = (float)image_r(*pixel) * alpha_layer + (float)image_r(*frame_pixel) * alpha_frame * (1 - alpha_layer) / alpha_composite;
                green_composite = (float)image_g(*pixel) * alpha_layer + (float)image_g(*frame_pixel) * alpha_frame * (1 - alpha_layer) / alpha_composite;
                blue_composite = (float)image_b(*pixel) * alpha_layer + (float)image_b(*frame_pixel) * alpha_frame * (1 - alpha_layer) / alpha_composite;
                *frame_pixel = image_argb((uint8_t)(alpha_composite * 255.0), red_composite, green_composite, blue_composite);
            }
        }
    }
#ifdef MINIFB_IMPLEMENTATION

    mfb_update_state state = mfb_update_ex(f->window, f->buffer, f->width, f->height);
    if (state != STATE_OK)
    {
        printf("Error: could not update frame\r\n");
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