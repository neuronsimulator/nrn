
/******************************************************************************
 *
 * File: scoplib.h
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989
 *   Duke University
 *
 * scoplib.h,v 1.2 1997/04/18 14:42:13 hines Exp
 *
 ******************************************************************************/

/****************************************************************/
/*								*/
/*  This file declares all the SCoP library functions that can	*/
/*  be called by the user to describe or solve his model's	*/
/*  equations.							*/
/*								*/
/****************************************************************/

/* Memory allocation routines */

double *makevector();		/* (length) */
int freevector();		/* (vector_address) */
double **makeptrvector();	/* (length) */
int freeptrvector();		/* (vector_address) */
double **makematrix();		/* (rows, columns) */
int freematrix();		/* (matrix_address) */
int zero_vector();		/* (vector, n) */
int zero_ptrvector();		/* (vector, n) */
int zero_matrix();		/* (matrix, rows, cols) */
int enqueue(), dequeue();	/* (&variable, vsize) */

/* Printing error messages */

int cls(),
    abort_run(),		/* (error_code) */
    prterr();			/* (message_string) */

/* Solution of first order ordinary differential equations */

int euler(), heun(), runge(), adams(), /* (neqn, &var, &der, p, indep, h, derfunc,
				    * first_time, temp) */
adeuler(), adrunge();		   /* (neqn, &var, &der, p, indep, h, derfunc,
				    * first_time, temp, maxerror) */

/* Implicit backwards eulerian integration.  Can find steady-state solution of
 * first-order odes by passing "infinite" time step h */

int _advance();			/* (n,&var,&der,&indep,h,derfunc,first_time,&p
				 * coef,linflag) */

/* Solution of boundary value problems */

int 
boundary(),			/* (npts, x, y, p, fval, gval, qval) */
shoot();			/* (nbound, indepindex, matchtime, matchvalue,
				 *  known, unknown, p, diff) */

/* Solution of parabolic partial differential equations */

int crank();			/* (n, y, fval, gval, p, dt, dx, first_time,
				 * bound_string) */

/* Definite integrals */

int romberg();			/* (a, b, func, p, integral) */
double legendre();		/* (a, b, func, p) */

/* Solution of simultaneous algebraic equations */

int 
simeq(), seidel(),		/* (n, coef, &soln) */
invert(),			/* (n, matrix) */
crout(),			/* (n, matrix, perm) */
solve(),			/* (n, matrix, const_vect, perm, &soln) */
tridiag(),			/* (n, super, diag, sub, const, &soln) */
newton(),			/* (n, &soln, p, funcval, &value) */
simplex(),			/* (n, &soln, p, funcval, &value) */
buildjacobian();		/* (n, &var, p, funcval, value, jacobian) */

/*  Sensitivity analysis */

int 
linearsens(),			/* (nvar,&var,p,param,funcval,coeff,newjac,
				 * &sens) */
steadysens(),			/* (nvar,&var,p,param,funcval,value,newjac,
				 * &sens) */
trajecsens(),			/* (nvar,&var,&der,param,first_time,newjac,
				 * &sens,&Dsens) */
envelope();			/* (&var, vsize, param, uncert, &sens, &plus,
				 * &minus) */

/* Curve-fitting and interpolation functions */

int derivs();			/* (nbase, x, y, mesh, der) */
double spline();		/* (nbase, x, y, mesh, der, x_inter) */
double force();			/* (t, filename) */
double stepforce();			/* (..., t, filename) */
#define deflate scoplib_deflate /* causes python gzip to fail if nrniv launched. i.e zlib declares global deflate */
int deflate();			/* (degree, coeff, root) */
int expfit();			/* (terms, reffile, amplitude, decay,
				 * &fiterror) */

/* General modeling functions */

double 
hyperbol(), revhyperbol(),	/* (x, max, K) */
sigmoid(), revsigmoid(),	/* (x, max, K, n) */
*lag();				/* (var, curt, lagt, vsize) */

/* Forcing functions */

int threshold();		/* (x, limit, mode) */
double 
harmonic(),			/* (t, period, amplitude, phase) */
squarewave(),			/* (t, period, amplitude) */
sawtooth(),			/* (t, period, amplitude) */
revsawtooth(),			/* (t, period, amplitude) */
ramp(),				/* (t, lag, height, duration) */
pulse(),			/* (t, lag, height, duration) */
perpulse(),			/* (t, lag, height, duration, delay) */
step(),				/* (t, jump_time, jump_height) */
perstep();			/* (t, lag, period, jump_height) */

/* Probability functions */

int 
setseed(),			/* (seed) */
poisrand();			/* (mean) */
double 
factorial(),			/* (n) */
scop_random(), exprand(),
normrand(),			/* (mean, std_dev) */
poisson(),			/* (x, mean) */
gauss(),			/* (x, mean, std_dev) */
scop_erf();				/* (x) */

/* ANSI C Math Functions (that we must include in scoplib when the host system
 * does not yet support them).
 */
#if MAC
#undef fmod
#endif
double fmod();
