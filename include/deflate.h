/*=============================================================================
                                    deflate.h
-------------------------------------------------------------------------------
deflate stream compression and decompression

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references
- https://tools.ietf.org/html/rfc1951
-----------------------------------------------------------------------------*/
#ifndef __deflate_h
#define __deflate_h
#include "error.h"
#include "stream.h"

/*----------------------------------------------------------------------------
  deflate
  ----------------------------------------------------------------------------
  compresses a stream using the deflate algorithm
  uncompressed  :  uncompressed data
  compressed    :  compressed data
  returns       :  success (0) or error code (see error.h)
-----------------------------------------------------------------------------*/
error deflate(stream *uncompressed, stream *compressed);

/*----------------------------------------------------------------------------
  inflate
  ----------------------------------------------------------------------------
  decompresses a stream using the deflate algorithm
  compressed    :  compressed data
  decompressed  :  decompressed data
  returns       :  success (0) or error code (see error.h)
-----------------------------------------------------------------------------*/
error inflate(stream *compressed, stream *decompressed);

#endif
