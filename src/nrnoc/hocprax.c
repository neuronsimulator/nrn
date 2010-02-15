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
extern int vector_capacity(void*);

static double efun();
static Symbol* hoc_efun_sym;

static double tolerance, machep, maxstepsize;
static long int printmode;
static long int nvar;

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
	fmin = 0.;
	nvar = (int)chkarg(1, 0., 1e6);
	funsav = hoc_efun_sym;
	hoc_efun_sym = hoc_lookup(gargstr(2));
	if (!hoc_efun_sym
	   || (hoc_efun_sym->type != FUNCTION
	      && hoc_efun_sym->type != FUN_BLTIN)) {
		hoc_execerror(gargstr(2), "not a function name");
	}
	
	if (maxstepsize == 0.) {
		hoc_execerror("call attr_praxis first to set attributes", 0);
	}
	machep = 1e-15;
		
	if (!hoc_is_pdouble_arg(3)) {
		void* vec = vector_arg(3);
		if (vector_capacity(vec) != nvar) {
			hoc_execerror("first arg not equal to size of Vector",0);
		}
		px = vector_vec(vec);
	}else{
		px = hoc_pgetarg(3);
	}
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

	minerr=1e9;
	err = praxis(&tolerance, &machep, &maxstepsize,	nvar, &printmode,
		px, efun, &fmin, after_quad);
	if (minerr < 1e9) {
		for (i=0; i<nvar; ++i) { px[i] = minarg[i]; }
	}
	err = minerr;
	if (minargsav) {
		free(minarg);
		minarg = minargsav;
		minerr = minerrsav;
		hoc_efun_sym = funsav;
		tolerance = tolerancesav;
		maxstepsize = maxstepsizesav;
		printmode = printmodesav;
	}
	ret(err);
}

void hoc_after_prax_quad(s) char* s; {
	hoc_obj_run(s, hoc_thisobject);
}

int attr_praxis() {
	tolerance = *getarg(1);
	maxstepsize = *getarg(2);
	printmode = (int)chkarg(3, 0., 3.);
	ret(0.);
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
	double err;
	extern double hoc_call_func();
	int i;
	hoc_pushx((double)n);
	hoc_pushpx(v);
	err =  hoc_call_func(hoc_efun_sym, 2);
	if (!stoprun && err < minerr) {
		minerr = err;
		for (i=0; i < n; ++i) {
			minarg[i] = v[i];
		}
	}	
	return err;
}	
