#ifndef _gz_h
#define _gz_h

#include <stdint.h>
#include <stdio.h>
#include "buffer.h"



buffer *gz_read(const char *filename);
void gz_write(const char *filename, buffer *buf);

#endif // _gz_h