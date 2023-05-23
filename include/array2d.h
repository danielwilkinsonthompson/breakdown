/*=============================================================================
                                  array2d.h
-------------------------------------------------------------------------------
allocate & free 2d arrays

dan@intspec.com
2014-07-24
-----------------------------------------------------------------------------*/
#ifndef __array2d_h
#define __array2d_h
#include <stdlib.h>


/*----------------------------------------------------------------------------
                                  prototypes
-----------------------------------------------------------------------------*/
void** malloc_2d(unsigned int dim1, unsigned int dim2, size_t element_size);
void** calloc_2d(unsigned int dim1, unsigned int dim2, size_t element_size);
void   free_2d  (void** array, unsigned int dim1);


#endif