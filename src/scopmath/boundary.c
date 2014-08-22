#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: boundary.c
 *
 * Copyright (c) 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "boundary.c,v 1.1.1.1 1994/10/12 17:22:19 hines Exp" ;
#endif

/*--------------------------------------------------------------*/
/*								*/
/*    BOUNDARY							*/
/*								*/
/*    Solution of the second order linear boundary value	*/
/*    problem							*/
/*								*/
/*		y" + f(x)y' + g(x)y = q(x)			*/
/*								*/
/*    using the finite differences method with npts mesh points.*/
/*    and user-supplied functions f, g, and q.			*/
/*    y(x[0]) and y(x[npts-1]) are known, and boundary computes	*/
/*    the values of y at the interior mesh points.		*/
/*								*/
/*  Returns: 0 if no error; 2 if tridiagonal matrix is singular	*/
/*			      or ill-conditioned		*/
/*								*/
/*  Calling sequence: boundary(npts, x, y, f, g, q)		*/
/*								*/
/*  Arguments:							*/
/*    Input:	npts	number of mesh points			*/
/*								*/
/*		x	double precision array of mesh points	*/
/*								*/
/*		y	double precision array of function	*/
/*			values at the mesh points.  Only the	*/
/*			values at the terminal points are known	*/
/*								*/
/*		f, g,	names of user-supplied double precision	*/
/*		q	functions with argument (x[i])		*/
/*								*/
/*    Output:	y	the function values at the interior	*/
/*			mesh points are included		*/
/*								*/
/*  Functions called: makevector(),freevector(), tridiag(),	*/
/*			f(xvalue), g(xvalue), q(xvalue)		*/
/*								*/
/*--------------------------------------------------------------*/
#include "scoplib.h"
int boundary(npts, x, y, f, g, q)
int npts;
double x[], y[], (*f) (), (*g) (), (*q) ();
{
    extern int diag(), freevector();
    extern double *makevector();
    int i, mesh, error;
    double *a, *b, *c, *d, h, temp;

    /* Calculate stepsize */

    h = x[1] - x[0];
    mesh = npts - 2;

    /* Allocate storage for tridiagonal system */

    a = makevector(mesh);
    b = makevector(mesh);
    c = makevector(mesh);
    d = makevector(mesh);

    /* Evaluate tridiagonal matrix and constant vector */

    for (i = 0; i < mesh; i++)
    {
	temp = (*f) (x[i + 1]) * h / 2.0;
	a[i] = 1. - temp;	/* subdiagonal elements */
	b[i] = -2. + (*g) (x[i + 1]) * h * h;	/* diagonal elements */
	c[i] = 1. + temp;	/* superdiagonal elements */
	d[i] = (*q) (x[i + 1]) * h * h;	/* constant vector */
    }

    /* Add end corrections */

    d[0] -= a[0] * y[0];
    d[mesh - 1] -= c[mesh - 1] * y[npts - 1];

    /* Solve equations and load solution into y array */

    error = tridiag(mesh, a, b, c, d, y + 1);

    freevector(a);
    freevector(b);
    freevector(c);
    freevector(d);

    return (error);
}
