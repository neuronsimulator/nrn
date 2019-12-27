#ifndef scoplib_ansi_h
#define scoplib_ansi_h
#if defined(__cplusplus)
extern "C" {
#endif

/* derived from nrn/src/scopmath/scoplib.h */
/* Memory allocation routines */

double *makevector(int);		/* (length) */
int freevector(double*);		/* (vector_address) */
double **makeptrvector(int);		/* (length) */
int freeptrvector(double**);		/* (vector_address) */
double **makematrix(int, int);		/* (rows, columns) */
int freematrix(double**);		/* (matrix_address) */
int zero_vector(double*, int);		/* (vector, n) */
int zero_ptrvector(double**, int);	/* (vector, n) */
int zero_matrix(double**, int, int);	/* (matrix, rows, cols) */
int enqueue(double*, int);		/* (&variable, vsize) */
int dequeue(double*, int);		/* (&variable, vsize) */

/* Printing error messages */

int cls();
int abort_run(int);		/* (error_code) */
int prterr();			/* (message_string) */

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

int simeq(int, double**, double*, int*);
int seidel();		/* (n, coef, &soln) */
int invert();			/* (n, matrix) */
int crout();			/* (n, matrix, perm) */
int solve();			/* (n, matrix, const_vect, perm, &soln) */
int tridiag();			/* (n, super, diag, sub, const, &soln) */
int newton();			/* (n, &soln, p, funcval, &value) */
int simplex();			/* (n, &soln, p, funcval, &value) */
int buildjacobian();		/* (n, &var, p, funcval, value, jacobian) */

int nrn_newton_thread();
void nrn_destroy_newtonspace();
int derivimplicit_thread();

/*  Sensitivity analysis */


int linearsens();		/* (nvar,&var,p,param,funcval,coeff,newjac,
				 * &sens) */
int steadysens();		/* (nvar,&var,p,param,funcval,value,newjac,
				 * &sens) */
int trajecsens();		/* (nvar,&var,&der,param,first_time,newjac,
				 * &sens,&Dsens) */
int envelope();			/* (&var, vsize, param, uncert, &sens, &plus,
				 * &minus) */

/* Curve-fitting and interpolation functions */

int derivs();			/* (nbase, x, y, mesh, der) */
double spline();		/* (nbase, x, y, mesh, der, x_inter) */
double force();			/* (t, filename) */
double stepforce();			/* (..., t, filename) */
int deflate();			/* (degree, coeff, root) */
int expfit();			/* (terms, reffile, amplitude, decay,
				 * &fiterror) */

/* General modeling functions */

double hyperbol();
double revhyperbol();	/* (x, max, K) */
double sigmoid();
double revsigmoid();	/* (x, max, K, n) */
double* lag();		/* (var, curt, lagt, vsize) */

/* Forcing functions */

int threshold();		/* (x, limit, mode) */

double harmonic();		/* (t, period, amplitude, phase) */
double squarewave();		/* (t, period, amplitude) */
double sawtooth();		/* (t, period, amplitude) */
double revsawtooth();		/* (t, period, amplitude) */
double ramp();			/* (t, lag, height, duration) */
double pulse();			/* (t, lag, height, duration) */
double perpulse();		/* (t, lag, height, duration, delay) */
double step();			/* (t, jump_time, jump_height) */
double perstep();		/* (t, lag, period, jump_height) */

/* Probability functions */

int setseed();			/* (seed) */
int poisrand();			/* (mean) */

double factorial(double);	/* (n) */
double scop_random();
double exprand(double);
double normrand(double, double);/* (mean, std_dev) */
double poisson(double, double);	/* (x, mean) */
double gauss(double, double, double);	/* (x, mean, std_dev) */
double scop_erf(double);	/* (x) */

#if defined(__cplusplus)
}
#endif
#endif
