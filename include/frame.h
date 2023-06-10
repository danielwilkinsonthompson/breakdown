/* =============================================================================
                                  frame.h
-------------------------------------------------------------------------------
frame compositor

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:
 - https://github.com/danielwilkinsonthompson/breakdown
 - https://github.com/emoon/minifb
-----------------------------------------------------------------------------*/
#ifndef __frame_h
#define __frame_h
#include <stdint.h> // type definitions

void frame_msleep(uint32_t microseconds);
#endif