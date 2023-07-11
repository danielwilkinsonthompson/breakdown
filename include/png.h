/*=============================================================================
                                    png.h
-------------------------------------------------------------------------------
read and write portable network graphics (png) files

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#ifndef __png_h
#define __png_h
#include "image.h"

/*----------------------------------------------------------------------------
  write
  complies with image_write_function of image.c
-----------------------------------------------------------------------------*/
void png_write(image *img, const char *filename);

/*----------------------------------------------------------------------------
  read
  complies with image_read_function of image.c
-----------------------------------------------------------------------------*/
image *png_read(const char *filename);

#endif