#include <../../nrnconf.h>
// solver interface to CVode
#include <InterViews/resource.h>

#include "nrnmpi.h"

extern "C" {
void cvode_fadvance();
void cvode_finitialize();
boolean at_time(double);
#if PARANEURON
extern double nrnmpi_dbl_allmin(double, int);
extern int nrnmpi_comm;
#endif
}

#include <math.h>
#include <stdlib.h>
#include "classreg.h"
#include "nrnoc2iv.h"
#include "datapath.h"
#include "cvodeobj.h"
#include "netcvode.h"
#include "membfunc.h"
#include "nrndaspk.h"
#include "tqueue.h"
#include "mymath.h"
#include "htlist.h"

//I have no idea why this is necessary
// but it stops codewarrior from complaining
// about illegal overloading in math.h
// and math.h alone just moves the problem
// to these
#if MAC
#include "d_avec.h"
#endif
//#include "shared/sundialstypes.h"
//#include "shared/nvector_serial.h"
#include "cvodes/cvodes.h"
#include "cvodes/cvodes_impl.h"
#include "cvodes/cvdense.h"
#include "cvodes/cvdiag.h"
#include "shared/dense.h"
#include "ida/ida.h"

extern "C" {
extern double dt, t;
extern int diam_changed;
extern int secondorder;
extern int nrn_cvode_;
extern int linmod_extra_eqn_count();
extern int nrn_modeltype();
extern int nrn_use_selfqueue_;
extern int use_cachevec;
extern void nrn_cachevec(int);

extern int cvode_active_;
extern Cvode* cvode_instance;
extern NetCvode* net_cvode_instance;
#if USENCS
extern void nrn2ncs_netcons();
#endif //USENCS
#if PARANEURON
extern N_Vector N_VNew_Parallel(int comm, long int local_length,
	long int global_length);
#endif
}

extern boolean nrn_use_fifo_queue_;
#if BBTQ == 5
extern boolean nrn_use_bin_queue_;
#endif

#undef SUCCESS
#define SUCCESS CV_SUCCESS

// interface to oc

static double solve(void* v) {
	NetCvode* d = (NetCvode*)v;
	double tstop = -1.;
	if (ifarg(1)) {
		tstop = *getarg(1);
	}
	int i = d->solve(tstop);
	if (i != SUCCESS) {
		hoc_execerror("variable step integrator error", 0);
	}
	return double(i);
}
static double statistics(void* v) {
	NetCvode* d = (NetCvode*)v;
	int i = -1;
	if (ifarg(1)) {
		i = (int)chkarg(1, -1, 1e6);
	}
	d->statistics(i);
	return 0.;
}
static double spikestat(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->spike_stat();
	return 0;
}
static double queue_mode(void* v) {
#if BBTQ == 3 || BBTQ == 4
	if (ifarg(1)) {
		nrn_use_fifo_queue_ = chkarg(1, 0, 1) ? true : false;
	}
	return double(nrn_use_fifo_queue_);
#endif
#if BBTQ == 5
	if (ifarg(1)) {
		nrn_use_bin_queue_ = chkarg(1, 0, 1) ? true : false;
	}
	if (ifarg(2)) {
#if NRNMPI
		nrn_use_selfqueue_ = chkarg(2, 0, 1) ? 1 : 0;
#else
	if (chkarg(2, 0, 1)) {
		hoc_warning("CVode.queue_mode with second arg == 1 requires",
		"configuration --with-mpi or related");
	}
#endif
	}
	return double(nrn_use_bin_queue_ + 2*nrn_use_selfqueue_);
#endif
	return 0.;
}
static double re_init(void* v) {
	if (cvode_active_) {
		NetCvode* d = (NetCvode*)v;
		d->re_init(t);
	}
	return 0.;
}
static double rtol(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		d->rtol(chkarg(1, 0., 1e9));
	}
	return d->rtol();
}
static double nrn_atol(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		d->atol(chkarg(1, 0., 1e9));
		d->structure_change();
	}
	return d->atol();
}
extern "C" {
	extern Symbol* hoc_get_last_pointer_symbol();
	extern void hoc_symbol_tolerance(Symbol*, double);
}

static double abstol(void* v) {
	NetCvode* d = (NetCvode*)v;
	Symbol* sym;
	if (hoc_is_str_arg(1)) {
		sym = d->name2sym(gargstr(1));
	}else{
		hoc_pgetarg(1);
		sym = hoc_get_last_pointer_symbol();
		if (nrn_vartype(sym) != STATE && sym->u.rng.type != VINDEX) {
			hoc_execerror(sym->name, "is not a STATE");
		}
	}
	if (ifarg(2)) {
		hoc_symbol_tolerance(sym, chkarg(2, 1e-30, 1e30));
		d->structure_change();
	}
	if (sym->extra && sym->extra->tolerance > 0.) {
		return sym->extra->tolerance;
	}else{
		return 1.;
	}
}

static double active(void* v) {
	if (ifarg(1)) {
		cvode_active_ = (int)chkarg(1, 0, 1);
		if (cvode_active_) {
			NetCvode* d = (NetCvode*)v;
			d->re_init(t);
		}
	}
	return cvode_active_;
}
static double stiff(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		d->stiff((int)chkarg(1, 0, 2));
	}
	return double(d->stiff());
}
static double maxorder(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		d->maxorder((int)chkarg(1, 0, 5));
	}
	return d->maxorder();
}
static double order(void* v) {
	NetCvode* d = (NetCvode*)v;
	int i = 0;
	if (ifarg(1)) {
		i = int(chkarg(1, 0, d->nlist()));
	}
	int o = d->order(i);
	return double(o);
}
static double minstep(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		d->minstep(chkarg(1, 0., 1e9));
	}
	return d->minstep();
}

static double maxstep(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		d->maxstep(chkarg(1, 0., 1e9));
	}
	return d->maxstep();
}

static double jacobian(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		d->jacobian((int)chkarg(1, 0, 2));
	}
	return double(d->jacobian());
}

static double states(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->states();
	return 0.;
}

static double dstates(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->dstates();
	return 0.;
}

static double error_weights(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->error_weights();
	return 0.;
}

static double acor(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->acor();
	return 0.;
}

static double statename(void* v) {
	NetCvode* d = (NetCvode*)v;
	int i = (int)chkarg(1,0,1e9);
	int style = 1;
	if (ifarg(3)) {
		style = (int)chkarg(3, 0, 2);
	}
	hoc_assign_str(hoc_pgargstr(2), d->statename(i, style));
	return 0.;
}

static double use_local_dt(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		int i = (int)chkarg(1, 0, 1);
		d->localstep(i);
	}
	return (double)d->localstep();
}

static double use_daspk(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		int i = (int)chkarg(1, 0, 1);
		d->use_daspk(i);
	}
	return (double)d->use_daspk();
}

static double dae_init_dteps(void* v) {
	if (ifarg(1)) {
		Daspk::dteps_ = chkarg(1, 1e-100, 1);
	}
	if (ifarg(2)) {
		Daspk::init_failure_style_ = (int)chkarg(2, 0, 013);
	}
	return Daspk::dteps_;
}

static double use_mxb(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		int i = (int)chkarg(1,0,1);
		if (use_sparse13 != i) {
			use_sparse13 = i;
			recalc_diam();
		}
	}
	return (double) use_sparse13;
}

static double cache_efficient(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		int i = (int)chkarg(1,0,1);
		nrn_cachevec(i);
	}
	return (double) use_cachevec;
}

static double condition_order(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		int i = (int)chkarg(1,1,2);
		d->condition_order(i);
	}
	return (double) d->condition_order();
}

static double debug_event(void* v) {
	NetCvode* d = (NetCvode*)v;
	if (ifarg(1)) {
		int i = (int)chkarg(1, 0, 10);
		d->print_event_ = i;
	}
	return (double)d->print_event_;
}

static double n_record(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->vecrecord_add();
	return 0.;
}

static double n_remove(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->vec_remove();
	return 0.;
}

static double simgraph_remove(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->simgraph_remove();
	return 0.;
}

static double state_magnitudes(void* v) {
	NetCvode* d = (NetCvode*)v;
	return d->state_magnitudes();
}

static double tstop_event(void* v) {
	NetCvode* d = (NetCvode*)v;
	double x = *getarg(1);
	if (ifarg(2)) {
		d->hoc_event(x, gargstr(2));
	}else{
		d->tstop_event(x);
	}
	return x;
}

static double current_method(void* v) {
	NetCvode* d = (NetCvode*)v;
	int modeltype = nrn_modeltype();
	int methodtype = secondorder; // 0, 1, or 2
	int localtype = 0;
	if (cvode_active_) {
		methodtype = 3;
		if (d->use_daspk()) {
			methodtype = 4;
		}else{
			localtype = d->localstep();
		}
	}
	return double( modeltype + 10*use_sparse13 + 100*methodtype + 1000*localtype );
}
static double peq(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->print_event_queue();
	return 1.;
}

static double event_queue_info(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->event_queue_info();
	return 1.;
}

static double store_events(void* v) {
	NetCvode* d = (NetCvode*)v;
	d->vec_event_store();
	return 1.;
}

static Object** netconlist(void* v) {
	NetCvode* d = (NetCvode*)v;
	return d->netconlist();
}

static double ncs_netcons(void* v) {
#if USENCS
	nrn2ncs_netcons();
#endif
	return 0.;
}

// for testing when there is actually no pc.transfer or pc.multisplit present
// forces the global step to be truly global across processors.
static double use_parallel(void* v) {
#if PARANEURON
	NetCvode* d = (NetCvode*)v;
	d->list()->use_partrans_ = true;
	d->structure_change();
	d->list()->mpicomm_ = nrnmpi_comm;
	return 1.0;
#else
	return 0.0;
#endif
}

static Member_func members[] = {
	"solve", solve,
	"atol", nrn_atol,
	"rtol", rtol,
	"re_init", re_init,
	"stiff", stiff,
	"active", active,
	"maxorder", maxorder,
	"minstep", minstep,
	"maxstep", maxstep,
	"jacobian", jacobian,
	"states", states,
	"dstates", dstates,
	"error_weights", error_weights,
	"acor", acor,
	"statename", statename,
	"atolscale", abstol,
	"use_local_dt", use_local_dt,
	"record", n_record,
	"record_remove", n_remove,
	"debug_event", debug_event,
	"order", order,
	"use_daspk", use_daspk,
	"event", tstop_event,
	"current_method", current_method,
	"use_mxb", use_mxb,
	"print_event_queue", peq,
	"event_queue_info", event_queue_info,
	"store_events", store_events,
	"condition_order", condition_order,
	"dae_init_dteps", dae_init_dteps,
	"simgraph_remove", simgraph_remove,
	"state_magnitudes", state_magnitudes,
	"ncs_netcons", ncs_netcons,
	"statistics", statistics,
	"spike_stat", spikestat,
	"queue_mode", queue_mode,
	"cache_efficient", cache_efficient,
	"use_parallel", use_parallel,
	0,0
};

static Member_ret_obj_func omembers[] = {
	"netconlist", netconlist,
	0, 0
};

static void* cons(Object*) {
#if 0
	NetCvode* d;
	if (net_cvode_instance) {
		hoc_execerror("Only one CVode instance allowed", 0);
	}else{
		d = new NetCvode(1);
		net_cvode_instance = d;
	}
	active(nil);
	return (void*) d;
#else
	return (void*)net_cvode_instance;
#endif
}
static void destruct(void* v) {
#if 0
	NetCvode* d = (NetCvode*)v;
	cvode_active_ = 0;
	delete d;
#endif
}
void Cvode_reg() {
	class2oc("CVode", cons, destruct, members, nil, omembers);
	net_cvode_instance = new NetCvode(1);
	Daspk::dteps_ = 1e-9; // change with cvode.dae_init_dteps(newval)
}

static int minit(CVodeMem cv_mem);
static int msetup(CVodeMem cv_mem, int convfail, N_Vector ypred,
                  N_Vector fpred, booleantype *jcurPtr, N_Vector vtemp,
                  N_Vector vtemp2, N_Vector vtemp3);
static int msolve(CVodeMem cv_mem, N_Vector b, N_Vector weight, N_Vector ycur,
                 N_Vector fcur);
static void mfree(CVodeMem cv_mem);
 
/* Functions Called by the CVODE Solver */
static void f(realtype t, N_Vector y, N_Vector ydot, void *f_data);

Cvode::Cvode(NetCvode* ncv) {
	cvode_constructor();
	ncv_ = ncv;
}
Cvode::Cvode() {
	cvode_constructor();
}
void Cvode::cvode_constructor() {
	ncv_ = nil;
	no_cap_count = 0;
	no_cap_child_count = 0;
	no_cap_node = nil;
	no_cap_child = nil;
	tqitem_ = nil;
	mem_ = nil;
#if NEOSIMorNCS
	neosim_self_events_ = nil;
#endif
	psl_th_ = nil;
	watch_list_ = nil;
	ste_list_ = nil;
	cv_memb_list_ = nil; // managed by NetCvode
	no_cap_memb_ = nil; // managed by Cvode
	before_breakpoint_ = nil; // managed by NetCvode
	after_solve_ = nil; // managed by NetCvode
	before_step_ = nil; // managed by NetCvode
	record_ = nil;
	play_ = nil;
	
	initialize_ = false;
	can_retreat_ = false;
	tstop_begin_ = 0.;
	tstop_end_ = 0.;
	use_daspk_ = false;
	daspk_ = nil;

	mem_ = nil;
	y_ = nil;
	atolnvec_ = nil;
	maxstate_ = nil;
	maxacor_ = nil;
	neq_ = 0;
	structure_change_ = true;
	pv_ = nil;
	pvdot_ = nil;
	atolvec_ = nil;
#if PARANEURON
	use_partrans_ = false;
	global_neq_ = 0;
	mpicomm_ = 0;
	opmode_ = 0;
#endif
}

double Cvode::gam() {
	if (mem_) {
		return ((CVodeMem)mem_)->cv_gamma;
	}else{
		return 1.;
	}
}

double Cvode::h() {
	if (mem_) {
		return ((CVodeMem)mem_)->cv_h;
	}else{
		return 0.;
	}	
}

boolean Cvode::at_time(double te) {
	if (initialize_) {
//printf("at_time initialize te=%g te-t0_=%g next_at_time_=%g\n", te, te-t0_, next_at_time_);
		if (t0_ < te && te < next_at_time_) {
//printf("next_at_time_=%g since te-t0_=%15.10g and next_at_time_-te=%g\n", te, te-t, next_at_time_-te);
			next_at_time_ = te;
		}
		if (MyMath::eq(te, t0_, NetCvode::eps(t0_))) {
//printf("at_time te=%g t-te=%g return 1\n", te, t - te);
			return 1;
		}
		return 0;
	}
	// No at_time event is inside our allowed integration interval.
	// such an event would be unhandled. It would be an error for
	// a model description to dynamically compute such an event.
	// The policy is strict that during at_time
	// event handling the next at_time event is known so that
	// the stop time can be set. This could be relaxed
	// only to the extent that whatever at_time is computed is
	// beyond the current step.
	if (nrn_cvode_) {
if (te <= tstop_ && te > t0_) {
printf("te=%g t0_=%g tn_=%g t_=%g t=%g\n", te, t0_, tn_, t_, t);
printf("te-t0_=%g  tstop_-te=%g\n", te - t0_, tstop_ - te);
}
		assert( te > tstop_ || te <= t0_);
	}
	return 0;
}

void Cvode::set_init_flag() {
//printf("set_init_flag t=%g t-t_=%g\n", t, t-t_);
	initialize_ = true;
	if (cvode_active_ && ++prior2init_ == 1) {
		record_continuous();
	}
}

N_Vector Cvode::nvnew(long int i) {
#if PARANEURON
	if (use_partrans_) {
		return N_VNew_Parallel(mpicomm_, i, global_neq_);
	}
#endif
	return N_VNew_Serial(i);
}

void Cvode::atolvec_alloc(int i) {
	if (i > 0) {
		// too bad the machEnv has to be done here but this is
		// the first and it depends on size
		// the call chain is init_prepare (frees) -> init_eqn -> here
		atolnvec_ = nvnew(i);
		atolvec_ = N_VGetArrayPointer(atolnvec_);
	}
}

Cvode::~Cvode() {
	delete_memb_list(no_cap_memb_);
#if NEOSIMorNCS
	if (neosim_self_events_) {
		delete neosim_self_events_;
	}
#endif
	if (cvode_instance == (Cvode*)this) {
		cvode_instance = nil;
	}
	if (daspk_) {
		delete daspk_;
	}
	if (y_) {
		N_VDestroy(y_);
	}
	if (atolnvec_) {
		N_VDestroy(atolnvec_);
	}
	if (mem_) {
		CVodeFree(mem_);
	}
	if (pv_) {
		delete [] pv_;
		delete [] pvdot_;
	}
	if (maxstate_) {
		delete [] maxstate_;
		delete [] maxacor_;
	}
	delete_prl();
//	delete [] iopt_;
//	delete [] ropt_;

	if (no_cap_node) {
		delete [] no_cap_node;
		delete [] no_cap_child;
	}
	if (watch_list_) {
		watch_list_->RemoveAll();
		delete watch_list_;
	}
}

void Cvode::stat_init() {
	advance_calls_ = interpolate_calls_ = init_calls_ = 0;
	ts_inits_ = f_calls_ = mxb_calls_ = jac_calls_ = 0;
	Daspk::first_try_init_failures_ = 0;
}

void Cvode::init_prepare() {
	if (init_global()) {
		if (y_) {
			N_VDestroy(y_);
			y_ = nil;
		}
		if (mem_) {
			CVodeFree(mem_);
			mem_ = nil;
		}
		if (atolnvec_) {
			N_VDestroy(atolnvec_);
			atolnvec_ = nil;
			atolvec_ = nil;
		}
		if (daspk_) {
			delete daspk_;
			daspk_ = nil;
		}
		init_eqn();
		if (neq_ > 0) {
			y_ = nvnew(neq_);
			if (use_daspk_) {
				alloc_daspk();
			}else{
				alloc_cvode();
			}
			if (maxstate_) {
				activate_maxstate(false);
				activate_maxstate(true);
			}
		}
	}
}

void Cvode::activate_maxstate(boolean on) {
	if (maxstate_) {
		delete [] maxstate_;
		delete [] maxacor_;
		maxstate_ = nil;
		maxacor_ = nil;
	}
	if (on && neq_ > 0) {
		int i;
		maxstate_ = new double[neq_];
		maxacor_ = new double[neq_];
		for (i=0; i < neq_; ++i) {
			maxstate_[i] = 0.;
			maxacor_[i] = 0.;
		}
	}
}

void Cvode::maxstate(boolean b) {
	if (maxstate_) {
		int i;
		double x;
		double* y = N_VGetArrayPointer(y_);
		for (i=0; i < neq_; ++i) {
			x = Math::abs(y[i]);
			if (maxstate_[i] < x) {
				maxstate_[i] = x;
			}
		}
	    if (b) {
		y = acorvec();
		for (i=0; i < neq_; ++i) {
			x = Math::abs(y[i]);
			if (maxacor_[i] < x) {
				maxacor_[i] = x;
			}
		}
	    }
	}
}

void Cvode::maxstate(double* pd) {
	int i;
	if (maxstate_) {
		for (i=0; i < neq_; ++i) {
			pd[i] = maxstate_[i];
		}
	}
}

void Cvode::maxacor(double* pd) {
	int i;
	if (maxacor_) {
		for (i=0; i < neq_; ++i) {
			pd[i] = maxacor_[i];
		}
	}
}

void Cvode::alloc_cvode() {
}

int Cvode::order() {
	int i = 0;
	if (use_daspk_) {
		if (daspk_->mem_) { IDAGetLastOrder(daspk_->mem_, &i); }
	}else{
		if (mem_) { CVodeGetLastOrder(mem_, &i); }
	}
	return i;
}
void Cvode::maxorder(int maxord) {
	if (use_daspk_) {
		if (daspk_->mem_) { IDASetMaxOrd(daspk_->mem_, maxord); }
	}else{
		if (mem_) { CVodeSetMaxOrd(mem_, maxord); }
	}
}
void Cvode::minstep(double x) {
	if (mem_) {
		if (x > 0.) {
			CVodeSetMinStep(mem_, x);
		}else{
			// CVodeSetMinStep requires x > 0 but
			// HMIN_DEFAULT is ZERO in cvodes.c
			((CVodeMem)mem_)->cv_hmin = 0.;
		}
	}
}
void Cvode::maxstep(double x) {
	if (mem_) { CVodeSetMaxStep(mem_, x); }
}

void Cvode::free_cvodemem() {
	if (mem_) {
		CVodeFree(mem_);
		mem_ = nil;
	}
}

int Cvode::cvode_init(double) {
	int iter;
	int err = SUCCESS;
	// note, a change in stiff_ due to call of stiff() destroys mem_
	gather_y(N_VGetArrayPointer(y_));
	if (mem_) {
		err = CVodeReInit(mem_, f, t0_, y_, CV_SV, &ncv_->rtol_, atolnvec_);
		//printf("CVodeReInit\n");
		if (err != SUCCESS){
			printf("Cvode %lx %s CVReInit error %d\n", (long)this, secname(v_node_[rootnodecount_]->sec), err);
			return err;
		}
	}else{
		mem_ = CVodeCreate(CV_BDF, ncv_->stiff() ? CV_NEWTON : CV_FUNCTIONAL);
		if (!mem_){
			hoc_execerror ("CVodeCreate error", 0);
		}
		CVodeMalloc(mem_, f, t0_, y_, CV_SV, &ncv_->rtol_, atolnvec_);
		if (err != SUCCESS){
			printf("Cvode %lx %s CVodeMalloc error %d\n", (long)this, secname(v_node_[rootnodecount_]->sec), err);
			return err;
		}
		maxorder(ncv_->maxorder());
		minstep(ncv_->minstep());
		maxstep(ncv_->maxstep());
//		CVodeSetInitStep(mem_, .01);
	}
	matmeth();
	((CVodeMem)mem_)->cv_gamma = 0.;
	((CVodeMem)mem_)->cv_h = 0.; // fun called before cvode sets this (though fun does not need it really)
	fun(t_, N_VGetArrayPointer(y_), nil);
	can_retreat_ = false;
	return err;
}

int Cvode::daspk_init(double tout) {
	return daspk_->init();
}

void Cvode::alloc_daspk() {
//printf("Cvode::alloc_daspk\n");
	daspk_ = new Daspk(this, neq_);
	// we do our own initialization since it is hard to
	// figure out which equations are algebraic and which
	// differential. eg. the no cap nodes can have a
	// circuit with capacitance connection. Extracellular
	// nodes may or may not have capacitors to ground.

}

int Cvode::advance_tn() {
	int err = SUCCESS;
	if (neq_ == 0) {
		t_ += 1e9;
		t = t_;
		tn_ = t_;
		return err;
	}
	cvode_instance = (Cvode*)this;
//printf("%d Cvode::advance_tn enter t_=%.20g t0_=%.20g tstop_=%.20g\n", nrnmpi_myid, t_, t0_, tstop_);
	if (t_ >= tstop_ - NetCvode::eps(t_)) {
//printf("init\n");
		++ts_inits_;
		tstop_begin_ = tstop_;
		tstop_end_ = tstop_ + 1.5*NetCvode::eps(tstop_);
		err = init(tstop_end_);
		// the above 1.5 is due to the fact that at_time will check
		// to see if the time is greater but not greater than eps*t0_
		// of the at_time for a return of 1.
		can_retreat_ = false;
	}else{
		++advance_calls_;
		// SOLVE after_cvode now interpreted as before cvode since
		// a step may be abandoned in part by interpolate in response
		// to events. Now at least the value of t is monotonic
		t = t_;
		do_nonode();
#if PARANEURON
		opmode_ = 1;
#endif
		if (use_daspk_) {
			err = daspk_advance_tn();
		}else{
			err = cvode_advance_tn();
		}
		can_retreat_ = true;
		maxstate(true);
	}
//printf("%d Cvode::advance_tn exit t_=%.20g t0_=%.20g tstop_=%.20g\n", nrnmpi_myid, t_, t0_, tstop_);
	return err;
}

int Cvode::solve() {
//printf("%d Cvode::solve %lx initialize = %d current_time=%g tn=%g\n", nrnmpi_myid, (long)this, initialize_, t_, tn());
	int err = SUCCESS;
	if (initialize_) {
		if (t_ >= tstop_ - NetCvode::eps(t_)) {
			++ts_inits_;
			tstop_begin_ = tstop_;
			tstop_end_ = tstop_ + 1.5*NetCvode::eps(tstop_);
			err = init(tstop_end_);
			can_retreat_ = false;
		}else{
			err = init(t_);
		}
	}else{
		err = advance_tn();
	}
//printf("Cvode::solve exit %lx current_time=%g tn=%g\n", (long)this, t_, tn());
	return err;
}

int Cvode::init(double tout) {
	int err = SUCCESS;
	++init_calls_;
//printf("%d Cvode_%lx::init tout=%g\n", nrnmpi_myid, (long)this, tout);
	initialize_ = true;
	t_ = tout;
	t0_ = t_;
	tn_ = t_;
	next_at_time_ = t_ + 1e5;
	cvode_instance = (Cvode*)this;
	init_prepare();
	if (neq_) {
#if PARANEURON
		opmode_ = 3;
#endif
		if (use_daspk_) {
			err = daspk_init(t_);
		}else{
			err = cvode_init(t_);
		}
	}
	tstop_ = next_at_time_ - NetCvode::eps(next_at_time_);
#if PARANEURON
	if (use_partrans_) {
		tstop_ =  nrnmpi_dbl_allmin(tstop_, mpicomm_);
	}
#endif
//printf("Cvode::init next_at_time_=%g tstop_=%.15g\n", next_at_time_, tstop_);
	initialize_ = false;
	prior2init_ = 0;
#if carbon
	maxstate((unsigned int)0);
#else
	maxstate(false);
#endif
	return err;
}

int Cvode::interpolate(double tout) {
	if (neq_ == 0) {
		t_ = tout;
		t = t_;
		return SUCCESS;
	}
//printf("Cvode::interpolate t_=%.15g t0_=%.15g tn_=%.15g tstop_=%.15g\n", t_, t0_, tn_, tstop_);
	if (!can_retreat_) {
		// but must be within the initialization domain
		assert (MyMath::le(tout, t_, 2.*NetCvode::eps(t_)));
		t = tout; // but leave t_ at the initialization point.
//		printf("no interpolation for event in the initialization interval t=%15g t_=%15g\n", t, t_);
		return SUCCESS;
	}
	if (MyMath::eq(tout, t_, NetCvode::eps(t_))) {
		t_ = tout;
		return SUCCESS;
	}
//if (initialize_) {
//printf("Cvode_%lx::interpolate assert error when initialize_ is true.\n t_=%g tout=%g tout-t_ = %g\n", (long)this, t_, tout, tout-t_);
//}
	assert(initialize_ == false); // or state discontinuity can be lost
//printf("interpolate t_=%g tout=%g delta t_-tout=%g\n", t_, tout, t_-tout);
#if 1
if (tout < t0_) {
//	if (tout < t0_ - NetCvode::eps(t0_)) { // t0_ not always == tn_ - h
		// after a CV_ONESTEP_TSTOP
		// so allow some fudge before printing error
		// The fudge case was avoided by returning from the
		// Cvode::handle_step when a first order condition check
		// puts an event on the queue equal to t_
printf("Cvode::interpolate assert error t0=%g tout-t0=%g eps*t_=%g\n", t0_, tout-t0_, NetCvode::eps(t_));
//	}
	tout = t0_;
}
if (tout > tn_) {
printf("Cvode::interpolate assert error tn=%g tn-tout=%g  eps*t_=%g\n", tn_, tn_-tout, NetCvode::eps(t_));
	tout = tn_;
}
#endif
// if there is a problem here it may be due to the at_time skipping interval
// since the integration will not go past tstop_ and will take up at tstop+2eps
// see the Cvode::advance_tn() implementation
	assert( tout >= t0() && tout <= tn());
	cvode_instance = (Cvode*)this;

	++interpolate_calls_;
#if PARANEURON
		opmode_ = 2;
#endif
	if (use_daspk_) {
		return daspk_->interpolate(tout);
	}else{
		return cvode_interpolate(tout);
	}
}

int Cvode::cvode_advance_tn() {
#if PRINT_EVENT
if (net_cvode_instance->print_event_ > 1) {
printf("Cvode::cvode_advance_tn %lx initialize_=%d tstop=%.20g t_=%.20g to ", (long)this, initialize_, tstop_, t_);
}
#endif
	CVodeSetStopTime(mem_, tstop_);
//printf("cvode_advance_tn begin t0_=%g t_=%g tn_=%g tstop=%g\n", t0_, t_, tn_, tstop_);
	int err = CVode(mem_, tstop_, y_, &t_, CV_ONE_STEP_TSTOP);
#if PRINT_EVENT
if (net_cvode_instance->print_event_ > 1) {
printf("t_=%.20g\n", t_);
}
#endif
	if (err < 0 ) {
		printf("CVode %lx %s advance_tn failed, err=%d.\n", (long)this, secname(v_node_[rootnodecount_]->sec), err);
		fun(t_, N_VGetArrayPointer(y_), nil); // make sure dstate/dt is valid
		return err;
	}
	// this is very bad, performance-wise. However cvode modifies its states
	// after a call to fun with the proper t.
#if 1
	fun(t_, N_VGetArrayPointer(y_), nil);
#else
	scatter_y(N_VGetArrayPointer(y_));
#endif
	tn_ = ((CVodeMem)mem_)->cv_tn;
	t0_ = tn_ - ((CVodeMem)mem_)->cv_h;
//printf("t=%.15g t_=%.15g tn()=%.15g tstop_=%.15g t0_=%.15g\n", t, t_, tn(), tstop_, t0_);
//	printf("t_=%g h=%g q=%d y=%g\n", t_, ((CVodeMem)mem_)->cv_h, ((CVodeMem)mem_)->cv_q, N_VIth(y_,0));
//printf("cvode_advance_tn end t0_=%g t_=%g tn_=%g\n", t0_, t_, tn_);
	return SUCCESS;
}

int Cvode::cvode_interpolate(double tout) {
#if PRINT_EVENT
if (net_cvode_instance->print_event_ > 1) {
printf("Cvode::cvode_interpolate %lx initialize_%d t=%.20g to ", (long)this, initialize_, t_);
}
#endif
	// avoid CVode-- tstop = 0.5 is behind  current t = 0.5
	// is this really necessary anymore. Maybe NORMAL mode ignores tstop
	CVodeSetStopTime(mem_, tstop_ + tstop_);
	int err = CVode(mem_, tout, y_, &t_, CV_NORMAL);
#if PRINT_EVENT
if (net_cvode_instance->print_event_ > 1) {
printf("%.20g\n", t_);
}
#endif
	if (err < 0) {
		printf("CVode %lx %s interpolate failed, err=%d.\n", (long)this, secname(v_node_[rootnodecount_]->sec), err);
		return err;
	}
	fun(t_, N_VGetArrayPointer(y_), nil);
//	printf("t_=%g h=%g q=%d y=%g\n", t_, ((CVodeMem)mem_)->cv_h, ((CVodeMem)mem_)->cv_q, N_VIth(y_,0));
	return SUCCESS;
}

int Cvode::daspk_advance_tn() {
	int flag, err;
	double tin;
	Cvode* csav = cvode_instance;
	cvode_instance = (Cvode*)this;
//printf("Cvode::solve test stop time t=%.20g tstop-t=%g\n", t, tstop_-t);
//printf("in Cvode::solve t_=%g tstop=%g calling  daspk_->solve(%g)\n", t_, tstop_, daspk_->tout_);
	err = daspk_->advance_tn(tstop_);
//printf("in Cvode::solve returning from daspk_->solve t_=%g initialize_=%d tstop=%g\n", t_, initialize__, tstop_);
	cvode_instance = csav;
	if (err < 0) {
		return err;
	}
	return SUCCESS;
}

double* Cvode::ewtvec() {
	if (use_daspk_) {
		return daspk_->ewtvec();
	}else{
		return N_VGetArrayPointer(((CVodeMem)mem_)->cv_ewt);
	}
}

double* Cvode::acorvec() {
	if (use_daspk_) {
		return daspk_->acorvec();
	}else{
		return N_VGetArrayPointer(((CVodeMem)mem_)->cv_acor);
	}
}

void Cvode::statistics() {
#if 1
	printf("\nCvode instance %lx %s statistics : %d %s states\n",
		(long)this, secname(v_node_[rootnodecount_]->sec), neq_,
		(use_daspk_ ? "IDA" : "CVode"));
	printf("   %d advance_tn, %d interpolate, %d init (%d due to at_time)\n", advance_calls_, interpolate_calls_, init_calls_, ts_inits_);
	printf("   %d function evaluations, %d mx=b solves, %d jacobian setups\n", f_calls_, mxb_calls_, jac_calls_);
	if (use_daspk_) {
		daspk_->statistics();
		return;
	}
#else
	printf("\nCVode Statistics.. \n\n");
	printf("internal steps = %d\nfunction evaluations = %d\n",
		iopt_[NST], iopt_[NFE]);
	printf("newton iterations = %d  setups = %d\n nonlinear convergence failures = %d\n\
 local error test failures = %ld\n",
		iopt_[NNI], iopt_[NSETUPS], iopt_[NCFN], iopt_[NETF]);
	printf("order=%d stepsize=%g\n", iopt_[QU], h());
#endif
}

void Cvode::matmeth() {
	switch ( ncv_->jacobian() ) {
	case 1:
		CVDense(mem_, neq_);
		break;
	case 2:
		CVDiag(mem_);
		break;
	default:
		((CVodeMem)mem_)->cv_linit = minit;
		((CVodeMem)mem_)->cv_lsetup = msetup;
		((CVodeMem)mem_)->cv_setupNonNull = TRUE; // but since our's does not do anything...
		((CVodeMem)mem_)->cv_lsolve = msolve;
		((CVodeMem)mem_)->cv_lfree = mfree;
		break;
	}
}

static int minit(CVodeMem) {
//	printf("minit\n");
	return CV_NO_FAILURES;
}

static int msetup(CVodeMem, int convfail,
	N_Vector yp, N_Vector fp, booleantype* jcurPtr,
	N_Vector, N_Vector, N_Vector)
{
//	printf("msetup\n");
	*jcurPtr = true;
	return cvode_instance->setup(N_VGetArrayPointer(yp), N_VGetArrayPointer(fp));
}

static int msolve(CVodeMem m, N_Vector b, N_Vector weight, N_Vector ycur, N_Vector fcur) {
//	printf("msolve gamma=%g gammap=%g gamrat=%g\n", m->cv_gamma, m->cv_gammap, m->cv_gamrat);
//	N_VIth(b, 0) /= (1. + m->cv_gamma);
//	N_VIth(b, 0) /= (1. + m->cv_gammap);
//	N_VIth(b,0) *= 2./(1. + m->cv_gamrat);
	return cvode_instance->solvex(N_VGetArrayPointer(b), N_VGetArrayPointer(ycur));
}

static void mfree(CVodeMem) {
//	printf("mfree\n");
}

static void f(realtype t, N_Vector y, N_Vector ydot, void *f_data) {
	//ydot[0] = -y[0];
//	N_VIth(ydot, 0) = -N_VIth(y, 0);
//printf("f(%g, %lx, %lx)\n", t, (long)y, (long)ydot);
	cvode_instance->fun(t, N_VGetArrayPointer(y), N_VGetArrayPointer(ydot));
}
