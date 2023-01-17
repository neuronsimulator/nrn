#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: expfit.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "deflate.hpp"
#include "errcodes.hpp"
#include "../oc/nrnassrt.h"
#include "scoplib.h"

#include <cmath>
#include <cstdio>

using namespace neuron::scopmath; // for errcodes.hpp
/****************************************************************
 *
 *  Abstract: expfit()
 *
 *    Exponential curve peeling by Handscomb's modification of
 *    Prony's method.  Only the real poles are found; the
 *    complex solutions are artifactual oscillatory components
 *    resulting from trying to fit the noise in the data.  The
 *    data must be sampled at equal intervals and stored in a
 *    reference data file.  There must be at least two more data
 *    points than the maximal number of exponentials to be fit
 *    to the data.
 *
 *    This function should be invoked in initmodel() and the function
 *
 *	y = A[0]*exp(k[0]*t) + ... + A[terms-1]*exp(k[terms-1]*t)
 *
 *    evaluated in model().
 *
 *  Calling sequence: expfit(terms, reffile, amplitude, lambda, error)
 *
 *  Arguments:
 *    Input:	*terms		double	maximum number of terms
 *					to try to fit.  If terms
 *					< 0, discard exponentials
 *					with amplitudes < error.
 *		*reffile	char	name of reference data file
 *    Output:	amplitude[]	double	pre-exponential coefficients
 *		lambda[]	double	time constants (Note that
 *					the zeroth elements of the
 *					amplitude and time constant
 *					arrays are used on return.)
 *		*error		double	standard error of estimate
 *					(Set to -1 if terms > 0)
 *
 *  Functions called: expinit(), simeq(), deflate(), testfit(),
 *		makevector(),freevector(),makematrix(),freematrix(),
 *		abs()
 *
 *  Returns: integer error code
 *
 *  Files accessed: reffile
 *
 ****************************************************************/
static int expinit(char* filename, double *deltat, double **data);
static int testfit(int ndata, double* data, double* terms, double* amplitude, double* lambda, double h, double* errfit);
int expfit(double* terms, char* reffile, double* amplitude, double* lambda, double* error) {
    int i, j, k, npts, dimen, ierr;
    double h, *x, **L, *coeff, *work;

    /*
     * Read in data file; expinit() returns number of data points
     */

    if ((npts = expinit(reffile, &h, &x)) <= 0)
	return (NODATA);

    if (*terms < 0.)
	dimen = (int) -(*terms - 0.1);
    else
	dimen = (int) (*terms + 0.1);

    /* Allocate storage for arrays */

    L = makematrix(dimen, dimen + 1);
    work = makevector(dimen + 1);
    coeff = makevector(dimen + 1);

    /* Compute least squares matrix */

    for (i = 0; i < dimen; i++)
    {
	for (j = 0; j <= i; j++)
	{
	    L[i][j] = 0.0;
	    for (k = 1; k <= npts - dimen; k++)
		L[i][j] += x[i + k] * x[j + k];
	    if (i != j)
		L[j][i] = L[i][j];
	}

	/* Compute constant vector */

	L[i][dimen] = 0.0;
	for (k = 1; k <= npts - dimen; k++)
	    L[i][dimen] -= x[i + k] * x[k - 1];
    }

    /*
     * Solve for coefficients of characteristic equation. Then solve
     * characteristic equation for values of basis functions stored in
     * lambda[0] to lambda[dimen-1].  Deflate() returns the number of
     * exponentials with real time constants.
     */
    if ((ierr = simeq(dimen, L, work, (int *)0)) != SUCCESS)
	goto FINISH;

    coeff[0] = 1.0;
    for (i = 1; i <= dimen; i++)
	coeff[i] = work[i - 1];
    if ((dimen = deflate((double) dimen, coeff, lambda)) < 0)
    {
	ierr = -dimen;
	goto FINISH;
    }
    else if (dimen == 0)
    {
	ierr = NO_SOLN;
	goto FINISH;
    }

    /* Set up least squares matrix for amplitudes */

    for (i = 0; i < dimen; i++)
    {
	for (j = 0; j <= i; j++)
	{
	    L[i][j] = 1.0;
	    for (k = 1; k < npts - dimen; k++)
		L[i][j] += pow(lambda[i] * lambda[j], (double) k);
	    if (i != j)
		L[j][i] = L[i][j];
	}

	/* Compute constant vector */

	L[i][dimen] = x[0];
	for (k = 1; k < npts - dimen; k++)
	    L[i][dimen] += pow(lambda[i], (double) k) * x[k];
    }

    /* Solve for amplitudes */

    if ((ierr = simeq(dimen, L, work, (int *)0)) != SUCCESS)
	goto FINISH;

    for (i = 0; i < dimen; i++)
	amplitude[i] = work[i];

    /* Convert basis function values to time constants */

    for (i = 0; i < dimen; i++)
	if (lambda[i] <= 0.0)
	    /* Basis functions must be positive to take logarithm */
	    amplitude[i] = 0.0;
	else
	    lambda[i] = log(lambda[i]) / h;

    /*
     * If trial number of exponential terms is negative, compute the standard
     * error of estimate and reject any exponential term with an amplitude
     * less than that error.
     */
    if (*terms < 0.)
	testfit(npts, x, terms, amplitude, lambda, h, error);
    else
	*error = -1.0;
    if (*terms <= ZERO)
	ierr = NO_SOLN;

FINISH:
    freevector(coeff);
    freevector(x);
    freevector(work);
    freematrix(L);
    return (ierr);
}

/*--------------------------------------------------------------------------
 *  expinit()
 *
 *    Reads data file
 *
 *  Calling sequence: expinit(filename, deltat, data)
 *
 *  Arguments:
 *    Input:
 *	filename	char*		name of data file
 *    Output:
 *	deltat		double *	constant spacing of data points
 *	data		double **	array of ordinate values for curve
 *					to be decomposed into exponentials
 *
 * Returns:
 *
 * Functions called: fopen(), fclose(), fgets()
 *
 * Files accessed: filename (input)
 *------------------------------------------------------------------------*/
static int expinit(char* filename, double *deltat, double **data) {
    FILE *refdata;
    int i, npts = -6;
    double temp;
    char tmpstr[81];

    /*
     * Open reference data file.  Count data points and allocate storage for
     * data
     */

    if ((refdata = fopen(filename, "r")) == NULL)
	return (0);
    while (fgets(tmpstr, 80, refdata) != NULL)
	npts++;
    *data = makevector(npts);
    rewind(refdata);

    for (i = 0; i < 7; i++)
	nrn_assert(fgets(tmpstr, 80, refdata));
    sscanf(tmpstr, "%lf %lf", &temp, *data);
    nrn_assert(fgets(tmpstr, 80, refdata));
    sscanf(tmpstr, "%lf %lf", deltat, *data + 1);
    *deltat -= temp;
    for (i = 2; i < npts; i++)
    {
	nrn_assert(fgets(tmpstr, 80, refdata));
	sscanf(tmpstr, "%lf %lf", &temp, *data + i);
    }

    fclose(refdata);
    return (npts);
}

/*--------------------------------------------------------------------------
 *  testfit()
 *
 *    Rejects exponentials whose amplitudes are less than the standard error
 *    of estimate
 *
 *  Calling sequence:
 *	testfit(ndata, data, terms, amplitude, lambda, h, errfit)
 *
 *  Arguments:
 *    Input:
 *	ndata		int *		number of data points
 *	data		double *	array of ordinate values for curve to be
 *					decomposed into exponentials
 *	terms		double*		negative of number of exponential terms
 *	amplitude	double*		array of amplitudes of exponentials
 *	lambda		double*		array of decay constants of exponentials
 *	h		double		spacing of data points
 *    Output:
 *	terms		double*		number of exponential terms with
 *					amplitudes > standard error of estimate
 *	errfit		double*		standard error of estimate
 *
 * Returns:
 *
 * Functions called: exp(), sqrt(), fabs()
 *
 * Files accessed:
 *------------------------------------------------------------------------*/

static int testfit(int ndata, double* data, double* terms, double* amplitude, double* lambda, double h, double* errfit) {
    int n, i, j;
    double temp;

    /* Compute standard error of estimate of the fit to the data */

    n = (int) -(*terms - 0.1);
    *errfit = 0.0;
    for (i = 0; i < ndata; i++)
    {
	temp = 0.0;
	for (j = 0; j < n; j++)
	    temp += amplitude[j] * exp(lambda[j] * i * h);
	temp -= data[i];
	*errfit += temp * temp;
    }
    *errfit = sqrt(*errfit / (ndata - n - 1));

    /* Reject any exponential with an amplitude less than the standard error */

    for (i = 0; i < n; i++)
    {
	if (fabs(amplitude[i]) < *errfit)
	    amplitude[i] = 0.0;
	if (fabs(amplitude[i]) <= ZERO)
	{
	    /* Ripple down rest of amplitude and lambda arrays */
	    for (j = i; j < n; j++)
	    {
		amplitude[j] = amplitude[j + 1];
		lambda[j] = lambda[j + 1];
	    }
	    amplitude[n] = 0.0;
	    lambda[n] = 0.0;
	    n--;
	}
    }
    *terms = (double) n;
    return 0;
}
