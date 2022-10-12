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

// Forward-declare for use in function pointer type declaration.
typedef struct NrnThread NrnThread;
typedef struct Memb_list Memb_list;

/* Memory allocation routines */
double* makevector(int length);
int freevector(double* vector);
double** makematrix(int nrows, int ncols);
int freematrix(double** matrix);
