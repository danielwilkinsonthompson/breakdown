/*=============================================================================
                                  bmp_test.c
-------------------------------------------------------------------------------
simple test of breakdown/file/bmp using MiniFB

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:
 - https://github.com/danielwilkinsonthompson/breakdown
 - https://github.com/emoon/minifb

TODO:
    - Pan image
    - Inspect pixel value
    - Zoom
-----------------------------------------------------------------------------*/

#include "MiniFB.h"
#include "bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static uint32_t *g_buffer = 0x0;

void mouse_scroll(struct mfb_window *window, mfb_key_mod mod, float deltaX, float deltaY)
{
    // TODO: pan image
    fprintf(stderr, "mouse_scroll > deltaX: %f\tdeltaY: %f\r\n", deltaX, deltaY);
}

void mouse_button(struct mfb_window *window, mfb_mouse_button button, mfb_key_mod mod, bool isPressed)
{
    // TODO: zoom image at mouse click
    if (isPressed)
    {
        int mouse_x = mfb_get_mouse_x(window);
        int mouse_y = mfb_get_mouse_y(window);
        fprintf(stderr, "mouse_btn > button: %d\tmod_key: %s\tmouse_x: %d\tmouse_y: %d\r\n", button, mfb_get_key_name(mod), mouse_x, mouse_y);
    }
}

int main(int argc, char *argv[])
{
    bmp *test;
    if (argc > 1)
        test = bmp_read(argv[1]);
    else
    {
        fprintf(stderr, "No file specified\r\n");
        return -1;
    }

    if (test == NULL)
    {
        return -2;
    }

    uint32_t width = test->header->width;
    uint32_t height = test->header->height;

    struct mfb_window *window = mfb_open("BMP Test", width, height);
    if (!window)
        return 0;

    g_buffer = (uint32_t *)malloc(width * height * 4);

    mfb_set_mouse_scroll_callback(window, mouse_scroll);
    mfb_set_mouse_button_callback(window, mouse_button);

    mfb_update_state state;
    do
    {
        for (int32_t row = 0; row < test->header->height; row++)
        {
            for (int32_t col = 0; col < test->header->width; col++)
            {
                g_buffer[(test->header->height - row) * test->header->width + col] = MFB_ARGB(0xff, test->pixel_data[row][col].red, test->pixel_data[row][col].green, test->pixel_data[row][col].blue);
            }
        }

        state = mfb_update(window, g_buffer);
        if (state != STATE_OK)
        {
            window = 0x0;
            break;
        }
    } while (mfb_wait_sync(window));

    return 0;
}
