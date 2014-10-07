#include <../../nrnconf.h>
/* must get some things linked from nrniv which otherwise would be gotten
	from ivoc. Needed only if linked statically */
	
#include <OS/string.h>
#include <InterViews/resource.h>
#include "symdir.h"
#include "datapath.h"
#include <nrnmpiuse.h>
#if HAVE_IV
#include "ivoc.h"
#endif

#include "oc2iv.h"
extern "C" {
extern double (*nrnpy_guigetval)(Object*);
extern void (*nrnpy_guisetval)(Object*, double);
extern int (*nrnpy_guigetstr)(Object*, char**);
}

#if defined(CYGWIN)
extern "C" {
extern int ncyg_fprintf();
}
#endif
#if NRNMPI
extern "C" {
	extern void nrn_timeout(int);
}
#endif

extern void nrn_vecsim_add(void*, bool);
extern void nrn_vecsim_remove(void*);

void nrn_nvkludge_dummy() {
	SymDirectory* s1 = new SymDirectory(-1);
	HocDataPaths* hp = new HocDataPaths();
#if HAVE_IV
	Oc::valid_stmt(0, 0);
#endif
#if defined(CYGWIN)
#if HAVE_IV
	Oc::valid_expr(0);
#endif
	hoc_func_table(0,0,0);
	hoc_spec_table(0,0);
	ncyg_fprintf();
#endif
	nrn_vecsim_add(NULL, false);
	nrn_vecsim_remove(NULL);
#if NRNMPI
	nrn_timeout(0);
#endif
	nrnpy_guigetval = 0;
	nrnpy_guisetval = 0;
	nrnpy_guigetstr = 0;
}

