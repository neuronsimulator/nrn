#include <../../nrnconf.h>
/*
Hoc interface to praxis.

See praxis.c in the scopmath library about
tolerance (t0), maxstepsize (h0), and printmode (prin).
These are set with
	attr_praxis(t0, h0, prin)
	
Minimise an interpreted hoc function or compiled hoc function with 
	double x[n]
	fit_praxis(n, "funname", &x[0], "afterquad statement")
where n is the number of variables on which funname depends and
x is a vector containing on entry a guess of the point of minimum
and on exit contains the estimated point of minimum.
(The third arg may be a Vector)
Funname will be called with 2 args to be retrieved by $1 and $&2[i].
afterquad statement (if the arg exists , will be executed
at the end of each main loop iteration (complete conjugate gradient search
followed by calculation of quadratic form)
eg.

double x[2]
attr_praxis(1e-5, 1, -1)
func f() {local t1, t2, t3
	return $0^2 + 2*$1^4
}	
fit_praxis(2, "f", &x)

After fit_praxis exits, (or on execution of afterquad stmt)
one can retrieve the i'th principal value
with pval_praxis(i). If you also want the i'th principal axis then use
double a[n]
pval = pval_praxis(i, &a)
also can use
pval = pval_praxis(i, Vector)
*/

#include "hocdec.h"
#include "parse.h"

extern int stoprun;
extern double praxis(), praxis_pval(), *praxis_paxis(), chkarg();
extern double *hoc_pgetarg();
extern Object* hoc_thisobject;

extern int hoc_is_pdouble_arg(int);
extern void vector_resize(void*, int);
extern double* vector_vec(void*);
extern void* vector_arg(int);
extern void* vector_new2(void* vec);
extern void vector_delete(void* vec);
extern int vector_capacity(void*);
extern Object** vector_pobj(void* v);
extern int nrn_praxis_ran_index;
extern Object** hoc_objgetarg(int);

static double efun(double*, int);
static Symbol* hoc_efun_sym;

static double tolerance, machep, maxstepsize;
static long int printmode;
static long int nvar;

double (*nrnpy_praxis_efun)(Object* pycallable, Object* hvec);
static Object* efun_py;
static Object* efun_py_arg;
static void* vec_py_save;

int stop_praxis() {
	int i = 1;
	if (ifarg(1)) {
		i = (int)chkarg(1, 0., 1e5);
	}
	i = praxis_stop(i);
	ret((double)i);
}

/* want to get best result if user does a stoprun */
static double minerr, *minarg; /* too bad this is not recursive */

int fit_praxis() {
	extern Symbol* hoc_lookup();
	extern char* gargstr();
	char* after_quad;	
	int i;
	double err, fmin;
	double* px;
	double minerrsav, *minargsav, maxstepsizesav, tolerancesav;
	long int printmodesav;
	Symbol* funsav;
	Object* efun_py_save, *efun_py_arg_save;
	void* vec_py_save_save;
	
	fmin = 0.;
	if (efun_py) {
		hoc_obj_unref(efun_py);
		efun_py = (Object*)0;
		hoc_obj_unref(efun_py_arg);
		efun_py_arg = (Object*)0;
		vector_delete(vec_py_save);
	}
    if (hoc_is_object_arg(1)) {
	assert(nrnpy_praxis_efun);
	efun_py = *hoc_objgetarg(1);
	hoc_obj_ref(efun_py);
	efun_py_arg = *vector_pobj(vector_arg(2));
	hoc_obj_ref(efun_py_arg);
	vec_py_save = vector_new2(efun_py_arg->u.this_pointer);
	nvar = vector_capacity(vec_py_save);
	px = vector_vec(vec_py_save);
    }else{
	nvar = (int)chkarg(1, 0., 1e6);
	funsav = hoc_efun_sym;
	hoc_efun_sym = hoc_lookup(gargstr(2));
	if (!hoc_efun_sym
	   || (hoc_efun_sym->type != FUNCTION
	      && hoc_efun_sym->type != FUN_BLTIN)) {
		hoc_execerror(gargstr(2), "not a function name");
	}
	
	if (!hoc_is_pdouble_arg(3)) {
		void* vec = vector_arg(3);
		if (vector_capacity(vec) != nvar) {
			hoc_execerror("first arg not equal to size of Vector",0);
		}
		px = vector_vec(vec);
	}else{
		px = hoc_pgetarg(3);
	}
    }
	if (maxstepsize == 0.) {
		hoc_execerror("call attr_praxis first to set attributes", 0);
	}
	machep = 1e-15;
		
	if (ifarg(4)) {
		after_quad = gargstr(4);
	}else{
		after_quad = (char*)0;
	}
	minerrsav = minerr;
	minargsav = minarg;
	minarg = (double*)ecalloc(nvar, sizeof(double));
	tolerancesav = tolerance;
	maxstepsizesav = maxstepsize;
	printmodesav = printmode;
	efun_py_save = efun_py;
	efun_py_arg_save = efun_py_arg;
	vec_py_save_save = vec_py_save;

	minerr=1e9;
	err = praxis(&tolerance, &machep, &maxstepsize,	nvar, &printmode,
		px, efun, &fmin, after_quad);
	err = minerr;
	if (minerr < 1e9) {
		for (i=0; i<nvar; ++i) { px[i] = minarg[i]; }
	}
	if (efun_py) {
		double* px = vector_vec(efun_py_arg->u.this_pointer);
		for (i=0; i < nvar; ++i) {
			px[i] = minarg[i];
		}
		hoc_obj_unref(efun_py);
		efun_py = (Object*)0;
		hoc_obj_unref(efun_py_arg);
		efun_py_arg = (Object*)0;
		vector_delete(vec_py_save);
		vec_py_save = (void*)0;
	}
	if (minargsav) {
		free(minarg);
		minarg = minargsav;
		minerr = minerrsav;
		hoc_efun_sym = funsav;
		tolerance = tolerancesav;
		maxstepsize = maxstepsizesav;
		printmode = printmodesav;
		efun_py = efun_py_save;
		efun_py_arg = efun_py_arg_save;
		vec_py_save = vec_py_save_save;
	}
	ret(err);
}

void hoc_after_prax_quad(s) char* s; {
	hoc_obj_run(s, hoc_thisobject);
}

int attr_praxis() {
    if (ifarg(2)) {
	tolerance = *getarg(1);
	maxstepsize = *getarg(2);
	printmode = (int)chkarg(3, 0., 3.);
	ret(0.);
    }else{
	int old = nrn_praxis_ran_index;
	if (ifarg(1)) {
	    	nrn_praxis_ran_index = (int)chkarg(1, 0., 1e9);
	}
	ret((double)old);
    }
}

int pval_praxis() {
	int i;
	i = (int)chkarg(1, 0., (double)(nvar-1));
	if (ifarg(2)) {
		int j;
		double *p1, *p2;
		p1 = praxis_paxis(i);
		if (!hoc_is_pdouble_arg(2)) {
			void* vec = vector_arg(2);
			vector_resize(vec, nvar);
			p2 = vector_vec(vec);
		}else{
			p2 = hoc_pgetarg(2);
		}
		for (j = 0; j < nvar; ++j) {
			p2[j] = p1[j];
		}
	}
	ret(praxis_pval(i));
}

static double efun(v, n)
	int n;
	double* v;
{
	int i;
	double err;
    if (efun_py) {
	double* px = vector_vec(efun_py_arg->u.this_pointer);
	for (i=0; i < n; ++i) {
		px[i] = v[i];
	}
    	err = nrnpy_praxis_efun(efun_py, efun_py_arg);
	for (i=0; i < n; ++i) {
		v[i] = px[i];
	}
    }else{
	extern double hoc_call_func();
	int i;
	hoc_pushx((double)n);
	hoc_pushpx(v);
	err =  hoc_call_func(hoc_efun_sym, 2);
    }
	if (!stoprun && err < minerr) {
		minerr = err;
		for (i=0; i < n; ++i) {
			minarg[i] = v[i];
		}
	}	
	return err;
}	
