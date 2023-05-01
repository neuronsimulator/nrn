#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: romberg.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "errcodes.hpp"
#include "newton_struct.h"
#include "scoplib.h"

#include <cmath>

using namespace neuron::scopmath;  // for errcodes.hpp
/****************************************************************/
/*								*/
/*  Abstract: romberg()						*/
/*								*/
/*    Romberg quadrature over interval [a, b].  Toler is the	*/
/*    convergence criterion.  Func is the function to be inte-	*/
/*    grated.  The result is stored in integral.		*/
/*								*/
/*  Calling sequence: romberg(a, b, func)			*/
/*								*/
/*  Returns: value of the integral				*/
/*								*/
/*  Arguments:							*/
/*    Input:	a, double	beginning of the integration	*/
/*				interval			*/
/*								*/
/*		b, double	end of the integration interval	*/
/*								*/
/*		func		name of the function to be	*/
/*				integrated			*/
/*								*/
/*  Functions called: func(x), supplied by user			*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/
#define CONTINUE 0
double romberg(double a, double b, double (*func)(double)) {
    int i, nhalve, nsteps, status = CONTINUE;
    double h, FA, store1, store2, *T, integral;

    /* Allocate storage for Romberg tableau */

    T = makevector(MAXHALVE + 1);

    /* First estimate of integral from elementary trapezoid rule */

    nhalve = 0;
    h = b - a;
    integral = T[0] = FA = ((*func)(a) + (*func)(b)) / 2.0;
    nsteps = 1;

    while (status == CONTINUE) {
        nhalve++;
        h /= 2.0;
        nsteps *= 2;
        store1 = T[0];

        /* Get new estimate of integral by composite trapezoid rule */

        T[0] = FA;
        for (i = 1; i < nsteps; i++)
            T[0] += (*func)(a + i * h);
        T[0] = h * T[0];

        /* Richardson extrapolation */

        for (i = 1; i <= nhalve; i++) {
            store2 = T[i];
            T[i] = T[i - 1] + (T[i - 1] - store1) / (pow(4., (double) i) - 1.0);
            store1 = store2;
        }

        /* Check for convergence or maximum iterations */

        if (fabs(T[nhalve] - integral) <= CONVERGE)
            status = SUCCESS;
        else if (nhalve >= MAXHALVE)
            status = EXCEED_ITERS;
        integral = T[nhalve];
    }

    freevector(T);
    if (status != SUCCESS) {
        abort_run(status);
    }
    return (integral);
}
