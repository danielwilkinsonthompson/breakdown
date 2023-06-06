/*=============================================================================
                                  image_test.c
-------------------------------------------------------------------------------
simple test of graphics/image using MiniFB

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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "MiniFB.h"
#include "image.h"

static uint32_t *g_buffer = 0x0;

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
    {
        printf("Test == NULL\r\n");
        return -2;
    }

    uint32_t width = test->width;
    uint32_t height = test->height;

    struct mfb_window *window = mfb_open("Image Test", width, height);
    if (!window)
    {
        printf("Could not create window\r\n");
        return -3;
    }

    g_buffer = (uint32_t *)malloc(width * height * 4);

    mfb_update_state state;
    do
    {
        for (int32_t row = 0; row < test->height; row++)
        {
            for (int32_t col = 0; col < test->width; col++)
            {
                g_buffer[(test->height - row) * test->width + col] = MFB_ARGB(0xff, test->pixel_data[row][col].red, test->pixel_data[row][col].green, test->pixel_data[row][col].blue);
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
