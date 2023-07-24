/*=============================================================================
                                    zlib.h
-------------------------------------------------------------------------------
read and write zlib compressed data streams

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://datatracker.ietf.org/doc/html/rfc1950
- https://datatracker.ietf.org/doc/html/rfc2083
-----------------------------------------------------------------------------*/
#ifndef __zlib_h
#define __zlib_h

#include "stream.h"
#include "error.h"

error zlib_decompress(stream *compressed, stream *decompressed);
error zlib_compress(stream *uncompressed, stream *compressed);

#endif