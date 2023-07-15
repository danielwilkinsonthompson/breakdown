#ifndef __hexdump_h__
#define __hexdump_h__

#include <stdio.h>

void hexdump(FILE *f, const void *data, size_t size);

#endif