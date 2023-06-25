/* =============================================================================
                                  frame.c
-------------------------------------------------------------------------------
frame compositor

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:
 - https://github.com/danielwilkinsonthompson/breakdown
 - https://github.com/emoon/minifb
-----------------------------------------------------------------------------*/

/*
TODO:
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

// Use minifb for these platforms
#if defined(__APPLE__) || defined(_WIN32) || defined(__ANDROID__) || defined(__wasm__) || defined(__linux__)
// #include "MiniFB.h"
#else
#error "Current platform is not supported by frame.c"
#endif

#if defined(__linux_) || defined(__APPLE__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

void frame_msleep(uint32_t ms)
{
#if defined(__linux__) || defined(__APPLE__)
    usleep(ms * 1000);
#elif defined(_WIN32)
    Sleep(ms);
#endif
}