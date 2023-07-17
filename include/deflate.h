/*=============================================================================
                                    deflate.h
-------------------------------------------------------------------------------
deflate stream compression and decompression

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

References
- https://tools.ietf.org/html/rfc1951
-----------------------------------------------------------------------------*/
#ifndef __deflate_h
#define __deflate_h
#include <stdint.h>
#include "buffer.h"

/*----------------------------------------------------------------------------
  deflate
  ----------------------------------------------------------------------------
  compresses a buffer using the deflate algorithm
  data     :  uncompressed data
  returns  :  compressed data
-----------------------------------------------------------------------------*/
buffer *deflate(buffer *data);

/*----------------------------------------------------------------------------
  inflate
  ----------------------------------------------------------------------------
  decompresses a buffer using the deflate algorithm
  zdata    :  compressed data
  inflated_size :  size of uncompressed data
  returns  :  uncompressed data
-----------------------------------------------------------------------------*/
buffer *inflate(buffer *zdata, size_t inflated_size);

#endif
