#include <../../nrnconf.h>
#include "nrnrt.h"
#if NRN_REALTIME // to end of file

#include <stdio.h>
#include "classreg.h"
#include "nrnoc2iv.h"
#include "parse.h"

extern "C" {
	extern int stoprun;
	extern double t;
	extern void nrn_fixed_step();
}

class NrnRealTime {
public:
	NrnRealTime();
	virtual ~NrnRealTime();
};

static double softrun(void* v) {
	double tstop_ = chkarg(1, 1e-9, 1e9);
	while (t < tstop_) {
		nrn_fixed_step();
		if (stoprun) { break; }
	}
	return 0.;
}

static double run(void* v) {
	int ovrn = 0;
	if (nrn_realtime_) {
		ovrn = nrn_rt_run(*getarg(1));
	}
	return (double)ovrn;
}

static double overrun(void* v) {
	return(double)nrnrt_overrun_;	
}

static double rth(void* v) {
	nrn_rtrun_thread_init();
	return 1.;
}
static Member_func members[] = {
	"run", run,
	"steptime", 0, // will be changed below
	"overrun", overrun,
	"use_runthread", rth,
	"softrun", softrun,
	0, 0
};

static void* cons(Object*) {
	NrnRealTime* m = new NrnRealTime();
	return (void*)m;
}

static void destruct(void* v) {
	NrnRealTime* m = (NrnRealTime*)v;
	delete m;
}

static void steer_val(void* v) {
	hoc_spop();
	hoc_pushpx(&nrn_rtstep_time_);
}

void NrnRealTime_reg() {
	class2oc("RealTime", cons, destruct, members, NULL, NULL, NULL);
	Symbol* sv = hoc_lookup("RealTime");
	Symbol* sx = hoc_table_lookup("steptime", sv->u.ctemplate->symtable);
	sx->type = VAR;
	sx->arayinfo = NULL;
	sv->u.ctemplate->steer = steer_val;
}

NrnRealTime::NrnRealTime() {
}

NrnRealTime::~NrnRealTime() {
}

#endif //NRN_REALTIME
