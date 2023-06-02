/*=============================================================================
                                    vector.c
-------------------------------------------------------------------------------
numerical analysis using 1-d arrays

© Daniel Wilkinson-Thompson 2015
daniel@wilkinson-thompson.com
-----------------------------------------------------------------------------*/
#include <stdlib.h> // malloc/free
#include <stdio.h>  // debugging
#include <stdarg.h> // varg functions
#include <string.h> // for memcpy
#include <math.h>   // NAN return types
#include <time.h>   // used to seed rand
#include "vector.h" // vect header
#include "csv.h"

/*----------------------------------------------------------------------------
  check c version
-----------------------------------------------------------------------------*/
#if defined(__STDC__)
#define C89
#if defined(__STDC_VERSION__)
#define C90
#if (__STDC_VERSION__ >= 199409L)
#define C94
#endif
#if (__STDC_VERSION__ >= 199901L)
#define C99
#endif
#endif
#endif

/*----------------------------------------------------------------------------
  _private prototypes
-----------------------------------------------------------------------------*/
void _vect_void_vargs(void (*func)(vect *), unsigned char argc, ...);
vect *_vect_gen_0param(vdata (*func)(void), vindex l);
vect *_vect_gen_1param(vdata (*func)(vdata), vindex l, vdata p);
vect *_vect_for_0param(vdata (*func)(vdata), vect *v);
vect *_vect_for_1param(vdata (*func)(vdata, vdata), vect *v, vdata c);
vect *_vect_1param_for(vdata (*func)(vdata, vdata), vdata c, vect *v);
vect *_vect_for_vect(vdata (*func)(vdata, vdata), vect *v, vect *u);
vect *_vect_for_cuml(vdata (*func)(vdata, vdata), vect *v);
vdata _vect_for_cond(vdata (*func)(vdata, vdata), vect *v);
void _vect_free(vect *a);
vdata _add(vdata a, vdata b);
vdata _sub(vdata a, vdata b);
vdata _mul(vdata a, vdata b);
vdata _div(vdata a, vdata b);
vdata _cp(vdata a);
vdata _rnd(void);
vdata _isinf(vdata a);
vdata _isnan(vdata a);
vdata _sign(vdata a);
vdata _mag2db(vdata a);
vdata _db2mag(vdata a);
vdata _mod(vdata a, vdata b);

/*----------------------------------------------------------------------------
  vector built-in functions (inefficient, but compact)
-----------------------------------------------------------------------------*/

// parametric generation
vect *vect_zeros(vindex l) { return vect_init(l); }
vect *vect_ones(vindex l) { return _vect_gen_1param(&_cp, l, 1.0); }
vect *vect_rand(vindex l) { return _vect_gen_0param(&_rnd, l); }
vect *vect_copy(vect *v) { return _vect_for_0param(&_cp, v); }
vect *vect_const(vindex l, vdata c) { return _vect_gen_1param(&_cp, l, c); }

// single vector elementwise
vect *vect_sin(vect *v) { return _vect_for_0param(&sin, v); }
vect *vect_cos(vect *v) { return _vect_for_0param(&cos, v); }
vect *vect_tan(vect *v) { return _vect_for_0param(&tan, v); }
vect *vect_asin(vect *v) { return _vect_for_0param(&asin, v); }
vect *vect_acos(vect *v) { return _vect_for_0param(&acos, v); }
vect *vect_atan(vect *v) { return _vect_for_0param(&atan, v); }
vect *vect_sinh(vect *v) { return _vect_for_0param(&sinh, v); }
vect *vect_cosh(vect *v) { return _vect_for_0param(&cosh, v); }
vect *vect_tanh(vect *v) { return _vect_for_0param(&tanh, v); }
vect *vect_sqrt(vect *v) { return _vect_for_0param(&sqrt, v); }
vect *vect_cbrt(vect *v) { return _vect_for_0param(&cbrt, v); }
vect *vect_abs(vect *v) { return _vect_for_0param(&fabs, v); }
vect *vect_exp(vect *v) { return _vect_for_0param(&exp, v); }
vect *vect_exp2(vect *v) { return _vect_for_0param(&exp2, v); }
vect *vect_log(vect *v) { return _vect_for_0param(&log, v); }
vect *vect_log10(vect *v) { return _vect_for_0param(&log10, v); }
vect *vect_log2(vect *v) { return _vect_for_0param(&log2, v); }
vect *vect_round(vect *v) { return _vect_for_0param(&round, v); }
vect *vect_floor(vect *v) { return _vect_for_0param(&floor, v); }
vect *vect_ceil(vect *v) { return _vect_for_0param(&ceil, v); }
vect *vect_isinf(vect *v) { return _vect_for_0param(&_isinf, v); }
vect *vect_isnan(vect *v) { return _vect_for_0param(&_isnan, v); }
vect *vect_mag2db(vect *v) { return _vect_for_0param(&_mag2db, v); }
vect *vect_db2mag(vect *v) { return _vect_for_0param(&_db2mag, v); }
vect *vect_erf(vect *v) { return _vect_for_0param(&erf, v); }
vect *vect_erfc(vect *v) { return _vect_for_0param(&erfc, v); }
vect *vect_lgamma(vect *v) { return _vect_for_0param(&lgamma, v); }
vect *vect_tgamma(vect *v) { return _vect_for_0param(&tgamma, v); }

// single vector and constant, elementwise
vect *vect_addc(vect *v, vdata c) { return _vect_for_1param(&_add, v, c); }
vect *vect_subc(vect *v, vdata c) { return _vect_for_1param(&_sub, v, c); }
vect *vect_mulc(vect *v, vdata c) { return _vect_for_1param(&_mul, v, c); }
vect *vect_divc(vect *v, vdata c) { return _vect_for_1param(&_div, v, c); }
vect *vect_powc(vect *v, vdata c) { return _vect_for_1param(&pow, v, c); }
vect *vect_rem(vect *v, vdata c) { return _vect_for_1param(&remainder, v, c); }
vect *vect_mod(vect *v, vdata c) { return _vect_for_1param(&fmod, v, c); }

// constant and single vector, elementwise
vect *vect_cpow(vdata c, vect *v) { return _vect_1param_for(&pow, c, v); }
vect *vect_exp10(vect *v) { return _vect_1param_for(&pow, 10, v); }

// dual vector elementwise
vect *vect_add(vect *v, vect *u) { return _vect_for_vect(&_add, v, u); }
vect *vect_sub(vect *v, vect *u) { return _vect_for_vect(&_sub, v, u); }
vect *vect_mul(vect *v, vect *u) { return _vect_for_vect(&_mul, v, u); }
vect *vect_div(vect *v, vect *u) { return _vect_for_vect(&_div, v, u); }
vect *vect_pow(vect *v, vect *u) { return _vect_for_vect(&pow, v, u); }
vect *vect_atan2(vect *v, vect *u) { return _vect_for_vect(&atan2, v, u); }
vect *vect_hypot(vect *v, vect *u) { return _vect_for_vect(&hypot, v, u); }

// simple functional
vdata vect_mean(vect *v) { return vect_sum(v) / v->length; }

/*----------------------------------------------------------------------------
 init
------------------------------------------------------------------------------
  create an empty vect of length l
    l :  vect length
    -
    v :  empty vect
-----------------------------------------------------------------------------*/
vect *vect_init(vindex l)
{
  vect *v;
  vdata *d;

  v = (vect *)malloc(sizeof(vect));
  if (v == NULL)
    return NULL;

  d = (vdata *)calloc(l, sizeof(vdata));
  if (d == NULL)
    return NULL;

  v->data = d;
  v->length = l;

  return v;
}

/*----------------------------------------------------------------------------
  printf
------------------------------------------------------------------------------
  print a vect to stdout
    v   :  vect to print
    fmt :  fprintf format specifier
-----------------------------------------------------------------------------*/
void vect_printf(const char *fmt, vect *v)
{
  // check inputs
  if (vect_invalid(v))
  {
    printf("NULL\n");
    return;
  }

  // print formatted values to file
  for (int i = 0; i < v->length; i++)
    printf(fmt, v->data[i]);

  printf("\n\n");

  return;
}

/*----------------------------------------------------------------------------
  fprintf
------------------------------------------------------------------------------
  print a vect to file/stream (e.g., stdout)
    f   :  pointer to output file/stream
    v   :  vect to print
    fmt :  fprintf format specifier
-----------------------------------------------------------------------------*/
void vect_fprintf(FILE *f, const char *fmt, vect *v)
{
  // check inputs
  if (vect_invalid(v))
  {
    fprintf(f, "NULL\n");
    return;
  }

  // print formatted values to file
  for (int i = 0; i < v->length; i++)
    fprintf(f, fmt, v->data[i]);

  return;
}

/*----------------------------------------------------------------------------
  read_csv
------------------------------------------------------------------------------
  import csv file into an array of vectors, one vector per column in file
    filename :  name of file to import
    cols     :  number of columns in the csv file
    -
    v[]      :  array of vectors
-----------------------------------------------------------------------------*/
vect **vect_read_csv(const char *filename, uint32_t *cols)
{
  vect **v;
  csv *c;

  c = csv_read(filename);
  if (csv_invalid(c))
    return NULL;

  v = (vect **)malloc(c->col * sizeof(vect *));
  if (v == NULL)
    return NULL;

  // allocate array of each element
  for (uint32_t col = 0; col < c->col; col++)
  {
    v[col] = vect_init(c->row);
    if (vect_invalid(v[col]))
    {
      printf("v[%d] is invalid\r\n", col);
      return NULL;
    }
  }

  // csv is [row][col] not [col][row], copy data
  for (csvindex row = 0; row < c->row; row++)
  {
    for (csvindex col = 0; col < c->col; col++)
    {
      v[col]->data[row] = (vdata)c->data[row][col];
    }
  }
  *cols = c->col;

  return v;
}

/*----------------------------------------------------------------------------
  write_csv
------------------------------------------------------------------------------
  export an array of vectors to csv file into, one column per vector
    v        :  array of vectors to write to csv
    cols     :  number of columns (i.e. vectors) to write to csv
    filename :  name of file to write
-----------------------------------------------------------------------------*/
void vect_write_csv(vect **v, const char *filename, uint32_t cols)
{
  csv *c;
  csvindex lines_written;

  if vect_invalid (v[0])
    return;

  c = csv_init(v[0]->length, cols);
  if (csv_invalid(c))
    return;

  // csv is [row][col] not [col][row], copy data
  for (csvindex row = 0; row < c->row; row++)
  {
    for (csvindex col = 0; col < c->col; col++)
    {
      c->data[row][col] = (csvdata)v[col]->data[row];
    }
  }

  // write data to file
  lines_written = csv_write(c, filename);
  if (lines_written != v[0]->length)
    fprintf(stderr, "Failed to write all data to %s", filename);
}

/*----------------------------------------------------------------------------
  max
------------------------------------------------------------------------------
  find maximum element value and its index
    v   :  vect to search
    ind :  index of maximum
    -
    m   :  maximum value of vect
-----------------------------------------------------------------------------*/
vdata vect_max(vect *v, vindex *ind)
{
  vdata m;

  // check inputs
  if (vect_invalid(v))
    return NAN;

  // find maximum
  m = v->data[0];
  *ind = 0;
  for (vindex i = 0; i < v->length; i++)
  {
    if (v->data[i] > m)
    {
      m = v->data[i];
      *ind = i;
    }
  }

  return m;
}

/*----------------------------------------------------------------------------
  min
------------------------------------------------------------------------------
  find minimum element value and its index
    v   :  vect to search
    ind :  index of minimum (pointer)
    -
    m   :  minimum value of vect
-----------------------------------------------------------------------------*/
vdata vect_min(vect *v, vindex *ind)
{
  vdata m;

  // check inputs
  if (vect_invalid(v))
    return NAN;

  // find minimum
  m = v->data[0];
  *ind = 0;
  for (vindex i = 0; i < v->length; i++)
  {
    if (v->data[i] < m)
    {
      m = v->data[i];
      *ind = i;
    }
  }

  return m;
}

/*----------------------------------------------------------------------------
  flip
------------------------------------------------------------------------------
  reverse the order of the vect
    v :  base vect
    -
    f :  flipped vect
-----------------------------------------------------------------------------*/
vect *vect_flip(vect *v)
{
  vect *f;

  // check inputs
  if (vect_invalid(v))
    return NULL;

  f = vect_init(v->length);
  if (vect_invalid(f))
    return NULL;

  // calculate sum
  for (vindex pt = 0; pt < v->length; pt++)
    f->data[v->length - 1 - pt] = v->data[pt];

  return f;
}

/*----------------------------------------------------------------------------
  sum
------------------------------------------------------------------------------
  calculate the sum of all elements in a vect
    v :  base vect
    -
    s :  calculated sum
-----------------------------------------------------------------------------*/
vdata vect_sum(vect *v)
{
  vdata s = 0;

  // check inputs
  if (vect_invalid(v))
    return NAN;

  // calculate sum
  for (vindex i = 0; i < v->length; i++)
    s += v->data[i];

  return s;
}

/*----------------------------------------------------------------------------
  prod
------------------------------------------------------------------------------
  calculate the product of all elements in a vect
    v :  base vect
    -
    p :  calculated product
-----------------------------------------------------------------------------*/
vdata vect_prod(vect *v)
{
  vdata p = 1;

  // check inputs
  if (vect_invalid(v))
    return NAN;

  // calculate product
  for (vindex i = 0; i < v->length; i++)
    p *= v->data[i];

  return p;
}

/*----------------------------------------------------------------------------
  cumsum
------------------------------------------------------------------------------
  calculate the cumulative sum of all elements in a vect
    v :  base vect
    -
    s :  calculated sum
-----------------------------------------------------------------------------*/
vect *vect_cumsum(vect *v)
{
  vdata s = 0;
  vect *cs;

  // check inputs
  if (vect_invalid(v))
    return NULL;

  // init output
  cs = vect_init(v->length);

  // calculate sum
  for (vindex i = 0; i < v->length; i++)
  {
    s += v->data[i];
    cs->data[i] = s;
  }

  return cs;
}

/*----------------------------------------------------------------------------
  interp
------------------------------------------------------------------------------
  linearly interpolate vect by factor i
    v :  vect to interpolate
    i :  r-spacing = v-spacing/i
    -
    r :  v interpolated i times

  if (i < 1) i = 1
----------------------------------------------------------------------------*/
vect *vect_interp(vect *v, vindex i)
{
  vect *r;
  vdata slope, ly, uy;
  vindex lx, mx, ux;

  // check inputs
  if (vect_invalid(v))
    return NULL;

  if (i < 1)
  {
    r = vect_copy(v);
    if (vect_invalid(r))
      return NULL;

    return r;
  }

  // init output
  r = vect_init(i * (v->length - 1) + 1);
  if (vect_invalid(r))
    return NULL;

  // interpolate
  for (vindex pt = 0; pt < (v->length - 1); pt++)
  {
    lx = pt * i;
    ux = (pt + 1) * i;
    ly = v->data[pt];
    uy = v->data[pt + 1];

    slope = (uy - ly) / (ux - lx);
    for (mx = lx; mx < ux; mx++)
      r->data[mx] = slope * (mx - lx) + ly;
  }
  r->data[r->length - 1] = v->data[v->length - 1];

  return r;
}

/*----------------------------------------------------------------------------
  decimate
------------------------------------------------------------------------------
  decimate by factor m
    v :  vect to decimate
    m :  r-spacing = v-spacing*i
    -
    r :  v decimated m times

  if (m <= 1) m = 1
----------------------------------------------------------------------------*/
vect *vect_decimate(vect *v, vindex m)
{
  vect *r;

  // check inputs
  if (vect_invalid(v))
    return NULL;

  if (m <= 1)
  {
    r = vect_copy(v);
    if vect_invalid (r)
      return NULL;

    return r;
  }

  // init output
  r = vect_init((v->length) / m);
  if (vect_invalid(r))
    return NULL;

  // decimate
  for (vindex pt = 0; pt < r->length; pt++)
    r->data[pt] = v->data[pt * m];

  return r;
}

/*----------------------------------------------------------------------------
  resample
------------------------------------------------------------------------------
  resample v by interpolating by i and decimating by m
    v :  vect to resample
    i :  interpolation factor
    m :  decimated factor
    -
    r :  v resampled by i/m

----------------------------------------------------------------------------*/
vect *vect_resample(vect *v, vindex i, vindex m)
{
  vect *it;
  vect *r;

  // check inputs
  if (vect_invalid(v))
    return NULL;

  // interpolate
  it = vect_interp(v, i);
  if (vect_invalid(it))
    return NULL;

  // init output
  r = vect_decimate(it, m);
  if (vect_invalid(r))
    return NULL;

  return r;
}

/*----------------------------------------------------------------------------
  subset
------------------------------------------------------------------------------
  copy subset of v in range [l, u]
    v :  vect to copy
    l :  index of first element to copy
    u :  index of last element to copy
    -
    s :  subset of v in range [l, u]
-----------------------------------------------------------------------------*/
vect *vect_subset(vect *v, vindex l, vindex u)
{
  vect *s;
  vindex tmp;

  // check inputs
  if (vect_invalid(v))
    return NULL;

  if (u < l)
  {
    tmp = l;
    l = u;
    u = tmp;
  }

  if (l > v->length)
    return NULL;

  l = max(0, l);
  u = min(u, v->length - 1);

  // init output
  s = vect_init(u - l + 1);
  if (vect_invalid(s))
    return NULL;

  // populate subset
  for (vindex pt = l; pt <= u; pt++)
    s->data[pt - l] = v->data[pt];

  return s;
}

/*----------------------------------------------------------------------------
  insert
------------------------------------------------------------------------------
  insert u into v starting at index i
    v :  base vector
    u :  vector to insert
    s :  starting index of insertion
    -
    m :  merged vector
-----------------------------------------------------------------------------*/
vect *vect_insert(vect *v, vect *u, vindex s)
{
  vect *m;

  // check inputs
  if (vect_invalid(v) || vect_invalid(u))
    return NULL;

  if (s >= v->length)
    return vect_copy(v);

  // init output
  m = vect_init(max(u->length + s, v->length));
  if (vect_invalid(m))
    return NULL;

  // populate subset of v
  for (vindex ptv = 0; ptv < v->length; ptv++)
    m->data[ptv] = v->data[ptv];

  // populate subset of u
  for (vindex ptu = 0; ptu < u->length; ptu++)
    m->data[s + ptu] = u->data[ptu];

  return m;
}

/*----------------------------------------------------------------------------
  concat
------------------------------------------------------------------------------
  concatenate vects v and u
    v :  vect to start
    u :  vect to concatenate
    -
    c :  concatenated vector
-----------------------------------------------------------------------------*/
vect *vect_concat(vect *v, vect *u)
{
  vect *c;

  // check inputs
  if (vect_invalid(v) || vect_invalid(u))
    return NULL;

  // init output
  c = vect_init(v->length + u->length);
  if (vect_invalid(c))
    return NULL;

  // populate concatenated vector
  for (vindex vpt = 0; vpt < v->length; vpt++)
    c->data[vpt] = v->data[vpt];

  for (vindex upt = 0; upt < u->length; upt++)
    c->data[v->length + upt] = u->data[upt];

  return c;
}

/*----------------------------------------------------------------------------
  smooth
------------------------------------------------------------------------------
  perform n-point moving average on v
    v :  base vect
    n :  number of samples to average
    -
    s :  smoothed output vect

  if n is even, n = n - 1
  the first (n+1)/2 elements are identical, as are the last (n+1)/2
-----------------------------------------------------------------------------*/
vect *vect_smooth(vect *v, vindex n)
{
  vect *s;
  vdata ma;

  if (vect_invalid(v))
    return NULL;

  if (n >= v->length) // can't average more points than exist
  {
    ma = vect_mean(v);
    s = vect_const(v->length, ma);
    if (vect_invalid(s))
      return NULL;

    return s;
  }

  if (remainder(n, 2) == 0) // n needs to be odd
    n--;

  s = vect_init(v->length);
  if (vect_invalid(s))
    return NULL;

  for (vindex pt = 0; pt < (v->length - n + 1); pt++)
  {
    ma = 0;
    for (vindex i = 0; i < n; i++)
      ma += v->data[pt + i];

    s->data[pt + (n - 1) / 2] = ma / n;
  }

  for (vindex start_pt = 0; start_pt < (n - 1) / 2; start_pt++)
    s->data[start_pt] = v->data[(n - 1) / 2];

  for (vindex end_pt = v->length - (n - 1) / 2; end_pt < v->length; end_pt++)
    s->data[end_pt] = v->data[v->length - (n + 1) / 2];

  return s;
}

/*----------------------------------------------------------------------------
  xcorr
------------------------------------------------------------------------------
  calculate the cross-correlation of two vects
    v :  base vect
    u :  second vect for cross-correlation
    s :  span of cross-correlation
    -
    x :  cross-correlation of v and u

  output length is ((2 * s) + 1), with centre value denoting xcorr @ s = 0
-----------------------------------------------------------------------------*/
vect *vect_xcorr(vect *v, vect *u, vindex s)
{
  vect *x;
  vect *norm_v, *norm_u;
  vect *sub_v, *sub_u;
  vect *vu;
  vindex end_v, end_u;
  vindex dont_care;
  vindex lo, hi;
  vindex u_lo, u_hi, v_lo, v_hi;

  // check inputs
  if (vect_invalid(v) || vect_invalid(u))
    return NULL;

  // init output
  x = vect_init((2 * s) + 1);
  if (vect_invalid(x))
    return NULL;

  // normalise both vects
  norm_v = vect_divc(v, vect_max(v, &dont_care));
  norm_u = vect_divc(u, vect_max(u, &dont_care));
  if (vect_invalid(norm_v) || vect_invalid(norm_u))
    return NULL;

  // define range of cross-correlation calculation
  end_v = norm_v->length - 1;
  end_u = norm_u->length - 1;
  lo = (u->length + v->length) / 2 - s;
  hi = (u->length + v->length) / 2 + s;

  // calcualte cross-correlation
  for (vindex i = lo; i <= hi; i++)
  {

    u_lo = max(((long int)end_u) - ((long int)i), 0);
    u_hi = min(((long int)end_u), ((long int)end_u) - (((long int)i) - ((long int)end_v)));
    v_lo = max(((long int)i - (long int)end_u), 0);
    v_hi = min((long int)end_v, (long int)i);

    sub_u = vect_subset(norm_u, u_lo, u_hi);
    sub_v = vect_subset(norm_v, v_lo, v_hi);
    vu = vect_mul(sub_v, sub_u);

    x->data[i - lo] = vect_sum(vu) / sub_u->length * norm_u->length;

    // clean up
    vect_free(sub_v, sub_u, vu);
  }

  return x;
}

/*----------------------------------------------------------------------------
  diff
------------------------------------------------------------------------------
  calculate element-wise forward difference of a vect
    v :  base vect
    -
    d :  calculated difference vect
-----------------------------------------------------------------------------*/
vect *vect_diff(vect *v)
{
  vect *d;

  // check inputs
  if (vect_invalid(v))
    return NULL;

  // intialise output
  d = vect_init(v->length - 1);
  if (vect_invalid(d))
    return NULL;

  // calculate difference
  for (vindex i = 0; i < (v->length - 1); i++)
    d->data[i] = v->data[i + 1] - v->data[i];

  return d;
}

/*----------------------------------------------------------------------------
  peaks
------------------------------------------------------------------------------
  find the peaks (maxima or minima) within a vector
    v :  base vect
    t :  threshold for what constitutes a peak
          (>0 => find maxima, <0 => find minima)
    n :  maximum number of peaks to find
    m :  pointer to return peak values
    i :  pointer to return peak indexes (malloc'ed, must be free'd)
    -
    f :  number of peaks found
-----------------------------------------------------------------------------*/
vindex vect_peaks(vect *v, vdata t, vindex n, vect *m, vindex *i)
{
  vdata mn;
  vdata mx;
  vindex mn_ind;
  vindex mx_ind;
  vdata d;
  vindex f = 0;

  vindex *i_temp;
  vect *m_temp;

  unsigned char rising = 1;
  unsigned char find_max = (t >= 0);
  t = fabs(t);

  i = (vindex *)malloc(n * sizeof(vindex));
  m = vect_init(n);

  if (vect_invalid(m) || i == NULL)
    return 0;

  // search through all points in the vector
  mn = v->data[0];
  mx = v->data[0];
  for (vindex pt = 1; pt < v->length; pt++)
  {
    d = v->data[pt];

    if (d < mn) // is this the new local minimum?
    {
      mn = d;
      mn_ind = pt;
    }

    if (d > mx) // is this the new local maximum?
    {
      mx = d;
      mx_ind = pt;
    }

    // if this point is not local max/min, last point may have been
    if (rising == 1) // we were moving up curve
    {
      if (d < (mx - t)) // if we are now moving down
      {
        rising = 0; // previous point was maximum
        if (find_max == 1)
        {
          m->data[f] = mx; // store if looking for maxima
          i[f] = mx_ind;
          f++;
        }
      }
    }
    else // we were moving down curve
    {
      if (d > (mn + t)) // if we are now moving up
      {
        rising = 1; // previous point was minimum
        if (find_max == 0)
        {
          m->data[f] = mn; // store if looking for minima
          i[f] = mn_ind;
          f++;
        }
      }
    }
  }

  // trim size of returned values
  m_temp = m;
  i_temp = i;

  i = malloc(f * sizeof(vindex));
  memcpy(i, i_temp, f * sizeof(vindex));
  free(i_temp);

  m = vect_subset(m_temp, 0, f);
  vect_free(m_temp);

  return f;
}

/*----------------------------------------------------------------------------
  find_shift
------------------------------------------------------------------------------
  calculate the shift between two vects based on cross-correlation
    v :  base vect
    u :  vect to shift
    r :  search range of shift [-r, r]
    i :  degree of interpolation
    -
    s :  calculated shift
-----------------------------------------------------------------------------*/
vdata vect_find_shift(vect *v, vect *u, vindex r, unsigned int i)
{
  vindex ind;
  double s, ma;
  vect *v_dif, *u_dif;
  vect *v_itp, *u_itp;
  vect *xc;

  // check inputs
  if (vect_invalid(v) || vect_invalid(u))
    return NAN;

  // more accurate to calculate xcorr of diff(v) and diff(u)
  v_dif = vect_diff(v);
  u_dif = vect_diff(u);
  if (vect_invalid(v_dif) || vect_invalid(u_dif))
    return NAN;

  // interpolate vects for greater shift resolution
  v_itp = vect_interp(v_dif, i);
  u_itp = vect_interp(u_dif, i);
  if (vect_invalid(v_itp) || vect_invalid(u_itp))
    return NAN;

  // find shift that gives maximum cross-correlation
  xc = vect_xcorr(v_itp, u_itp, r * i);

  ma = vect_max(xc, &ind);
  s = ((double)ind - (((double)r) * ((double)i)) - 1) / ((double)i);

  // clean up
  vect_free(v_itp, u_itp, v_dif, u_dif, xc);

  return (vdata)s;
}

/*----------------------------------------------------------------------------
  trigger
------------------------------------------------------------------------------
  find the first element matching the trigger t with slope s
    v :  base vect
    t :  value of trigger
    s :  slope of trigger (>0 => +ve, <=0 => -ve)
    -
    p :  position of trigger point in vector
-----------------------------------------------------------------------------*/
vindex vect_trigger(vect *v, vdata t, int s)
{
  vindex p = 0;
  vindex pt;
  vdata a, b;

  a = v->data[0] - t;

  for (pt = 0; pt < (v->length - 1); pt++)
  {
    b = v->data[p + 1] - t;

    // trigger between a & b, with correct slope
    if (((a * b) <= 0) && (((b - a) * s) >= 0))
    {
      if (fabs(a) < fabs(b))
        p = pt;
      else
        p = pt + 1;
    }

    // move to next point
    a = b;
  }

  return p;
}

/*----------------------------------------------------------------------------
  norm
------------------------------------------------------------------------------
  calculate the norm of every element in a vect
    v :  base vect
    -
    n :  norm of vect
-----------------------------------------------------------------------------*/
vdata vect_norm(vect *v)
{
  vdata s, n;

  // check input
  if (vect_invalid(v))
    return NAN;

  // set up output
  s = 0;

  // loop through vect
  for (vindex i = 0; i < v->length; i++)
  {
    s += (vdata)pow((double)v->data[i], (double)2);
  }

  // return output
  n = sqrt(s);
  return n;
}

/*----------------------------------------------------------------------------
  linspace
------------------------------------------------------------------------------
  generate a vect of n linearly spaced points in the range a to b
    a :  start of range
    b :  end of range
    n :  number of points
    -
    l :  vect of linearly spaced values
-----------------------------------------------------------------------------*/
vect *vect_linspace(vdata a, vdata b, vindex n)
{
  vect *ep;
  vect *l;

  // construct endpoint vector
  ep = vect_init(2);
  ep->data[0] = a;
  ep->data[1] = b;

  // check input
  if (n <= 2)
    return ep;

  // calculate output
  l = vect_interp(ep, n - 1);
  if (vect_invalid(l))
    return NULL;

  // clean up
  vect_free(ep);

  // return output
  return l;
}

/*----------------------------------------------------------------------------
  logspace
------------------------------------------------------------------------------
  generate a vect of n logarithmically spaced points in the range a to b
    a :  start of range
    b :  end of range
    n :  number of points
    -
    l :  vect of linearly spaced values
-----------------------------------------------------------------------------*/
vect *vect_logspace(vdata a, vdata b, vindex n)
{
  vect *l;
  vect *ex;

  // calculate output
  l = vect_linspace(a, b, n);
  if (vect_invalid(l))
    return NULL;

  // calculate power of l.^10
  ex = vect_cpow(10, l);

  // clean up
  vect_free(l);

  // return output
  return ex;
}

/*----------------------------------------------------------------------------
  var
------------------------------------------------------------------------------
  compute the variance of a vect
    v :  vect
    -
    x :  calculated variance
-----------------------------------------------------------------------------*/
vdata vect_var(vect *v)
{
  vdata m, s, x;
  vect *d, *p;

  // check input
  if (vect_invalid(v))
    return NAN;

  // compute variance
  m = vect_mean(v);
  d = vect_subc(v, m);
  p = vect_powc(d, 2);
  s = vect_sum(p);
  x = s / (v->length - 1);

  // clean up
  vect_free(d, p);

  return x;
}

/*----------------------------------------------------------------------------
  std
------------------------------------------------------------------------------
  compute the standard deviation of a vect
    v :  vect
    -
    s :  calculated standard deviation
-----------------------------------------------------------------------------*/
vdata vect_std(vect *v)
{
  vdata s, va;

  if (vect_invalid(v))
    return NAN;

  // compute std from var
  va = vect_var(v);
  s = (vdata)sqrt(va);

  return s;
}

/*============================================================================
                             _private_functions_
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  _primitive
----------------------------------------------------------------------------*/
vdata _add(vdata a, vdata b) { return a + b; }
vdata _sub(vdata a, vdata b) { return a - b; }
vdata _mul(vdata a, vdata b) { return a * b; }
vdata _div(vdata a, vdata b) { return a / b; }
vdata _max(vdata a, vdata b) { return max(a, b); }
vdata _min(vdata a, vdata b) { return min(a, b); }
vdata _cp(vdata a) { return a; }
vdata _isinf(vdata a) { return isinf(a); }
vdata _isnan(vdata a) { return isnan(a); }
vdata _sign(vdata a) { return ((signbit(a)) ? (-1) : (1)); }
vdata _db2mag(vdata a) { return 20 * log10(a); }
vdata _mag2db(vdata a) { return pow(10, a / 20); }

vdata _rnd(void)
{
  static int first = 1;

  if (first)
  {
    srand(time(0)); // seed prng
    for (unsigned int i = 0; i < 128; i++)
      rand(); // warm up prng
    first = 0;
  }

  return ((vdata)rand()) / RAND_MAX;
}

/*----------------------------------------------------------------------------
  _void_vargs
------------------------------------------------------------------------------
  call function on multiple vects
    func :  function to run on each vect - must have void return type
    argc :  vect count
-----------------------------------------------------------------------------*/
void _vect_void_vargs(void (*func)(vect *), unsigned char argc, ...)
{
  vect *v;
  va_list argv;

  // parse input arguments
  va_start(argv, argc);
  for (int i = 0; i < argc; i++)
  {
    v = (vect *)va_arg(argv, vect *);
    func(v);
  }
  va_end(argv);

  return;
}

/*----------------------------------------------------------------------------
  _gen_0param
------------------------------------------------------------------------------
  generate a vector of length l using one parameter p
    func :  function to generate each element - must have numeric return type
    l    :  vector to operate on
    p    :  paramater
    -
    r    :  vector to return
-----------------------------------------------------------------------------*/
vect *_vect_gen_0param(vdata (*func)(void), vindex l)
{
  vect *r;

  // check inputs
  if ((l <= 0) || (func == NULL))
    return NULL;

  // init output
  r = vect_init(l);
  if (vect_invalid(r))
    return NULL;

  // parse vector
  for (int i = 0; i < l; i++)
    r->data[i] = (vdata)func();

  return r;
}

/*----------------------------------------------------------------------------
  _gen_1param
------------------------------------------------------------------------------
  generate a vector of length l using one parameter p
    func :  function to generate each element - must have numeric return type
    l    :  vector to operate on
    p    :  paramater
    -
    r    :  vector to return
-----------------------------------------------------------------------------*/
vect *_vect_gen_1param(vdata (*func)(vdata), vindex l, vdata p)
{
  vect *r;

  // check inputs
  if ((l > 0) || (func == NULL))
    return NULL;

  // init output
  r = vect_init(l);
  if (vect_invalid(r))
    return NULL;

  // parse vector
  for (int i = 0; i < l; i++)
    r->data[i] = (vdata)func(p);

  return r;
}

/*----------------------------------------------------------------------------
  _for_none
------------------------------------------------------------------------------
  call function on every element of a vect with no other parameters
    func :  function to run on each element - must have numeric return type
    v    :  vector to operate on
    -
    r    :  vector to return
-----------------------------------------------------------------------------*/
vect *_vect_for_0param(vdata (*func)(vdata), vect *v)
{
  vect *r;

  // check inputs
  if (vect_invalid(v) || (func == NULL))
    return NULL;

  // init output
  r = vect_init(v->length);
  if (vect_invalid(r))
    return NULL;

  // parse vector
  for (int i = 0; i < v->length; i++)
    r->data[i] = (vdata)func(v->data[i]);

  return r;
}

/*----------------------------------------------------------------------------
  _for_1param
------------------------------------------------------------------------------
  call function on every element of a vect with constant a second parameter
    func :  function to run on each element - must return numeric and
            operate on (vdata, vdata) inputs
    v    :  vector to operate on
    c    :  additional parameter
    -
    r    :  vector to return
-----------------------------------------------------------------------------*/
vect *_vect_for_1param(vdata (*func)(vdata, vdata), vect *v, vdata c)
{
  vect *r;

  // check inputs
  if (vect_invalid(v) || (func == NULL))
    return NULL;

  // init output
  r = vect_init(v->length);
  if (vect_invalid(r))
    return NULL;

  // parse vector
  for (int i = 0; i < v->length; i++)
    r->data[i] = (vdata)func(v->data[i], c);

  return r;
}

/*----------------------------------------------------------------------------
  _1param_for
------------------------------------------------------------------------------
  call function on constant with element as second parameter
    func :  function to run on each element - must return numeric and
            operate on (vdata, vdata) inputs
    c    :  constant parameter
    v    :  vector
    -
    r    :  vector to return
-----------------------------------------------------------------------------*/
vect *_vect_1param_for(vdata (*func)(vdata, vdata), vdata c, vect *v)
{
  vect *r;

  // check inputs
  if (vect_invalid(v) || (func == NULL))
    return NULL;

  // init output
  r = vect_init(v->length);
  if (vect_invalid(r))
    return NULL;

  // parse vector
  for (int i = 0; i < v->length; i++)
    r->data[i] = (vdata)func(c, v->data[i]);

  return r;
}

/*----------------------------------------------------------------------------
  _for_vect
------------------------------------------------------------------------------
  call two-param function elementwise on every element of v and u
    func :  function to run on each element - must return numeric and
            operate on (vdata, vdata) inputs
    v    :  first vector to operate on
    u    :  second vector to operate on
    -
    r    :  vector to return (r->length = min(v->length, u->length))
-----------------------------------------------------------------------------*/
vect *_vect_for_vect(vdata (*func)(vdata, vdata), vect *v, vect *u)
{
  vect *r;
  vindex l;

  // check inputs
  if (vect_invalid(v) || vect_invalid(u) || (func == NULL))
    return NULL;

  l = min(v->length, u->length);

  // init outputs
  r = vect_init(l);
  if (vect_invalid(r))
    return NULL;

  // parse vectors
  for (int i = 0; i < l; i++)
    r->data[i] = (vdata)func(v->data[i], u->data[i]);

  return r;
}

/*----------------------------------------------------------------------------
  _free
------------------------------------------------------------------------------
  free a single vect
    v :  vect to free
-----------------------------------------------------------------------------*/
void _vect_free(vect *v)
{
  if (v == NULL)
    return;

  free(v->data);
  v->data = NULL;
  free(v);

  return;
}
