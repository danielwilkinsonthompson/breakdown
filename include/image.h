/*=============================================================================
                                  image.h
-------------------------------------------------------------------------------
read and write images

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
references:
 - https://github.com/danielwilkinsonthompson/breakdown
-----------------------------------------------------------------------------*/
#ifndef __image_h
#define __image_h
#include <stdio.h>  // debugging, file read/write
#include <stdint.h> // type definitions

/*----------------------------------------------------------------------------
  typdefs
-----------------------------------------------------------------------------*/
typedef uint32_t image_pixel;
typedef struct image_t
{
  uint32_t height;
  uint32_t width;
  image_pixel *pixel_data; // pixel[x][y] = pixel_data[x + y * width]
} image;

/*----------------------------------------------------------------------------
  macros for subpixel construction/access
-----------------------------------------------------------------------------*/
#define image_argb(a, r, g, b) (((image_pixel)a) << 24) | (((image_pixel)r) << 16) | (((image_pixel)g) << 8) | ((image_pixel)b)
#define image_a(argb) ((uint8_t)((argb & 0xFF000000) >> 24))
#define image_r(argb) ((uint8_t)((argb & 0x00FF0000) >> 16))
#define image_g(argb) ((uint8_t)((argb & 0x0000FF00) >> 8))
#define image_b(argb) ((uint8_t)((argb & 0x000000FF)))

/*----------------------------------------------------------------------------
  invalid
  ----------------------------------------------------------------------------
  true when image is not propoerly initialised
-----------------------------------------------------------------------------*/
#define image_invalid(img) ((img == NULL) || (img->pixel_data == NULL))

/*----------------------------------------------------------------------------
  init
  ----------------------------------------------------------------------------
  create empty image
  height  : image height in pixels
  width   : image width in pixels
-----------------------------------------------------------------------------*/
image *image_init(uint32_t height, uint32_t width);

/*----------------------------------------------------------------------------
  read
  ----------------------------------------------------------------------------
  read image from file
  filename  : name of file on disk (extension required)
-----------------------------------------------------------------------------*/
image *image_read(const char *filename);

/*----------------------------------------------------------------------------
  write
  ----------------------------------------------------------------------------
  write image to file
  img       : image to be written to disk
  filename  : name of file to be written (extension required)
-----------------------------------------------------------------------------*/
void image_write(image *img, const char *filename);

/*----------------------------------------------------------------------------
  printf
  ----------------------------------------------------------------------------
  print image info to console
  format  : format of subpixels (%d for decimal, %x for hex)
  img     : image to print
-----------------------------------------------------------------------------*/
void image_printf(const char *format, image *img);

/*----------------------------------------------------------------------------
  fprintf
  ----------------------------------------------------------------------------
  print image info to file
  f       : file pointer
  format  : number format (e.g. decimal, hex)
  img     : image to print
  left    : top left pixel of region to print
  top     : top left pixel of region to print
  width   : size of region to print
  height  : size of region to print
-----------------------------------------------------------------------------*/
void image_fprintf(FILE *f, const char *format, image *img, uint32_t left, uint32_t top, uint32_t width, uint32_t height);

/*----------------------------------------------------------------------------
  free
  ----------------------------------------------------------------------------
  free image
  img   : image to free
-----------------------------------------------------------------------------*/
void image_free(image *img);
#endif
