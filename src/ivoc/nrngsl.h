#ifndef nrngsl_h
#define nrngsl_h

#define BASE double
#define GSL_ERROR(a,b) hoc_execerror(a, "b")
#define FUNCTION(a,b) nrn ## a ## _ ## b
#define ATOMIC double
#define VECTOR(a,stride,i) ((a)[(stride)*(i)])

int
FUNCTION(gsl_fft_halfcomplex,radix2_transform) (BASE data[],
                                                const size_t stride,
                                                const size_t n);
#endif
