#pragma once
/** @file scoplib.h
 *  @copyright (c) 1984-9 Duke University
 *
 *  This file declares all the SCoP library functions that can be called by the
 *  user to describe or solve his model's equations.
 */
#include "crout.hpp"
#include "crout_thread.hpp"
#include "dimplic.hpp"
#include "euler.hpp"
#include "euler_thread.hpp"
#include "newton.hpp"
#include "newton_thread.hpp"
#include "row_view.hpp"
#include "runge.hpp"
#include "simeq.hpp"
#include "sparse.hpp"
#include "sparse_thread.hpp"
#include "ssimplic.hpp"
#include "ssimplic_thread.hpp"
#include "newton_struct.h"

/* Memory allocation routines */
int zero_matrix(double** matrix, int rows, int cols);

/* Printing error messages */
int abort_run(int error_code);
int prterr(const char* message_string);

/* Implicit backwards eulerian integration.  Can find steady-state solution of
 * first-order odes by passing "infinite" time step h */
int _advance(int ninits,
             int n,
             int* s,
             int* d,
             double* p,
             double* t,
             double dt,
             int (*fun)(),
             double*** pcoef,
             int linflag);

/* Solution of boundary value problems */
int boundary(int npts,
             double* x,
             double* y,
             double (*f)(double),
             double (*g)(double),
             double (*q)(double));

/* Solution of parabolic partial differential equations */
int crank(int n,
          double* y,
          double* fval,
          double* gval,
          double dt,
          double dx,
          double t,
          double** bound,
          double** pwork);

/* Definite integrals */
double romberg(double a, double b, int (*func)());
double legendre(double a, double b, int (*func)());

/* Solution of simultaneous algebraic equations */
int invert(int n, double** matrix);
int tridiag(int n, double* a, double* b, double* c, double* d, double* soln);

/* Curve-fitting and interpolation functions */
int derivs(int nbase, double* x, double* y, double* h, double* der);
double spline(int nbase, double* x, double* y, double* h, double* der, double x_inter);
double force(double t, char* filename);
double stepforce(int* reset_integ, double* old_value, double t, char* filename);
int expfit(double* terms, char* reffile, double* amplitude, double* lambda, double* error);

/* General modeling functions */
double hyperbol(double x, double max, double K);
double revhyperbol(double x, double max, double K);
double sigmoid(double x, double max, double K, double n);
double revsigmoid(double x, double max, double K, double n);
double* lag(double* var, double curt, double lagt, int vsize);

/* Forcing functions */
double threshold(int* reset_integ, double* old_value, double x, double limit, char* mode);
double harmonic(double t, double period, double amplitude, double offset);
double squarewave(int* reset_integ, double* old_value, double t, double period, double amplitude);
double sawtooth(int* reset_integ, double* old_value, double t, double period, double amplitude);
double revsawtooth(int* reset_integ, double* old_value, double t, double period, double amplitude);
double ramp(int* reset_integ,
            double* old_value,
            double t,
            double lag,
            double height,
            double duration);
double pulse(int* reset_integ,
             double* old_value,
             double t,
             double lag,
             double height,
             double duration);
double perpulse(int* reset_integ,
                double* old_value,
                double t,
                double lag,
                double height,
                double duration,
                double delay);
double step(int* reset_integ, double* old_value, double t, double jumpt, double jump);
double perstep(int* reset_integ,
               double* old_value,
               double t,
               double lag,
               double period,
               double jump);

/* Probability functions */
int poisrand(double mean);

double factorial(double n);
double scop_random();
double exprand(double mean);
double normrand(double mean, double std_dev);
double poisson(double x, double mean);
double gauss(double x, double mean, double std_dev);
double scop_erf(double z);

void hoc_after_prax_quad(char*);
double* praxis_paxis(int);
double praxis_pval(int);
int praxis_stop(int);
double praxis(double* t0,
              double* machep,
              double* h0,
              long int nval,
              long int* prin,
              double* x,
              double (*f)(double*, long int),
              double* fmin,
              char* after_quad);
