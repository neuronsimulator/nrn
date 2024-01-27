#include <../../nrnconf.h>
// solver interface to CVode

#include "nrnmpi.h"

void cvode_finitialize();
extern void (*nrn_multisplit_setup_)();

extern int hoc_return_type_code;

#include <cmath>
#include <cstdlib>
#include "classreg.h"
#include "nrnoc2iv.h"
#include "datapath.h"
#include "cvodeobj.h"
#include "netcvode.h"
#include "membfunc.h"
#include "nrn_ansi.h"
#include "nrncvode.h"
#include "nrndaspk.h"
#include "nrniv_mf.h"
#include "nrnpy.h"
#include "tqueue.h"
#include "mymath.h"
#include "htlist.h"
#include <nrnmutdec.h>

#if NRN_ENABLE_THREADS
static MUTDEC
#endif

// Use of the above static mutex was broken by changeset 7ffd95c in 2014
// when a MUTDEC was added explicitly to the NetCvode class namespace to
// handle interthread send events.
static void static_mutex_for_at_time(bool b) {
    if (b) {
        MUTCONSTRUCT(1)
    } else {
        MUTDESTRUCT
    }
}

// I have no idea why this is necessary
// but it stops codewarrior from complaining
// about illegal overloading in math.h
// and math.h alone just moves the problem
// to these
//#include "shared/sundialstypes.h"
//#include "shared/nvector_serial.h"
#include "cvodes/cvodes.h"
#include "cvodes/cvodes_impl.h"
#include "cvodes/cvdense.h"
#include "cvodes/cvdiag.h"
#include "shared/dense.h"
#include "ida/ida.h"
#include "nonvintblock.h"

extern double dt, t;
#define nt_dt nrn_threads->_dt
#define nt_t  nrn_threads->_t
extern int secondorder;
extern int linmod_extra_eqn_count();
extern int nrn_modeltype();
extern int nrn_use_selfqueue_;
extern void (*nrnthread_v_transfer_)(NrnThread*);
extern void (*nrnmpi_v_transfer_)();

extern NetCvode* net_cvode_instance;
extern short* nrn_is_artificial_;
#if USENCS
extern void nrn2ncs_netcons();
#endif  // USENCS
#if NRNMPI
extern "C" {
extern N_Vector N_VNew_Parallel(int comm, long int local_length, long int global_length);
extern N_Vector N_VNew_NrnParallelLD(int comm, long int local_length, long int global_length);
}  // extern "C"
#endif

extern bool nrn_use_fifo_queue_;
#if BBTQ == 5
extern bool nrn_use_bin_queue_;
#endif

#undef SUCCESS
#define SUCCESS CV_SUCCESS

// interface to oc

static double solve(void* v) {
    NetCvode* d = (NetCvode*) v;
    double tstop = -1.;
    if (ifarg(1)) {
        tstop = *getarg(1);
    }
    tstopunset;
    int i = d->solve(tstop);
    tstopunset;
    if (i != SUCCESS) {
        hoc_execerror("variable step integrator error", 0);
    }
    t = nt_t;
    dt = nt_dt;
    return double(i);
}
static double statistics(void* v) {
    NetCvode* d = (NetCvode*) v;
    int i = -1;
    if (ifarg(1)) {
        i = (int) chkarg(1, -1, 1e6);
    }
    d->statistics(i);
    return 0.;
}
static double spikestat(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->spike_stat();
    return 0;
}
static double queue_mode(void* v) {
    hoc_return_type_code = 1;  // integer
#if BBTQ == 4
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
    return double(nrn_use_bin_queue_ + 2 * nrn_use_selfqueue_);
#endif
    return 0.;
}

void nrn_extra_scatter_gather(int direction, int tid);

static double re_init(void* v) {
    if (cvode_active_) {
        NetCvode* d = (NetCvode*) v;
        d->re_init(t);
    } else {
        nrn_extra_scatter_gather(1, 0);
    }
    return 0.;
}
static double rtol(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        d->rtol(chkarg(1, 0., 1e9));
    }
    return d->rtol();
}
static double nrn_atol(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        d->atol(chkarg(1, 0., 1e9));
        d->structure_change();
    }
    return d->atol();
}
extern Symbol* hoc_get_last_pointer_symbol();
extern void hoc_symbol_tolerance(Symbol*, double);

static double abstol(void* v) {
    NetCvode* d = (NetCvode*) v;
    Symbol* sym;
    if (hoc_is_str_arg(1)) {
        sym = d->name2sym(gargstr(1));
    } else {
        sym = hoc_get_last_pointer_symbol();
        if (!sym) {
            hoc_execerror(
                "Cannot find the symbol associated with the pointer when called from Python",
                "Use a string instead of pointer argument");
        }
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
    } else {
        return 1.;
    }
}

static double active(void* v) {
    if (ifarg(1)) {
        cvode_active_ = (int) chkarg(1, 0, 1);
        if (cvode_active_) {
            static_cast<NetCvode*>(v)->re_init(nt_t);
        }
    }
    hoc_return_type_code = 2;  // boolean
    return cvode_active_;
}
static double stiff(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        d->stiff((int) chkarg(1, 0, 2));
    }
    hoc_return_type_code = 1;  // integer
    return double(d->stiff());
}
static double maxorder(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        d->maxorder((int) chkarg(1, 0, 5));
    }
    hoc_return_type_code = 1;  // integer
    return d->maxorder();
}
static double order(void* v) {
    NetCvode* d = (NetCvode*) v;
    int i = 0;
    hoc_return_type_code = 1;  // integer
    if (ifarg(1)) {
        // only thread 0
        i = int(chkarg(1, 0, d->p->nlcv_ - 1));
    }
    int o = d->order(i);
    return double(o);
}
static double minstep(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        d->minstep(chkarg(1, 0., 1e9));
    }
    return d->minstep();
}

static double maxstep(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        d->maxstep(chkarg(1, 0., 1e9));
    }
    return d->maxstep();
}

static double jacobian(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        d->jacobian((int) chkarg(1, 0, 2));
    }
    hoc_return_type_code = 1;  // int
    return double(d->jacobian());
}

static double states(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->states();
    return 0.;
}

static double dstates(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->dstates();
    return 0.;
}

extern double nrn_hoc2fun(void* v);
extern double nrn_hoc2scatter_y(void* v);
extern double nrn_hoc2gather_y(void* v);
extern double nrn_hoc2fixed_step(void* v);

static double error_weights(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->error_weights();
    return 0.;
}

static double acor(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->acor();
    return 0.;
}

static double statename(void* v) {
    NetCvode* d = (NetCvode*) v;
    int i = (int) chkarg(1, 0, 1e9);
    int style = 1;
    if (ifarg(3)) {
        style = (int) chkarg(3, 0, 2);
    }
    hoc_assign_str(hoc_pgargstr(2), d->statename(i, style).c_str());
    return 0.;
}

static double use_local_dt(void* v) {
    NetCvode* d = (NetCvode*) v;
    hoc_return_type_code = 2;  // boolean
    if (ifarg(1)) {
        int i = (int) chkarg(1, 0, 1);
        d->localstep(i);
    }
    return (double) d->localstep();
}

static double use_daspk(void* v) {
    NetCvode* d = (NetCvode*) v;
    hoc_return_type_code = 2;  // boolean
    if (ifarg(1)) {
        int i = (int) chkarg(1, 0, 1);
        if ((i != 0) != d->use_daspk()) {
            d->use_daspk(i);
        }
    }
    return (double) d->use_daspk();
}

static double dae_init_dteps(void* v) {
    if (ifarg(1)) {
        Daspk::dteps_ = chkarg(1, 1e-100, 1);
    }
    if (ifarg(2)) {
        Daspk::init_failure_style_ = (int) chkarg(2, 0, 013);
    }
    return Daspk::dteps_;
}

static double use_mxb(void* v) {
    hoc_return_type_code = 2;  // boolean
    if (ifarg(1)) {
        int i = (int) chkarg(1, 0, 1);
        if (use_sparse13 != i) {
            use_sparse13 = i;
            recalc_diam();
        }
    }
    return (double) use_sparse13;
}

static double cache_efficient(void* v) {
    // Perhaps a warning on cache_efficient(True) and an error on cache_efficient(False) would be
    // justified.
    hoc_return_type_code = 2;  // boolean
    return 1.0;
}

static double use_long_double(void* v) {
    NetCvode* d = (NetCvode*) v;
    hoc_return_type_code = 2;  // boolean
    if (ifarg(1)) {
        int i = (int) chkarg(1, 0, 1);
        d->use_long_double_ = i;
        d->structure_change();
    }
    return (double) d->use_long_double_;
}

static double condition_order(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        int i = (int) chkarg(1, 1, 2);
        d->condition_order(i);
    }
    hoc_return_type_code = 1;  // integer
    return (double) d->condition_order();
}

static double debug_event(void* v) {
    NetCvode* d = (NetCvode*) v;
    if (ifarg(1)) {
        int i = (int) chkarg(1, 0, 10);
        d->print_event_ = i;
    }
    hoc_return_type_code = 1;  // integer
    return (double) d->print_event_;
}

static double n_record(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->vecrecord_add();
    return 0.;
}

static double n_remove(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->vec_remove();
    return 0.;
}

static double simgraph_remove(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->simgraph_remove();
    return 0.;
}

static double state_magnitudes(void* v) {
    NetCvode* d = (NetCvode*) v;
    return d->state_magnitudes();
}

static double tstop_event(void* v) {
    NetCvode* d = (NetCvode*) v;
    double x = *getarg(1);
    if (!cvode_active_) {  // watch out for fixed step roundoff if x
        // close to n*dt
        double y = x / nrn_threads->_dt;
        if (y > 1 && fabs(floor(y + 1e-6) - y) < 1e-6) {
            // printf("reduce %g to avoid fixed step roundoff\n", x);
            x -= nrn_threads->_dt / 4.;
        }
    }
    if (ifarg(2)) {
        Object* ppobj = nullptr;
        int reinit = 0;
        if (ifarg(3)) {
            ppobj = *hoc_objgetarg(3);
            if (!ppobj || ppobj->ctemplate->is_point_ <= 0 ||
                nrn_is_artificial_[ob2pntproc(ppobj)->prop->_type]) {
                hoc_execerror(hoc_object_name(ppobj), "is not a POINT_PROCESS");
            }
            reinit = int(chkarg(4, 0, 1));
        }
        if (hoc_is_object_arg(2)) {
            d->hoc_event(x, nullptr, ppobj, reinit, *hoc_objgetarg(2));
        } else {
            d->hoc_event(x, gargstr(2), ppobj, reinit);
        }
    } else {
        d->hoc_event(x, 0, 0, 0);
    }
    return x;
}

static double current_method(void* v) {
    NetCvode* d = (NetCvode*) v;
    hoc_return_type_code = 1;  // integer
    int modeltype = nrn_modeltype();
    int methodtype = secondorder;  // 0, 1, or 2
    int localtype = 0;
    if (cvode_active_) {
        methodtype = 3;
        if (d->use_daspk()) {
            methodtype = 4;
        } else {
            localtype = d->localstep();
        }
    }
    return double(modeltype + 10 * use_sparse13 + 100 * methodtype + 1000 * localtype);
}
static double peq(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->print_event_queue();
    return 1.;
}

static double event_queue_info(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->event_queue_info();
    return 1.;
}

static double store_events(void* v) {
    NetCvode* d = (NetCvode*) v;
    d->vec_event_store();
    return 1.;
}

static Object** netconlist(void* v) {
    NetCvode* d = (NetCvode*) v;
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
#if NRNMPI
    // assume single thread and global step
    NetCvode* d = (NetCvode*) v;
    assert(d->gcv_);
    d->gcv_->use_partrans_ = true;
    d->structure_change();
    return 1.0;
#else
    return 0.0;
#endif
}

static double nrn_structure_change_count(void* v) {
    hoc_return_type_code = 1;  // integer
    return double(structure_change_cnt);
}

static double nrn_diam_change_count(void* v) {
    hoc_return_type_code = 1;  // integer
    return double(diam_change_cnt);
}

using ExtraScatterList = std::vector<Object*>;
static ExtraScatterList* extra_scatterlist[2];  // 0 scatter, 1 gather

void nrn_extra_scatter_gather(int direction, int tid) {
    ExtraScatterList* esl = extra_scatterlist[direction];
    if (esl) {
        nrn_thread_error("extra_scatter_gather not allowed with multiple threads");
        for (Object* callable: *esl) {
            if (!neuron::python::methods.hoccommand_exec(callable)) {
                hoc_execerror("extra_scatter_gather runtime error", 0);
            }
        }
    }
}

static double extra_scatter_gather(void* v) {
    int direction = int(chkarg(1, 0, 1));
    Object* o = *hoc_objgetarg(2);
    check_obj_type(o, "PythonObject");
    ExtraScatterList* esl = extra_scatterlist[direction];
    if (!esl) {
        esl = new ExtraScatterList;
        extra_scatterlist[direction] = esl;
    }
    esl->push_back(o);
    hoc_obj_ref(o);
    return 0.;
}

static double extra_scatter_gather_remove(void* v) {
    Object* o = *hoc_objgetarg(1);
    for (int direction = 0; direction < 2; ++direction) {
        ExtraScatterList* esl = extra_scatterlist[direction];
        if (esl) {
            for (auto it = esl->begin(); it != esl->end();) {
                Object* o1 = *it;
                // if esl exists then python exists
                if (neuron::python::methods.pysame(o, o1)) {
                    it = esl->erase(it);
                    hoc_obj_unref(o1);
                } else {
                    ++it;
                }
            }
        }
    }
    return 0.;
}

static double use_fast_imem(void* v) {
    auto i = nrn_use_fast_imem;
    hoc_return_type_code = 2;  // boolean
    if (ifarg(1)) {
        nrn_use_fast_imem = chkarg(1, 0., 1.);
        nrn_fast_imem_alloc();
    }
    return i;
}

static double poolshrink(void*) {
    extern void nrn_poolshrink(int);
    int i = 0;
    if (ifarg(1)) {
        i = int(chkarg(1, 0., 1.));
    }
    nrn_poolshrink(i);
    return double(i);
}

static double free_event_queues(void*) {
    free_event_queues();
    return 0;
}

static Member_func members[] = {{"solve", solve},
                                {"atol", nrn_atol},
                                {"rtol", rtol},
                                {"re_init", re_init},
                                {"stiff", stiff},
                                {"active", active},
                                {"maxorder", maxorder},
                                {"minstep", minstep},
                                {"maxstep", maxstep},
                                {"jacobian", jacobian},
                                {"states", states},
                                {"dstates", dstates},
                                {"error_weights", error_weights},
                                {"acor", acor},
                                {"statename", statename},
                                {"atolscale", abstol},
                                {"use_local_dt", use_local_dt},
                                {"record", n_record},
                                {"record_remove", n_remove},
                                {"debug_event", debug_event},
                                {"order", order},
                                {"use_daspk", use_daspk},
                                {"event", tstop_event},
                                {"current_method", current_method},
                                {"use_mxb", use_mxb},
                                {"print_event_queue", peq},
                                {"event_queue_info", event_queue_info},
                                {"store_events", store_events},
                                {"condition_order", condition_order},
                                {"dae_init_dteps", dae_init_dteps},
                                {"simgraph_remove", simgraph_remove},
                                {"state_magnitudes", state_magnitudes},
                                {"ncs_netcons", ncs_netcons},
                                {"statistics", statistics},
                                {"spike_stat", spikestat},
                                {"queue_mode", queue_mode},
                                {"cache_efficient", cache_efficient},
                                {"use_long_double", use_long_double},
                                {"use_parallel", use_parallel},
                                {"f", nrn_hoc2fun},
                                {"yscatter", nrn_hoc2scatter_y},
                                {"ygather", nrn_hoc2gather_y},
                                {"fixed_step", nrn_hoc2fixed_step},
                                {"structure_change_count", nrn_structure_change_count},
                                {"diam_change_count", nrn_diam_change_count},
                                {"extra_scatter_gather", extra_scatter_gather},
                                {"extra_scatter_gather_remove", extra_scatter_gather_remove},
                                {"use_fast_imem", use_fast_imem},
                                {"poolshrink", poolshrink},
                                {"free_event_queues", free_event_queues},
                                {nullptr, nullptr}};

static Member_ret_obj_func omembers[] = {{"netconlist", netconlist}, {nullptr, nullptr}};

static void* cons(Object*) {
#if 0
	NetCvode* d;
	if (net_cvode_instance) {
		hoc_execerror("Only one CVode instance allowed", 0);
	}else{
		d = new NetCvode(1);
		net_cvode_instance = d;
	}
	active(nullptr);
	return (void*) d;
#else
    return (void*) net_cvode_instance;
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
    class2oc("CVode", cons, destruct, members, NULL, omembers, NULL);
    net_cvode_instance = new NetCvode(1);
    Daspk::dteps_ = 1e-9;  // change with cvode.dae_init_dteps(newval)
}

/* Functions Called by the CVODE Solver */

static int minit(CVodeMem cv_mem);
static int msetup(CVodeMem cv_mem,
                  int convfail,
                  N_Vector ypred,
                  N_Vector fpred,
                  booleantype* jcurPtr,
                  N_Vector vtemp,
                  N_Vector vtemp2,
                  N_Vector vtemp3);
static int msolve(CVodeMem cv_mem, N_Vector b, N_Vector weight, N_Vector ycur, N_Vector fcur);
static int msolve_lvardt(CVodeMem cv_mem,
                         N_Vector b,
                         N_Vector weight,
                         N_Vector ycur,
                         N_Vector fcur);
static void mfree(CVodeMem cv_mem);
static void f_gvardt(realtype t, N_Vector y, N_Vector ydot, void* f_data);
static void f_lvardt(realtype t, N_Vector y, N_Vector ydot, void* f_data);
static CVRhsFn pf_;

static void msolve_thread(neuron::model_sorted_token const&, NrnThread&);
static void* msolve_thread_part1(NrnThread*);
static void* msolve_thread_part2(NrnThread*);
static void* msolve_thread_part3(NrnThread*);
static void f_thread(neuron::model_sorted_token const&, NrnThread&);
static void f_thread_transfer_part1(neuron::model_sorted_token const&, NrnThread&);
static void f_thread_transfer_part2(neuron::model_sorted_token const&, NrnThread&);
static void* f_thread_ms_part1(NrnThread*);
static void* f_thread_ms_part2(NrnThread*);
static void* f_thread_ms_part3(NrnThread*);
static void* f_thread_ms_part4(NrnThread*);
static void* f_thread_ms_part34(NrnThread*);

Cvode::Cvode(NetCvode* ncv) {
    cvode_constructor();
    ncv_ = ncv;
}
Cvode::Cvode() {
    cvode_constructor();
}
void Cvode::cvode_constructor() {
    nthsizes_ = nullptr;
    nth_ = nullptr;
    ncv_ = nullptr;
    ctd_ = nullptr;
    tqitem_ = nullptr;
    mem_ = nullptr;
#if NEOSIMorNCS
    neosim_self_events_ = nullptr;
#endif
    initialize_ = false;
    can_retreat_ = false;
    tstop_begin_ = 0.;
    tstop_end_ = 0.;
    use_daspk_ = false;
    daspk_ = nullptr;

    mem_ = nullptr;
    y_ = nullptr;
    atolnvec_ = nullptr;
    maxstate_ = nullptr;
    maxacor_ = nullptr;
    neq_ = 0;
    structure_change_ = true;
#if NRNMPI
    use_partrans_ = false;
    global_neq_ = 0;
    opmode_ = 0;
#endif
}

double Cvode::gam() {
    if (mem_) {
        return ((CVodeMem) mem_)->cv_gamma;
    } else {
        return 1.;
    }
}

double Cvode::h() {
    if (mem_) {
        return ((CVodeMem) mem_)->cv_h;
    } else {
        return 0.;
    }
}

bool Cvode::at_time(double te, NrnThread* nt) {
    if (initialize_) {
        // printf("%d at_time initialize te=%g te-t0_=%g next_at_time_=%g\n", nt->id, te, te-t0_,
        // next_at_time_);
        MUTLOCK
        if (t0_ < te && te < next_at_time_) {
            // printf("%d next_at_time_=%g since te-t0_=%15.10g and next_at_time_-te=%g\n", nt->id,
            // te, te-nt->_t, next_at_time_-te);
            next_at_time_ = te;
        }
        MUTUNLOCK
        if (MyMath::eq(te, t0_, NetCvode::eps(t0_))) {
            // printf("at_time te=%g t-te=%g return 1\n", te, t - te);
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
    if (nt->_vcv) {
        if (te <= tstop_ && te > t0_) {
            Printf("te=%g t0_=%g tn_=%g t_=%g t=%g\n", te, t0_, tn_, t_, nt_t);
            Printf("te-t0_=%g  tstop_-te=%g\n", te - t0_, tstop_ - te);
        }
        assert(te > tstop_ || te <= t0_);
    }
    return 0;
}

void Cvode::set_init_flag() {
    // printf("set_init_flag t_=%g prior2init_=%d\n", t_, prior2init_);
    initialize_ = true;
    if (cvode_active_ && ++prior2init_ == 1) {
        record_continuous();
    }
}

N_Vector Cvode::nvnew(long int n) {
#if NRNMPI
    if (use_partrans_) {
        if (net_cvode_instance->use_long_double_) {
            return N_VNew_NrnParallelLD(0, n, global_neq_);
        } else {
            return N_VNew_Parallel(0, n, global_neq_);
        }
    }
#endif
    if (nctd_ > 1) {
        assert(n == neq_);
        if (!nthsizes_) {
            nthsizes_ = new long int[nrn_nthread];
            for (int i = 0; i < nrn_nthread; ++i) {
                nthsizes_[i] = ctd_[i].nvsize_;
            }
        }
#if 1
        int sum = 0;
        for (int i = 0; i < nctd_; ++i) {
            sum += nthsizes_[i];
        }
        assert(sum == neq_);
#endif
        if (net_cvode_instance->use_long_double_) {
            return N_VNew_NrnThreadLD(n, nctd_, nthsizes_);
        } else {
            return N_VNew_NrnThread(n, nctd_, nthsizes_);
        }
    }
    if (net_cvode_instance->use_long_double_) {
        return N_VNew_NrnSerialLD(n);
    } else {
        return N_VNew_Serial(n);
    }
}

void Cvode::atolvec_alloc(int i) {
    if (i > 0) {
        // too bad the machEnv has to be done here but this is
        // the first and it depends on size
        // the call chain is init_prepare (frees) -> init_eqn -> here
        atolnvec_ = nvnew(i);
    }
}

Cvode::~Cvode() {
#if NEOSIMorNCS
    if (neosim_self_events_) {
        delete neosim_self_events_;
    }
#endif
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
    if (maxstate_) {
        N_VDestroy(maxstate_);
        N_VDestroy(maxacor_);
    }
    if (nthsizes_) {
        delete[] nthsizes_;
    }
    //	delete [] iopt_;
    //	delete [] ropt_;
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
            y_ = nullptr;
        }
        if (mem_) {
            CVodeFree(mem_);
            mem_ = nullptr;
        }
        if (atolnvec_) {
            N_VDestroy(atolnvec_);
            atolnvec_ = nullptr;
        }
        if (daspk_) {
            delete daspk_;
            daspk_ = nullptr;
        }
        init_eqn();
        if (neq_ > 0) {
            y_ = nvnew(neq_);
            if (use_daspk_) {
                alloc_daspk();
            } else {
                alloc_cvode();
            }
            if (maxstate_) {
                activate_maxstate(false);
                activate_maxstate(true);
            }
        }
    }
}

void Cvode::activate_maxstate(bool on) {
    if (maxstate_) {
        N_VDestroy(maxstate_);
        N_VDestroy(maxacor_);
        maxstate_ = nullptr;
        maxacor_ = nullptr;
    }
    if (on && neq_ > 0) {
        maxstate_ = nvnew(neq_);
        maxacor_ = nvnew(neq_);
        N_VConst(0.0, maxstate_);
        N_VConst(0.0, maxacor_);
    }
}

static bool maxstate_b;
static Cvode* maxstate_cv;
static void* maxstate_thread(NrnThread* nt) {
    maxstate_cv->maxstate(maxstate_b, nt);
    return 0;
}
void Cvode::maxstate(bool b, NrnThread* nt) {
    if (!maxstate_) {
        return;
    }
    if (!nt) {
        if (nrn_nthread > 1) {
            maxstate_cv = this;
            maxstate_b = b;
            nrn_multithread_job(maxstate_thread);
            return;
        }
        nt = nrn_threads;
    }
    CvodeThreadData& z = ctd_[nt->id];
    int i;
    double x;
    double* y = n_vector_data(y_, nt->id);
    double* m = n_vector_data(maxstate_, nt->id);
    for (i = 0; i < z.nvsize_; ++i) {
        x = std::abs(y[i]);
        if (m[i] < x) {
            m[i] = x;
        }
    }
    if (b) {
        y = n_vector_data(acorvec(), nt->id);
        m = n_vector_data(maxacor_, nt->id);
        for (i = 0; i < z.nvsize_; ++i) {
            x = std::abs(y[i]);
            if (m[i] < x) {
                m[i] = x;
            }
        }
    }
}

void Cvode::maxstate(double* pd) {
    int i;
    NrnThread* nt;
    if (maxstate_) {
        FOR_THREADS(nt) {
            double* m = n_vector_data(maxstate_, nt->id);
            int n = ctd_[nt->id].nvsize_;
            int o = ctd_[nt->id].nvoffset_;
            for (i = 0; i < n; ++i) {
                pd[i + o] = m[i];
            }
        }
    }
}

void Cvode::maxacor(double* pd) {
    int i;
    NrnThread* nt;
    if (maxacor_) {
        FOR_THREADS(nt) {
            double* m = n_vector_data(maxacor_, nt->id);
            int n = ctd_[nt->id].nvsize_;
            int o = ctd_[nt->id].nvoffset_;
            for (i = 0; i < n; ++i) {
                pd[i + o] = m[i];
            }
        }
    }
}

void Cvode::alloc_cvode() {}

int Cvode::order() {
    int i = 0;
    if (use_daspk_) {
        if (daspk_->mem_) {
            IDAGetLastOrder(daspk_->mem_, &i);
        }
    } else {
        if (mem_) {
            CVodeGetLastOrder(mem_, &i);
        }
    }
    return i;
}
void Cvode::maxorder(int maxord) {
    if (use_daspk_) {
        if (daspk_->mem_) {
            IDASetMaxOrd(daspk_->mem_, maxord);
        }
    } else {
        if (mem_) {
            CVodeSetMaxOrd(mem_, maxord);
        }
    }
}
void Cvode::minstep(double x) {
    if (mem_) {
        if (x > 0.) {
            CVodeSetMinStep(mem_, x);
        } else {
            // CVodeSetMinStep requires x > 0 but
            // HMIN_DEFAULT is ZERO in cvodes.cpp
            ((CVodeMem) mem_)->cv_hmin = 0.;
        }
    }
}
void Cvode::maxstep(double x) {
    if (use_daspk_) {
        if (daspk_->mem_) {
            IDASetMaxStep(daspk_->mem_, x);
        }
    } else {
        if (mem_) {
            CVodeSetMaxStep(mem_, x);
        }
    }
}

void Cvode::free_cvodemem() {
    if (mem_) {
        CVodeFree(mem_);
        mem_ = nullptr;
    }
}

void NetCvode::set_CVRhsFn() {
    MUTDESTRUCT
    static_mutex_for_at_time(false);
    if (single_) {
        pf_ = f_gvardt;
        if (nrn_nthread > 1) {
            MUTCONSTRUCT(1)
            static_mutex_for_at_time(true);
        }
    } else {
        pf_ = f_lvardt;
    }
}

int Cvode::cvode_init(double) {
    int err = SUCCESS;
    // note, a change in stiff_ due to call of stiff() destroys mem_
    gather_y(y_);
    // TODO: this needs changed if want to support more than one thread or local variable timestep
    nrn_nonvint_block_ode_reinit(neq_, N_VGetArrayPointer(y_), 0);
    if (mem_) {
        err = CVodeReInit(mem_, pf_, t0_, y_, CV_SV, &ncv_->rtol_, atolnvec_);
        // printf("CVodeReInit\n");
        if (err != SUCCESS) {
            Printf("Cvode %p %s CVReInit error %d\n",
                   this,
                   secname(ctd_[0].v_node_[ctd_[0].rootnodecount_]->sec),
                   err);
            return err;
        }
    } else {
        mem_ = CVodeCreate(CV_BDF, ncv_->stiff() ? CV_NEWTON : CV_FUNCTIONAL);
        if (!mem_) {
            hoc_execerror("CVodeCreate error", 0);
        }
        maxorder(ncv_->maxorder());  // Memory Leak if changed after CVodeMalloc
        minstep(ncv_->minstep());
        maxstep(ncv_->maxstep());
        CVodeMalloc(mem_, pf_, t0_, y_, CV_SV, &ncv_->rtol_, atolnvec_);
        if (err != SUCCESS) {
            Printf("Cvode %p %s CVodeMalloc error %d\n",
                   this,
                   secname(ctd_[0].v_node_[ctd_[0].rootnodecount_]->sec),
                   err);
            return err;
        }
        //		CVodeSetInitStep(mem_, .01);
    }
    matmeth();
    ((CVodeMem) mem_)->cv_gamma = 0.;
    ((CVodeMem) mem_)->cv_h = 0.;  // fun called before cvode sets this (though fun does not need it
                                   // really)
    // fun(t_, N_VGetArrayPointer(y_), nullptr);
    auto const sorted_token = nrn_ensure_model_data_are_sorted();
    std::pair<Cvode*, neuron::model_sorted_token const&> opaque{this, sorted_token};
    pf_(t_, y_, nullptr, &opaque);
    can_retreat_ = false;
    return err;
}

int Cvode::daspk_init(double tout) {
    return daspk_->init();
}

void Cvode::alloc_daspk() {
    // printf("Cvode::alloc_daspk\n");
    daspk_ = new Daspk(this, neq_);
    // we do our own initialization since it is hard to
    // figure out which equations are algebraic and which
    // differential. eg. the no cap nodes can have a
    // circuit with capacitance connection. Extracellular
    // nodes may or may not have capacitors to ground.
}

int Cvode::advance_tn(neuron::model_sorted_token const& sorted_token) {
    int err = SUCCESS;
    if (neq_ == 0) {
        t_ += 1e9;
        if (nth_) {
            nth_->_t = t_;
        } else {
            nt_t = t_;
        }
        tn_ = t_;
        return err;
    }
    // printf("%d Cvode::advance_tn enter t_=%.20g t0_=%.20g tstop_=%.20g\n", nrnmpi_myid, t_, t0_,
    // tstop_);
    if (t_ >= tstop_ - NetCvode::eps(t_)) {
        // printf("init\n");
        ++ts_inits_;
        tstop_begin_ = tstop_;
        tstop_end_ = tstop_ + 1.5 * NetCvode::eps(tstop_);
        err = init(tstop_end_);
        // the above 1.5 is due to the fact that at_time will check
        // to see if the time is greater but not greater than eps*t0_
        // of the at_time for a return of 1.
        can_retreat_ = false;
    } else {
        ++advance_calls_;
        // SOLVE after_cvode now interpreted as before cvode since
        // a step may be abandoned in part by interpolate in response
        // to events. Now at least the value of t is monotonic
        if (nth_) {
            nth_->_t = t_;
        } else {
            nt_t = t_;
        }
        do_nonode(sorted_token, nth_);
#if NRNMPI
        opmode_ = 1;
#endif
        if (use_daspk_) {
            err = daspk_advance_tn();
        } else {
            err = cvode_advance_tn(sorted_token);
        }
        can_retreat_ = true;
        maxstate(true);
    }
    // printf("%d Cvode::advance_tn exit t_=%.20g t0_=%.20g tstop_=%.20g\n", nrnmpi_myid, t_, t0_,
    // tstop_);
    return err;
}

int Cvode::solve() {
    // printf("%d Cvode::solve %p initialize = %d current_time=%g tn=%g\n", nrnmpi_myid, this,
    // initialize_, t_, tn());
    int err = SUCCESS;
    if (initialize_) {
        if (t_ >= tstop_ - NetCvode::eps(t_)) {
            ++ts_inits_;
            tstop_begin_ = tstop_;
            tstop_end_ = tstop_ + 1.5 * NetCvode::eps(tstop_);
            err = init(tstop_end_);
            can_retreat_ = false;
        } else {
            err = init(t_);
        }
    } else {
        err = advance_tn(nrn_ensure_model_data_are_sorted());
    }
    // printf("Cvode::solve exit %p current_time=%g tn=%g\n", this, t_, tn());
    return err;
}

int Cvode::init(double tout) {
    int err = SUCCESS;
    ++init_calls_;
    // printf("%d Cvode_%p::init tout=%g\n", nrnmpi_myid, this, tout);
    initialize_ = true;
    t_ = tout;
    t0_ = t_;
    tn_ = t_;
    next_at_time_ = t_ + 1e5;
    init_prepare();
    if (neq_) {
#if NRNMPI
        opmode_ = 3;
#endif
        if (use_daspk_) {
            err = daspk_init(t_);
        } else {
            err = cvode_init(t_);
        }
    }
    tstop_ = next_at_time_ - NetCvode::eps(next_at_time_);
#if NRNMPI
    if (use_partrans_) {
        tstop_ = nrnmpi_dbl_allmin(tstop_);
    }
#endif
    // printf("Cvode::init next_at_time_=%g tstop_=%.15g\n", next_at_time_, tstop_);
    initialize_ = false;
    prior2init_ = 0;
    maxstate(false);
    return err;
}

int Cvode::interpolate(double tout) {
    NrnThread* _nt;
    if (neq_ == 0) {
        t_ = tout;
        if (nth_) {
            nth_->_t = t_;
        } else {
            FOR_THREADS(_nt) {
                _nt->_t = t_;
            }
        }
        return SUCCESS;
    }
    // printf("Cvode::interpolate t_=%.15g t0_=%.15g tn_=%.15g tstop_=%.15g\n", t_, t0_, tn_,
    // tstop_);
    if (!can_retreat_) {
        // but must be within the initialization domain
        assert(MyMath::le(tout, t_, 2. * NetCvode::eps(t_)));
        if (nth_) {  // lvardt
            nth_->_t = tout;
        } else {
            FOR_THREADS(_nt) {
                _nt->_t = tout;  // but leave t_ at the initialization point.
            }
        }
        //		printf("no interpolation for event in the initialization interval t=%15g t_=%15g\n",
        // nrn_threads->t, t_);
        return SUCCESS;
    }
    if (MyMath::eq(tout, t_, NetCvode::eps(t_))) {
        t_ = tout;
        return SUCCESS;
    }
    // if (initialize_) {
    // printf("Cvode_%p::interpolate assert error when initialize_ is true.\n t_=%g tout=%g tout-t_
    // = %g\n", this, t_, tout, tout-t_);
    //}
    assert(initialize_ == false);  // or state discontinuity can be lost
// printf("interpolate t_=%g tout=%g delta t_-tout=%g\n", t_, tout, t_-tout);
#if 1
    if (tout < t0_) {
        //	if (tout < t0_ - NetCvode::eps(t0_)) { // t0_ not always == tn_ - h
        // after a CV_ONESTEP_TSTOP
        // so allow some fudge before printing error
        // The fudge case was avoided by returning from the
        // Cvode::handle_step when a first order condition check
        // puts an event on the queue equal to t_
        Printf("Cvode::interpolate assert error t0=%g tout-t0=%g eps*t_=%g\n",
               t0_,
               tout - t0_,
               NetCvode::eps(t_));
        //	}
        tout = t0_;
    }
    if (tout > tn_) {
        Printf("Cvode::interpolate assert error tn=%g tn-tout=%g  eps*t_=%g\n",
               tn_,
               tn_ - tout,
               NetCvode::eps(t_));
        tout = tn_;
    }
#endif
    // if there is a problem here it may be due to the at_time skipping interval
    // since the integration will not go past tstop_ and will take up at tstop+2eps
    // see the Cvode::advance_tn() implementation
    assert(tout >= t0() && tout <= tn());

    ++interpolate_calls_;
#if NRNMPI
    opmode_ = 2;
#endif
    if (use_daspk_) {
        return daspk_->interpolate(tout);
    } else {
        return cvode_interpolate(tout);
    }
}

int Cvode::cvode_advance_tn(neuron::model_sorted_token const& sorted_token) {
#if PRINT_EVENT
    if (net_cvode_instance->print_event_ > 1) {
        Printf("Cvode::cvode_advance_tn %p %d initialize_=%d tstop=%.20g t_=%.20g to ",
               this,
               nth_ ? nth_->id : 0,
               initialize_,
               tstop_,
               t_);
    }
#endif
    std::pair<Cvode*, neuron::model_sorted_token const&> opaque{this, sorted_token};
    CVodeSetFdata(mem_, &opaque);
    CVodeSetStopTime(mem_, tstop_);
    // printf("cvode_advance_tn begin t0_=%g t_=%g tn_=%g tstop=%g\n", t0_, t_, tn_, tstop_);
    int err = CVode(mem_, tstop_, y_, &t_, CV_ONE_STEP_TSTOP);
    CVodeSetFdata(mem_, nullptr);
#if PRINT_EVENT
    if (net_cvode_instance->print_event_ > 1) {
        Printf("t_=%.20g\n", t_);
    }
#endif
    if (err < 0) {
        Printf("CVode %p %s advance_tn failed, err=%d.\n",
               this,
               secname(ctd_[0].v_node_[ctd_[0].rootnodecount_]->sec),
               err);
        pf_(t_, y_, nullptr, &opaque);
        return err;
    }
    // this is very bad, performance-wise. However cvode modifies its states
    // after a call to fun with the proper t.
    pf_(t_, y_, nullptr, &opaque);
    tn_ = ((CVodeMem) mem_)->cv_tn;
    t0_ = tn_ - ((CVodeMem) mem_)->cv_h;
    // printf("t=%.15g t_=%.15g tn()=%.15g tstop_=%.15g t0_=%.15g\n", nrn_threads->t, t_, tn(),
    // tstop_, t0_); 	printf("t_=%g h=%g q=%d y=%g\n", t_, ((CVodeMem)mem_)->cv_h,
    //((CVodeMem)mem_)->cv_q, N_VIth(y_,0)); printf("cvode_advance_tn end t0_=%g t_=%g tn_=%g\n",
    // t0_, t_, tn_);
    return SUCCESS;
}

int Cvode::cvode_interpolate(double tout) {
#if PRINT_EVENT
    if (net_cvode_instance->print_event_ > 1) {
        Printf("Cvode::cvode_interpolate %p %d initialize_%d t=%.20g to ",
               this,
               nth_ ? nth_->id : 0,
               initialize_,
               t_);
    }
#endif
    // avoid CVode-- tstop = 0.5 is behind  current t = 0.5
    // is this really necessary anymore. Maybe NORMAL mode ignores tstop
    auto const sorted_token = nrn_ensure_model_data_are_sorted();
    std::pair<Cvode*, neuron::model_sorted_token const&> opaque{this, sorted_token};
    CVodeSetFdata(mem_, &opaque);
    CVodeSetStopTime(mem_, tstop_ + tstop_);
    int err = CVode(mem_, tout, y_, &t_, CV_NORMAL);
    CVodeSetFdata(mem_, nullptr);
#if PRINT_EVENT
    if (net_cvode_instance->print_event_ > 1) {
        Printf("%.20g\n", t_);
    }
#endif
    if (err < 0) {
        Printf("CVode %p %s interpolate failed, err=%d.\n",
               this,
               secname(ctd_[0].v_node_[ctd_[0].rootnodecount_]->sec),
               err);
        return err;
    }
    pf_(t_, y_, nullptr, &opaque);
    //	printf("t_=%g h=%g q=%d y=%g\n", t_, ((CVodeMem)mem_)->cv_h, ((CVodeMem)mem_)->cv_q,
    // N_VIth(y_,0));
    return SUCCESS;
}

int Cvode::daspk_advance_tn() {
    int err;
    // printf("Cvode::solve test stop time t=%.20g tstop-t=%g\n", t, tstop_-t);
    // printf("in Cvode::solve t_=%g tstop=%g calling  daspk_->solve(%g)\n", t_, tstop_,
    // daspk_->tout_);
    err = daspk_->advance_tn(tstop_);
    // printf("in Cvode::solve returning from daspk_->solve t_=%g initialize_=%d tstop=%g\n", t_,
    // initialize__, tstop_);
    if (err < 0) {
        return err;
    }
    return SUCCESS;
}

N_Vector Cvode::ewtvec() {
    if (use_daspk_) {
        return daspk_->ewtvec();
    } else {
        return ((CVodeMem) mem_)->cv_ewt;
    }
}

N_Vector Cvode::acorvec() {
    if (use_daspk_) {
        return daspk_->acorvec();
    } else {
        return ((CVodeMem) mem_)->cv_acor;
    }
}

void Cvode::statistics() {
#if 1
    Printf("\nCvode instance %p %s statistics : %d %s states\n",
           this,
           secname(ctd_[0].v_node_[ctd_[0].rootnodecount_]->sec),
           neq_,
           (use_daspk_ ? "IDA" : "CVode"));
    Printf("   %d advance_tn, %d interpolate, %d init (%d due to at_time)\n",
           advance_calls_,
           interpolate_calls_,
           init_calls_,
           ts_inits_);
    Printf("   %d function evaluations, %d mx=b solves, %d jacobian setups\n",
           f_calls_,
           mxb_calls_,
           jac_calls_);
    if (use_daspk_) {
        daspk_->statistics();
        return;
    }
#else
    Printf("\nCVode Statistics.. \n\n");
    Printf("internal steps = %d\nfunction evaluations = %d\n", iopt_[NST], iopt_[NFE]);
    Printf(
        "newton iterations = %d  setups = %d\n nonlinear convergence failures = %d\n\
 local error test failures = %ld\n",
        iopt_[NNI],
        iopt_[NSETUPS],
        iopt_[NCFN],
        iopt_[NETF]);
    Printf("order=%d stepsize=%g\n", iopt_[QU], h());
#endif
}

void Cvode::matmeth() {
    switch (ncv_->jacobian()) {
    case 1:
        CVDense(mem_, neq_);
        break;
    case 2:
        CVDiag(mem_);
        break;
    default:
        // free previous method
        if (((CVodeMem) mem_)->cv_lfree) {
            ((CVodeMem) mem_)->cv_lfree((CVodeMem) mem_);
            ((CVodeMem) mem_)->cv_lfree = NULL;
        }

        ((CVodeMem) mem_)->cv_linit = minit;
        ((CVodeMem) mem_)->cv_lsetup = msetup;
        ((CVodeMem) mem_)->cv_setupNonNull = TRUE;  // but since our's does not do anything...
        if (nth_) {                                 // lvardt
            ((CVodeMem) mem_)->cv_lsolve = msolve_lvardt;
        } else {
            ((CVodeMem) mem_)->cv_lsolve = msolve;
        }
        ((CVodeMem) mem_)->cv_lfree = mfree;
        break;
    }
}

static int minit(CVodeMem) {
    //	printf("minit\n");
    return CV_NO_FAILURES;
}

static int msetup(CVodeMem m,
                  int convfail,
                  N_Vector yp,
                  N_Vector fp,
                  booleantype* jcurPtr,
                  N_Vector,
                  N_Vector,
                  N_Vector) {
    //	printf("msetup\n");
    *jcurPtr = true;
    auto* const cv =
        static_cast<std::pair<Cvode*, neuron::model_sorted_token const&>*>(m->cv_f_data)->first;
    return cv->setup(yp, fp);
}

static N_Vector msolve_b_;
static N_Vector msolve_ycur_;
static Cvode* msolve_cv_;
static int msolve(CVodeMem m, N_Vector b, N_Vector weight, N_Vector ycur, N_Vector fcur) {
    //	printf("msolve gamma=%g gammap=%g gamrat=%g\n", m->cv_gamma, m->cv_gammap, m->cv_gamrat);
    //	N_VIth(b, 0) /= (1. + m->cv_gamma);
    //	N_VIth(b, 0) /= (1. + m->cv_gammap);
    //	N_VIth(b,0) *= 2./(1. + m->cv_gamrat);
    auto* const f_typed_data = static_cast<std::pair<Cvode*, neuron::model_sorted_token const&>*>(
        m->cv_f_data);
    msolve_cv_ = f_typed_data->first;
    auto const& sorted_token = f_typed_data->second;
    Cvode& cv = *msolve_cv_;
    ++cv.mxb_calls_;
    if (cv.ncv_->stiff() == 0) {
        return 0;
    }
    if (cv.gam() == 0.) {
        return 0;
    }  // i.e. (I - gamma * J)*x = b means x = b
    msolve_b_ = b;
    msolve_ycur_ = ycur;
    if (nrn_multisplit_setup_ && nrn_nthread > 1) {
        nrn_multithread_job(msolve_thread_part1);
        nrn_multithread_job(msolve_thread_part2);
        nrn_multithread_job(msolve_thread_part3);
    } else {
        nrn_multithread_job(sorted_token, msolve_thread);
    }
    return 0;
}
static int msolve_lvardt(CVodeMem m, N_Vector b, N_Vector weight, N_Vector ycur, N_Vector fcur) {
    auto* const f_typed_data = static_cast<std::pair<Cvode*, neuron::model_sorted_token const&>*>(
        m->cv_f_data);
    auto* const cv = f_typed_data->first;
    auto const& sorted_token = f_typed_data->second;
    ++cv->mxb_calls_;
    if (cv->ncv_->stiff() == 0) {
        return 0;
    }
    if (cv->gam() == 0.) {
        return 0;
    }
    cv->nth_->_vcv = cv;
    cv->solvex_thread(sorted_token, cv->n_vector_data(b, 0), cv->n_vector_data(ycur, 0), cv->nth_);
    cv->nth_->_vcv = 0;
    return 0;
}
static void msolve_thread(neuron::model_sorted_token const& sorted_token, NrnThread& nt) {
    int i = nt.id;
    Cvode* cv = msolve_cv_;
    nt._vcv = cv;
    cv->solvex_thread(sorted_token,
                      cv->n_vector_data(msolve_b_, i),
                      cv->n_vector_data(msolve_ycur_, i),
                      &nt);
    nt._vcv = 0;
}
static void* msolve_thread_part1(NrnThread* nt) {
    int i = nt->id;
    Cvode* cv = msolve_cv_;
    nt->_vcv = cv;
    cv->solvex_thread_part1(cv->n_vector_data(msolve_b_, i), nt);
    return 0;
}
static void* msolve_thread_part2(NrnThread* nt) {
    Cvode* cv = msolve_cv_;
    cv->solvex_thread_part2(nt);
    return 0;
}
static void* msolve_thread_part3(NrnThread* nt) {
    int i = nt->id;
    Cvode* cv = msolve_cv_;
    cv->solvex_thread_part3(cv->n_vector_data(msolve_b_, i), nt);
    nt->_vcv = 0;
    return 0;
}

static void mfree(CVodeMem) {
    //	printf("mfree\n");
}

static realtype f_t_;
static N_Vector f_y_;
static N_Vector f_ydot_;
static Cvode* f_cv_;
static void f_gvardt(realtype t, N_Vector y, N_Vector ydot, void* f_data) {
    auto* const f_typed_data = static_cast<std::pair<Cvode*, neuron::model_sorted_token const&>*>(
        f_data);
    f_cv_ = f_typed_data->first;
    // ydot[0] = -y[0];
    //	N_VIth(ydot, 0) = -N_VIth(y, 0);
    // printf("f(%g, %p, %p)\n", t, y, ydot);
    ++f_cv_->f_calls_;
    f_t_ = t;
    f_y_ = y;
    f_ydot_ = ydot;
    if (nrn_nthread > 1 || nrnmpi_numprocs > 1) {
        if (nrn_multisplit_setup_) {
            nrn_multithread_job(f_thread_ms_part1);
            nrn_multithread_job(f_thread_ms_part2);
            if (nrnthread_v_transfer_) {
                nrn_multithread_job(f_thread_ms_part3);
                if (nrnmpi_v_transfer_) {
                    (*nrnmpi_v_transfer_)();
                }
                nrn_multithread_job(f_thread_ms_part4);
            } else {
                nrn_multithread_job(f_thread_ms_part34);
            }
        } else if (nrnthread_v_transfer_) {
            nrn_multithread_job(f_typed_data->second, f_thread_transfer_part1);
            if (nrnmpi_v_transfer_) {
                (*nrnmpi_v_transfer_)();
            }
            nrn_multithread_job(f_typed_data->second, f_thread_transfer_part2);
        } else {
            nrn_multithread_job(f_typed_data->second, f_thread);
        }
    } else {
        nrn_multithread_job(f_typed_data->second, f_thread);
    }
}
static void f_lvardt(realtype t, N_Vector y, N_Vector ydot, void* f_data) {
    auto* const f_typed_data = static_cast<std::pair<Cvode*, neuron::model_sorted_token const&>*>(
        f_data);
    auto* const cv = f_typed_data->first;
    auto const& sorted_token = f_typed_data->second;
    ++cv->f_calls_;
    cv->nth_->_vcv = cv;
    cv->fun_thread(sorted_token, t, cv->n_vector_data(y, 0), cv->n_vector_data(ydot, 0), cv->nth_);
    cv->nth_->_vcv = 0;
}

static void f_thread(neuron::model_sorted_token const& sorted_token, NrnThread& ntr) {
    auto* const nt = &ntr;
    int i = nt->id;
    Cvode* cv = f_cv_;
    nt->_vcv = cv;
    cv->fun_thread(
        sorted_token, f_t_, cv->n_vector_data(f_y_, i), cv->n_vector_data(f_ydot_, i), &ntr);
    nt->_vcv = 0;
}
static void f_thread_transfer_part1(neuron::model_sorted_token const& sorted_token,
                                    NrnThread& ntr) {
    auto* const nt = &ntr;
    int i = nt->id;
    Cvode* cv = f_cv_;
    nt->_vcv = cv;
    cv->fun_thread_transfer_part1(sorted_token, f_t_, cv->n_vector_data(f_y_, i), nt);
}
static void f_thread_transfer_part2(neuron::model_sorted_token const& sorted_token,
                                    NrnThread& ntr) {
    auto* const nt = &ntr;
    int i = nt->id;
    Cvode* cv = f_cv_;
    cv->fun_thread_transfer_part2(sorted_token, cv->n_vector_data(f_ydot_, i), &ntr);
    nt->_vcv = 0;
}
static void* f_thread_ms_part1(NrnThread* nt) {
    int i = nt->id;
    Cvode* cv = f_cv_;
    nt->_vcv = cv;
    cv->fun_thread_ms_part1(f_t_, cv->n_vector_data(f_y_, i), nt);
    return 0;
}
static void* f_thread_ms_part2(NrnThread* nt) {
    Cvode* cv = f_cv_;
    cv->fun_thread_ms_part2(nt);
    return 0;
}
static void* f_thread_ms_part3(NrnThread* nt) {
    Cvode* cv = f_cv_;
    cv->fun_thread_ms_part3(nt);
    return 0;
}
static void* f_thread_ms_part4(NrnThread* nt) {
    int i = nt->id;
    Cvode* cv = f_cv_;
    cv->fun_thread_ms_part4(cv->n_vector_data(f_ydot_, i), nt);
    return 0;
}
static void* f_thread_ms_part34(NrnThread* nt) {
    int i = nt->id;
    Cvode* cv = f_cv_;
    cv->fun_thread_ms_part34(cv->n_vector_data(f_ydot_, i), nt);
    nt->_vcv = 0;
    return 0;
}
