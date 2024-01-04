/*=============================================================================
                                    zip.h
-------------------------------------------------------------------------------
read and write zip files (only flat structure supported)

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com

references:
- https://en.wikipedia.org/wiki/ZIP_(file_format)
- https://docs.fileformat.com/compression/zip/
-----------------------------------------------------------------------------*/
#ifndef __zip_h
#define __zip_h
#include <stdio.h>  // debugging, file read/write
#include <stdint.h> // type definitions

typedef struct zip_t zip;

/*
  What do we need to do?
  - Read: opens a zip archive 
  - Write: 

*/


/*----------------------------------------------------------------------------
  open
  open zip archive; if file does not exist, create it
-----------------------------------------------------------------------------*/
zip *zip_open(const char *path);

// // add file to zip archive
// void zip_add(zip *zip, const char *path);

// // remove file from zip archive
// void zip_remove(zip *zip, const char *path);

// // extract file from zip archive
// void zip_extract(zip *zip, const char *path);

/*----------------------------------------------------------------------------
  list
  list all files in zip archive (array of strings)
-----------------------------------------------------------------------------*/
const char **zip_list(zip *zip);

/*----------------------------------------------------------------------------
  defrag
  defragment zip archive
-----------------------------------------------------------------------------*/
void zip_defrag(zip *zip);

#endif // __zip_h