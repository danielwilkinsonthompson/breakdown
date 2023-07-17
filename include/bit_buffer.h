#ifndef __bit_buffer_h
#define __bit_buffer_h

#include <stdint.h>
#include <stdlib.h>
#include "buffer.h"

typedef struct bit_buffer_t
{
  uint8_t *data;
  size_t size;
  uint8_t *byte;
  uint8_t bit;
} bit_buffer;

// function names are a little dangerous
bit_buffer *from_buffer(buffer *buf);
buffer *to_buffer(bit_buffer *bit_buf);
void *bread(bit_buffer *bit_buf, size_t n_bits);
void *bwrite(bit_buffer *bit_buf, uint8_t *data, size_t n_bits);

void bit_buffer_print(bit_buffer *bit_buf);

#endif