#include <../../nrnconf.h>
// solver CVode stub to allow cvode as dll for mswindows version.

#include <cmath>
#include "classreg.h"
#include "cvodeobj.h"
#include "nrncvode.h"
#include "nrniv_mf.h"
#include "nrnoc2iv.h"
#include "datapath.h"
#if USECVODE
#include "cvodeobj.h"
#include "netcvode.h"
#else
class Cvode;
#endif

void cvode_finitialize(double t0);
void nrncvode_set_t(double);

extern double dt, t;
#define nt_t  nrn_threads->_t
#define nt_dt nrn_threads->_dt
extern int nrn_use_daspk_;

NetCvode* net_cvode_instance;
void deliver_net_events(NrnThread*);
void nrn_deliver_events(NrnThread*);
void init_net_events();
void nrn_record_init();
void nrn_play_init();
void fixed_play_continuous(NrnThread* nt);
void nrn_solver_prepare();
static void check_thresh(NrnThread*);

// for fixed step thread
void deliver_net_events(NrnThread* nt) {
    int i;
    if (net_cvode_instance) {
        net_cvode_instance->check_thresh(nt);
        net_cvode_instance->deliver_net_events(nt);
    }
}

// handle events during finitialize()
void nrn_deliver_events(NrnThread* nt) {
    double tsav = nt->_t;
    if (net_cvode_instance) {
        net_cvode_instance->deliver_events(tsav, nt);
    }
    nt->_t = tsav;
}

void clear_event_queue() {
    if (net_cvode_instance) {
        net_cvode_instance->clear_events();
    }
}

void free_event_queues() {
    if (net_cvode_instance) {
        net_cvode_instance->free_event_pools();
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

void fixed_play_continuous(NrnThread* nt) {
    if (net_cvode_instance) {
        net_cvode_instance->fixed_play_continuous(nt);
    }
}

void fixed_record_continuous(neuron::model_sorted_token const& cache_token, NrnThread& nt) {
    if (net_cvode_instance) {
        net_cvode_instance->fixed_record_continuous(cache_token, nt);
    }
}

void nrn_solver_prepare() {
    if (net_cvode_instance) {
        net_cvode_instance->solver_prepare();
    }
}

void cvode_fadvance(double tstop) {  // tstop = -1 means single step
#if USECVODE
    int err;
    if (net_cvode_instance) {
        if (tree_changed || v_structure_change || diam_changed) {
            net_cvode_instance->re_init();
        }
        nrn_random_play();
        err = net_cvode_instance->solve(tstop);
        if (err != 0) {
            printf("err=%d\n", err);
            hoc_execerror("variable step integrator error", 0);
        }
        t = nt_t;
        dt = nt_dt;
    }
#endif
}

void cvode_finitialize(double t0) {
#if USECVODE
    if (net_cvode_instance) {
        net_cvode_instance->re_init(t0);
    }
#endif
}

bool at_time(NrnThread* nt, double te) {
#if USECVODE
    if (cvode_active_ && nt->_vcv) {
        return ((Cvode*) nt->_vcv)->at_time(te, nt);
    }
#endif
    double x = te - 1e-11;
    if (x <= nt->_t && x > (nt->_t - nt->_dt)) {
        return true;
    }
    return false;
}

void nrncvode_set_t(double tt) {
    NetCvode* nc = net_cvode_instance;
    if (nc->gcv_) {
        Cvode& cv = *nc->gcv_;
        cv.tn_ = cv.t_ = cv.t0_ = tt;
    } else {
        for (int i = 0; i < nc->pcnt_; ++i) {
            NetCvodeThreadData& p = nc->p[i];
            for (int j = 0; j < p.nlcv_; ++j) {
                Cvode& cv = p.lcv_[j];
                cv.tn_ = cv.t_ = cv.t0_ = tt;
            }
        }
    }
}
