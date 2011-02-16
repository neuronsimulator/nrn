#include <../../nrnconf.h>
/* (C) Copr. 1986-92 Numerical Recipes Software #.,. */

/* Algorithms for Fourier Transform Spectral Methods */

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#undef myfabs
#if MAC
#if __GNUC__ < 4
#define myfabs std::fabs
#else
#define myfabs ::fabs
#endif
#else
#define myfabs fabs
#endif

#define SQUARE(a) ((x_=(a)) == 0.0 ? 0.0 : x_*x_)

extern "C" {
void hoc_execerror(const char*, const char*);
extern void nrn_exit(int);
}

#include "nrngsl_real_radix2.c"
#include "nrngsl_hc_radix2.c"

void nrngsl_realft(double* data, unsigned long n, int direction) {
	if (direction == 1) {
		nrngsl_fft_real_radix2_transform(data, 1, n);
	}else{
		nrngsl_fft_halfcomplex_radix2_inverse(data, 1, n);
	}
}

/* 
  convlv()  -- convolution of a read data set with a response function
  the respns function must be stored in wrap-around order 
*/

/* frequency domain format copy from gsl fft to NRC fft */
void nrn_gsl2nrc(double* x, double* y, unsigned long n) {
	unsigned long i, n2;
	n2 = n/2;
	y[0] = x[0];
	if (n < 2) { return; }
	y[1] = x[n2];
	for (i=1; i < n2; ++i) {
		y[2*i] = x[i];
		y[2*i+1] = -x[n-i];
	}
}
void nrn_nrc2gsl(double* x, double* y, unsigned long n) {
	double xn, xn2;
	unsigned long i, n2;
	n2 = n/2;
	xn = n;
	xn2 = .5*xn;
	y[0] = x[0]*xn2;
	if (n < 2) { return; }
	y[n2] = x[1]*xn2;
	for (i=1; i < n2; ++i) {
		y[i] = x[2*i]*xn2;
		y[n-i] = -x[2*i+1]*xn2;
	}
}

void nrn_convlv(double* data, unsigned long n, double* respns, unsigned long m,
	int isign, double* ans)
{
	// data and respns are modified.
	unsigned long i;
	double scl, x_;

	int n2 = n/2;
	for (i=1;i<=(m-1)/2;i++) {
		respns[n-i]=respns[m-i];
	}
	for (i=(m+1)/2;i < n-(m-1)/2;i++) {
		respns[i]=0.0;
	}
	nrngsl_realft(data, n, 1);
	nrngsl_realft(respns, n, 1);
	ans[0] = data[0]*respns[0];
	for (i=1; i < n2; ++i) {
		if (isign == 1) {
			ans[i] = data[i]*respns[i] - data[n-i]*respns[n-i];
			ans[n-i] = data[i]*respns[n-i] + data[n-i]*respns[i];
		} else if (isign == -1) {
			if ((scl = SQUARE(ans[i-1])+SQUARE(ans[i])) == 0.0) {
hoc_execerror("Deconvolving at response zero in nrn_convlv", 0);
			}
			scl *= 2.0;
			ans[i] = (data[i]*respns[i] + data[n-i]*respns[n-i])/scl;
			ans[i]=(data[i]*respns[n-i] - data[n-i]*respns[i])/scl;
		} else {
hoc_execerror("No meaning for isign in nrn_convlv", 0);
		}
	}
	ans[n2] = data[n2]*respns[n2];
	nrngsl_realft(ans, n, -1);
}


/* 
  correl()  -- correlation of two real data sets
  see http://www.cv.nrao.edu/course/astr534/FourierTransforms.html SF16
*/
void nrn_correl(double* x, double* y, unsigned long n, double* z)
{
	nrngsl_realft(x, n, 1);
	nrngsl_realft(y, n, 1);
	z[0] = x[0]*y[0];
	int n2 = n/2;
	for (int i=1; i < n2; ++i) {
		z[i] = x[i]*y[i] + x[n-i]*y[n-i];
		// with following sign, produces same result as old NRC code
		z[n-i] = y[i]*x[n-i] - y[n-i]*x[i];
	}
	z[n2] = x[n2]*y[n2];
	nrngsl_realft(z, n, -1);
}


/* 
  spctrm()  -- power spectrum using fourier transforms
  modified to take array rather than data stream.
*/

#define WINDOW(j,a,b) (1.0-myfabs((((j)-1)-(a))*(b)))       /* Bartlett */

void nrn_spctrm(double* data, double* p, int m, int k)
{
  hoc_execerror("nrn_spctrm in ivoc/fourier.cpp not implemented", 0);
}
#undef WINDOW
