#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: newton.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "errcodes.h"
#include "newton_struct.h"
#include "nrnoc_ml.h"
#include "scoplib.h"

#include <cmath>
#include <cstdlib>
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
int buildjacobian(int n, int *index, int (*pfunc)(), double *value,
                  double **jacobian, Memb_list *ml, std::size_t iml) {
  auto const x = [ml, iml](std::size_t var) -> double & {
    return ml->data(iml, var);
  };
  double *const high_value{makevector(n)};
  double *const low_value{makevector(n)};

  // Compute partial derivatives by central finite differences
  if (index) {
    for (int j = 0; j < n; ++j) {
      double const increment{std::max(std::abs(0.02 * x(index[j])), STEP)};
      x(index[j]) += increment;
      pfunc();
      for (int i = 0; i < n; ++i) {
        high_value[i] = value[i];
      }
      x(index[j]) -= 2.0 * increment;
      pfunc();
      for (int i = 0; i < n; ++i) {
        low_value[i] = value[i];
        // Insert partials into jth column of Jacobian matrix.
        jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
      }
      // Restore original variable and function values.
      x(index[j]) += increment;
      pfunc();
    }
  } else {
    for (int j = 0; j < n; ++j) {
      double const increment{std::max(std::abs(0.02 * x(j)), STEP)};
      x(j) += increment;
      pfunc();
      for (int i = 0; i < n; ++i) {
        high_value[i] = value[i];
      }
      x(j) -= 2.0 * increment;
      pfunc();
      for (int i = 0; i < n; ++i) {
        low_value[i] = value[i];
        // Insert partials into jth column of Jacobian matrix.
        jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
      }
      // Restore original variable and function values.
      x(j) += increment;
      pfunc();
    }
  }
  freevector(high_value);
  freevector(low_value);
  return 0;
}

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
int newton(int n, int *index, int (*pfunc)(), double *value, Memb_list *ml,
           std::size_t iml) {
  auto const x = [ml, iml](std::size_t var) -> double & {
    return ml->data(iml, var);
  };
  int i, count = 0, error, *perm;
  double **jacobian, *delta_x, change = 1.0, max_dev, temp;

  /*
   * Create arrays for Jacobian, variable increments, function values, and
   * permutation vector
   */

  delta_x = makevector(n);
  jacobian = makematrix(n, n);
  perm = (int *)malloc((unsigned)(n * sizeof(int)));

  /* Iteration loop */

  while (count++ < MAXITERS) {
    if (change > MAXCHANGE) {
      /*
       * Recalculate Jacobian matrix if solution has changed by more
       * than MAXCHANGE
       */

      buildjacobian(n, index, pfunc, value, jacobian, ml, iml);
      for (i = 0; i < n; i++)
        value[i] = -value[i]; /* Required correction to
                               * function values */

      if ((error = crout(n, jacobian, perm)) != SUCCESS)
        break;
    }
    solve(n, jacobian, value, perm, delta_x, (int *)0);

    /* Update solution vector and compute norms of delta_x and value */

    change = 0.0;
    if (index) {
      for (i = 0; i < n; i++) {
        if (fabs(x(index[i])) > ZERO &&
            (temp = fabs(delta_x[i] / x(index[i]))) > change)
          change = temp;
        x(index[i]) += delta_x[i];
      }
    } else {
      for (i = 0; i < n; i++) {
        if (fabs(x(i)) > ZERO && (temp = fabs(delta_x[i] / x(i))) > change)
          change = temp;
        x(i) += delta_x[i];
      }
    }
    (*pfunc)(); /* Evaluate function values with new solution */
    max_dev = 0.0;
    for (i = 0; i < n; i++) {
      value[i] = -value[i]; /* Required correction to function
                             * values */
      if ((temp = fabs(value[i])) > max_dev)
        max_dev = temp;
    }

    /* Check for convergence or maximum iterations */

    if (change <= CONVERGE && max_dev <= ZERO)
      break;
    if (count == MAXITERS) {
      error = EXCEED_ITERS;
      break;
    }
  } /* end of while loop */

  free((char *)perm);
  freevector(delta_x);
  freematrix(jacobian);
  return (error);
}
