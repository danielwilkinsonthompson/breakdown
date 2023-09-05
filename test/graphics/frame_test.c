/*=============================================================================
                                frame_test.c
-------------------------------------------------------------------------------
simple test of frame compositor

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "MiniFB.h"
#include "draw.h"
#include "error.h"
#include "image.h"
#include "layer.h"
#include "frame.h"
#include "gui.h"

bool needs_resizing = false;
bool needs_redrawing = true;

void mouse_button(struct mfb_window *window, mfb_mouse_button button, mfb_key_mod mod, bool isPressed)
{
    mfb_mouse_button buttonPressed = button;
    if (isPressed)
    {
        int mouse_x = mfb_get_mouse_x(window);
        int mouse_y = mfb_get_mouse_y(window);
#ifdef DEBUG
        fprintf(stderr, "frame_test: mouse_button: button: %d\tmod_key: %d\tmouse_x: %d\tmouse_y: %d\r\n", button, mod, mouse_x, mouse_y);
#endif
    }
}

void key_pressed(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool isPressed)
{
    if ((key == KB_KEY_ESCAPE) || ((key == KB_KEY_W) && (mod == KB_MOD_SUPER)) || ((key == KB_KEY_Q) && (mod == KB_MOD_SUPER)))
        mfb_close(window);
}

void resize_window(struct mfb_window *window, int width, int height)
{
    needs_resizing = true;
    needs_redrawing = true;
}

int main(int argc, char *argv[])
{
#if defined(DEBUG)
    printf("frame_test: running in debug mode\r\n");
#endif
    error status = success;
    image *test1, *test1_resized;
    image *test2 = NULL;
    image *test2_resized;
    layer *l1, *l2;
    frame *f;
    if (argc > 1)
    {
        test1 = image_read(argv[1]);
        if (test1 == NULL)
            return EXIT_FAILURE;
    }
    if (argc > 2)
    {
        test2 = image_read(argv[2]);
        if (test2 == NULL)
            return EXIT_FAILURE;
    }
    if (argc <= 1)
    {
        fprintf(stderr, "No file specified\r\n");
        return EXIT_FAILURE;
    }

    f = frame_init_with_options(400, 400, "Frame Test", false, false, true, false, 2.0);
    if (f == NULL)
        return EXIT_FAILURE;

    l1 = frame_add_layer(f);
    if (l1 == NULL)
        return EXIT_FAILURE;

    draw_image(l1, 0, 0, l1->position.width, l1->position.height, test1);
    draw_rectangle(l1, 100, 100, 400, 400, image_argb(255, 0, 0, 255));
    draw_line(l1, 200, 200, 300, 300, image_argb(255, 255, 255, 255));
    draw_polyline(l1, (int32_t[]){200, 300, 300, 200}, (int32_t[]){200, 200, 300, 300}, 4, image_argb(255, 255, 255, 255));

    draw_pixel(l1, 200, 200, image_argb(255, 255, 0, 0));
    draw_pixel(l1, 200, 201, image_argb(255, 255, 0, 0));
    draw_pixel(l1, 201, 200, image_argb(255, 255, 0, 0));
    draw_pixel(l1, 201, 201, image_argb(255, 255, 0, 0));

    draw_pixel(l1, 500, 500, image_argb(255, 0, 255, 0));
    draw_pixel(l1, 500, 501, image_argb(255, 0, 255, 0));
    draw_pixel(l1, 501, 500, image_argb(255, 0, 255, 0));
    draw_pixel(l1, 501, 501, image_argb(255, 0, 255, 0));

    if (test2 != NULL)
    {
        l2 = frame_add_layer(f);
        if (l2 == NULL)
            return EXIT_FAILURE;

        draw_image(l2, 0, 0, l2->position.width, l2->position.height, test2);
    }

    mfb_set_mouse_button_callback(f->window, mouse_button);
    mfb_set_keyboard_callback(f->window, key_pressed);
    mfb_set_resize_callback(f->window, resize_window);

    while (status == success)
    {
        if (needs_resizing == true)
        {
            error e = frame_resize(f, mfb_get_window_width(f->window), mfb_get_window_height(f->window));
            if (e != success)
                return EXIT_FAILURE;

            needs_resizing = false;
        }
        if (needs_redrawing == true)
        {
            status = frame_draw(f);

            if (status != success)
                return EXIT_FAILURE;
            needs_redrawing = false;
        }
        else
        {
            if (mfb_update_events(f->window) != STATE_OK)
                status = io_error;
        }
        frame_msleep(100);
    };

    return EXIT_SUCCESS;
}
