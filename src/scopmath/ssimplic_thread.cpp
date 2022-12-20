#include <../../nrnconf.h>
#include "errcodes.hpp"
#include "scoplib.h"

using namespace neuron::scopmath; // for errcodes.hpp
#define s_(arg)	p[s[arg]]

extern double _modl_get_dt_thread(NrnThread*);
extern void _modl_set_dt_thread(double, NrnThread*);
static int check_state(int n, int* s, double* p);

int _ss_sparse_thread(void** v, int n, int* s, int* d, double* p, double* t, double dt,
					  int (*fun)(void*, double*, double*, Datum*, Datum*, NrnThread*),
					  int linflag, Datum* ppvar, Datum* thread, NrnThread* nt)
{
	int err, i;
	double ss_dt;
	
	ss_dt = 1e9;
	_modl_set_dt_thread(ss_dt, nt);
	
if (linflag) { /*iterate linear solution*/
		err = sparse_thread(v, n, s, d, p, t, ss_dt, fun, 0, ppvar, thread, nt);
} else {
#define NIT 7
	for (i = 0; i < NIT; i++) {
		err = sparse_thread(v, n, s, d, p, t, ss_dt, fun, 1, ppvar, thread, nt);
		if (err) {
		   break;	/* perhaps we should re-start */
		}
		if (check_state(n, s, p)) {
		   err = sparse_thread(v, n, s, d, p, t, ss_dt, fun, 0, ppvar, thread, nt);
		   break;
		}
	}		
	if (i >= NIT) {
		err = 1;
	}
}

	_modl_set_dt_thread(dt, nt);
	return err;
}

int _ss_derivimplicit_thread(int n, int* slist, int* dlist, double* p,
  int (*fun)(double*, Datum*, Datum*, NrnThread*),
  Datum* ppvar, Datum* thread, NrnThread* nt) {
	int err, i;
	double dtsav;
	
	dtsav = _modl_get_dt_thread(nt);
	_modl_set_dt_thread(1e-9, nt);
	
   err = derivimplicit_thread(n, slist, dlist, p, fun, ppvar, thread, nt);

	_modl_set_dt_thread(dtsav, nt);
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
