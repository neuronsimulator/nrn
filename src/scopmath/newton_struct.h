#pragma once
// This header has to be both valid C and C++ because the scoplib sources are
// compiled as C.
#ifdef __cplusplus
#include "hocdec.h" // the real Datum
#include <cstddef>
using std::size_t;
extern "C" {
#else
#include "stddef.h"
// All we need is for Datum* to pass untouched through C
typedef struct Datum Datum;
#endif

/* avoid incessant alloc/free memory */
typedef struct NewtonSpace {
	int n;
	double* delta_x;
	double** jacobian;
	int* perm;
	double* high_value;
	double* low_value;
	double* rowmax;
} NewtonSpace;

// Forward-declare for use in function pointer type declaration.
typedef struct NrnThread NrnThread;
typedef struct Memb_list Memb_list;

/* Memory allocation routines */
double* makevector(int length);
int freevector(double* vector);
double** makematrix(int nrows, int ncols);
int freematrix(double** matrix);

int nrn_crout_thread(NewtonSpace* ns, int n, double** a, int* perm);
void nrn_scopmath_solve_thread(int n, double** a,
 double* b, int* perm, double* p, int* y);
typedef int (*newton_fptr_t)(Memb_list*, size_t, Datum*, Datum*, NrnThread*);
int nrn_newton_thread(NewtonSpace* ns, int n, int* index, double** x,  newton_fptr_t pfunc, double* value, void* ppvar, void* thread, void* nt, Memb_list* ml, size_t iml);
NewtonSpace* nrn_cons_newtonspace(int n);
void nrn_destroy_newtonspace(NewtonSpace* ns);

#ifdef __cplusplus
} // extern "C"
#endif
