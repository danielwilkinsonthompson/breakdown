/*=============================================================================
                                image_test.c
-------------------------------------------------------------------------------
simple test of graphics/image using MiniFB as the frame buffer

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:
 - https://github.com/danielwilkinsonthompson/breakdown
 - https://github.com/emoon/minifb

TODO:
    - Inspect pixel value
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "MiniFB.h"
#include "image.h"
#include "frame.h"

static uint32_t *g_buffer = 0x0;

void mouse_scroll(struct mfb_window *window, mfb_key_mod mod, float deltaX, float deltaY)
{
    fprintf(stderr, "mouse_scroll > deltaX: %f\tdeltaY: %f\r\n", deltaX, deltaY);
}

void mouse_button(struct mfb_window *window, mfb_mouse_button button, mfb_key_mod mod, bool isPressed)
{
    mfb_mouse_button buttonPressed = button;
    if (isPressed)
    {
        int mouse_x = mfb_get_mouse_x(window);
        int mouse_y = mfb_get_mouse_y(window);
        fprintf(stderr, "mouse_btn > button: %d\tmod_key: %d\tmouse_x: %d\tmouse_y: %d\r\n", button, mod, mouse_x, mouse_y);
    }
}

void key_pressed(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool isPressed)
{
    if ((key == KB_KEY_ESCAPE) || ((key == KB_KEY_W) && (mod == KB_MOD_SUPER)))
        mfb_close(window);
}

int main(int argc, char *argv[])
{
    image *test;
    if (argc > 1)
        test = image_read(argv[1]);
    else
    {
        fprintf(stderr, "No file specified\r\n");
        return -1;
    }

    if (test == NULL)
        return -1;

    image_write(test, "writeback.bmp");

    uint32_t width = test->width;
    uint32_t height = test->height;

    struct mfb_window *window = mfb_open("Image Test", width, height);
    if (!window)
        return -3;

    g_buffer = test->pixel_data;
    mfb_set_mouse_scroll_callback(window, mouse_scroll);
    mfb_set_mouse_button_callback(window, mouse_button);
    mfb_set_keyboard_callback(window, key_pressed);

    mfb_update_state state = mfb_update(window, g_buffer);
    do
    {
        state = mfb_update_events(window);
        if (state != STATE_OK)
        {
            window = 0x0;
            break;
        }
        frame_msleep(100); // 10Hz refresh
    } while (mfb_wait_sync(window));

    return 0;
}
