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
  image_pixel **pixel_data;
} image;

typedef image *(*image_read_function)(const char *filename);
typedef void (*image_write_function)(image *img, const char *filename);

typedef struct image_type_t
{
  const char *extension;
  image_read_function read;
  image_write_function write;
} image_type;

/*----------------------------------------------------------------------------
  macros for subpixel construction
-----------------------------------------------------------------------------*/
#define image_argb(a, r, g, b) (((image_pixel)a) << 24) | (((image_pixel)r) << 16) | (((image_pixel)g) << 8) | ((image_pixel)b)
#define image_a(argb) ((uint8_t)((argb & 0xFF000000) >> 24))
#define image_r(argb) ((uint8_t)((argb & 0x00FF0000) >> 16))
#define image_g(argb) ((uint8_t)((argb & 0x0000FF00) >> 8))
#define image_b(argb) ((uint8_t)((argb & 0x000000FF)))

/*----------------------------------------------------------------------------
  invalid
-----------------------------------------------------------------------------*/
// validation
#define image_invalid(img) ((img == NULL) || (img->pixel_data == NULL))

/*----------------------------------------------------------------------------
  read
-----------------------------------------------------------------------------*/
image *image_read(const char *filename);

/*----------------------------------------------------------------------------
  write
-----------------------------------------------------------------------------*/
void image_write(image *img, const char *filename);

/*----------------------------------------------------------------------------
  printf
-----------------------------------------------------------------------------*/
void image_printf(const char *format, image *img);

/*----------------------------------------------------------------------------
  fprintf
-----------------------------------------------------------------------------*/
void image_fprintf(FILE *f, const char *format, image *img, uint32_t left_start, uint32_t bottom_start, uint32_t width, uint32_t height);

/*----------------------------------------------------------------------------
  free
-----------------------------------------------------------------------------*/
void image_free(image *img);
#endif
