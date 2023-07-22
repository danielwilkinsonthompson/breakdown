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

#define buffer_invalid(buf) ((buf) == NULL || (buf)->data == NULL || (buf)->length == 0)

buffer *buffer_init(size_t length);
buffer *buffer_write(buffer *buf, uint8_t *data, size_t length);
buffer *buffer_read(buffer *buf, uint8_t *data, size_t length);
buffer *buffer_copy(buffer *buf);
buffer *buffer_item(buffer *buf, size_t index);
buffer *buffer_concatenate(buffer *buf, buffer *buf2);
void buffer_free(buffer *buf);
void buffer_print(buffer *buf);

#endif