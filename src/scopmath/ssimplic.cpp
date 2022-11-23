#include <../../nrnconf.h>
#include "errcodes.h"
#include "scoplib.h"
#define s_(arg)	p[s[arg]]

extern void _modl_set_dt(double);

static int check_state(int n, int* s, double* p);

int _ss_sparse(void** v, int n, int* s, int* d, double** p, double* t, double dt, int (*fun)(), double** pcoef, int linflag) {
	int err, i;
	double ss_dt;

	ss_dt=1e9;
	_modl_set_dt(ss_dt);

if (linflag) { /*iterate linear solution*/
		err = sparse(v, n, s, d, p, t, ss_dt, fun, pcoef, 0);
} else {
#define NIT 7
	for (i = 0; i < NIT; i++) {
		err = sparse(v, n, s, d, p, t, ss_dt, fun, pcoef, 1);
		if (err) {
		   break;	/* perhaps we should re-start */
		}
		if (check_state(n, s, p)) {
		   err = sparse(v, n, s, d, p, t, ss_dt, fun, pcoef, 0);
		   break;
		}
	}
	if (i >= NIT) {
		err = 1;
	}
}

	_modl_set_dt(dt);
	return err;
}

int _ss_derivimplicit(int _ninits, int n, int* slist, int* dlist, double** p, double* pt, double dt,
					  int(*fun)(), double** ptemp)
{
	int err, i;
	double ss_dt;

	ss_dt=1e9;
	_modl_set_dt(ss_dt);

   err = derivimplicit(_ninits, n, slist, dlist, p, pt, ss_dt, fun, ptemp);

	_modl_set_dt(dt);
	return err;
}

static int check_state(int n, int* s, double* p)
{
	int i, flag;

	flag = 1;
	for (i=0; i<n; i++) {
		if ( s_(i) < -1e-6) {
			s_(i) = 0.;
			flag = 0;
		}
	}
	return flag;
}
