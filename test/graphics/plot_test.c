/*=============================================================================
                                plot_test.c
-------------------------------------------------------------------------------
simple test of plotting

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "MiniFB.h"
#include "error.h"
#include "layer.h"
#include "frame.h"
#include "csv.h"
#include "plot.h"

bool needs_redrawing = true;

int main(int argc, char *argv[])
{
#if defined(DEBUG)
  printf("plot_test: running in debug mode\r\n");
#endif
  error status = success;
  csv *test1 = NULL;
  plot *plt = NULL;
  if (argc > 1)
  {
    test1 = csv_read(argv[1]);
    if (test1 == NULL)
      return EXIT_FAILURE;
  }
  if (argc <= 1)
  {
    fprintf(stderr, "No file specified\r\n");
    return EXIT_FAILURE;
  }

  printf("Initialising plot\r\n");
  plt = plot_init("Basic plot", "Time", "Distance");

  plot_add_trace(plt, (int32_t[]){0, 1, 2, 3, 4, 5}, (int32_t[]){2, 3, 4, 5, 6, 7}, 6, "Turtle Position", image_argb(255, 128, 100, 100));

  while (status == success)
  {
    if (plt->window->needs_redraw == true)
    {
      status = frame_draw(plt->window);

      if (status != success)
        return EXIT_FAILURE;
    }
    else
    {
      if (mfb_update_events(plt->window->window) != STATE_OK)
        status = io_error;
    }
    frame_msleep(100);
  };

  return EXIT_SUCCESS;
}
