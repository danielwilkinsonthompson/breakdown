/*=============================================================================
                                    csv.h
-------------------------------------------------------------------------------
read csv files

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
TODO:
  - Move function documentation to csv.h
BUG:
  - If last line of file is not empty \n, last row of csv is missed
-----------------------------------------------------------------------------*/
#ifndef __csv_h
#define __csv_h
#include <stdio.h>  // debugging
#include <stdarg.h> // variable argument functions

/*----------------------------------------------------------------------------
                                 typdefs
-----------------------------------------------------------------------------*/
typedef float csvdata;         // csv precision
typedef unsigned int csvindex; // csv size
typedef struct csv_t
{ // csv-type
  csvdata **data;
  csvindex row;
  csvindex col;
} csv;

#define CSV_WRITE_FORMAT "%+5.5e" // standard csv_write format

/*----------------------------------------------------------------------------
                                function macros
-----------------------------------------------------------------------------*/
// debugging
#ifndef CSV_DEBUG_FORMAT
#define CSV_DEBUG_FORMAT "%f"
#endif

#define csv_debug_value(v)                  \
  fprintf(stdout, #v " = \n");              \
  csv_fprintf(v, stdout, CSV_DEBUG_FORMAT); \
  fprintf(stdout, "\n\n"); // better to print to stderr if available
#define csv_debug_command(c)        \
  fprintf(stdout, ">> " #c "\n\n"); \
  c;
#define csv_invalid(c) ((c == NULL) || (c->data == NULL) || (c->row == 0) || (c->col == 0))

/*----------------------------------------------------------------------------
                                  prototypes
-----------------------------------------------------------------------------*/
csv *csv_init(csvindex row, csvindex col);
void csv_free(csv *c);
csv *csv_read(const char *filename);
int csv_write(csv *c, const char *filename);
void csv_fprintf(csv *c, FILE *f, const char *format);

#endif
