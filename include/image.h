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
typedef union image_pixel_t
{
  uint32_t u32;
  struct
  {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
  };
} image_pixel;

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

enum image_extensions
{
  BMP
  // PNG
};

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
