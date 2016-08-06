#ifndef newton_struct_h
#define newton_struct_h

#include "coreneuron/mech/mod2c_core_thread.h"

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

typedef int (*FUN)(_threadargsproto_);


extern int nrn_crout_thread(NewtonSpace* ns, int n, double** a, int* perm);

extern void nrn_scopmath_solve_thread(int n, double** a, double* value,
  int* perm, double* delta_x, int* s, _threadargsproto_);

extern int nrn_newton_thread(NewtonSpace* ns, int n, int* s,
 FUN pfunc, double* value, _threadargsproto_);

static void nrn_buildjacobian_thread(NewtonSpace* ns,
  int n, int* s, FUN pfunc, double* value, double** jacobian, _threadargsproto_);

extern NewtonSpace* nrn_cons_newtonspace(int n);
extern void nrn_destroy_newtonspace(NewtonSpace* ns);
 
#endif
