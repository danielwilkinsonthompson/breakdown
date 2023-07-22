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

buffer *buffer_write(buffer *buf, uint8_t *data, size_t length)
{
    if (buf->length < length)
    {
        buf->data = (uint8_t *)realloc(buf->data, sizeof(uint8_t) * length);
        if (buf->data == NULL)
            goto malloc_error;
        buf->length = length;
    }

    // unfortunately, this always overwrites buffer contents
    for (size_t i = 0; i < length; i++)
        buf->data[i] = data[i];

    return buf;

malloc_error:
    buffer_free(buf);
    return NULL;
}

buffer *buffer_read(buffer *buf, uint8_t *data, size_t length)
{
    if (buf->length < length)
        return NULL;

    for (size_t i = 0; i < length; i++)
        data[i] = buf->data[i];

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
