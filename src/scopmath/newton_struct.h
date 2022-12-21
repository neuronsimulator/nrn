#pragma once
// avoid incessant alloc/free memory
struct NewtonSpace {
	int n;
	double* delta_x;
	double** jacobian;
	int* perm;
	double* high_value;
	double* low_value;
	double* rowmax;
};

/* Memory allocation routines */
double* makevector(int length);
int freevector(double* vector);
double** makematrix(int nrows, int ncols);
int freematrix(double** matrix);

int nrn_crout_thread(NewtonSpace* ns, int n, double** a, int* perm);
void nrn_scopmath_solve_thread(int n, double** a, double* b, int* perm, double* p, int* y);
