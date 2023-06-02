/*=============================================================================
                                    vector.h
-------------------------------------------------------------------------------
numpy-style numerical analysis with 1d arrays

© Daniel Wilkinson-Thompson 2023
daniel@wilkinson-thompson.com
TODO
  - move function descriptions to header file
  - complete the following functions
  - integral
  - acorr
  - corr
  - bessel
  - search
  - fit
  - cumprod
  - delay
  - trapz
  - bisect
  - newton-raphson
  - find
  - sort
  - rise_time
  - settling_time
  - thd
  - snr (std/mean)
  - fft
  - ifft
  - spect
  - pwelch
  - fwhm
  - convolve
  - deconvolve
  - shift
  -
-----------------------------------------------------------------------------*/
#ifndef __vect_h
#define __vect_h
#include <stdio.h>  // debugging
#include <stdint.h> // types
#include <stdarg.h> // variable argument functions

/*----------------------------------------------------------------------------
  typdefs
-----------------------------------------------------------------------------*/
typedef double vdata;        // vect precision
typedef unsigned int vindex; // vect size
typedef struct vect_t        // vect-type
{
  vdata *data;
  vindex length;
} vect;

/*----------------------------------------------------------------------------
  function macros
-----------------------------------------------------------------------------*/
// debugging
#ifndef DEBUG_FORMAT
#define DEBUG_FORMAT "%2.3f\n"
#endif
#define vect_debug_value(v)              \
  fprintf(stderr, #v " = \n");           \
  vect_fprintf(stderr, DEBUG_FORMAT, v); \
  fprintf(stderr, "\n\n");
#define vect_debug_command(v)       \
  fprintf(stderr, ">> " #v "\n\n"); \
  v;

// varg functions
#define _nobjs(...) (sizeof((vect *[]){__VA_ARGS__}) / sizeof(vect *))
#define vect_free(...) _vect_void_vargs(_vect_free, _nobjs(__VA_ARGS__), __VA_ARGS__)

// validation
#define vect_invalid(v) ((v == NULL) || (v->data == NULL) || (v->length == 0))

// type-independent min/max
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/*----------------------------------------------------------------------------
  prototypes
-----------------------------------------------------------------------------*/
// standard
vect *vect_init(vindex length);
void vect_printf(const char *fmt, vect *v);
void vect_fprintf(FILE *f, const char *fmt, vect *v);
// void vect_free(...) - vargs macro

// file io
vect **vect_read_csv(const char *filename, uint32_t *cols);
void vect_write_csv(vect **v, const char *filename, uint32_t cols);

// generation
vect *vect_zeros(vindex l);
vect *vect_ones(vindex l);
vect *vect_rand(vindex l);
vect *vect_const(vindex l, vdata c);
vect *vect_linspace(vdata a, vdata b, vindex n);
vect *vect_logspace(vdata a, vdata b, vindex n);
vect *vect_copy(vect *v);

// logarithmic
vect *vect_exp(vect *v);
vect *vect_exp2(vect *v);
vect *vect_exp10(vect *v);
vect *vect_log(vect *v);
vect *vect_log2(vect *v);
vect *vect_log10(vect *v);

// power
vect *vect_pow(vect *v, vect *u);
vect *vect_powc(vect *v, vdata p);
vect *vect_cpow(vdata c, vect *v);
vect *vect_hypot(vect *v, vect *u);
vect *vect_sqrt(vect *v);
vect *vect_cbrt(vect *v);

// algebraic
vect *vect_abs(vect *v);
vect *vect_mul(vect *v, vect *u);
vect *vect_div(vect *v, vect *u);
vect *vect_add(vect *v, vect *u);
vect *vect_sub(vect *v, vect *u);
vect *vect_addc(vect *v, vdata c);
vect *vect_subc(vect *v, vdata c);
vect *vect_divc(vect *v, vdata c);
vect *vect_mulc(vect *v, vdata c);

// statistics
vdata vect_sum(vect *v);
vect *vect_cumsum(vect *v);
vdata vect_prod(vect *v);
// vdata vect_cumprod(vect *v);
vdata vect_norm(vect *v);
vdata vect_mean(vect *v);
vdata vect_std(vect *v);
vdata vect_var(vect *v);
vdata vect_max(vect *v, vindex *ind);
vdata vect_min(vect *v, vindex *ind);
vdata vect_range(vect *v);

// rounding
vect *vect_round(vect *v);
vect *vect_ceil(vect *v);
vect *vect_floor(vect *v);
vect *vect_rem(vect *v, vdata c);
vect *vect_mod(vect *v, vdata c);

// trigonometry
vect *vect_sin(vect *v);
vect *vect_cos(vect *v);
vect *vect_tan(vect *v);
vect *vect_asin(vect *v);
vect *vect_acos(vect *v);
vect *vect_atan(vect *v);
vect *vect_sinh(vect *v);
vect *vect_cosh(vect *v);
vect *vect_tanh(vect *v);
vect *vect_atan2(vect *v, vect *u);

// square
// tri
// chirp

// error/gamma functions
vect *vect_erf(vect *v);
vect *vect_erfc(vect *v);
vect *vect_lgamma(vect *v);
vect *vect_tgamma(vect *v);

// classification
vect *vect_isinf(vect *v);
vect *vect_isnan(vect *v);
vect *vect_sign(vect *v);
// vect* vect_find(vect *v);

// resampling
vect *vect_interp(vect *v, vindex i);
vect *vect_decimate(vect *v, vindex m);
vect *vect_resample(vect *v, vindex i, vindex m);

// calculus
vect *vect_diff(vect *v);
// vect* vect_trapz(vect *v);
//

//
vect *vect_flip(vect *v);
// shift
// sort

// filtering
vect *vect_smooth(vect *v, vindex n);
// vect* vect_bessel2(vect *v, vindex n);
// vect* vect_bessel4(vect *v, vindex n);

// sets
vect *vect_subset(vect *v, vindex l, vindex u);
vect *vect_concat(vect *v, vect *u);
vect *vect_insert(vect *v, vect *u, vindex s);
// split
// window

// analysis
vect *vect_xcorr(vect *v, vect *u, vindex s);
vdata vect_find_shift(vect *v, vect *u, vindex r, unsigned int i);
vdata vect_corr(vect *v, vect *u);
vindex vect_trigger(vect *v, vdata t, int s);
vindex vect_peaks(vect *v, vdata t, vindex n, vect *m, vindex *i);
// vindex vect_search();
// vindex vect_bisect();
// vindex vect_newton();

// dsp
// conv
// deconv

// vect* vect_ets_fold(vect *v);

#endif
