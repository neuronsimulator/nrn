#include <../../nrnconf.h>
// differential algebraic system solver interface to DDASPK

// DDASPK is translated from fortran with f2c. Hence all args are
// call by reference

#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include "spmatrix.h"
#include "nrnoc2iv.h"
#include "cvodeobj.h"
#include "nrndaspk.h"
#include "netcvode.h"
#include "nrn_ansi.h"
#include "vecplay_tplus.h"
#include "nrndae.h"
#include "ida/ida.h"
#include "ida/ida_impl.h"
#include "mymath.h"

// the state of the g - d2/dx2 matrix for voltages
#define INVALID  0
#define NO_CAP   1
#define SETUP    2
#define FACTORED 3
static int solve_state_;

// prototypes

double Daspk::dteps_;


extern void nrndae_dkres(double*, double*, double*);
extern void nrndae_dkpsol(double);
extern int nrndae_battery_ic_project();
extern void nrndae_seed_yp_from_f(double* f, double* yp);
extern int nrndae_complete_yp_from_forcing(double* yp, const std::vector<NrnForcingTPlus>& forcing);
extern void nrndae_append_dforce_to_forcing_list(double tt, std::vector<NrnForcingTPlus>& out);
extern void nrn_solve(NrnThread*);
extern int nrn_sparse13_soft_fail;
extern int nrn_sparse13_factor_error();
void nrn_daspk_init_step(double, double, int);
void nrn_cable_battery_ic();
#if EXTRACELLULAR
void nrn_extracellular_battery_ic();
#endif
// this is private in ida.cpp but we want to check if our initialization
// is good. Unfortunately ewt is set on the first call to solve which
// is too late for us.
extern "C" {
extern booleantype IDAEwtSet(IDAMem IDA_mem, N_Vector ycur);
}  // extern "C"

// extern double t, dt;
#define nt_dt nrn_threads->_dt
#define nt_t  nrn_threads->_t

static void daspk_nrn_solve(NrnThread* nt) {
    nrn_solve(nt);
}

static int res_gvardt(realtype t, N_Vector y, N_Vector yp, N_Vector delta, void* rdata);

static int minit(IDAMem);

static int msetup(IDAMem mem,
                  N_Vector y,
                  N_Vector ydot,
                  N_Vector delta,
                  N_Vector tempv1,
                  N_Vector tempv2,
                  N_Vector tempv3);

static int msolve(IDAMem mem, N_Vector b, N_Vector ycur, N_Vector ypcur, N_Vector deltacur);

static int mfree(IDAMem);


// at least in DARWIN the following is already declared so avoid conflict
#define thread_t nrn_thread_t

// residual
static N_Vector nvec_y;
static N_Vector nvec_yp;
static N_Vector nvec_delta;
static double thread_t;
static double thread_cj;
static int thread_ier;
static Cvode* thread_cv;
static void* res_thread(NrnThread* nt) {
    int i = nt->id;
    Cvode* cv = thread_cv;
    int ier = cv->res(thread_t,
                      cv->n_vector_data(nvec_y, i),
                      cv->n_vector_data(nvec_yp, i),
                      cv->n_vector_data(nvec_delta, i),
                      nt);
    if (ier != 0) {
        thread_ier = ier;
    }
    return 0;
}
static int res_gvardt(realtype t, N_Vector y, N_Vector yp, N_Vector delta, void* rdata) {
    thread_cv = (Cvode*) rdata;
    nvec_y = y;
    nvec_yp = yp;
    nvec_delta = delta;
    thread_t = t;
    thread_ier = 0;
    nrn_multithread_job(res_thread);
    return thread_ier;
}

// linear solver specific allocation and initialization
static int minit(IDAMem) {
    return IDA_SUCCESS;
}

// linear solver preparation for subsequent calls to msolve
// approximation to jacobian. Everything necessary for solving P*x = b
static int msetup(IDAMem mem, N_Vector y, N_Vector yp, N_Vector, N_Vector, N_Vector, N_Vector) {
    Cvode* cv = (Cvode*) mem->ida_rdata;
    ++cv->jac_calls_;
    return 0;
}

/* solve P*x = b */
static void* msolve_thread(NrnThread* nt) {
    int i = nt->id;
    Cvode* cv = thread_cv;
    int ier = cv->psol(
        thread_t, cv->n_vector_data(nvec_y, i), cv->n_vector_data(nvec_yp, i), thread_cj, nt);
    if (ier != 0) {
        thread_ier = ier;
    }
    return 0;
}
static int msolve(IDAMem mem, N_Vector b, N_Vector w, N_Vector ycur, N_Vector, N_Vector) {
    thread_cv = (Cvode*) mem->ida_rdata;
    thread_t = mem->ida_tn;
    nvec_y = ycur;
    nvec_yp = b;
    thread_cj = mem->ida_cj;
    thread_ier = 0;
    nrn_multithread_job(msolve_thread);
    if (nrn_sparse13_factor_error()) {
        // Recoverable linear solve failure (e.g. singular J during IDA_Y_INIT).
        return 1;
    }
    return thread_ier;
}

static int mfree(IDAMem) {
    return IDA_SUCCESS;
}

Daspk::Daspk(Cvode* cv, int neq) {
    //	printf("Daspk::Daspk\n");
    cv_ = cv;
    yp_ = cv->nvnew(neq);
    delta_ = cv->nvnew(neq);
    parasite_ = cv->nvnew(neq);
    use_parasite_ = false;
    spmat_ = nullptr;
    mem_ = nullptr;
    audit_pre_t_ = 0.;
    audit_pre_max_abs_ = 0.;
    audit_pre_wrms_ = -1.;
    audit_pre_neq_ = 0;
    audit_pre_valid_ = false;
    audit_pre_res_valid_ = false;
}

Daspk::~Daspk() {
    //	printf("Daspk::~Daspk\n");
    N_VDestroy(delta_);
    N_VDestroy(yp_);
    if (mem_) {
        IDAFree((IDAMem) mem_);
    }
}

void Daspk::ida_init() {
    int ier;
    if (mem_) {
        ier = IDAReInit(
            mem_, res_gvardt, cv_->t_, cv_->y_, yp_, IDA_SV, &cv_->ncv_->rtol_, cv_->atolnvec_);
        if (ier < 0) {
            hoc_execerror("IDAReInit error", 0);
        }
    } else {
        IDAMem mem = (IDAMem) IDACreate();
        if (!mem) {
            hoc_execerror("IDAMalloc error", 0);
        }
        IDASetRdata(mem, cv_);
        ier = IDAMalloc(
            mem, res_gvardt, cv_->t_, cv_->y_, yp_, IDA_SV, &cv_->ncv_->rtol_, cv_->atolnvec_);
        mem->ida_linit = minit;
        mem->ida_lsetup = msetup;
        mem->ida_lsolve = msolve;
        mem->ida_lfree = mfree;
        mem->ida_setupNonNull = false;
        mem_ = mem;
    }
}

void Daspk::info() {}


// last two bits, 0 error, 1 warning, 2 apply parasitic
// if init_failure_style & 010, then use the original method
int Daspk::init_failure_style_;
int Daspk::init_try_again_;
int Daspk::first_try_init_failures_;
int Daspk::init_mode_;
int Daspk::calcic_fallback_count_;
int Daspk::ic_init_count_ = 0;
int Daspk::ic_mode3_ok_count_ = 0;
int Daspk::ic_mode3_fallback_count_ = 0;
int Daspk::ic_forcing_play_inits_ = 0;
int Daspk::ic_forcing_dforce_inits_ = 0;
int Daspk::ic_forcing_fd_inits_ = 0;
int Daspk::last_ic_path_mode_ = -1;
int Daspk::last_ic_forcing_flags_ = 0;
int Daspk::audit_level_ = 0;
double Daspk::audit_t_select_ = 0.;
int Daspk::audit_armed_ = 0;
int Daspk::audit_serial_ = 0;
std::string Daspk::audit_path_;
std::vector<NrnForcingTPlus> Daspk::last_forcing_tplus_;
double Daspk::last_forcing_t_ = 0.;

const std::vector<NrnForcingTPlus>& Daspk::last_forcing_tplus() {
    return last_forcing_tplus_;
}

double Daspk::last_forcing_t() {
    return last_forcing_t_;
}

void Daspk::reset_ic_stats() {
    ic_init_count_ = 0;
    ic_mode3_ok_count_ = 0;
    ic_mode3_fallback_count_ = 0;
    ic_forcing_play_inits_ = 0;
    ic_forcing_dforce_inits_ = 0;
    ic_forcing_fd_inits_ = 0;
    calcic_fallback_count_ = 0;
    last_ic_path_mode_ = -1;
    last_ic_forcing_flags_ = 0;
}

int Daspk::last_ic_path_mode() {
    return last_ic_path_mode_;
}
int Daspk::last_ic_forcing_flags() {
    return last_ic_forcing_flags_;
}
int Daspk::ic_mode3_ok_count() {
    return ic_mode3_ok_count_;
}
int Daspk::ic_mode3_fallback_count() {
    return ic_mode3_fallback_count_;
}

void Daspk::print_ic_stats() {
    if (!ic_init_count_ && !calcic_fallback_count_ && !first_try_init_failures_) {
        return;
    }
    Printf("   IDA IC: %d reinit(s)", ic_init_count_);
    if (ic_mode3_ok_count_ || ic_mode3_fallback_count_) {
        Printf("; mode3 ok=%d fallback=%d", ic_mode3_ok_count_, ic_mode3_fallback_count_);
    }
    if (ic_forcing_play_inits_ || ic_forcing_dforce_inits_ || ic_forcing_fd_inits_) {
        Printf("; free y' from play=%d dforce=%d fd=%d",
               ic_forcing_play_inits_,
               ic_forcing_dforce_inits_,
               ic_forcing_fd_inits_);
    }
    Printf("\n");
    if (calcic_fallback_count_) {
        Printf("   %d IDA IC mode 1/3 residual failure(s) fell back to heuristic\n",
               calcic_fallback_count_);
    }
    if (first_try_init_failures_) {
        Printf("   %d First try Initialization failures\n", first_try_init_failures_);
    }
}

static void do_ode_thread(neuron::model_sorted_token const& sorted_token, NrnThread& ntr) {
    auto* const nt = &ntr;
    int i;
    Cvode* cv = thread_cv;
    nt->_t = cv->t_;
    cv->do_ode(sorted_token, ntr);
    CvodeThreadData& z = cv->ctd_[nt->id];
    double* yp = cv->n_vector_data(nvec_yp, nt->id);
    for (i = z.neq_v_; i < z.nvsize_; ++i) {
        yp[i] = *(z.pvdot_[i]);
    }
}

#if 0
static double check(double t, Daspk* ida) {
    res_gvardt(t, ida->cv_->y_, ida->yp_, ida->delta_, ida->cv_);
    double norm = N_VWrmsNorm(ida->delta_, ((IDAMem) (ida->mem_))->ida_ewt);
    Printf("ida check t=%.15g norm=%g\n", t, norm);
#if 0
    for (int i=0; i < ida->cv_->neq_; ++i) {
        printf(" %3d %22.15g %22.15g %22.15g\n", i,
N_VGetArrayPointer(ida->cv_->y_)[i],
N_VGetArrayPointer(ida->yp_)[i],
N_VGetArrayPointer(ida->delta_)[i]);
    }
#endif
    return norm;
}
#endif

int Daspk::check_init_residual() {
    extern double t;
    t = cv_->t_;
    if (!IDAEwtSet((IDAMem) mem_, cv_->y_)) {
        hoc_execerror("Bad Ida error weight vector", 0);
    }
    use_parasite_ = false;
    res_gvardt(cv_->t_, cv_->y_, yp_, parasite_, cv_);
    double norm = N_VWrmsNorm(parasite_, ((IDAMem) mem_)->ida_ewt);
    if (norm > 1.) {
        switch (init_failure_style_ & 03) {
        case 0:
            Printf("IDA initialization failure, weighted norm of residual=%g\n", norm);
            return IDA_ERR_FAIL;
        case 1:
            Printf("IDA initialization warning, weighted norm of residual=%g\n", norm);
            break;
        case 2:
            Printf("IDA initialization warning, weighted norm of residual=%g\n", norm);
            use_parasite_ = true;
            t_parasite_ = nt_t;
            Printf("  subtracting (for next 1e-6 ms): f(y', y, %g)*exp(-1e7*(t-%g))\n", nt_t, nt_t);
            break;
        }
        if (init_try_again_ < 0) {
            ++first_try_init_failures_;
            init_try_again_ += 1;
            int err = init();
            init_try_again_ = 0;
            return err;
        }
        // style 1 or 2: accept with warning
        return 0;
    }
    return 0;
}

// Fill y_ and yp_ using the legacy nano-step heuristic (no residual check).
static void seed_y_yp_heuristic(Daspk* d) {
    Cvode* cv = d->cv_;
    N_Vector yp = d->yp_;
    N_VConst(0., yp);

    double tt = cv->t_;
    double dtinv = 1. / Daspk::dteps_;
    if (Daspk::init_failure_style_ & 010) {
        cv->play_continuous(tt);
        nrn_daspk_init_step(tt, Daspk::dteps_, 1);
        nrn_daspk_init_step(tt, Daspk::dteps_, 1);
        cv->daspk_gather_y(yp);
        cv->play_continuous(tt);
        nrn_daspk_init_step(tt, Daspk::dteps_, 1);
        cv->daspk_gather_y(cv->y_);
        N_VLinearSum(dtinv, cv->y_, -dtinv, yp, yp);
    } else {
        cv->play_continuous(tt);
        nrn_daspk_init_step(tt, Daspk::dteps_, 1);
        nrn_daspk_init_step(tt, Daspk::dteps_, 1);

        cv->daspk_gather_y(cv->y_);
        tt = cv->t_ + Daspk::dteps_;
        cv->play_continuous(tt);
        nrn_daspk_init_step(tt, Daspk::dteps_, 0);
        cv->gather_ydot(yp);
        N_VScale(dtinv, yp, yp);
    }
    thread_cv = cv;
    nvec_yp = yp;
    nrn_multithread_job(nrn_ensure_model_data_are_sorted(), do_ode_thread);
}

int Daspk::init_heuristic() {
    extern double t;
    seed_y_yp_heuristic(this);
    ida_init();
    t = cv_->t_;
    return check_init_residual();
}

int Daspk::init_ida_y_init() {
    extern double t;
    // Seed: voltage/DAE yp = 0; mechanism ODE yp = f(y) via do_ode.
    // Suitable when residual uniquely determines y at that yp (e.g. pure
    // algebraic LinearMechanism with invertible g). For folded capacitors
    // C*(v1'-v2'), dF/dy is singular when cj=0 (IDA_Y_INIT), so CalcIC may
    // soft-fail and mode 1 falls back to the heuristic.
    N_VConst(0., yp_);
    double tt = cv_->t_;
    cv_->play_continuous(tt);
    cv_->daspk_gather_y(cv_->y_);
    thread_cv = cv_;
    nvec_yp = yp_;
    nrn_multithread_job(nrn_ensure_model_data_are_sorted(), do_ode_thread);
    ida_init();
    t = cv_->t_;

    // tout1 only sets integration direction / rough t scale for IDACalcIC.
    realtype tout1 = tt + 1.0;
    if (tout1 == tt) {
        tout1 = tt + 1e-3;
    }
    nrn_sparse13_soft_fail = 1;
    int ier = IDACalcIC(mem_, IDA_Y_INIT, tout1);
    nrn_sparse13_soft_fail = 0;
    if (ier != IDA_SUCCESS) {
        Printf("IDACalcIC(IDA_Y_INIT) failed, err=%d\n", ier);
        return ier;
    }
    // Corrected y is already in cv_->y_ (IDA y0); scatter into NEURON structures.
    cv_->daspk_scatter_y(cv_->y_);
    t = cv_->t_;
    nt_t = cv_->t_;
    return check_init_residual();
}

// Mode 3 step after y-project: set yp from C*yp = f(y) at fixed y (continuous
// limit). Residual G = C*yp - f (same assembly as Cvode::res). Unlike the
// dteps companion nano-step, there is no O(dteps) bias in y'.
//
// Handles: diagonal membrane cm, identity mechanism ODEs, simple LM mass
// (via nrndae_seed_yp_from_f), and single-layer xc when present. Algebraic
// rows (c=0) leave yp=0 — residual can only clear if f already vanishes.
static void seed_yp_from_Cy_eq_f(Daspk* d) {
    Cvode* cv = d->cv_;
    NrnThread* nt = nrn_threads;
    CvodeThreadData& z = cv->ctd_[0];
    double tt = cv->t_;
    nt->_t = tt;
    cv->play_continuous(tt);
    auto const sorted = nrn_ensure_model_data_are_sorted();
    // f(y) for G = C*yp - f  (same first half as Cvode::res)
    nrn_rhs(sorted, *nt);
    cv->do_ode(sorted, *nt);
    cv->gather_ydot(d->delta_);
    double* F = cv->n_vector_data(d->delta_, 0);
    double* yp = cv->n_vector_data(d->yp_, 0);

    N_VConst(0., d->yp_);

#if EXTRACELLULAR
    // Extracellular + membrane mass are coupled (see Cvode::res):
    //   delta[vi] = F[vi] - c*(yp_vi - yp_vx0)
    //   delta[vx0] = F[vx0] + c*(yp_vi - yp_vx0) - (xc chain)
    // Electrode current sits only in F[vi].  Solving C yp = F then requires
    //   c*(yp_vi - yp_vx0) = F[vi]
    //   and for 1-layer xc:  cx*yp_vx0 = F[vi] + F[vx0]
    // so yp_vx0 = (F[vi]+F[vx0])/cx, not F[vx0]/cx alone (the old seed left
    // residual ±I_electrode on the vext row after a source step).
    if (z.cmlext_) {
        assert(z.cmlext_->ml.size() == 1);
        Memb_list* mlx = &z.cmlext_->ml[0];
        Memb_list* mlc = (z.cmlcap_ && z.cmlcap_->ml.size() == 1) ? &z.cmlcap_->ml[0] : nullptr;
        int n = mlx->nodecount;
        for (int i = 0; i < n; ++i) {
            Node* nd = mlx->nodelist[i];
            int jx = nd->eqn_index_;      // vext[0]
            int jv = nd->eqn_index_ - 1;  // vi
            double c = 0.;
            if (mlc) {
                // same node order as cmlcap list when both present
                for (int k = 0; k < mlc->nodecount; ++k) {
                    if (mlc->nodelist[k] == nd) {
                        c = 1e-3 * mlc->data(k, 0);
                        break;
                    }
                }
            }
            if (nrn_nlayer_extracellular == 1) {
                double cx = 1e-3 * mlx->data(i, neuron::extracellular::xc_index, 0);
                if (cx > 0.) {
                    if (c > 0.) {
                        // Coupled cm+xc: electrode in F[vi] must charge both
                        // (yp_vx = (F_vi+F_vx)/cx, yp_vi = yp_vx + F_vi/c)
                        yp[jx] = (F[jv] + F[jx]) / cx;
                        yp[jv] = yp[jx] + F[jv] / c;
                    } else {
                        // No membrane mass (e.g. zero-area node): no cm coupling
                        // in res; use diagonal xc only. Algebraic vi needs F[vi]=0.
                        yp[jx] = F[jx] / cx;
                    }
                } else if (c > 0.) {
                    // xc algebraic, membrane capacitive: only relative rate from F[vi]
                    yp[jv] = F[jv] / c;
                }
            } else {
                // Multi-layer (default nlayer is often 2).  res couples membrane
                // only to vext[0], and layer caps in a chain to ground:
                //   c*(yp_vi-yp0)=F[vi]
                //   cx_k*(yp[k]-yp[k+1]) carries F[vi]+F[0]+...+F[k] toward ground
                // so the outermost rate is
                //   yp[last]=(F[vi]+sum_k F[vext[k]])/cx_last
                // then walk inward.  (Old seed used F[layer]/cx alone and left
                // electrode residual on outer rows.)
                int nlay = nrn_nlayer_extracellular;
                // cumulative F from membrane + all layers
                double f_cum = F[jv];
                for (int k = 0; k < nlay; ++k) {
                    f_cum += F[jx + k];
                }
                // outermost
                int k = nlay - 1;
                int jj = jx + k;
                double cx = 1e-3 * mlx->data(i, neuron::extracellular::xc_index, k);
                if (cx > 0.) {
                    yp[jj] = f_cum / cx;
                }
                // remove this layer's F and step inward
                for (k = nlay - 2; k >= 0; --k) {
                    f_cum -= F[jx + k + 1];
                    jj = jx + k;
                    cx = 1e-3 * mlx->data(i, neuron::extracellular::xc_index, k);
                    if (cx > 0.) {
                        yp[jj] = yp[jj + 1] + f_cum / cx;
                    }
                }
                if (c > 0.) {
                    yp[jv] = yp[jx] + F[jv] / c;
                }
            }
        }
    } else
#endif
        // Capacitive membrane without extracellular: c*vm' = f
        if (z.cmlcap_) {
        assert(z.cmlcap_->ml.size() == 1);
        Memb_list* ml = &z.cmlcap_->ml[0];
        int n = ml->nodecount;
        for (int i = 0; i < n; ++i) {
            Node* nd = ml->nodelist[i];
            int j = nd->eqn_index_ - 1;
            double c = 1e-3 * ml->data(i, 0);
            if (c == 0.) {
                continue;  // algebraic membrane node
            }
            yp[j] = F[j] / c;
        }
    }

    // Mechanism ODEs: identity mass y' = f_ode
    for (int i = z.neq_v_; i < z.nvsize_; ++i) {
        yp[i] = F[i];
    }

    // LinearMechanism mass (diagonal / lag / simple floating difference)
    nrndae_seed_yp_from_f(F, yp);

    // A2/A4: free y' in null(C) from play / dforce / FD (db/dt).
    // Uses Daspk::last_forcing_tplus_ collected at the start of Daspk::init.
    const int fflags = nrndae_complete_yp_from_forcing(yp, Daspk::last_forcing_tplus());
    Daspk::last_ic_forcing_flags_ = fflags;
    if (fflags & NRN_IC_FORCING_PLAY) {
        ++Daspk::ic_forcing_play_inits_;
    }
    if (fflags & NRN_IC_FORCING_DFORCE) {
        ++Daspk::ic_forcing_dforce_inits_;
    }
    if (fflags & NRN_IC_FORCING_FD) {
        ++Daspk::ic_forcing_fd_inits_;
    }
}

// After residual failure: classify largest residual equations (algebraic vs
// tiny-cm differential) to aid singular / inconsistent IC diagnosis.
static void diagnose_battery_ic_residual(Daspk* d) {
    Cvode* cv = d->cv_;
    if (cv->neq_ <= 0) {
        return;
    }
    // Residual left in parasite_ by check_init_residual
    double* r = cv->n_vector_data(d->parasite_, 0);
    double* yp = cv->n_vector_data(d->yp_, 0);
    CvodeThreadData& z = cv->ctd_[0];

    // Per-eq diagonal c estimate for voltage equations (0 = algebraic)
    std::vector<double> cdiag(cv->neq_, 0.);
    for (int i = z.neq_v_; i < z.nvsize_ && i < cv->neq_; ++i) {
        cdiag[i] = 1.;  // mechanism ODE identity
    }
    if (z.cmlcap_) {
        Memb_list* ml = &z.cmlcap_->ml[0];
        for (int i = 0; i < ml->nodecount; ++i) {
            Node* nd = ml->nodelist[i];
            int j = nd->eqn_index_ - 1;
            if (j >= 0 && j < cv->neq_) {
                cdiag[j] = 1e-3 * ml->data(i, 0);
            }
        }
    }

    // Top few residual equations by |r|
    constexpr int ntop = 5;
    int idx[ntop];
    double ar[ntop];
    for (int k = 0; k < ntop; ++k) {
        idx[k] = -1;
        ar[k] = -1.;
    }
    for (int i = 0; i < cv->neq_; ++i) {
        double a = std::fabs(r[i]);
        for (int k = 0; k < ntop; ++k) {
            if (a > ar[k]) {
                for (int m = ntop - 1; m > k; --m) {
                    ar[m] = ar[m - 1];
                    idx[m] = idx[m - 1];
                }
                ar[k] = a;
                idx[k] = i;
                break;
            }
        }
    }

    constexpr double tiny_c = 1e-12;  // 1e-3 * cm with cm ~ 1e-9 uF/cm2 scale
    Printf("  battery IC residual diagnosis (top residual eqs):\n");
    for (int k = 0; k < ntop && idx[k] >= 0; ++k) {
        if (ar[k] <= 0.) {
            break;
        }
        int i = idx[k];
        double c = (i < (int) cdiag.size()) ? cdiag[i] : 0.;
        const char* kind = "unknown";
        if (i >= z.neq_v_) {
            kind = "mechanism-ODE";
        } else if (c == 0.) {
            kind = "algebraic (c=0): y may need to change; y' cannot clear residual";
        } else if (c < tiny_c) {
            kind = "near-singular c: huge |y'| or treat as algebraic / ideal clamp";
        } else {
            kind = "capacitive";
        }
        Printf("    eq %d  |res|=%.6g  c~%.3g  y'=%.6g  — %s\n", i, ar[k], c, yp[i], kind);
    }
}

int Daspk::init_battery() {
    extern double t;
    // Mode 3: (1) hold continuous content (LM + extracellular), (2) y' from
    // C*y' = f(y) at fixed y. Nano-step heuristic is only a fallback (caller).
    double tt = cv_->t_;
    cv_->play_continuous(tt);
    // Sync Node <-> IDA y (vi/vext transform) before projectors read Node.v.
    // Also evaluate residual once so POINT_PROCESS BREAKPOINT (IClamp/PWL)
    // and play side effects match the post-event t+ world before project.
    cv_->daspk_gather_y(cv_->y_);
    N_VConst(0., yp_);
    res_gvardt(tt, cv_->y_, yp_, delta_, cv_);
    cv_->daspk_scatter_y(cv_->y_);
    int berr = nrndae_battery_ic_project();
    if (berr != 0) {
        Printf("nrndae_battery_ic_project failed, err=%d\n", berr);
        return berr;
    }
    // Free zero-area cable nodes (electrode at 0/1); hold CAP-list voltages.
    nrn_cable_battery_ic();
#if EXTRACELLULAR
    // Hold Vm on CAP nodes and xc content; free zero-area Vm.
    nrn_extracellular_battery_ic();
#endif
    // Projected states → IDA N_Vector (vi,vext transform in gather).
    cv_->daspk_gather_y(cv_->y_);
    cv_->daspk_scatter_y(cv_->y_);
    seed_yp_from_Cy_eq_f(this);

    ida_init();
    t = cv_->t_;
    nt_t = cv_->t_;
    cv_->daspk_gather_y(cv_->y_);
    cv_->daspk_scatter_y(cv_->y_);
    int err = check_init_residual();
    if (err != 0) {
        diagnose_battery_ic_residual(this);
    }
    return err;
}

void Daspk::audit_set_level(int level) {
    audit_level_ = level;
    if (level <= 0) {
        audit_armed_ = 0;
    }
}

int Daspk::audit_level() {
    return audit_level_;
}

void Daspk::audit_arm_at(double t) {
    audit_t_select_ = t;
    audit_armed_ = 1;
}

double Daspk::audit_t_select() {
    return audit_t_select_;
}

void Daspk::audit_set_file(const char* path) {
    if (!path || !path[0]) {
        audit_path_.clear();
    } else {
        audit_path_ = path;
    }
}

const char* Daspk::audit_file() {
    return audit_path_.c_str();
}

FILE* Daspk::audit_open_out() {
    if (audit_path_.empty()) {
        return stdout;
    }
    FILE* f = fopen(audit_path_.c_str(), "a");
    if (!f) {
        Printf("dae_init_audit: cannot open '%s' for append; using stdout\n", audit_path_.c_str());
        return stdout;
    }
    return f;
}

static void audit_copy_nv_out(Cvode* cv, N_Vector nv, std::vector<double>& out) {
    out.resize(cv->neq_);
    int k = 0;
    for (int tid = 0; tid < cv->nctd_; ++tid) {
        double* p = cv->n_vector_data(nv, tid);
        int n = cv->ctd_[tid].nvsize_;
        for (int i = 0; i < n; ++i) {
            out[k++] = p[i];
        }
    }
}

static void audit_copy_nv_in(Cvode* cv, const std::vector<double>& in, N_Vector nv) {
    int k = 0;
    for (int tid = 0; tid < cv->nctd_; ++tid) {
        double* p = cv->n_vector_data(nv, tid);
        int n = cv->ctd_[tid].nvsize_;
        for (int i = 0; i < n; ++i) {
            p[i] = in[k++];
        }
    }
}

bool Daspk::audit_should_fire() const {
    if (audit_level_ <= 0 || !audit_armed_) {
        return false;
    }
    // First reinit with t >= select (user knows the time of interest).
    return cv_->t_ >= audit_t_select_ - NetCvode::eps(audit_t_select_);
}

// Summarize residual already in delta_ (no res call).
static void audit_residual_stats(Cvode* cv,
                                 N_Vector delta,
                                 N_Vector y_for_ewt,
                                 void* mem,
                                 double* max_abs,
                                 double* wrms) {
    *max_abs = 0.;
    for (int tid = 0; tid < cv->nctd_; ++tid) {
        double* d = cv->n_vector_data(delta, tid);
        int n = cv->ctd_[tid].nvsize_;
        for (int i = 0; i < n; ++i) {
            double a = std::fabs(d[i]);
            if (a > *max_abs) {
                *max_abs = a;
            }
        }
    }
    *wrms = -1.;
    if (mem) {
        if (IDAEwtSet((IDAMem) mem, y_for_ewt)) {
            *wrms = N_VWrmsNorm(delta, ((IDAMem) mem)->ida_ewt);
        }
    }
}

void Daspk::audit_save_pre_from_delta() {
    if (audit_level_ <= 0) {
        return;
    }
    // Caller has just evaluated res into delta_ at cv_->t_ with continuous play
    // synchronized to that t (advance end, or interpolate/retreat to event).
    audit_copy_nv_out(cv_, cv_->y_, audit_y_pre_);
    audit_copy_nv_out(cv_, yp_, audit_yp_pre_);
    audit_copy_nv_out(cv_, delta_, audit_r_pre_);
    audit_pre_t_ = cv_->t_;
    audit_pre_neq_ = cv_->neq_;
    audit_residual_stats(cv_, delta_, cv_->y_, mem_, &audit_pre_max_abs_, &audit_pre_wrms_);
    audit_pre_valid_ = (audit_pre_neq_ > 0 && (int) audit_y_pre_.size() == audit_pre_neq_);
    audit_pre_res_valid_ = audit_pre_valid_ && ((int) audit_r_pre_.size() == audit_pre_neq_);
}

void Daspk::audit_eval_residual(N_Vector y, N_Vector yp, double* max_abs, double* wrms) {
    res_gvardt(cv_->t_, y, yp, delta_, cv_);
    audit_residual_stats(cv_, delta_, y, mem_, max_abs, wrms);
}

void Daspk::audit_dump_panel(FILE* f,
                             const char* title,
                             N_Vector y,
                             N_Vector yp,
                             N_Vector delta,
                             double max_abs,
                             double wrms,
                             int max_rows) {
    fprintf(f, "--- %s ---\n", title);
    if (wrms >= 0.) {
        fprintf(f, "  WRMS(residual)=%.6g  max|residual|=%.6g  neq=%d\n", wrms, max_abs, cv_->neq_);
    } else {
        fprintf(f, "  WRMS(residual)=n/a  max|residual|=%.6g  neq=%d\n", max_abs, cv_->neq_);
    }
    if (audit_level_ < 2 || cv_->neq_ <= 0) {
        return;
    }
    // Rank equations by |residual|; print top max_rows (or all if neq small).
    struct Row {
        int i;
        double y, yp, r, ar;
    };
    std::vector<Row> rows;
    rows.reserve(cv_->neq_);
    int k = 0;
    for (int tid = 0; tid < cv_->nctd_; ++tid) {
        double* py = cv_->n_vector_data(y, tid);
        double* pyp = cv_->n_vector_data(yp, tid);
        double* pr = cv_->n_vector_data(delta, tid);
        int n = cv_->ctd_[tid].nvsize_;
        for (int i = 0; i < n; ++i, ++k) {
            Row row;
            row.i = k;
            row.y = py[i];
            row.yp = pyp[i];
            row.r = pr[i];
            row.ar = std::fabs(pr[i]);
            rows.push_back(row);
        }
    }
    int nprint = cv_->neq_;
    if (max_rows > 0 && nprint > max_rows) {
        std::partial_sort(rows.begin(),
                          rows.begin() + max_rows,
                          rows.end(),
                          [](const Row& a, const Row& b) { return a.ar > b.ar; });
        nprint = max_rows;
        fprintf(f,
                "  (top %d of %d eqs by |residual|; form: residual ~ c*y' - f(y))\n",
                nprint,
                cv_->neq_);
    } else {
        fprintf(f, "  (all eqs; residual ~ c*y' - f(y))\n");
    }
    fprintf(f, "  %6s %16s %16s %16s\n", "eq", "y", "y'", "residual");
    for (int i = 0; i < nprint; ++i) {
        const Row& r = rows[i];
        fprintf(f, "  %6d %16.8g %16.8g %16.8g\n", r.i, r.y, r.yp, r.r);
    }
}

int Daspk::init() {
#if 0
printf("Daspk_init t_=%20.12g t-t_=%g t0_-t_=%g mode=%d\n",
cv_->t_, t-cv_->t_, cv_->t0_-cv_->t_, init_mode_);
#endif
    const bool do_audit = audit_should_fire();

    // A1/A4: continuous Vector.play forcing t+ plus optional LM.dforce / FD b'.
    // Play state (ubound_index_) is already post-event when reinit runs.
    last_forcing_t_ = cv_->t_;
    nrn_collect_forcing_tplus(cv_->t_, last_forcing_tplus_);
    nrndae_append_dforce_to_forcing_list(cv_->t_, last_forcing_tplus_);
    // Stdout when audit level >= 1 and this reinit is not already writing a
    // three-panel dump (that dump includes the same block).
    if (audit_level_ >= 1 && !do_audit && !last_forcing_tplus_.empty()) {
        nrn_dump_forcing_tplus(stdout, last_forcing_t_, last_forcing_tplus_);
    }

    // Capture panel B *before* the projector changes y (and possibly yp).
    std::vector<double> yB, ypB, rB;
    double maxB = 0., wrmsB = -1.;
    bool have_B = false;
    // Panel A: continuous pre-reinit snapshot (prefer post-interpolate residual).
    std::vector<double> yA, ypA, rA;
    double tA = 0., maxA_saved = 0., wrmsA_saved = -1.;
    bool have_A = false;
    bool have_A_res = false;
    if (do_audit) {
        if (audit_pre_valid_ && audit_pre_neq_ == cv_->neq_) {
            yA = audit_y_pre_;
            ypA = audit_yp_pre_;
            tA = audit_pre_t_;
            have_A = true;
            if (audit_pre_res_valid_ && (int) audit_r_pre_.size() == audit_pre_neq_) {
                rA = audit_r_pre_;
                maxA_saved = audit_pre_max_abs_;
                wrmsA_saved = audit_pre_wrms_;
                have_A_res = true;
            }
        }
        cv_->play_continuous(cv_->t_);
        cv_->daspk_gather_y(cv_->y_);
        if (have_A) {
            audit_copy_nv_in(cv_, ypA, yp_);
        } else {
            N_VConst(0., yp_);
        }
        audit_eval_residual(cv_->y_, yp_, &maxB, &wrmsB);
        audit_copy_nv_out(cv_, cv_->y_, yB);
        audit_copy_nv_out(cv_, yp_, ypB);
        audit_copy_nv_out(cv_, delta_, rB);
        have_B = true;
        // Scatter post-event y back so projector sees the same model state.
        cv_->daspk_scatter_y(cv_->y_);
    }

    int path_mode = init_mode_;
    bool fell_back = false;
    int err = 0;
    last_ic_forcing_flags_ = 0;
    ++ic_init_count_;
    if (init_mode_ == 0) {
        err = init_heuristic();
        path_mode = 0;
    } else if (init_mode_ == 3) {
        // Hold continuous content (LM / extracellular), then y' from C*y'=f(y)
        // plus free y' from forcing t+ (play / dforce / FD).
        // On residual failure, fall back to nano-step heuristic (unless audit
        // is armed — keep pure mode-3 state for panel C).
        err = init_battery();
        path_mode = 3;
        if (err != 0) {
            if (do_audit) {
                Printf(
                    "mode 3 IC residual failed (err=%d, forcing_flags=0x%x); audit armed — not "
                    "falling back to heuristic\n",
                    err,
                    last_ic_forcing_flags_);
            } else {
                Printf(
                    "mode 3 IC residual failed (err=%d, forcing_flags=0x%x); falling back to "
                    "nano-step heuristic IC\n",
                    err,
                    last_ic_forcing_flags_);
                ++calcic_fallback_count_;
                ++ic_mode3_fallback_count_;
                fell_back = true;
                err = init_heuristic();
                path_mode = 0;
            }
        } else {
            ++ic_mode3_ok_count_;
        }
    } else {
        err = init_ida_y_init();
        path_mode = init_mode_;
        if (err != 0 && init_mode_ == 1) {
            Printf(
                "IDACalcIC residual/project failed (err=%d); falling back to nano-step "
                "heuristic IC\n",
                err);
            ++calcic_fallback_count_;
            fell_back = true;
            err = init_heuristic();
            path_mode = 0;
        }
    }
    last_ic_path_mode_ = path_mode;

    if (do_audit && have_B) {
        // Preserve post-IC (y, yp) before temporarily loading A/B for printing.
        std::vector<double> yC, ypC;
        audit_copy_nv_out(cv_, cv_->y_, yC);
        audit_copy_nv_out(cv_, yp_, ypC);

        FILE* f = audit_open_out();
        const bool close_f = (f != stdout);
        ++audit_serial_;
        fprintf(f,
                "\n=== IDA IC three-panel audit  serial=%d  t=%.15g  requested_mode=%d  "
                "path_mode=%d  fallback=%d  err=%d ===\n",
                audit_serial_,
                cv_->t_,
                init_mode_,
                path_mode,
                fell_back ? 1 : 0,
                err);
        if (have_A) {
            fprintf(f,
                    "  note: reinit after discontinuity (NET_RECEIVE / Vector.play / at_time / "
                    "...)\n");
        } else {
            fprintf(f, "  note: no continuous pre-snapshot (typical of finitialize)\n");
        }
        if (init_mode_ == 3 && path_mode == 3 && do_audit) {
            fprintf(f,
                    "  note: mode 3 panel C: C*y'=f(y) seed + free y' from forcing t+ "
                    "(null(C) / Z'G y'=Z'b') flags=0x%x"
                    "%s\n",
                    last_ic_forcing_flags_,
                    err != 0 ? "; residual failed; fallback suppressed (audit armed)" : "");
            if (last_ic_forcing_flags_ & NRN_IC_FORCING_APPLIED) {
                fprintf(f,
                        "  note: free y' sources:%s%s%s\n",
                        (last_ic_forcing_flags_ & NRN_IC_FORCING_PLAY) ? " play" : "",
                        (last_ic_forcing_flags_ & NRN_IC_FORCING_DFORCE) ? " dforce" : "",
                        (last_ic_forcing_flags_ & NRN_IC_FORCING_FD) ? " fd" : "");
            } else if (err == 0) {
                fprintf(f,
                        "  note: no free-y' forcing applied (C*y'=f seed only; ok if u'=0 or no "
                        "singular C)\n");
            }
        }
        // A1: always show forcing t+ in the three-panel dump when present
        nrn_dump_forcing_tplus(f, last_forcing_t_, last_forcing_tplus_);

        double maxA = 0., wrmsA = -1.;
        if (have_A) {
            // Use residual captured with continuous play at tA (typically the
            // interpolate/retreat residual). Do not re-call res after the jump.
            audit_copy_nv_in(cv_, yA, cv_->y_);
            audit_copy_nv_in(cv_, ypA, yp_);
            if (have_A_res) {
                audit_copy_nv_in(cv_, rA, delta_);
                maxA = maxA_saved;
                wrmsA = wrmsA_saved;
            } else {
                // Fallback only if residual was not stored (should be rare).
                double t_save = cv_->t_;
                cv_->t_ = tA;
                audit_eval_residual(cv_->y_, yp_, &maxA, &wrmsA);
                cv_->t_ = t_save;
            }
            char title[192];
            snprintf(title,
                     sizeof title,
                     "A pre (continuous at t=%.15g; residual from retreat/step, not re-eval after "
                     "jump)",
                     tA);
            audit_dump_panel(f, title, cv_->y_, yp_, delta_, maxA, wrmsA, 40);
        } else {
            fprintf(f,
                    "--- A pre ---  (unavailable: no prior continuous snapshot; e.g. "
                    "finitialize)\n");
        }

        // Panel B: vectors captured before the projector (no re-eval).
        audit_copy_nv_in(cv_, yB, cv_->y_);
        audit_copy_nv_in(cv_, ypB, yp_);
        audit_copy_nv_in(cv_, rB, delta_);
        audit_dump_panel(f,
                         "B post-event pre-IC (y after discontinuity; y' from pre or 0)",
                         cv_->y_,
                         yp_,
                         delta_,
                         maxB,
                         wrmsB,
                         40);

        // Panel C: post-IC state (saved above)
        audit_copy_nv_in(cv_, yC, cv_->y_);
        audit_copy_nv_in(cv_, ypC, yp_);
        double maxC = 0., wrmsC = -1.;
        audit_eval_residual(cv_->y_, yp_, &maxC, &wrmsC);
        audit_dump_panel(f, "C post-IC", cv_->y_, yp_, delta_, maxC, wrmsC, 40);

        double max_dy = 0.;
        int i_max = -1;
        for (size_t i = 0; i < yB.size() && i < yC.size(); ++i) {
            double d = std::fabs(yC[i] - yB[i]);
            if (d > max_dy) {
                max_dy = d;
                i_max = (int) i;
            }
        }
        fprintf(f, "--- summary ---\n");
        fprintf(f, "  max|res| A/B/C = %.6g / %.6g / %.6g\n", maxA, maxB, maxC);
        if (wrmsA >= 0. || wrmsB >= 0. || wrmsC >= 0.) {
            fprintf(f, "  WRMS     A/B/C = %.6g / %.6g / %.6g\n", wrmsA, wrmsB, wrmsC);
        }
        fprintf(f, "  max|y_C - y_B| = %.6g  (eq %d)\n", max_dy, i_max);
        if (have_A && (int) yC.size() == (int) yA.size()) {
            double max_from_pre = 0.;
            int j_max = -1;
            for (size_t i = 0; i < yA.size(); ++i) {
                double d = std::fabs(yC[i] - yA[i]);
                if (d > max_from_pre) {
                    max_from_pre = d;
                    j_max = (int) i;
                }
            }
            fprintf(f, "  max|y_C - y_A| = %.6g  (eq %d)\n", max_from_pre, j_max);
        }
        fprintf(f, "=== end IDA IC audit ===\n\n");
        if (close_f) {
            fclose(f);
        }
        audit_armed_ = 0;
        // Restore post-IC into model and IDA vectors.
        audit_copy_nv_in(cv_, yC, cv_->y_);
        audit_copy_nv_in(cv_, ypC, yp_);
        cv_->daspk_scatter_y(cv_->y_);
    }

    // After a successful IC: continuous residual at this IC time for a later A
    // if the next reinit has no intervening interpolate (unusual).
    if (err == 0 && audit_level_ > 0) {
        // check_init_residual left residual in parasite_ / via res; re-eval once
        // so delta_ matches current y,yp at cv_->t_.
        double max_abs = 0., wrms = -1.;
        audit_eval_residual(cv_->y_, yp_, &max_abs, &wrms);
        audit_save_pre_from_delta();
    }
    return err;
}

int Daspk::advance_tn(double tstop) {
    // printf("Daspk::advance_tn(%g)\n", tstop);
    double tn = cv_->tn_;
    IDASetStopTime(mem_, tstop);
    int ier = IDASolve(mem_, tstop, &cv_->t_, cv_->y_, yp_, IDA_ONE_STEP_TSTOP);
    if (ier < 0) {
        // printf("DASPK advance_tn error\n");
        return ier;
    }
#if 0
	if (ier > 0 && t < cv_->t_) {
		// interpolation to tstop does not call res. So we have to.
		cv_->res(cv_->t_, N_VGetArrayPointer(cv_->y_), N_VGetArrayPointer(yp_), N_VGetArrayPointer(delta_));
		assert(MyMath::eq(t, cv_->t_, NetCvode::eps(t)));
	}
#else
    // this is very bad, performance-wise. However ida modifies its states
    // after a call to fun with the proper t.
    res_gvardt(cv_->t_, cv_->y_, yp_, delta_, cv_);
#endif
    cv_->t0_ = tn;
    cv_->tn_ = cv_->t_;
    // Continuous residual at step end. If an event retreats via interpolate(),
    // that call overwrites this snapshot with the true pre-event point.
    if (ier >= 0) {
        audit_save_pre_from_delta();
    }
    // printf("Daspk::advance_tn complete.\n");
    return ier;
}

int Daspk::interpolate(double tt) {
    // printf("Daspk::interpolate %.15g\n", tt);
    assert(tt >= cv_->t0_ && tt <= cv_->tn_);
    int ier = IDAGetSolution(mem_, tt, cv_->y_, yp_);
    if (ier < 0) {
        Printf("DASPK interpolate error\n");
        return ier;
    }
    cv_->t_ = tt;
    // interpolation does not call res. So we have to.
    // Continuous play is synchronized to tt inside res — this is the natural
    // pre-event residual for panel A (play / NetCon / at_time retreat).
    res_gvardt(cv_->t_, cv_->y_, yp_, delta_, cv_);
    if (ier >= 0) {
        audit_save_pre_from_delta();
    }
    // if(MyMath::eq(t, cv_->t_, NetCvode::eps(cv_->t_))) {
    // printf("t=%.15g t_=%.15g\n", t, cv_->t_);
    //}
    //	assert(MyMath::eq(t, cv_->t_, NetCvode::eps(cv_->t_)));
    return ier;
}

void Daspk::statistics() {
#if 0
	printf("rwork size = %d\n", iwork_[18-1]);
	printf("iwork size = %d\n", iwork_[17-1]);
	printf("Number of time steps = %d\n", iwork_[11-1]);
	printf("Number of residual evaluations = %d\n", iwork_[12-1]);
	printf("Number of Jac evaluations = %d\n", iwork_[13-1]);
	printf("Number of preconditioner solves = %d\n", iwork_[21-1]);
	printf("Number of nonlinear iterations = %d\n", iwork_[19-1]);
	printf("Number of linear iterations = %d\n", iwork_[20-1]);
	double avlin = double(iwork_[20-1])/double(iwork_[19-1]);
	printf("Average Krylov subspace dimension = %g\n", avlin);
	printf("nonlinear conv. failures = %d\n", iwork_[15-1]);
	printf("linear conv. failures = %d\n", iwork_[16-1]);
#endif
    print_ic_stats();
}

static void* daspk_scatter_thread(NrnThread* nt) {
    thread_cv->daspk_scatter_y(thread_cv->n_vector_data(nvec_y, nt->id), nt->id);
    return 0;
}
void Cvode::daspk_scatter_y(N_Vector y) {
    thread_cv = this;
    nvec_y = y;
    nrn_multithread_job(daspk_scatter_thread);
}
void Cvode::daspk_scatter_y(double* y, int tid) {
    // the dependent variables in daspk are vi,vx,etc
    // whereas in the node structure we need vm, vx
    // note that a corresponding transformation for gather_ydot is
    // not needed since the matrix solve is already with respect to vi,vx
    // in all cases. (i.e. the solution vector is in the right hand side
    // and refers to vi, vx.
    scatter_y(nrn_ensure_model_data_are_sorted(), y, tid);
    // transform the vm+vext to vm
    CvodeThreadData& z = ctd_[tid];
    if (z.cmlext_) {
        assert(z.cmlext_->ml.size() == 1);
        Memb_list* ml = &z.cmlext_->ml[0];
        int i, n = ml->nodecount;
        for (i = 0; i < n; ++i) {
            Node* nd = ml->nodelist[i];
            nd->v() -= nd->extnode->v[0];
        }
    }
}
static void* daspk_gather_thread(NrnThread* nt) {
    thread_cv->daspk_gather_y(thread_cv->n_vector_data(nvec_y, nt->id), nt->id);
    return 0;
}
void Cvode::daspk_gather_y(N_Vector y) {
    thread_cv = this;
    nvec_y = y;
    nrn_multithread_job(daspk_gather_thread);
}
void Cvode::daspk_gather_y(double* y, int tid) {
    gather_y(y, tid);
    // transform vm to vm+vext
    CvodeThreadData& z = ctd_[tid];
    if (z.cmlext_) {
        assert(z.cmlext_->ml.size() == 1);
        Memb_list* ml = &z.cmlext_->ml[0];
        int i, n = ml->nodecount;
        for (i = 0; i < n; ++i) {
            Node* nd = ml->nodelist[i];
            int j = nd->eqn_index_;
            y[j - 1] += y[j];
        }
    }
}

// for res and psol the equations for c*yp = f(y) are
// cast in the form G(t,y,yp) = f(y) - c*yp
// So res calculates delta = f(y) - c*yp
// and psol solves (c*cj - df/dy)*x = -b
// Note that since cvode uses  J = 1 - gam*df/dy and
// ida uses J = df/dy - cj*df/dyp that is the origin of the use of -b in our
// psol and also why all the non-voltage odes are scaled by dt at the
// end of it.

int Cvode::res(double tt, double* y, double* yprime, double* delta, NrnThread* nt) {
    CvodeThreadData& z = ctd_[nt->id];
    ++f_calls_;
    nt->_t = tt;

#if 0
printf("Cvode::res enter tt=%g\n", tt);
for (int i=0; i < z.nvsize_; ++i) {
	printf("   %d %g %g %g\n", i, y[i], yprime[i], delta[i]);
}
#endif
    nt->_vcv = this;             // some models may need to know this
    daspk_scatter_y(y, nt->id);  // vi, vext, channel states, linmod non-node y.
    // rhs of cy' = f(y)
    play_continuous_thread(tt, nt);
    auto const sorted_token = nrn_ensure_model_data_are_sorted();
    nrn_rhs(sorted_token, *nt);
    do_ode(sorted_token, *nt);
    // accumulate into delta
    gather_ydot(delta, nt->id);

    // now calculate -c*yp. i.e.
    // cm*vm' + c_linmod*vi' internal current balance
    // cx*vx' + c_linmod*vx' external current balance
    // c_linmod*y' non-node linmod states
    // y' mechanism states

    // this can be accumulated into delta in several stages
    // -cm*vm'and -cx*vx for current balance equation delta's
    // -c_linmod*yp (but note that the node yp yp(vm)+yp(vx))
    // subtract yp from mechanism state delta's

#if 0
    static int res_ = 0;
    ++res_;
    printf("Cvode::res after ode and gather_ydot into delta\n");
    for (int i=0; i < z.nvsize_; ++i) {
        printf("   %d %g %g %g\n", i, y[i], yprime[i], delta[i]);
    }
#endif
    // the cap nodes : see nrnoc/capac.cpp for location of cm, etc.
    // these are not in same order as for cvode but are in
    // spmatrix order mixed with nocap nodes and extracellular
    // therefore we use the Node.eqn_index to calculate the delta index.
    //	assert(use_sparse13 == true && nlayer <= 1);
    assert(use_sparse13 == true);
    if (z.cmlcap_) {
        assert(z.cmlcap_->ml.size() == 1);
        Memb_list* ml = &z.cmlcap_->ml[0];
        int n = ml->nodecount;
        auto const vec_sav_rhs = nt->node_sav_rhs_storage();
        for (int i = 0; i < n; ++i) {
            Node* nd = ml->nodelist[i];
            int j = nd->eqn_index_ - 1;
            Extnode* nde = nd->extnode;
            double cdvm;
            if (nde) {
                cdvm = 1e-3 * ml->data(i, 0) * (yprime[j] - yprime[j + 1]);
                delta[j] -= cdvm;
                delta[j + 1] += cdvm;
                // i_cap
                ml->data(i, 1) = cdvm;
#if I_MEMBRANE
                // add i_cap to i_ion which is in sav_g
                // this will be copied to i_membrane below
                *nde->param[neuron::extracellular::sav_rhs_index_ext()] += cdvm;
#endif
            } else {
                cdvm = 1e-3 * ml->data(i, 0) * yprime[j];
                delta[j] -= cdvm;
                ml->data(i, 1) = cdvm;
            }
            if (vec_sav_rhs) {
                int j = nd->v_node_index;
                vec_sav_rhs[j] += cdvm;
                vec_sav_rhs[j] *= NODEAREA(nd) * 0.01;
            }
        }
    }
    // See nrnoc/excelln.cpp for location of cx.
    if (z.cmlext_) {
        assert(z.cmlext_->ml.size() == 1);
        Memb_list* ml = &z.cmlext_->ml[0];
        int n = ml->nodecount;
        for (int i = 0; i < n; ++i) {
            Node* nd = ml->nodelist[i];
            int j = nd->eqn_index_;
#if EXTRACELLULAR
#if I_MEMBRANE
            // i_membrane = sav_rhs --- even for zero area nodes
            ml->data(i, neuron::extracellular::i_membrane_index) =
                ml->data(i, neuron::extracellular::sav_rhs_index);
#endif /*I_MEMBRANE*/
            if (nrn_nlayer_extracellular == 1) {
                // only works for one layer
                // otherwise loop over layer,
                // xc is (pd + 2*(nlayer))[layer]
                // and deal with yprime[i+layer]-yprime[i+layer+1]
                delta[j] -= 1e-3 *
                            ml->data(i, neuron::extracellular::xc_index, 0 /* 0th/only layer */) *
                            yprime[j];
            } else {
                int k = nrn_nlayer_extracellular - 1;
                int jj = j + k;
                delta[jj] -= 1e-3 * ml->data(i, neuron::extracellular::xc_index, k) * (yprime[jj]);
                for (k = nrn_nlayer_extracellular - 2; k >= 0; --k) {
                    // k=0 refers to stuff between layer 0 and 1
                    // j is for vext[0]
                    jj = j + k;
                    auto const x = 1e-3 * ml->data(i, neuron::extracellular::xc_index, k) *
                                   (yprime[jj] - yprime[jj + 1]);
                    delta[jj] -= x;
                    delta[jj + 1] += x;  // last one in iteration is nlayer-1
                }
            }
#endif /*EXTRACELLULAR*/
        }
    }

    nrndae_dkres(y, yprime, delta);

    // the ode's
    for (int i = z.neq_v_; i < z.nvsize_; ++i) {
        delta[i] -= yprime[i];
    }

    for (int i = 0; i < z.nvsize_; ++i) {
        delta[i] *= -1.;
    }
    if (daspk_->use_parasite_ && tt - daspk_->t_parasite_ < 1e-6) {
        double fac = exp(1e7 * (daspk_->t_parasite_ - tt));
        double* tps = n_vector_data(daspk_->parasite_, nt->id);
        for (int i = 0; i < z.nvsize_; ++i) {
            delta[i] -= tps[i] * fac;
        }
    }
    before_after(sorted_token, z.after_solve_, nt);
#if 0
printf("Cvode::res exit res_=%d tt=%20.12g\n", res_, tt);
for (int i=0; i < z.nvsize_; ++i) {
	printf("   %d %g %g %g\n", i, y[i], yprime[i], delta[i]);
}
#endif
    nt->_vcv = 0;
#if 0
double e = 0;
for (int i=0; i < z.nvsize_; ++i) {
	e += delta[i]*delta[i];
}
printf("Cvode::res %d e=%g t=%.15g\n", res_, e, tt);
#endif
    return 0;
}

int Cvode::psol(double tt, double* y, double* b, double cj, NrnThread* _nt) {
    CvodeThreadData& z = ctd_[_nt->id];
    ++mxb_calls_;
    int i;
    _nt->_t = tt;

#if 0
printf("Cvode::psol tt=%g solvestate=%d \n", tt, solve_state_);
for (i=0; i < z.nvsize_; ++i) {
printf(" %g", b[i]);
}
printf("\n");
#endif

    // IDACalcIC(IDA_Y_INIT) sets cj = 0 (solve all y given yp; J = dG/dy only).
    // Avoid 1/cj and skip the mechanism-ODE 1/cj scaling in that case.
    if (cj == 0.0) {
        _nt->cj = 0.0;
        _nt->_dt = 1.0;
    } else {
        _nt->cj = cj;
        _nt->_dt = 1. / cj;
    }

    _nt->_vcv = this;
    daspk_scatter_y(y, _nt->id);  // I'm not sure this is necessary.
    if (solve_state_ == INVALID) {
        nrn_lhs(nrn_ensure_model_data_are_sorted(),
                *_nt);  // designed to setup M*[dvm+dvext, dvext, dy] = ...
        solve_state_ = SETUP;
    }
    if (solve_state_ == SETUP) {
        // if using sparse 13 then should factor
        solve_state_ = FACTORED;
    }
    scatter_ydot(b, _nt->id);
#if 0
printf("before nrn_solve matrix cj=%g\n", cj);
spPrint(sp13mat_, 1,1,1);
printf("before nrn_solve actual_rhs=\n");
for (i=0; i < z.neq_v_; ++i) {
	printf("%d %g\n", i+1, actual_rhs[i+1]);
}
#endif
    daspk_nrn_solve(_nt);  // not the cvode one
    if (nrn_sparse13_factor_error()) {
        // Force next psol to rebuild via nrn_lhs (spClear resets sparse Error).
        solve_state_ = INVALID;
        return 1;  // recoverable; IDACalcIC may fail soft when J singular
    }
#if 0
//printf("after nrn_solve matrix\n");
//spPrint(sp13mat_, 1,1,1);
printf("after nrn_solve actual_rhs=\n");
for (i=0; i < neq_v_; ++i) {
	printf("%d %g\n", i+1, actual_rhs[i+1]);
}
#endif
    solve_state_ = INVALID;  // but not if using sparse13
    solvemem(nrn_ensure_model_data_are_sorted(), _nt);
    gather_ydot(b, _nt->id);
    // the ode's of the form m' = (minf - m)/mtau in model descriptions compute
    // b = b/(1 + dt*mtau) since cvode required J = 1 - gam*df/dy
    // so we need to scale those results by 1/cj.
    // When cj==0 (IDA_Y_INIT), leave solvemem results unscaled; mechanism-ODE
    // quality in that limit may still need refinement (see ida_y_init notes).
    if (cj != 0.0) {
        for (i = z.neq_v_; i < z.nvsize_; ++i) {
            b[i] *= _nt->_dt;
        }
    }
#if 0
for (i=0; i < z.nvsize_; ++i) {
printf(" %g", b[i]);
}
printf("\n");
#endif
    _nt->_vcv = 0;
    return 0;
}

N_Vector Daspk::ewtvec() {
    return ((IDAMem) mem_)->ida_ewt;
}

N_Vector Daspk::acorvec() {
    return ((IDAMem) mem_)->ida_delta;
}
