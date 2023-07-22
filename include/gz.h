#ifndef _gz_h
#define _gz_h

#include <stdint.h>
#include <stdio.h>
#include "buffer.h"

#ifndef GZ_MAX_BUFFER_SIZE // define at compile time to override
#define GZ_MAX_BUFFER_SIZE 32768
#endif

buffer *gz_read(const char *filename);
void gz_write(const char *filename, buffer *buf);

#endif // _gz_h