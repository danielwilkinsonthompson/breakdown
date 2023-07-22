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
#include "error.h"
#include "stream.h"

/*----------------------------------------------------------------------------
  deflate
  ----------------------------------------------------------------------------
  compresses a buffer using the deflate algorithm
  uncompressed  :  uncompressed data
  compressed    :  compressed data
  returns       :  error
-----------------------------------------------------------------------------*/
error deflate(stream *uncompressed, stream *compressed);

/*----------------------------------------------------------------------------
  inflate
  ----------------------------------------------------------------------------
  decompresses a buffer using the deflate algorithm
  compressed    :  compressed data
  decompressed  :  decompressed data
  returns       :  error
-----------------------------------------------------------------------------*/
error inflate(stream *compressed, stream *decompressed);

#endif
