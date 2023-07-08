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
#include "error.h"
#include "image.h"
#include "frame.h"

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
        fprintf(stderr, "mouse_btn > button: %d\tmod_key: %d\tmouse_x: %d\tmouse_y: %d\r\n", button, mod, mouse_x, mouse_y);
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
#ifdef DEBUG
    fprintf(stderr, "resize > width: %d\theight: %d\r\n", width, height);
#endif
    needs_resizing = true;
    needs_redrawing = true;
}

int main(int argc, char *argv[])
{
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

    // f = frame_init(test_resized->width, test_resized->height, "Test");
    f = frame_init_with_options(400, 400, "Frame Test", false, false, true, false, 2.0);
    if (f == NULL)
        return EXIT_FAILURE;

    l1 = frame_add_layer(f, 0);
    if (l1 == NULL)
        return EXIT_FAILURE;

    test1_resized = image_resize(test1, l1->height, l1->width);
    if (test1_resized == NULL)
        return EXIT_FAILURE;

    free(l1->buffer);
    l1->buffer = test1_resized->pixel_data;
    free(test1_resized);
#ifdef DEBUG
    printf("layer[0] bitmap alpha = %0.3f\r\n", ((float)image_a(l1->buffer[0])) / 255.0);
#endif

    if (test2 != NULL)
    {
        l2 = frame_add_layer(f, 1);
        if (l2 == NULL)
            return EXIT_FAILURE;

        test2_resized = image_resize(test2, l2->height, l2->width);
        if (test2_resized == NULL)
            return EXIT_FAILURE;

        free(l2->buffer);
        l2->buffer = test2_resized->pixel_data;
        free(test2_resized);

#ifdef DEBUG
        printf("layer[1] bitmap alpha = %0.3f\r\n", ((float)image_a(l2->buffer[0])) / 255.0);
#endif
    }

    mfb_set_mouse_button_callback(f->window, mouse_button);
    mfb_set_keyboard_callback(f->window, key_pressed);
    mfb_set_resize_callback(f->window, resize_window);
    // mfb_set_active_callback(f->window, active);
    // int redraw_count = 100;

    while (status == success)
    {
        if (needs_resizing == true)
        {
            error e = frame_resize(f, mfb_get_window_width(f->window), mfb_get_window_height(f->window));
            if (e != success)
                return EXIT_FAILURE;

            test1_resized = image_resize(test1, l1->height, l1->width);
            if (test1_resized == NULL)
                return EXIT_FAILURE;

            free(l1->buffer);
            l1->buffer = test1_resized->pixel_data;
            free(test1_resized);

            if (test2 != NULL)
            {
                test2_resized = image_resize(test2, l2->height, l2->width);
                if (test2_resized == NULL)
                    return EXIT_FAILURE;

                free(l2->buffer);
                l2->buffer = test2_resized->pixel_data;
                free(test2_resized);
            }
            needs_resizing = false;
        }
        if (needs_redrawing == true)
        {
            status = frame_draw(f);
            if (status != success)
                return EXIT_FAILURE;
            // if (mfb_is_window_active(f->window) == false)
            //     mfb_wait_sync(f->window);
            // else
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
