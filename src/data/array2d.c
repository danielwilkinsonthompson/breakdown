/*=============================================================================
                                   array2d.c
-------------------------------------------------------------------------------
allocate & free 2d arrays

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include "array2d.h"

/*----------------------------------------------------------------------------
  malloc_2d
------------------------------------------------------------------------------
  allocates a 2d array of the form array[dim1][dim2]
    dim1 :  size of first dimension
    dim2 :  size of second dimension
    element_size :  size of elements
    -
    array :  pointer to 2d array
-----------------------------------------------------------------------------*/
void **malloc_2d(unsigned int dim1, unsigned int dim2, size_t element_size)
{
  void **array;

  // check inputs
  if ((dim1 < 1) || (dim2 < 1) || (element_size < 1))
    return NULL;

  // allocate array of arrays
  array = (void **)malloc(dim1 * sizeof(void *));
  if (array == NULL)
    return NULL;

  // allocate array of each element
  for (unsigned int i = 0; i < dim1; i++)
  {
    array[i] = (void *)malloc(dim2 * element_size);
    if (array[i] == NULL)
      return NULL;
  }

  return array;
}

/*----------------------------------------------------------------------------
  calloc_2d
------------------------------------------------------------------------------
  allocates a 2d array[dim1][dim2] with contents set to zero
    dim1 :  size of first dimension
    dim2 :  size of second dimension
    element_size :  size of elements
    -
    array :  pointer to 2d array
-----------------------------------------------------------------------------*/
void **calloc_2d(unsigned int dim1, unsigned int dim2, size_t element_size)
{
  void **array;

  // check inputs
  if ((dim1 < 1) || (dim2 < 1) || (element_size < 1))
    return NULL;

  // allocate array of arrays
  array = (void **)malloc(dim1 * sizeof(void *));
  if (array == NULL)
    return NULL;

  // allocate array of each element
  for (unsigned int i = 0; i < dim1; i++)
  {
    array[i] = (void *)calloc(dim2, element_size);
    if (array[i] == NULL)
      return NULL;
  }

  return array;
}

/*----------------------------------------------------------------------------
  free_2d
------------------------------------------------------------------------------
  frees a 2d array[dim1][dim2]
    dim1 :  size of first dimension
-----------------------------------------------------------------------------*/
void free_2d(void **array, unsigned int dim1)
{
  for (unsigned int i = 0; i < dim1; i++)
    free(array[i]);

  free(array);

  return;
}