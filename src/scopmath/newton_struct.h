#pragma once
#include "hocdec.h" // the real Datum

/* avoid incessant alloc/free memory */
struct NewtonSpace {
	int n;
	double* delta_x;
	double** jacobian;
	int* perm;
	double* high_value;
	double* low_value;
	double* rowmax;
};

// Forward-declare for use in function pointer type declaration.
struct NrnThread;
struct Memb_list;

/* Memory allocation routines */
double* makevector(int length);
int freevector(double* vector);
double** makematrix(int nrows, int ncols);
int freematrix(double** matrix);

int nrn_crout_thread(NewtonSpace* ns, int n, double** a, int* perm);
void nrn_scopmath_solve_thread(int n, double** a, double* b, int* perm, double* p, int* y);
using newton_fptr_t = int (*)(Memb_list*, size_t, Datum*, Datum*, NrnThread*);
int nrn_newton_thread(NewtonSpace* ns, int n, int* index, newton_fptr_t pfunc, double* value, Datum* ppvar, Datum* thread, NrnThread* nt, Memb_list* ml, size_t iml);
NewtonSpace* nrn_cons_newtonspace(int n);
void nrn_destroy_newtonspace(NewtonSpace* ns);
