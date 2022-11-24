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
#include "oc_ansi.h"

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
static void nrn_buildjacobian_thread(NewtonSpace *ns, int n, int *index,
                                     newton_fptr_t pfunc, double *value,
                                     double **jacobian, Datum *ppvar,
                                     Datum *thread, NrnThread *nt,
                                     Memb_list *ml, std::size_t iml) {
  auto const x = [ml, iml](size_t var) -> double & {
    return ml->data(iml, var);
  };
  auto* const high_value = ns->high_value;
  auto* const low_value = ns->low_value;
  // Compute partial derivatives by central finite differences.
  if (index) {
    for (int j = 0; j < n; j++) {
      double const increment{std::max(std::abs(0.02 * x(index[j])), STEP)};
      x(index[j]) += increment;
      pfunc(ml, iml, ppvar, thread, nt);
      for (int i = 0; i < n; i++) {
        high_value[i] = value[i];
      }
      x(index[j]) -= 2.0 * increment;
      pfunc(ml, iml, ppvar, thread, nt);
      for (int i = 0; i < n; i++) {
        low_value[i] = value[i];
        // Insert partials into jth column of Jacobian matrix
        jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
      }
      // Restore original variable and function values.
      x(index[j]) += increment;
      pfunc(ml, iml, ppvar, thread, nt);
    }
  } else {
    for (int j = 0; j < n; j++) {
      double const increment{std::max(std::abs(0.02 * x(j)), STEP)};
      x(j) += increment;
      pfunc(ml, iml, ppvar, thread, nt);
      for (int i = 0; i < n; i++) {
        high_value[i] = value[i];
      }
      x(j) -= 2.0 * increment;
      pfunc(ml, iml, ppvar, thread, nt);
      for (int i = 0; i < n; i++) {
        low_value[i] = value[i];
        // Insert partials into jth column of Jacobian matrix
        jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
      }
      // Restore original variable and function values.
      x(j) += increment;
      pfunc(ml, iml, ppvar, thread, nt);
    }
  }
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
int nrn_newton_thread(NewtonSpace *ns, int n, int *index, newton_fptr_t pfunc,
                      double *value, Datum *ppvar, Datum *thread, NrnThread *nt,
                      Memb_list *ml, std::size_t iml) {
  auto const x = [ml, iml](size_t var) -> double & {
    return ml->data(iml, var);
  };
  // Create arrays for Jacobian, variable increments, function values, and permutation vector
  double* const delta_x{ns->delta_x};
  double** const jacobian{ns->jacobian};
  int* const perm{ns->perm};
  // Iteration loop
  int count{}, error{};
  double change{1.0};
  while (count++ < MAXITERS) {
    if (change > MAXCHANGE) {
      // Recalculate Jacobian matrix if solution has changed by more than MAXCHANGE.
      nrn_buildjacobian_thread(ns, n, index, pfunc, value, jacobian, ppvar,
                               thread, nt, ml, iml);
      for (int i = 0; i < n; i++) {
        // Required correction to function values.
        value[i] = -value[i];
      }

      if ((error = nrn_crout_thread(ns, n, jacobian, perm)) != SUCCESS) {
        break;
      }
    }
    nrn_scopmath_solve_thread(n, jacobian, value, perm, delta_x, nullptr);

    // Update solution vector and compute norms of delta_x and value.
    change = 0.0;
    if (index) {
      for (int i = 0; i < n; i++) {
        if (double temp; fabs(x(index[i])) > ZERO &&
            (temp = fabs(delta_x[i] / x(index[i]))) > change) {
          change = temp;
        }
        x(index[i]) += delta_x[i];
      }
    } else {
      for (int i = 0; i < n; i++) {
        if (double temp; fabs(x(i)) > ZERO && (temp = fabs(delta_x[i] / x(i))) > change) {
          change = temp;
        }
        x(i) += delta_x[i];
      }
    }
    // Evaluate function values with new solution.
    pfunc(ml, iml, ppvar, thread, nt);
    double max_dev{0.0};
    for (int i = 0; i < n; i++) {
      // Required correction to function values.
      value[i] = -value[i];
      if (double temp; (temp = fabs(value[i])) > max_dev) {
        max_dev = temp;
      }
    }

    // Check for convergence or maximum iterations
    if (change <= CONVERGE && max_dev <= ZERO) {
      break;
    }
    if (count == MAXITERS) {
      error = EXCEED_ITERS;
      break;
    }
  }
  return error;
}

NewtonSpace *nrn_cons_newtonspace(int n) {
  NewtonSpace *ns = (NewtonSpace *)hoc_Emalloc(sizeof(NewtonSpace));
  ns->n = n;
  ns->delta_x = makevector(n);
  ns->jacobian = makematrix(n, n);
  ns->perm = (int *)hoc_Emalloc((unsigned)(n * sizeof(int)));
  ns->high_value = makevector(n);
  ns->low_value = makevector(n);
  ns->rowmax = makevector(n);
  return ns;
}

void nrn_destroy_newtonspace(NewtonSpace *ns) {
  free((char *)ns->perm);
  freevector(ns->delta_x);
  freematrix(ns->jacobian);
  freevector(ns->high_value);
  freevector(ns->low_value);
  freevector(ns->rowmax);
  free(ns);
}
