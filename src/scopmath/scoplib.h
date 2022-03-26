#pragma once
/** @file scoplib.h
 *  @copyright (c) 1984, 1985, 1986, 1987, 1988, 1989 Duke University
 *
 *  This file declares all the SCoP library functions that can be called by the
 *  user to describe or solve his model's	equations.
 *
 *  This header has to be both valid C and C++ because the scoplib sources are
 *  compiled as C. This file is the result of merging the parallel version
 *  scoplib_ansi.h back into scoplib.h to avoid duplication and inconsistency.
 */
#include "newton_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Memory allocation routines */
int zero_vector(double* vector, int n);
int zero_ptrvector(double** ptrvector, int n);
int zero_matrix(double** matrix, int rows, int cols);

/* Printing error messages */
int abort_run(int error_code);
int prterr(const char* message_string);

/* Solution of first order ordinary differential equations */
int adams(int ninits,
          int n,
          int* y,
          int* d,
          double* p,
          double* t,
          double h,
          int (*dy)(),
          double** work);
int euler(int ninits,
          int neqn,
          int* var,
          int* der,
          double* p,
          double* t,
          double dt,
          int (*func)(),
          double** work);
int heun(int ninits,
         int neqn,
         int* var,
         int* der,
         double* p,
         double* t,
         double dt,
         int (*func)(),
         double** work);
int runge(int ninits,
          int n,
          int* y,
          int* d,
          double* p,
          double* t,
          double h,
          int (*dy)(),
          double** work);

int adeuler(int ninits,
            int neqn,
            int* var,
            int* der,
            double* p,
            double* t,
            double dt,
            int (*func)(),
            double** work,
            double maxerror);
int adrunge(int ninits,
            int n,
            int* y,
            int* d,
            double* p,
            double* t,
            double dt,
            int (*dy)(),
            double** work,
            double maxerror);

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
int boundary(int npts, double* x, double* y, double (*f)(), double (*g)(), double (*q)());

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
int simeq(int n, double** coef, double* soln, int* index);
int seidel(int n, double** coef, double* soln, int* index);
int invert(int n, double** matrix);
int crout(int n, double** a, int* perm);
int solve(int n, double** a, double** b, int* perm, double* p, int* y);
int tridiag(int n, double* a, double* b, double* c, double* d, double* soln);
int newton(int n, int* index, double* x, int (*pfunc)(), double* value);
int simplex(int nparms, int* parms, double* pp, int (*pfunc)(), int* value);
int buildjacobian(int n, int* index, double* x, int (*pfunc)(), double* value, double** jacobian);

/* Curve-fitting and interpolation functions */
int derivs(int nbase, double* x, double* y, double* h, double* der);
double spline(int nbase, double* x, double* y, double* h, double* der, double x_inter);
double force(double t, char* filename);
double stepforce(int* reset_integ, double* old_value, double t, char* filename);
/* Using deflate as a global symbol name is unfortunate and clashes with zlib;
 * hopefully this can improved by porting the scoplib sources to C++ and moving
 * them into an appropriate namespace. In the meantime, hack around this with
 * the preprocessor.
 */
#define deflate scoplib_deflate
int deflate(double degree, double* pcoeff, double* root);
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
#ifdef __cplusplus
} // extern "C"
#endif
