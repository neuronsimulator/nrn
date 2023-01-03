#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: tridiag.c
 *
 * Copyright (c) 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "errcodes.hpp"
#include "scoplib.h"

#include <cmath>
#include <cstdlib>

using namespace neuron::scopmath; // for errcodes.hpp
/****************************************************************/
/*								*/
/*  Abstract: tridiag()						*/
/*								*/
/*    Solves a system of simultaneous linear equations whose	*/
/*    coefficients form a tridiagonal matrix.  The vectors	*/
/*    containing the elements of the three diagonals are	*/
/*    destroyed in the process.  TOLER is the minimal value a	*/
/*    diagonal element can be without being considered zero	*/
/*    (currently set to 1.e-20).				*/
/*								*/
/*  Calling sequence:  tridiag(n, a, b, c, d, soln)		*/
/*								*/
/*  Arguments:							*/
/*    Input:	abs(n), int	number of equations; if n > 0,	*/
/*				perform LU decomposition first	*/
/*		a[], double	elements of the subdiagonal	*/
/*		b[], double	elements of the main diagonal	*/
/*		c[], double	elements of the superdiagonal	*/
/*		d[], double	elements of constant vector	*/
/*								*/
/*  Returns: 0 if no error; 2 if tridiagonal matrix is singular	*/
/*			      or ill-conditioned		*/
/*								*/
/*  Output:	*soln[], double	solution vector			*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/
int tridiag(int n, double* a, double *b, double* c, double* d, double* soln) {
    int k, nn;
    double denom;

    nn = abs(n);

    /* Perform LU decomposition on tridiagonal matrix */

    for (k = 1; k < nn; ++k)
    {
	/* Check for singularity */
	denom = b[k - 1];
	if (fabs(denom) < ROUNDOFF)
	    return (SINGULAR);

	/* Skip lower triangle if matrix unchanged */

	if (n > 0)
	{
	    /* Note that a[0] and c[nn-2] are not present in matrix */
	    a[k] /= denom;
	    b[k] -= a[k] * c[k - 1];
	}

	/* Transform constant vector */

	d[k] -= a[k] * d[k - 1];
    }

    /* Perform back substitution */

    soln[nn - 1] = d[nn - 1] / b[nn - 1];
    for (k = nn - 2; k >= 0; --k)
	soln[k] = (d[k] - c[k] * (soln[k + 1])) / b[k];

    return (SUCCESS);
}
