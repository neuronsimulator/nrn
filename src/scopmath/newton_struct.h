#ifndef newton_struct_h
#define newton_struct_h
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

typedef int (*FUN)(double*, void*, void*, void*);


extern void* hoc_Emalloc(unsigned long);
extern void hoc_malchk();
#define emalloc(arg) hoc_Emalloc(arg); hoc_malchk()
extern int freevector();
extern int freematrix();
extern double* makevector(int n);
extern double** makematrix(int n, int m);
extern int nrn_crout_thread(NewtonSpace* ns, int n, double** a, int* perm);
extern void nrn_scopmath_solve_thread(int n, double** a,
 double* b, int* perm, double* p, int* y);
extern int nrn_newton_thread(NewtonSpace* ns, int n, int* index, double* x,
 FUN pfunc, double* value, void* ppvar, void* thread, void* nt);
static void nrn_buildjacobian_thread(NewtonSpace* ns,
  int n, int* index, double* x, FUN pfunc,
  double* value, double** jacobian, void* ppvar, void* thread, void* nt);
extern NewtonSpace* nrn_cons_newtonspace(int n);
extern void nrn_destroy_newtonspace(NewtonSpace* ns);
 
#endif
