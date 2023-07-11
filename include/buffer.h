/*=============================================================================
                                    buffer.h
-------------------------------------------------------------------------------
pointer-safe buffers

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#ifndef __buffer_h
#define __buffer_h
#include <stdlib.h>
#include <stdint.h>

typedef struct buffer_t
{
    size_t length;
    uint8_t *data;
} buffer;

#endif