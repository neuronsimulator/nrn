: width = yvec.width(xvec, yval)  sensitive to noise in yvec
NEURON {
	SUFFIX nothing
}

VERBATIM
#include "hocdec.h"
static double width(void* vv) {
	int i, nx, ny, i1, i2;
	double *x, *y;
	double h;
	h = *getarg(2);
	ny = vector_instance_px(vv, &y);
	nx = vector_arg_px(1, &x);
	if (nx != ny) return 0.;
	for (i1 = 0; i1 < nx; ++i1) {
		if (y[i1] >= h) { break;}
	}
	for (i2 = i1+1; i2 < nx; ++i2) {
		if (y[i2] <= h) { break; }
	}
	return x[i2] - x[i1];
}

/* xval, yval must be sorted in increasing order for xval */
/* xval is relative to the peak of the data */
/* xpeak is the peak location of the simulation */
/* xfitness is a measure of match in the x-dimension relative to
peak location for particular values of y */
/* yfitness is a measure of match in the y-dimension relative to
peak location for particular values of relative x */

static double xfitness(void* vv) {
	int nx, ny, nyval, nxval, i, j;
	double sum, d, xpeak, *y, *x, *yval, *xval;
	ny = vector_instance_px(vv, &y);
	nx = vector_arg_px(1, &x);
	if (nx != ny) { hoc_execerror("vectors not same size", 0); }
	xpeak = *getarg(2);
	nyval = vector_arg_px(3, &yval);
	nxval = vector_arg_px(4, &xval);
	j = 0;
	sum = 0.;
	for (i = 0; i < nx; ++i) {
		if (y[i] >= yval[j]) {
			d = (x[i] - xpeak) - xval[j];
			sum += d*d;
			++j;
			if (j >= nxval) return sum;
			if (x[i] > xpeak) break;
		}
	}
	for (++i; i < nx; ++i) {
		if (y[i] <= yval[j]) {
			d = (x[i] - xpeak) - xval[j];
			sum += d*d;
			++j;
			if (j >= nxval) return sum;
		}
	}
	return 1e9;
}
static double yfitness(void* vv) {
	int nx, ny, nyval, nxval, i, j;
	double sum, d, xpeak, *y, *x, *yval, *xval;
	ny = vector_instance_px(vv, &y);
	nx = vector_arg_px(1, &x);
	if (nx != ny) { hoc_execerror("vectors not same size", 0); }
	xpeak = *getarg(2);
	nyval = vector_arg_px(3, &yval);
	nxval = vector_arg_px(4, &xval);
	j = 0;
	sum = 0.;
	for (i = 0; i < nx; ++i) {
		if (x[i] - xpeak >= xval[j]) {
			d = y[i] - yval[j];
			sum += d*d;
			++j;
			if (j >= nxval) return sum;
		}
	}
	return 1e9;
}


static double firstpeak(void* vv) {
	int ny, i;
	double *y;
	ny = vector_instance_px(vv, &y) - 1;
	i = 0;
	while (i < ny) {
		if (y[i] >= -20) {
			if (y[i] > y[i+1]) {
				return (double) i;
			}
			i = i + 1;
		} else {
			i = i + 2;
		}
	}
	return 0.;
}
ENDVERBATIM

PROCEDURE install_vector_fitness() {
VERBATIM
  {static int once; if (!once) { once = 1;
	install_vector_method("width", width);
	install_vector_method("xfitness", xfitness);
	install_vector_method("yfitness", yfitness);
	install_vector_method("firstpeak", firstpeak);
  }}
ENDVERBATIM
}
