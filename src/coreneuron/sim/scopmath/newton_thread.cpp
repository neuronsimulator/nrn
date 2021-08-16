/******************************************************************************
 *
 * File: newton.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

/*------------------------------------------------------------*/
/*                                                            */
/*  NEWTON                                        	      */
/*                                                            */
/*    Iteratively solves simultaneous nonlinear equations by  */
/*    Newton's method, using a Jacobian matrix computed by    */
/*    finite differences.				      */
/*                                                            */
/*  Returns: 0 if no error; 2 if matrix is singular or ill-   */
/*			     conditioned; 1 if maximum	      */
/*			     iterations exceeded	      */
/*                                                            */
/*  Calling sequence: newton(n, x, p, pfunc, value)	      */
/*                                                            */
/*  Arguments:                                                */
/*                                                            */
/*    Input: n, integer, number of variables to solve for.    */
/*                                                            */
/*           x, pointer to array  of the solution             */
/*		vector elements	possibly indexed by index     */
/*                                                            */
/*	     p,  array of parameter values		      */
/*                                                            */
/*           pfunc, pointer to function which computes the    */
/*               deviation from zero of each equation in the  */
/*               model.                                       */
/*                                                            */
/*	     value, pointer to array to array  of             */
/*		 the function values.			      */
/*                                                            */
/*    Output: x contains the solution value or the most       */
/*               recent iteration's result in the event of    */
/*               an error.                                    */
/*                                                            */
/*  Functions called: makevector, freevector, makematrix,     */
/*		      freematrix			      */
/*		      buildjacobian, crout, solve	      */
/*                                                            */
/*------------------------------------------------------------*/

#include <math.h>
#include <stdlib.h>

#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"
#include "coreneuron/sim/scopmath/errcodes.h"
#include "coreneuron/sim/scopmath/newton_struct.h"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {
#define ix(arg) ((arg) *_STRIDE)
#define s_(arg) _p[s[arg] * _STRIDE]

int nrn_newton_thread(NewtonSpace* ns,
                      int n,
                      int* s,
                      NEWTFUN pfunc,
                      double* value,
                      _threadargsproto_) {
    int count = 0, error = 0;
    double change = 1.0, max_dev, temp;
    int done = 0;
    /*
     * Create arrays for Jacobian, variable increments, function values, and
     * permutation vector
     */
    double* delta_x = ns->delta_x;
    double** jacobian = ns->jacobian;
    int* perm = ns->perm;
    /* Iteration loop */
    while (!done) {
        if (count++ >= MAXITERS) {
            error = EXCEED_ITERS;
            done = 2;
        }
        if (!done && change > MAXCHANGE) {
            /*
             * Recalculate Jacobian matrix if solution has changed by more
             * than MAXCHANGE
             */

            nrn_buildjacobian_thread(ns, n, s, pfunc, value, jacobian, _threadargs_);
            for (int i = 0; i < n; i++)
                value[ix(i)] = -value[ix(i)]; /* Required correction to
                                               * function values */
            error = nrn_crout_thread(ns, n, jacobian, perm, _threadargs_);
            if (error != SUCCESS) {
                done = 2;
            }
        }

        if (!done) {
            nrn_scopmath_solve_thread(n, jacobian, value, perm, delta_x, (int*) 0, _threadargs_);

            /* Update solution vector and compute norms of delta_x and value */

            change = 0.0;
            if (s) {
                for (int i = 0; i < n; i++) {
                    if (fabs(s_(i)) > ZERO && (temp = fabs(delta_x[ix(i)] / (s_(i)))) > change)
                        change = temp;
                    s_(i) += delta_x[ix(i)];
                }
            } else {
                for (int i = 0; i < n; i++) {
                    if (fabs(s_(i)) > ZERO && (temp = fabs(delta_x[ix(i)] / (s_(i)))) > change)
                        change = temp;
                    s_(i) += delta_x[ix(i)];
                }
            }
            newtfun(pfunc); /* Evaluate function values with new solution */
            max_dev = 0.0;
            for (int i = 0; i < n; i++) {
                value[ix(i)] = -value[ix(i)]; /* Required correction to function
                                               * values */
                if ((temp = fabs(value[ix(i)])) > max_dev)
                    max_dev = temp;
            }

            /* Check for convergence or maximum iterations */

            if (change <= CONVERGE && max_dev <= ZERO) {
                // break;
                done = 1;
            }
        }
    } /* end of while loop */

    return (error);
}

/*------------------------------------------------------------*/
/*                                                            */
/*  BUILDJACOBIAN                                 	      */
/*                                                            */
/*    Creates the Jacobian matrix by computing partial deriv- */
/*    atives by finite central differences.  If the column    */
/*    variable is nonzero, an increment of 2% of the variable */
/*    is used.  STEP is the minimum increment allowed; it is  */
/*    currently set to 1.0E-6.                                */
/*                                                            */
/*  Returns: no return variable                               */
/*                                                            */
/*  Calling sequence:					      */
/*	 buildjacobian(n, index, x, pfunc, value, jacobian)       */
/*                                                            */
/*  Arguments:                                                */
/*                                                            */
/*    Input: n, integer, number of variables                  */
/*                                                            */
/*           x, pointer to array of addresses of the solution */
/*		vector elements				      */
/*                                                            */
/*	     p, array of parameter values		      */
/*                                                            */
/*           pfunc, pointer to function which computes the    */
/*                     deviation from zero of each equation   */
/*                     in the model.                          */
/*                                                            */
/*	     value, pointer to array of addresses of function */
/*		       values				      */
/*                                                            */
/*    Output: jacobian, double, computed jacobian matrix      */
/*                                                            */
/*  Functions called:  user-supplied function with argument   */
/*                     (p) to compute vector of function      */
/*		       values for each equation.	      */
/*		       makevector(), freevector()	      */
/*                                                            */
/*------------------------------------------------------------*/

#define max(x, y) (fabs(x) > y ? x : y)

void nrn_buildjacobian_thread(NewtonSpace* ns,
                              int n,
                              int* index,
                              NEWTFUN pfunc,
                              double* value,
                              double** jacobian,
                              _threadargsproto_) {
#define x_(arg) _p[(arg) *_STRIDE]
    double* high_value = ns->high_value;
    double* low_value = ns->low_value;

    /* Compute partial derivatives by central finite differences */

    for (int j = 0; j < n; j++) {
        double increment = max(fabs(0.02 * (x_(index[j]))), STEP);
        x_(index[j]) += increment;
        newtfun(pfunc);
        for (int i = 0; i < n; i++)
            high_value[ix(i)] = value[ix(i)];
        x_(index[j]) -= 2.0 * increment;
        newtfun(pfunc);
        for (int i = 0; i < n; i++) {
            low_value[ix(i)] = value[ix(i)];

            /* Insert partials into jth column of Jacobian matrix */

            jacobian[i][ix(j)] = (high_value[ix(i)] - low_value[ix(i)]) / (2.0 * increment);
        }

        /* Restore original variable and function values. */

        x_(index[j]) += increment;
        newtfun(pfunc);
    }
}

NewtonSpace* nrn_cons_newtonspace(int n, int n_instance) {
    NewtonSpace* ns = (NewtonSpace*) emalloc(sizeof(NewtonSpace));
    ns->n = n;
    ns->n_instance = n_instance;
    ns->delta_x = makevector(n * n_instance * sizeof(double));
    ns->jacobian = makematrix(n, n * n_instance);
    ns->perm = (int*) emalloc((unsigned) (n * n_instance * sizeof(int)));
    ns->high_value = makevector(n * n_instance * sizeof(double));
    ns->low_value = makevector(n * n_instance * sizeof(double));
    ns->rowmax = makevector(n * n_instance * sizeof(double));
    nrn_newtonspace_copyto_device(ns);
    return ns;
}

void nrn_destroy_newtonspace(NewtonSpace* ns) {
    nrn_newtonspace_delete_from_device(ns);
    free((char*) ns->perm);
    freevector(ns->delta_x);
    freematrix(ns->jacobian);
    freevector(ns->high_value);
    freevector(ns->low_value);
    freevector(ns->rowmax);
    free((char*) ns);
}
}  // namespace coreneuron
