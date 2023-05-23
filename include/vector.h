/*=============================================================================
                                    vector.h
-------------------------------------------------------------------------------
vector description

daniel@wilkinson-thompson.com
2014-07-18
-----------------------------------------------------------------------------*/
#ifndef __vector_h
#define __vector_h


/*----------------------------------------------------------------------------
                                   includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>   // debugging


/*----------------------------------------------------------------------------
                                   switches
 ----------------------------------------------------------------------------*/
#define DEBUG
#define VARGS
//#define TERSE


/*----------------------------------------------------------------------------
                              conditional includes
 ----------------------------------------------------------------------------*/
#ifdef VARGS
#include <stdarg.h>  // variable argument functions
#endif


/*----------------------------------------------------------------------------
                           constants, typdefs & enums
 ----------------------------------------------------------------------------*/
#ifndef PRECISION  // use '-DPRECISOIN=double' compiler flag to override
#define PRECISION float
#endif
#ifndef VECT_PREC  // use '-DVECT_PREC=double' compiler flag to override
#define VECT_PREC PRECISION
#endif

// vector data-type (configurable precision + vector length)
typedef struct vector_t {
  VECT_PREC   *data;  // configurable VECT_PREC data
  unsigned int length;
} vector;


/*----------------------------------------------------------------------------
                             function macros
 ----------------------------------------------------------------------------*/
/*--- debugging support ---*/
#ifdef  DEBUG
#ifndef DEBUG_FORMAT
#define DEBUG_FORMAT "%2.3f"
#endif
#define vector_debug_value(v) fprintf(stdout, #v " = \n"); vector_fprintf(v, stdout, DEBUG_FORMAT); fprintf(stdout, "\n\n");
#define vector_debug_command(v) fprintf(stdout, ">> " #v "\n\n"); v;
#endif

// check vector
#define vector_null(v) ((v == NULL) || (v->data == NULL) || (v->length == 0))

/*--- variable argument function support ---*/
#ifdef  VARGS
// variable argument helpers
#define _nargs(...) (sizeof((VECT_PREC[]){ __VA_ARGS__ })/sizeof(VECT_PREC))
#define _nobjs(...) (sizeof((vector*[]){ __VA_ARGS__ })/sizeof(vector*))
// variable argument function macros
#define vector_define(...)  _vector_def_vargs (_nargs( __VA_ARGS__ ), __VA_ARGS__ )
#define vector_free(...) _vector_void_vargs(_vector_free, _nobjs( __VA_ARGS__ ), __VA_ARGS__ )
#define vector_add(...)  _vector_casc_vargs(_vector_add, _nobjs( __VA_ARGS__ ), __VA_ARGS__ )
#define vector_subtract(...)  _vector_casc_vargs(_vector_sub, _nobjs( __VA_ARGS__ ), __VA_ARGS__ )
#define vector_multiply(...)  _vector_casc_vargs(_vector_mul, _nobjs( __VA_ARGS__ ), __VA_ARGS__ )
#define vector_divide(...)  _vector_casc_vargs(_vector_div, _nobjs( __VA_ARGS__ ), __VA_ARGS__ )


#else
// non-variable argument function macros
#define vector_free(v)      _vector_free(v);
#define vector_add(v)       _vector_add(v);
#define vector_subtract(v)  _vector_sub(v);
#define vector_multiply(v)  _vector_mul(v);
#define vector_divide(v)    _vector_div(v);
#endif

/*--- terse function aliases ---*/
#ifdef TERSE
// debug
#define vdbv(v)  vector_debug_value(v)
#define vdbc(v)  vector_debug_command(v)

// standard object functions
#define vini(v)  vector_initialise(v)
#define vdef(v)  vector_define(v)
#define vfre(v)  vector_free(v)
#define vspf(v)  vector_sprintf(v)
#define vfpf(v)  vector_fprintf(v)
#define vcpy(v)  vector_copy(v)

// quick define
#define vzer(v)  vector_zeros(v)
#define vone(v)  vector_ones(v)
#define vcon(v)  vector_constant(v)
#define vlin(v)  vector_linspace(v)
#define vlog(v)  vector_logspace(v)

// simple vector mathematics
#define vmin(v)  vector_minimum(v)
#define vmax(v)  vector_maximum(v)
#define vsum(v)  vector_sum(v)
#define vadd(v)  vector_add(v)
#define vsub(v)  vector_subtract(v)
#define vmul(v)  vector_multiply(v)
#define vdiv(v)  vector_divide(v)
#define vadc(v)  vector_add_constant(v)
#define vsuc(v)  vector_subtract_constand(v)
#define vmuc(v)  vector_multiply_constant(v)
#define vdic(v)  vector_divide_constant(v)

// data manipulation
#define vcat(v)  vector_concatenate(v)
#define vdec(v)  vector_decimate(v)
#define vinp(v)  vector_interpolate(v)
#define vres(v)  vector_resample(v)
#define vsat(v)  vector_saturate(v)
#define vset(v)  vector_subset(v)
#define vrss(v)  vector_replace_subset(v)

// data analysis
#define vdif(v)  vector_difference(v)
#define vfit(v)  vector_fit_function(v)
#define vint(v)  vector_integrate(v)
#define vmam(v)  vector_maxima
#define vmim(v)  vector_minima
#define vpek(v)  vector_peaks
#define vser(v)  vector_search(v)

// signal processing
#define vspm(v)  vector_spectrum(v)
#define vfft(v)  vector_fft(v)
#define vift(v)  vector_ifft(v)
#define vthd(v)  vector_thd(v)
#define vxco(v)  vector_cross_correlation(v)
#define vaco(v)  vector_autocorrelation(v)

// glue functions
#define vfun(v)  vector_function_evaluate(v)

#endif


/*----------------------------------------------------------------------------
                              standard functions
 ----------------------------------------------------------------------------*/
vector* vector_initialise(unsigned int length);         // create empty vector
void  vector_sprintf(vector *a, char *str, const char *format); // string
void  vector_fprintf(vector *a, FILE *f  , const char *format); // file


/*----------------------------------------------------------------------------
                           vector-specific functions
 ----------------------------------------------------------------------------*/
VECT_PREC vector_maximum(vector *a, unsigned int *ind);
VECT_PREC vector_minimum(vector *a, unsigned int *ind);
VECT_PREC vector_sum(vector *v);
//vector*   vector_add(vector *v, vector *u);



vector*   vector_cross_correlation(vector *a, vector *b);
vector*   vector_saturate(vector *a, VECT_PREC min, VECT_PREC max);
vector*   vector_decimate(vector *a, unsigned int divisor);
vector*   vector_linspace(VECT_PREC start, VECT_PREC stop, unsigned int pts);
vector*   vector_function_evaluate(vector *a, VECT_PREC (*func)(VECT_PREC));
vector*   vector_interpolate(vector *a, unsigned int multiplier);
VECT_PREC vector_find_shift(vector *v, vector *u);
vector*   vector_add_constant(vector *v, unsi)

/*
trim library to only those functions required for hymap-c


vector_find_shift
  vector_null
  vector_difference
  vector_interpolate
  vector_cross_correlation
  vector_maximum
  vector_free

** no need for overloaded functions **



initialise  - @implemented @tested
free        - @implemented
copy        - @implemented
fprintf     - @implemented @tested

zeros       - @implemented
multiply    - @implemented
divide_constant - @implemented

sum         - @implemented
absolute    - @implemented
maximum     - @implemented
subset      - @implemented
interpolate - @implemented
cross_correlation - @implemented
find_shift -- @implemented

difference  - @implemented

---

add_const   - @implemented
sub_const   - @implemented
mul_const   - @implemented
div_const   - @implemented

acorr
corr

logspace

zeros
ones
const -- name??
search // stdlib.h : void *bsearch(const void *key, const void *base, size_t nitems, size_t size, int (*compar)(const void *, const void *));
peaks
maxima
minima
ffit
thd
fft
spect
subset
concat
resample
norm
std
rand
copy
smooth
log
log10

*/


/*----------------------------------------------------------------------------
                              private functions
 ----------------------------------------------------------------------------*/
// standard
#ifdef VARGS
void  _vector_free_vargs(unsigned char argc, ... );
vector* _vector_def_vargs (unsigned char argc, ... );
void  _vector_void_vargs(void  (*func)(vector*), unsigned char argc, ... );
vector* _vector_casc_vargs(vector* (*func)(vector*, vector*), unsigned char argc, ... );
#endif

// vector-specific
void    _vector_free(vector *a);
vector* _vector_add (vector *a, vector *b);
vector* _vector_sub (vector *a, vector *b);
vector* _vector_mul (vector *a, vector *b);
vector* _vector_div (vector *a, vector *b);
vector* _vector_divide_constant(vector *a, )

#endif
