#include <stdio.h>
#include <stdlib.h>
#include "hexdump.h"
#include "buffer.h"

buffer *buffer_init(size_t length)
{
    buffer *buf = (buffer *)malloc(sizeof(buffer));
    if (buf == NULL)
        return NULL;

    buf->data = (uint8_t *)malloc(sizeof(uint8_t) * length);
    if (buf->data == NULL)
        return NULL;

    buf->length = length;

    return buf;
}

void buffer_print(buffer *buf)
{
    hexdump(stderr, (const void *)buf->data, buf->length);
}

void buffer_free(buffer *buf)
{
    if (buf != NULL)
    {
        if (buf->data != NULL)
            free(buf->data);

        free(buf);
    }
}
