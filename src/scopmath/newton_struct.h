#pragma once
// This header has to be both valid C and C++ because the scoplib sources are
// compiled as C.
#ifdef __cplusplus
extern "C" {
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
typedef union Datum Datum;
typedef struct NrnThread NrnThread;

/* Memory allocation routines */
double* makevector(int length);
int freevector(double* vector);
double** makematrix(int nrows, int ncols);
int freematrix(double** matrix);

int nrn_crout_thread(NewtonSpace* ns, int n, double** a, int* perm);
void nrn_scopmath_solve_thread(int n, double** a,
 double* b, int* perm, double* p, int* y);
int nrn_newton_thread(NewtonSpace* ns, int n, int* index, double* x,
 int (*pfunc)(double *, Datum *, Datum *, NrnThread *), double* value, void* ppvar, void* thread, void* nt);
NewtonSpace* nrn_cons_newtonspace(int n);
void nrn_destroy_newtonspace(NewtonSpace* ns);

#ifdef __cplusplus
} // extern "C"
#endif
