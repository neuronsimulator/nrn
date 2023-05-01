#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: crank.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "newton_struct.h"
#include "scoplib.h"

/*--------------------------------------------------------------*/
/*								*/
/*  Abstract: crank()						*/
/*								*/
/*    Sets up and solves the tridiagonal system of equations	*/
/*    for solution of parabolic partial differential equations	*/
/*								*/
/*	dy/dt = f(x, y, t)*d2y/dx2 + g(x, y, t)			*/
/*								*/
/*    by the Crank-Nicolson method.				*/
/*								*/
/*	A * y(t + dt) = B * y(t) + g(x, y, t) * dt		*/
/*								*/
/*    where A and B are tridiagonal matrices whose elements are	*/
/*    functions of the coefficient of the second order partial	*/
/*    derivative in the differential equation to be solved.	*/
/*								*/
/*  Returns: 0 if no error; 2 if tridiagonal matrix is singular	*/
/*			      or ill-conditioned		*/
/*								*/
/*  Calling sequence:						*/
/*		crank(n, y, fval, gval, dt, dx, t, bound)	*/
/*								*/
/*  Arguments							*/
/*    Input:	n, int		number of grid points		*/
/*								*/
/*		*y, double	current values of variables to	*/
/*				be solved for			*/
/*								*/
/*		*fval, double	values of f(x, y, t) at x	*/
/*								*/
/*		*gval, double	values of g(x, y, t) at x	*/
/*								*/
/*		dt, double	time step			*/
/*								*/
/*		dx, double	spacing of grid points		*/
/*								*/
/*		t, double	value of independent variable	*/
/*				else = 0			*/
/*								*/
/*		**bound, double	boundary conditions		*/
/*								*/
/*    Output:	*y, double	updated values of the state	*/
/*		   		variable at the grid points	*/
/*								*/
/*  Functions called: bounds(), tridiag(), makevector(),	*/
/*								*/
/*--------------------------------------------------------------*/
static int bounds(int n,
                  double* a,
                  double* b,
                  double* c,
                  double* d,
                  double* y,
                  double* fval,
                  double dt,
                  double dx,
                  double** bound);
int crank(int n,
          double* y,
          double* fval,
          double* gval,
          double dt,
          double dx,
          double t,
          double** bound,
          double** pwork) {
    int i, error;
    double temp, r;
    double *main_diag, *sub_diag, *sup_diag, *const_vec;

    if (!*pwork) {
        *pwork = makevector(4 * n);
    }
    main_diag = *pwork;
    sub_diag = main_diag + n;
    sup_diag = sub_diag + n;
    const_vec = sup_diag + n;

    temp = dt / (dx * dx);
    for (i = 0; i < n; i++) {
        r = temp * fval[i];
        /*
         * Compute diagonals of tridiagonal matrix A.  Note that sub_diag[0]
         * and sup_diag[n-1] are ignored in tridiag().
         */
        main_diag[i] = 2. + 2. * r;
        sub_diag[i] = sup_diag[i] = -r;

        /* Set up constant vector */

        const_vec[i] = (2. - 2. * r) * (y[i]) + 2. * gval[i] * dt;
        if (i > 0)
            const_vec[i] += r * (y[i - 1]);
        if (i < n - 1)
            const_vec[i] += r * (y[i + 1]);
    }

    /*
     * If there are derivative constraints at the boundaries, correct the
     * values of matrix and constant vector elements at the terminal grid
     * points
     */
    bounds(n, sub_diag, main_diag, sup_diag, const_vec, y, fval, dt, dx, bound);

    /* Solve tridiagonal system */

    error = tridiag(n, sub_diag, main_diag, sup_diag, const_vec, y);

    return (error);
}


/*-----------------------------------------------------------------------------
 *
 *  BOUNDS
 *
 *  Calling sequence: bounds(n, a, b, c, d, y, fval, dt, dx, bound)
 *
 *  Arguments:
 *	n	int	number of mesh points
 *	a[]	double	subdiagonal of tridiagonal matrix
 *	b[]	double	diagonal of tridiagonal matrix
 *	c[]	double	superdiagonal of tridiagonal matrix
 *	d[]	double	constant vector
 *	y[]	double	vector state variable
 *	fval[]	double	vector of values of f(x, y, t) at x
 *	dt	double	time step
 *	dx	double	spacing of mesh points
 *	**bound	double	boundary constraints
 *
 *  Returns:
 *
 *  Functions called:
 *
 *  Files accessed:
 *---------------------------------------------------------------------------*/

static int bounds(int n,
                  double* a,
                  double* b,
                  double* c,
                  double* d,
                  double* y,
                  double* fval,
                  double dt,
                  double dx,
                  double** bound) {
    int i;
    double temp;

    /*
     * Correct the matrix and constant vector elements corresponding to the
     * boundary conditions.
     */
    temp = dt / (dx * dx);
    for (i = 0; i < 4; i++) {
        if (bound[i] == (double*) 0)
            continue;

        switch (i) {
        case 0: /* dy[0]/dx = value */
            c[0] -= temp * fval[0];
            d[0] += temp * fval[0] * (y[1] - 4. * dx * (*bound[0]));
            break;

        case 1: /* dy[n-1]/dx = value */
            a[n - 1] -= temp * fval[n - 1];
            d[n - 1] += temp * fval[n - 1] * (y[n - 2] + 4. * dx * (*bound[1]));
            break;

        case 2: /* y[0] = bound */
            a[0] = 0.;
            b[0] = 1.;
            c[0] = 0.;
            d[0] = *bound[2];
            break;

        case 3: /* y[n-1] = bound */
            a[n - 1] = 0.;
            b[n - 1] = 1.;
            c[n - 1] = 0.;
            d[n - 1] = *bound[3];
            break;
        }
    }
    return 0;
}
