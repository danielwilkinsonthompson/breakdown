#include <stdio.h>
#include <stdlib.h>
#include "zip.h"

int main(int argc, char *argv[])
{
  const char *filename;

  if (argc > 1)
    filename = argv[1];
  else
  {
    printf("No filename provided\n");
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return EXIT_FAILURE;
  }

  zip *z = zip_open(filename);

  return 0;
}
