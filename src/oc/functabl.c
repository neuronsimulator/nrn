#include <../../nrnconf.h>
#include "hoc.h"

#define INTERPOLATE 1

typedef struct TableArg {
	int nsize;
	double* argvec;	/* if nil use min,max */
	double min;
	double max;
#if INTERPOLATE
	double frac;	/* temporary storage */
#endif
} TableArg;

typedef struct FuncTable {
	double* table;
	TableArg* targs;	
	double value; /* if constant this is it */
} FuncTable;

static int arg_index(TableArg* ta, double x)
{
	int j;
		
	if (ta->argvec) {
		int t0, t1;
#if INTERPOLATE
		ta->frac = 0.;
#endif
		t0 = 0;
		t1 = ta->nsize-1;
		if (x <= ta->argvec[t0]) {
			j = 0;
		}else if (x >= ta->argvec[t1]) {
			j = t1;
		}else{
			while (t0 < (t1-1)) {
				j = (t0 + t1)/2;
#if 0
printf("x[%d]=%g  x[%d]=%g  x[%d]=%g\n", t0, ta->argvec[t0], j, ta->argvec[j], t1, ta->argvec[t1]);
#endif
				if (ta->argvec[j] <= x) {
					t0 = j;
				}else{
					t1 = j;
				}
			}
			j = t0;
#if INTERPOLATE
ta->frac = (x - ta->argvec[j])/(ta->argvec[j+1] - ta->argvec[j]);
#if 0
printf("x[%d]=%g  frac=%g  x=%g\n", j, ta->argvec[j], ta->frac, x);
#endif
#endif
		}
	}else{
#if INTERPOLATE
		ta->frac = 0.;
#endif
		if (x <= ta->min) {
			j = 0;
		}else if (x >= ta->max) {
			j = ta->nsize -1;
		}else{
			double d, x1;
			d = (ta->max - ta->min)/((double)(ta->nsize - 1));
			x1 = (x - ta->min)/d;
			j = (int)x1;
#if INTERPOLATE
			ta->frac = x1 - (double)j;
#endif
		}
	}
	return j;
}

static double inter(double frac, double* tab, int j)
{
	if (frac > 0.) {
		return (1. - frac)*tab[j] + frac*tab[j+1];
	}else{
		return tab[j];
	}
}

static double interp(double frac, double x1, double x2)
{
	if (frac > 0.) {
		return (1. - frac)*x1 + frac*x2;
	}else{
		return x1;
	}
}

double hoc_func_table(void* vpft, int n, double* args)
{
	int i, j;
	double* tab;
	FuncTable* ft = (FuncTable*)vpft;
	if (!ft) {
		hoc_execerror("table not specified in hoc_func_table", (char*)0);
	}
	tab = ft->table;
	j = 0;
	for (i=0; i < n; ++i) {
		j = (j * ft->targs[i].nsize) + arg_index(ft->targs + i, args[i]);
/*
printf("calculating j: i=%d args=%g j=%d frac=%g\n", i, args[i], j, ft->targs[i].frac);
*/
	}
#if INTERPOLATE
	if (n == 1) {
		return inter(ft->targs[0].frac, tab, j);
	}else if (n == 2) {
		/* adjacent indices ar adjacent values of y dimension */
		double y1, y2;
		double fy = ft->targs[1].frac;
		y1 = inter(fy, tab, j);
/*
printf("calculating y1: fy=%g j=%d y1=%g, tabj=%g\n", fy, j, y1, tab[j]);
*/
		if (ft->targs[0].frac) {	/* x dimension fraction */
			int j1 = j + ft->targs[1].nsize;
			y2 = inter(fy, tab, j1);
/*
printf("calculating y2: fx=%g fy=%g j1=%d y2=%g tabj1=%g\n", ft->targs[0].frac, fy, j1, y2, tab[j1]);
*/
			return interp(ft->targs[0].frac, y1, y2);
		}else{
			return y1;
		}
	}else{
		return tab[j];
	}
#else
	return tab[j];
#endif
}

void hoc_spec_table(void** vppt, int n)
{
	int i, argcnt;
	FuncTable** ppt = (FuncTable**)vppt;
	FuncTable* ft;
	TableArg* ta;
	if (!*ppt) {
		*ppt = (FuncTable*) ecalloc(1, sizeof(FuncTable));
		(*ppt)->targs = (TableArg*) ecalloc(n, sizeof(TableArg));
	}
	ft = *ppt;
	ta = ft->targs;
	argcnt = 2;
	if (!ifarg(2)) { /* set to constant */
		ft->value = *getarg(1);
		ft->table = &ft->value;
		for (i=0; i < n; ++i) {
			ta[i].nsize = 1;
			ta[i].argvec = (double*)0;
			ta[i].min = 1e20;
			ta[i].max = 1e20;
		}
		return;
	}
  if (hoc_is_object_arg(1)) {
	int ns;
	if (n > 1) {
hoc_execerror("Vector arguments allowed only for functions", "of one variable");
	}
	ns = vector_arg_px(1, &ft->table);
	ta[0].nsize = vector_arg_px(2, &ta[0].argvec);
	if (ns != ta[0].nsize) {
		hoc_execerror("Vector arguments not same size", (char*)0);
	}
  }else{
	for (i = 0; i < n; ++i) {
  		ta[i].nsize = *getarg(argcnt++);
		if (ta[i].nsize < 1) {
			hoc_execerror("size arg < 1 in hoc_spec_table", (char*)0);
		}
		if (hoc_is_double_arg(argcnt)) {
			ta[i].min = *getarg(argcnt++);
			ta[i].max = *getarg(argcnt++);
			if (ta[i].max < ta[i].min) {
				hoc_execerror("min > max in hoc_spec_table", (char*)0);
			}
			ta[i].argvec = (double*)0;
		}else{
			ta[i].argvec = hoc_pgetarg(argcnt++);
		}
	}
	ft->table = hoc_pgetarg(1);
  }
}
