/*=============================================================================
                                    bmp.h
-------------------------------------------------------------------------------
read and write bitmap images

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://github.com/marc-q/libbmp/blob/master/libbmp.h
- https://en.wikipedia.org/wiki/BMP_file_format
-----------------------------------------------------------------------------*/
#ifndef __bmp_h
#define __bmp_h
#include "image.h" // base image code

/*----------------------------------------------------------------------------
  write
  ----------------------------------------------------------------------------
  writes an image to disk
  img      :  image to write, as defined in image.h
  filename :  name of file (should include .bmp extension)
-----------------------------------------------------------------------------*/
void bmp_write(image *img, const char *filename);

/*----------------------------------------------------------------------------
  read
  ----------------------------------------------------------------------------
  reads an image from disk
  filename :  name of file to read (should include .bmp extension)
  returns  :  image as defined in image.h
-----------------------------------------------------------------------------*/
image *bmp_read(const char *filename);

#endif