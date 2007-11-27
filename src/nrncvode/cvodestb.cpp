#include <../../nrnconf.h>
// solver CVode stub to allow cvode as dll for mswindows version.

#include <InterViews/resource.h>
#include "classreg.h"
#include "nrnoc2iv.h"
#include "datapath.h"
#if USECVODE
#include "cvodeobj.h"
#include "netcvode.h"
#else
class Cvode;
#endif

extern "C" {
void cvode_fadvance(double);
void cvode_finitialize();
boolean at_time(double);

extern double dt, t;
extern int nrn_cvode_;
extern void nrn_random_play();
extern int cvode_active_;
extern int nrn_use_daspk_;

Cvode* cvode_instance;
NetCvode* net_cvode_instance;
void deliver_net_events();
void nrn_deliver_events(double);
void clear_event_queue();
void init_net_events();
void nrn_record_init();
void nrn_play_init();
void fixed_record_continuous();
void fixed_play_continuous();
void nrn_solver_prepare();
}


// for fixed step fadvance
void deliver_net_events() {
	if (net_cvode_instance) {
		net_cvode_instance->deliver_net_events();
	}
}

// handle events during finitialize()
void nrn_deliver_events(double til) {
	double tsav = t;
	if (net_cvode_instance) {
		net_cvode_instance->deliver_events(til);
	}
	t = tsav;
}

void clear_event_queue() {
	if (net_cvode_instance) {
		net_cvode_instance->clear_events();
	}
}

void init_net_events() {
	if (net_cvode_instance) {
		net_cvode_instance->init_events();
	}
}

void nrn_record_init() {
	if (net_cvode_instance) {
		net_cvode_instance->record_init();
	}
}

void nrn_play_init() {
	if (net_cvode_instance) {
		net_cvode_instance->play_init();
	}
}

void fixed_play_continuous() {
	if (net_cvode_instance) {
		net_cvode_instance->fixed_play_continuous();
	}
}

void fixed_record_continuous() {
	if (net_cvode_instance) {
		net_cvode_instance->fixed_record_continuous();
	}
}

void nrn_solver_prepare() {
	if (net_cvode_instance) {
		net_cvode_instance->solver_prepare();
	}
}

void cvode_fadvance(double tstop) { // tstop = -1 means single step
#if USECVODE
	int err;
	if (net_cvode_instance) {
		nrn_random_play();
		err = net_cvode_instance->solve(tstop);
		if (err != 0) {
			printf("err=%d\n", err);
			hoc_execerror("variable step integrator error", 0);
		}
	}
#endif
}

void cvode_finitialize(){
#if USECVODE
	if (net_cvode_instance) {
		net_cvode_instance->re_init();
	}
#endif
}

boolean at_time(double te) {
#if USECVODE
	if (cvode_active_ && cvode_instance) {
		return cvode_instance->at_time(te);
	}
#endif
	double x = te - 1e-11;
	if (x <= t && x > (t - dt)) {
		return 1;
	}
	return 0;
}
