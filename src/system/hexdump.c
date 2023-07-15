#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hexdump.h"

void hexdump(FILE *f, const void *data, size_t size)
{
    const uint8_t *d = (const uint8_t *)data;
    size_t i, j;
    for (i = 0; i < size; i += 16)
    {
        fprintf(f, "%08zx: ", i);
        for (j = 0; j < 16; j++)
        {
            if (i + j < size)
            {
                fprintf(f, "%02x ", d[i + j]);
            }
            else
            {
                fprintf(f, "   ");
            }
        }
        fprintf(f, "|");
        for (j = 0; j < 16; j++)
        {
            if (i + j < size)
            {
                fprintf(f, "%c", (d[i + j] >= 32 && d[i + j] < 127) ? d[i + j] : '.');
            }
        }
        fprintf(f, "|\n");
    }
}