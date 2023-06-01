/*=============================================================================
                                    csv.h
-------------------------------------------------------------------------------
read csv files

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#define _CRT_SECURE_NO_WARNINGS // win32 hack for fopen
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array2d.h"
#include "csv.h"

/*----------------------------------------------------------------------------
                              private prototypes
-----------------------------------------------------------------------------*/
int _file_dims(char *filename, unsigned int *col, unsigned int *row);

/*----------------------------------------------------------------------------
  init
------------------------------------------------------------------------------
  create an empty csv data struct
    row :  number of rows
    col :  number of columns
    -
    c   :  empty csv data struct
-----------------------------------------------------------------------------*/
csv *csv_init(csvindex row, csvindex col)
{
  csv *c;
  csvdata **d;

  // check inputs
  if ((row < 1) || (col < 1))
    return NULL;

  // allocate output
  c = (csv *)malloc(sizeof(csv));
  if (c == NULL)
    return NULL;

  c->row = row;
  c->col = col;

  d = (csvdata **)malloc_2d(row, col, sizeof(csvdata));
  if (d == NULL)
  {
    csv_free(c);
    return NULL;
  }
  c->data = d;

  if (csv_invalid(c))
    return NULL;

  return c;
}

/*----------------------------------------------------------------------------
  free
------------------------------------------------------------------------------
  free csv data struct
    c :  csv data struct
-----------------------------------------------------------------------------*/
void csv_free(csv *c)
{
  if (c == NULL)
    return;

  free_2d((void **)c->data, c->row);
  free(c);
}

/*----------------------------------------------------------------------------
  read
------------------------------------------------------------------------------
  read data from a csv file
    filename :  name of file to read
    -
    d        :  struct containing csv data
-----------------------------------------------------------------------------*/
csv *csv_read(char *filename)
{
  csv *d;
  FILE *f;
  csvindex row, col;
  csvindex rows, cols;
  char c;
  char str[256];
  unsigned char s_pt = 0;
  unsigned char in_parens = 0;
  unsigned char new_row, new_col;

  // check inputs
  if (filename == NULL)
    return NULL;

  // calculate number of rows and columns in file
  if (_file_dims(filename, &rows, &cols) != 0)
    return NULL;

  if ((rows < 1) || (cols < 1))
    return NULL;

  // allocate memory for csv data
  d = csv_init(rows, cols);
  if (d == NULL)
    return NULL;

  // open file
  f = fopen(filename, "r");
  if (f == NULL)
    return NULL;

  row = 0;
  col = 0;
  new_row = 0;
  new_col = 0;

  // loop through population of rows and columns
  while (1)
  {
    c = (char)fgetc(f);
    if (feof(f))
      break;

    // new line and comma characters are valid within quotes
    if (c == '\"')
      in_parens = !(in_parens);

    if (in_parens == 0)
    {
      if (c == '\n')
      {
        c = '\0';
        new_row = 1;
      }
      else
      {
        if (c == ',')
        {
          c = '\0';
          new_col = 1;
        }
      }
    }

    str[s_pt] = c;
    s_pt++;
    if (c == '\0')
    {
      d->data[row][col] = (csvdata)atof(str);
      s_pt = 0;
    }
    if (new_row == 1)
    {
      col = 0;
      row++;
      new_row = 0;
      new_col = 0;
    }
    if (new_col == 1)
    {
      col++;
      new_col = 0;
    }
    if (row >= rows)
      break;
  }

  fclose(f);

  return d;
}

/*----------------------------------------------------------------------------
  csv_write
------------------------------------------------------------------------------
  write csv to file using CSV_WRITE_FORMAT
    c        :  csv struct to print
    filename :  name of output file
-----------------------------------------------------------------------------*/
int csv_write(csv *c, char *filename)
{
  FILE *f;

  // check inputs
  if (csv_invalid(c))
    return -1;

  if (filename == NULL)
    return -1;

  // open file
  f = fopen(filename, "w");
  if (f == NULL)
    return -1;

  // write to file and close
  csv_fprintf(c, f, CSV_WRITE_FORMAT);

  // close file
  fclose(f);

  return 1;
}

/*----------------------------------------------------------------------------
  fprintf
------------------------------------------------------------------------------
  print csv to file
    c   :  csv struct to print
    f   :  pointer to output file
    fmt :  fprintf format specifier
-----------------------------------------------------------------------------*/
void csv_fprintf(csv *c, FILE *f, const char *format)
{
  // check inputs
  if (csv_invalid(c))
  {
    fprintf(f, "NULL\n");
    return;
  }

  // print formatted values to file
  for (csvindex row = 0; row < c->row; row++)
  {
    for (csvindex col = 0; col < c->col; col++)
    {
      fprintf(f, format, c->data[row][col]);
      if (col < (c->col - 1))
        fprintf(f, ",");
    }
    fprintf(f, "\n");
  }

  return;
}

/*------------------------------- private -----------------------------------*/

/*----------------------------------------------------------------------------
  _file_dims
------------------------------------------------------------------------------
  determine number of columns and rows within csv file (very inefficient!)
    f   :  FILE pointer to csv file
    col :  pointer to store number of columns
    row :  pointer to store number of rows
    -
    err :  error code
       0 = no errors
      -1 = an input pointer was NULL
      -2 = could not open file
-----------------------------------------------------------------------------*/
int _file_dims(char *filename, unsigned int *row, unsigned int *col)
{
  char c;
  FILE *f;
  unsigned char in_parens = 0;

  // check inputs
  if ((filename == NULL) || (col == NULL) || (row == NULL))
    return -1;

  f = fopen(filename, "r");
  if (f == NULL)
    return -2;

  // initialise outputs
  *col = 1; // col = number of ',' in first line + 1
  *row = 0; // row = number of '\n' in file (at least MATLAB's version)

  // count number of rows and columns
  while (1)
  {
    c = (char)fgetc(f);
    if (feof(f))
      break;

    if (c == '\"')
      in_parens = !(in_parens);

    if (in_parens == 0)
    {
      if (c == '\n')
      {
        (*row)++;
      }
      else
      {
        if ((*row) == 1)
        {
          if (c == ',')
            (*col)++;
        }
      }
    }
  }
  fclose(f);

  return 0;
}
