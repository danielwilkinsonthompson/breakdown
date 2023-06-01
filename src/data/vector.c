/*=============================================================================
                                    vector.c
-------------------------------------------------------------------------------
vector description

daniel@wilkinson-thompson.com
2014-07-19
-----------------------------------------------------------------------------*/
#include <stdlib.h>  // malloc/free
#include <stdio.h>   // debugging
#include <stdarg.h>  // varg functions
#include <string.h>  // strlen
#include <math.h>    // sqrt for decimation filter cut-off
#include "vector.h"  // vector header


/*----------------------------------------------------------------------------
                              standard functions
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 init :   create an empty vector of length l
   l  :   vector length
 ----------------------------------------------------------------------------*/
vector* vector_initialise(unsigned int l)
{
  vector *v;
  VECT_PREC *d;

  v = (vector*) malloc(sizeof(vector));
  if (v == NULL)
    return NULL;

  d = (VECT_PREC*) calloc(l, sizeof(VECT_PREC));
  if (d == NULL)
    return NULL;

  v->data   = d;
  v->length = l;

  return v;
}

/*----------------------------------------------------------------------------
 _def_vargs :   define a vector from variable length array input
   argc     :   input count
 ----------------------------------------------------------------------------*/
vector* _vector_def_vargs(unsigned char argc, ...)
{
  va_list argv;

  vector *v = vector_init(argc);
  if (vector_null(v))
    return NULL;

  va_start(argv, argc);
  for (int i = 0; i < argc; i++ )
      v->data[i] = (VECT_PREC) va_arg(argv, double);
  va_end(argv);

  return v;
}

/*----------------------------------------------------------------------------
 _void_vargs :   operate on multiple vectors using func, which has void output
   func      :   function pointer to run on each vector
   argc      :   vector count
 ----------------------------------------------------------------------------*/
void  _vector_void_vargs(void (*func)(vector*), unsigned char argc, ... )
{
  vector *v;
  va_list argv;

  va_start(argv, argc);
  for (int i = 0; i < argc; i++)
  {
    v = (vector *) va_arg( argv, vector* );
    func(v);
  }
  va_end(argv);

  return;
}

/*----------------------------------------------------------------------------
 _casc_vargs :   operate on multiple vectors using func, cascading outputs
   func      :   function pointer to run on each vector
   argc      :   vector count
 ----------------------------------------------------------------------------*/
vector* _vector_casc_vargs(vector* (*func)(vector*, vector*), unsigned char argc, ... )
{
  vector *v, *u, *r;
  va_list argv;

  va_start(argv, argc);

  v = (vector *) va_arg(argv, vector*);
  if (vector_null(v))
    return NULL;

  for (int i = 1;  i < argc;  i++)
  {
    u = (vector *) va_arg(argv, vector*);
    if (vector_null(u))
      return NULL;

    r = func(v, u);
    if (vector_null(r))
      return NULL;

    if (i > 1)
      vector_free(v);

    v = r;
  }

  va_end(argv);

  return v;
}

/*----------------------------------------------------------------------------
 _free :   free a single vector
   v   :   vector to free
 ----------------------------------------------------------------------------*/
void _vector_free(vector *v)
{
  if (v == NULL)
    return;

  free(v->data);
  v->data = NULL;
  free(v);

  return;
}

/*----------------------------------------------------------------------------
  sprintf
------------------------------------------------------------------------------
  print a vector as a formatted string
    v   :  vector to print
    str :  pointer of output string
    fmt :  sprintf format specifier
 ----------------------------------------------------------------------------*/
void vector_sprintf(vector *v, char *str, const char *fmt)
{
  int len;
  char *style;

  if (vector_null(v))  {
    sprintf(str, "NULL\n");
    return;
  }

  style = (char*) calloc(64, sizeof(char));
  if (style == NULL)
    return;

  sprintf(style, "%s\n", fmt);
  len = strlen(style);

  for(int i = 0;  i < v->length;  i++)
    sprintf(str + (i * (len +2) ), style, v->data[i]);

  free(style);

  return;
}

/*----------------------------------------------------------------------------
  fprintf
------------------------------------------------------------------------------
  print a vector to file
    v   :  vector to print
    f   :  pointer to output file
    fmt :  fprintf format specifier
 ----------------------------------------------------------------------------*/
void vector_fprintf(vector *v, FILE* f, const char* fmt)
{
  char *style;

  if (vector_null(v)) {
    fprintf(f, "NULL\n");
    return;
  }

  style = (char*) calloc(64, sizeof(char));
  if (style == NULL)
    return;

  sprintf(style, "%s\n", fmt);

  for(int i = 0;  i < v->length;  i++)
    fprintf(f, style, v->data[i]);

  free(style);

  return;
}

/*----------------------------------------------------------------------------
                           vector-specific functions
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  maximum
------------------------------------------------------------------------------
  find maximum element value and its index in vector
    v   :  vector to search
    ind :  index of maximum
 ----------------------------------------------------------------------------*/
VECT_PREC vector_maximum(vector *v, unsigned int *ind)
{
  VECT_PREC m;

  if (vector_null(v))
    return (VECT_PREC) 0.0;

  m    = v->data[0];
  *ind = 0;

  for(int i = 1; i < v->length; i++)
  {
    if (v->data[i] > m)
    {
      m    = v->data[i];
      *ind = i;
    }
  }

  return m;
}

/*----------------------------------------------------------------------------
  minimum
------------------------------------------------------------------------------
  find minimum value within a vector, and its index
    v   :  vector to search
    ind :  index of minimum
 ----------------------------------------------------------------------------*/
VECT_PREC vector_minimum(vector *v, unsigned int *ind)
{
  VECT_PREC m;

  if (vector_null(v))
    return (VECT_PREC) 0.0;

  m    = v->data[0];
  *ind = 0;

  for(int i = 1;  i < v->length;  i++)
  {
    if (v->data[i] < m)
    {
      m    = v->data[i];
      *ind = i;
    }
  }

  return m;
}

/*----------------------------------------------------------------------------
  vector_sum
------------------------------------------------------------------------------
  calculate the sum of all elements in a vector
    v :  base vector
    -
    r :  calculated sum
-----------------------------------------------------------------------------*/
VECT_PREC vector_sum(vector *v)
{
  VECT_PREC r;

  // check inputs
  if (vector_null(v))
    return NaN;

  for (int i = 0;  i < v->length;  i++)
    r += v->data[i];

  return r;
}

/*----------------------------------------------------------------------------
 _add  :   add two vectors, element by element
   v,u :   vectors to add (v + u)
 ----------------------------------------------------------------------------*/
vector* _vector_add(vector *v, vector *u)
{
  vector *r;

  if (vector_null(v) || vector_null(u) || (v->length != u->length))
    return NULL;

  r = vector_init(v->length);
  if (vector_null(r))
    return NULL;

  for (int i = 0;  i < v->length;  i++)
    r->data[i] = v->data[i] + u->data[i];

  return r;
}

/*----------------------------------------------------------------------------
 _sub  :   subtract two vectors, element by element
   a,b :   vectors to subtract (a - b)
 ----------------------------------------------------------------------------*/
vector* _vector_sub(vector *v, vector *u)
{
  vector *c;

  if (vector_null(v) || vector_null(u) || (v->length != u->length))
    return NULL;

  c = vector_init(v->length);
  if (vector_null(c))
    return NULL;

  for (int i = 0;  i < v->length;  i++)
    c->data[i] = v->data[i] - u->data[i];

  return c;
}

/*----------------------------------------------------------------------------
 _mul  :   multiply two vectors, element by element
   a,b :   vectors to multiply (a * b)
 ----------------------------------------------------------------------------*/
vector* _vector_mul(vector *v, vector *u)
{
  vector *c;

  if (vector_null(v) || vector_null(u) || (v->length != u->length))
    return NULL;

  c = vector_init(v->length);
  if (vector_null(c))
    return NULL;

  for (int i = 0;  i < v->length;  i++)
    c->data[i] = v->data[i] * u->data[i];

  return c;
}

/*----------------------------------------------------------------------------
 _div  :   divide two vectors, element by element
   a,b :   vectors to divide (a / b)
 ----------------------------------------------------------------------------*/
vector* _vector_div(vector *v, vector *u)
{
  vector *c;

  if (vector_null(v) || vector_null(u) || (v->length != u->length))
    return NULL;

  c = vector_init(v->length);
  if (vector_null(c))
    return NULL;

  for (int i = 0;  i < v->length;  i++)
    c->data[i] = v->data[i] / u->data[i];

  return c;
}

/*----------------------------------------------------------------------------
  add_constant
------------------------------------------------------------------------------
  add a constant to a vector, element by element
    v  :   base vector
    c  :   constant
    -
    r  :   vector with constant added
 ----------------------------------------------------------------------------*/
vector* vector_add_constant(vector *v, VECT_PREC c)
{
  vector *r;

  if (vector_null(v))
    return NULL;

  r = vector_init(v->length);
  if (vector_null(r))
    return NULL;

  for (int i = 0;  i < v->length;  i++)
    r->data[i] = v->data[i] + c;

  return r;
}

/*----------------------------------------------------------------------------
  subtract_constant
------------------------------------------------------------------------------
  subtract a constant from a vector, element by element
    v  :   base vector
    c  :   constant
    -
    r  :   vector with constant subtracted
 ----------------------------------------------------------------------------*/
vector* vector_subtract_constant(vector *v, VECT_PREC c)
{
  vector *r;

  if (vector_null(v))
    return NULL;

  r = vector_init(v->length);
  if (vector_null(r))
    return NULL;

  for (int i = 0;  i < v->length;  i++)
    r->data[i] = v->data[i] - c;

  return r;
}

/*----------------------------------------------------------------------------
  multiply_constant
------------------------------------------------------------------------------
  multiply a vector by a constant, element by element
    v  :   base vector
    c  :   constant
    -
    r  :   vector multiplied by constant
 ----------------------------------------------------------------------------*/
vector* vector_multiply_constant(vector *v, VECT_PREC c)
{
  vector *r;

  if (vector_null(v))
    return NULL;

  r = vector_init(v->length);
  if (vector_null(r))
    return NULL;

  for (int i = 0;  i < v->length;  i++)
    r->data[i] = v->data[i] * c;

  return r;
}

/*----------------------------------------------------------------------------
  divide_constant
------------------------------------------------------------------------------
  divide a vector by a constant, element by element
    v  :   base vector
    c  :   constant
    -
    r  :   vector divided by constant
 ----------------------------------------------------------------------------*/
vector* vector_divide_constant(vector *v, VECT_PREC c)
{
  vector *r;

  if (vector_null(v))
    return NULL;

  r = vector_init(v->length);
  if (vector_null(r))
    return NULL;

  for (int i = 0;  i < v->length;  i++)
    r->data[i] = v->data[i] / c;

  return r;
}

/*----------------------------------------------------------------------------
  saturate
------------------------------------------------------------------------------
  saturate vector range to [min, max]
   a   :  vector to saturate
   min :  minimum of range
   max :  maximum of range
 ----------------------------------------------------------------------------*/
vector* vector_saturate(vector *v, VECT_PREC min, VECT_PREC max)
{
  VECT_PREC tmp;
  vector *c;

  if (vector_null(v))
    return NULL;

  if (min > max)
  {
    tmp = min;
    min = max;
    max = tmp;
  }

  c = vector_init(v->length);
  if (vector_null(c))
    return NULL;

  for (int i=0; i < v->length; i++)
  {
    if (v->data[i] > max)
      v->data[i] = max;

    if (v->data[i] < min)
      v->data[i] = min;
  }

  return c;
}

/*----------------------------------------------------------------------------
  decimate
------------------------------------------------------------------------------
  decimate vector to return every n-th point
    v :  base vector
    n :  decimation ratio
    -
    r :  decimated vector

  currently only moving-average pre-decimation filtering, aliasing may occur
-----------------------------------------------------------------------------*/
vector* vector_decimate(vector *v, unsigned int n)
{
  vector *f, *r;
  int i, j;
  VECT_PREC ma, s;
  VECT_PREC fc;
  unsigned int l;

  // check inputs
  if (vector_null(v) || (n == 0))
    return NULL;

  f = vector_init(v->length);
  if (vector_null(f))
    return NULL;

  // initialise output
  r = vector_init(v->length/n);
  if (vector_null(r))
    return NULL;

  // pre-filter input vector
  fc = 0.4 / ((float) n);
  l = (unsigned int) (sqrt(0.2 + fc*fc) / (2 * fc));
  ma = (VECT_PREC) (0.5 / ((float) l));
  for (i = 0;  i < l;  i++)
  {
    f->data[i] = v[0];
    f->data[f->length - 1 - i] = v[v->length - 1];
  }
  for (i = 0;  i < (v->length - 1 - (2*l));  i++)
  {
    s = 0;
    for (j = 0;  j < (2*l);  j++)
      s += v->data[i + j + l];

    f->data[i + l] = ma * s;
  }

  // decimate output vector
  for (int i = 0;   i < v->length;   i++)
    r->data[i] = f->data[i*divisor];

  vector_free(f);

  return r;
}

/*----------------------------------------------------------------------------
  linspace
------------------------------------------------------------------------------
  create vector of linearly spaced points in range [lo, up]
    lo :  lower bound of range
    up :  upper bound of range
    pt :  number of points
    -
    r  :  vector of linearly-spaced points

  if (pt == 0) r = NULL
  if (pt == 1) r->data[0] = lo
-----------------------------------------------------------------------------*/
vector* vector_linspace(VECT_PREC lo, VECT_PREC up, unsigned int pt)
{
  vector *r;
  VECT_PREC range;

  // check inputs
  if (pt == 0)
    return NULL;

  // initialise output
  r = vector_init(pt);
  if (vector_null(r))
    return NULL;

  // fill linspaced vector
  if (pt == 1)
  {
    r->data[0] = lo;
  }
  else
  {
    range = (up - lo)/(pt-1);
    for (int i = 0;  i < c->length;  i++)
      r->data[i] = lo + i*range;
  }

  return r;
}

/*----------------------------------------------------------------------------
  interpolate
------------------------------------------------------------------------------
  linearly interpolate vector by a factor m
    v :  vector to interpolate
    m :  r-spacing = v-spacing/m
    -
    r :  copy of v, interpolated m times

  if (m < 1) m = 1
----------------------------------------------------------------------------*/
vector* vector_interpolate(vector *v, unsigned int m)
{
  vector      *r;
  VECT_PREC    ly, uy;
  unsigned int lx, mx, ux, pt;

  // check inputs
  if (vector_null(v))
    return NULL;

  if (m < 1)
    m = 1;

  // initialise output
  r = vector_init((m-1) * (v->length));
  if (vector_null(r))
    return NULL;

  // interpolate
  for (pt = 0;  pt < (v->length - 1);  pt++)
  {
    lx = pt * m;
    ux = (pt + 1) * m;
    ly = v->data[pt];
    uy = v->data[pt+1];

    slope = (uy - ly) / (ux - lx);
    for (mx = lx;  mx < ux;  mx++)
      r->data[mx] = slope * (mx - lx) + ly;
  }
  r->data[r->length - 1] = v->data[v->length - 1];

  return r;
}

/*----------------------------------------------------------------------------
  subset
------------------------------------------------------------------------------
  subset of v in range [l, u]
    v :  vector to copy
    l :  index of first element to copy
    u :  index of last element to copy
    -
    r :  copy of v in range [l, u]
-----------------------------------------------------------------------------*/
vector* vector_subset(vector *v, unsigned int l, unsigned int u)
{
  vector      *r;
  unsigned int tmp;

  // check inputs
  if (vector_null(v) || (l >= v->length) || (u >= v->length))
    return NULL;

  if (u < l)
  {
    tmp = l;
    l   = u;
    u   = tmp;
  }

  // initialise output
  r = vector_init(u - l);
  if (vector_null(r))
    return NULL;

  // populate subset
  for (int pt = l;  pt <= u;  pt++)
    r->data[pt-l] = v->data[pt];

  return r;
}

/*----------------------------------------------------------------------------
  replace_subset
------------------------------------------------------------------------------
  replace subset of v with u, starting at index x
    v :  vector to modify
    u :  vector to copy into v
    x :  index of first element to replace
    -
    r :  copy of v with points replaced by u starting from location x

  if (v->length < (u->length + x)), only (v->length - x) points replaced
-----------------------------------------------------------------------------*/
vector* vector_replace_subset(vector *v, vector *u, unsigned int x)
{
  vector      *r;
  unsigned int lx, ux;

  // check inputs
  if (vector_null(v) || vector_null(u) || (x >= v->length))
    return NULL;

  if (x < (v->length - 1))
    lx = x;
  else
    lx = (v->length - 1);

  if ((x + u->length) < (v->length - 1))
    ux = (x + u->length);
  else
    ux = (v->length - 1);

  // initialise output
  r = vector_copy(v);
  if (vector_null(r))
    return NULL;

  // replace subset
  for (int pt = lx;  pt <= ux;  pt++)
    r->data[pt] = v->data[pt];

  return r;
}

/*----------------------------------------------------------------------------
  copy
------------------------------------------------------------------------------
  make a copy of v
    v :  vector to copy
    -
    r :  copy of v
-----------------------------------------------------------------------------*/
vector* vector_copy(vector *v)
{
  vector *r;

  // check inputs
  if (vector_null(v))
    return NULL;

  // initialise output
  r = vector_init(v->length);
  if (vector_null(r))
    return NULL;

  // copy data
  for (int pt = 0;  pt < v->length;  pt++)
    r->data[pt] = v->data[pt];

  return r;
}

/*----------------------------------------------------------------------------
  constant
------------------------------------------------------------------------------
  make a vector of length l filled with the constant c
    l :  length of vector
    c :  value to fill vector with
    -
    r :  vector of length l filled with constant c
-----------------------------------------------------------------------------*/
vector* vector_constant(unsigned int l, VECT_PREC c)
{
  vector *r;

  // initialise output
  r = vector_init(l);
  if (vector_null(r))
    return NULL;

  // initialise data
  for (int pt = 0;  pt < r->length;  pt++)
    r->data[pt] = c;

  return r;
}

/*----------------------------------------------------------------------------
  zeros
------------------------------------------------------------------------------
  make a vector of length l filled with 0
    l :  length of vector
    -
    r :  vector of length l filled with constant 0
-----------------------------------------------------------------------------*/
vector* vector_zeros(unsigned int l)
{
  vector *r;

  // initialise output
  r = vector_constant(l, 0);
  if (vector_null(r))
    return NULL;

  return r;
}

/*----------------------------------------------------------------------------
  ones
------------------------------------------------------------------------------
  make a vector of length l filled with 0
    l :  length of vector
    -
    r :  vector of length l filled with constant 0
-----------------------------------------------------------------------------*/
vector* vector_ones(unsigned int l)
{
  vector *r;

  // initialise output
  r = vector_constant(l, 1.0);
  if (vector_null(r))
    return NULL;

  return r;
}

/*----------------------------------------------------------------------------
  cross_correlation
------------------------------------------------------------------------------
  calculate the cross-correlation of two vectors
    v :  first vector to cross-correlate
    u :  second vector to cross-correlate
    s :  span of cross-correlation
    -
    r :  vector of length ((2 * s) + 1) filled with cross-correlation values
-----------------------------------------------------------------------------*/
vector* vector_cross_correlation(vector* v, vector* u, unsigned int s)
{
  VECT_PREC tot;
  vector *r;
  vector *norm_v, *norm_u;
  vector *sub_v, *sub_u;
  vector *vu;
  unsigned int lo, hi;
  unsigned int end_v, end_u;

  // check inputs
  if (vector_null(v) || vector_null(u))
    return NULL;

  // initialise output
  r = vector_init((2 * s) + 1);

  // normalise both vectors
  norm_v = vector_divide_constant(v, vector_maximum(v));
  norm_u = vector_divide_constant(u, vector_maximum(u));
  if (vector_null(norm_v) || vector_null(norm_u))
    return NULL;

  end_v = norm_v->length - 1;
  end_u = norm_u->length - 1;

  lo = (v->length + u->length)/2 - s;
  hi = (v->length + u->length)/2 + s;

  for (int i = lo;  i <= hi;  i++)
  {
    sub_u = vector_subset(norm_u, max(end_u - i, 0), min(end_u, end_u - (i - end_v)));
    sub_v = vector_subset(norm_v, max(i - end_u, 0), min(end_v, i));
    vu    = vector_multiply(sub_v, sub_u);

    r->data[i -lo] = vector_sum(vu) / sub_u->length * norm_u->length;

    vector_free(sub_v, sub_u, vu);
  }

  return r;
}

/*----------------------------------------------------------------------------
  find_shift
------------------------------------------------------------------------------
  calculate the shift between two vectors based on cross-correlation
    v :  base vector
    u :  vector to shift
    s :  maximum shift of u (u will be shifted by [-s, s])
    m :  interpolation multiple
    -
    r :  calculated shift
-----------------------------------------------------------------------------*/
VECT_PREC vector_find_shift(vector *v, vector *u, unsigned int s, unsigned int m)
{
  unsigned int ind;
  VECT_PREC r, ma;
  vector *v_dif, *u_dif;
  vector *v_itp, *u_itp, *xc;

  // check inputs
  if (vector_null(v) || vector_null(u))
    return NaN;

  // better to calculate cross-correlation of diff(v) and diff(u)
  v_dif = vector_difference(v);
  u_dif = vector_difference(u);
  if (vector_null(v_dif) || vector_null(u_dif))
    return NaN;

  v_itp = vector_interpolate(v_dif, m);
  if (vector_null(v_itp))
    return NaN;

  u_itp = vector_interpolate(u_dif, m);
  if (vector_null(u_itp))
    return NaN;

  xc = vector_cross_correlation(v_itp, u_itp, s*m);
  ma = vector_maximum(xc, &ind);
  r  = ( ((VECT_PREC) ind) - ((r->length + 1) / 2) )/ m ;

  vector_free(v_itp, u_itp, v_dif, u_dif, xc);

  return r;
}


/*----------------------------------------------------------------------------
  difference
------------------------------------------------------------------------------
  calculate forward difference of a vector
    v :  base vector
    -
    r :  calculated difference vector
-----------------------------------------------------------------------------*/
vector* vector_difference(vector *v)
{
  vector *r;

  // check inputs
  if (vector_null(v))
    return NULL;

  // intialise output
  r = vector_initialise(v->length - 1);
  if (vector_null(r))
    return NULL;

  // calculate difference
  for (int i = 0;  i < (v->length - 1);  i++)
    r->data[i] = v->data[i+1] - v->data[i];

  return r;
}


/*----------------------------------------------------------------------------
  absolute
------------------------------------------------------------------------------
  calculate absolute value of a vector, element-wise
    v :  base vector
    -
    r :  calculated absolute vector
-----------------------------------------------------------------------------*/
vector* vector_absolute(vector *v)
{
  vector *r;

  // check inputs
  if (vector_null(v))
    return NULL;

  // intialise output
  r = vector_initialise(v->length);
  if (vector_null(r))
    return NULL;

  // calculate absolute value
  for (int i = 0;  i < v->length;  i++)
    r->data[i] = fabs(v->data[i]);

  return r;
}
