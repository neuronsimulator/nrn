#include <../../nrnconf.h>
// differential algebraic system solver interface to DDASPK

// DDASPK is translated from fortran with f2c. Hence all args are
// call by reference

#include <stdio.h>
#include <math.h>
#include "spmatrix.h"
#include "nrnoc2iv.h"
#include "cvodeobj.h"
#include "nrndaspk.h"
#include "netcvode.h"
#include "nrn_ansi.h"
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
extern void nrn_solve(NrnThread*);
void nrn_daspk_init_step(double, double, int);
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
    nrn_multithread_job(msolve_thread);
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

int Daspk::init() {
    extern double t;
    int i;
#if 0
printf("Daspk_init t_=%20.12g t-t_=%g t0_-t_=%g\n",
cv_->t_, t-cv_->t_, cv_->t0_-cv_->t_);
#endif
    N_VConst(0., yp_);

    // the new initial condition is based on a dteps_ step backward euler
    // linear solution with respect to the old state in order to
    // start the following initial condition calculation with a "valid"
    // (in a linear system sense) initial state.

    double tt = cv_->t_;
    double dtinv = 1. / dteps_;
    if (init_failure_style_ & 010) {
        cv_->play_continuous(tt);
        nrn_daspk_init_step(tt, dteps_, 1);
        nrn_daspk_init_step(tt, dteps_, 1);
        cv_->daspk_gather_y(yp_);
        cv_->play_continuous(tt);
        nrn_daspk_init_step(tt, dteps_, 1);
        cv_->daspk_gather_y(cv_->y_);
        N_VLinearSum(dtinv, cv_->y_, -dtinv, yp_, yp_);
    } else {
#if 0
	cv_->play_continuous(tt);
	nrn_daspk_init_step(tt, dteps_, 1);
	cv_->daspk_gather_y(cv_->y_);
	tt = cv_->t_ + dteps_;
	cv_->play_continuous(tt);
	nrn_daspk_init_step(tt, dteps_, 1);
	cv_->daspk_gather_y(yp_);
	N_VLinearSum(dtinv, yp_, -dtinv, cv_->y_, yp_);
	cv_->daspk_scatter_y(cv_->y_);
#else
        cv_->play_continuous(tt);
        nrn_daspk_init_step(tt, dteps_, 1);  // first approx to y (and maybe good enough)
        nrn_daspk_init_step(tt, dteps_, 1);  // 2nd approx to y (trouble with 2sramp.hoc)

        cv_->daspk_gather_y(cv_->y_);
        tt = cv_->t_ + dteps_;
        cv_->play_continuous(tt);
        nrn_daspk_init_step(tt, dteps_, 0);  // rhs contains delta y (for v, vext, linmod
        cv_->gather_ydot(yp_);
        N_VScale(dtinv, yp_, yp_);
#endif
    }
    thread_cv = cv_;
    nvec_yp = yp_;
    nrn_multithread_job(nrn_ensure_model_data_are_sorted(), do_ode_thread);
    ida_init();
    t = cv_->t_;
#if 1
    // test
    // printf("test\n");
    if (!IDAEwtSet((IDAMem) mem_, cv_->y_)) {
        hoc_execerror("Bad Ida error weight vector", 0);
    }
    use_parasite_ = false;
    //	check(cv_->t_, this);
    res_gvardt(cv_->t_, cv_->y_, yp_, parasite_, cv_);
    double norm = N_VWrmsNorm(parasite_, ((IDAMem) mem_)->ida_ewt);
    // printf("norm=%g at t=%g\n", norm, t);
    if (norm > 1.) {
        switch (init_failure_style_ & 03) {
        case 0:
            Printf("IDA initialization failure, weighted norm of residual=%g\n", norm);
            return IDA_ERR_FAIL;
            break;
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
#if 0
for (i=0; i < cv_->neq_; ++i) {
	printf("   %d %g %g %g %g\n", i, nt_t, N_VGetArrayPointer(cv_->y_)[i], N_VGetArrayPointer(yp_)[i], N_VGetArrayPointer(delta_)[i]);
}
#endif
        if (init_try_again_ < 0) {
            ++first_try_init_failures_;
            init_try_again_ += 1;
            int err = init();
            init_try_again_ = 0;
            return err;
        }
        return 0;
    }
#endif

    return 0;
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
    res_gvardt(cv_->t_, cv_->y_, yp_, delta_, cv_);
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
    if (first_try_init_failures_) {
        Printf("   %d First try Initialization failures\n", first_try_init_failures_);
    }
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
    static int res_;
    int i;
    nt->_t = tt;
    res_++;

#if 0
printf("Cvode::res enter tt=%g\n", tt);
for (i=0; i < z.nvsize_; ++i) {
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
printf("Cvode::res after ode and gather_ydot into delta\n");
for (i=0; i < z.nvsize_; ++i) {
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
        for (i = 0; i < n; ++i) {
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
                int i = nd->v_node_index;
                vec_sav_rhs[i] += cdvm;
                vec_sav_rhs[i] *= NODEAREA(nd) * 0.01;
            }
        }
    }
    // See nrnoc/excelln.cpp for location of cx.
    if (z.cmlext_) {
        assert(z.cmlext_->ml.size() == 1);
        Memb_list* ml = &z.cmlext_->ml[0];
        int n = ml->nodecount;
        for (i = 0; i < n; ++i) {
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
    for (i = z.neq_v_; i < z.nvsize_; ++i) {
        delta[i] -= yprime[i];
    }

    for (i = 0; i < z.nvsize_; ++i) {
        delta[i] *= -1.;
    }
    if (daspk_->use_parasite_ && tt - daspk_->t_parasite_ < 1e-6) {
        double fac = exp(1e7 * (daspk_->t_parasite_ - tt));
        double* tps = n_vector_data(daspk_->parasite_, nt->id);
        for (i = 0; i < z.nvsize_; ++i) {
            delta[i] -= tps[i] * fac;
        }
    }
    before_after(sorted_token, z.after_solve_, nt);
#if 0
printf("Cvode::res exit res_=%d tt=%20.12g\n", res_, tt);
for (i=0; i < z.nvsize_; ++i) {
	printf("   %d %g %g %g\n", i, y[i], yprime[i], delta[i]);
}
#endif
    nt->_vcv = 0;
#if 0
double e = 0;
for (i=0; i < z.nvsize_; ++i) {
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

    _nt->cj = cj;
    _nt->_dt = 1. / cj;

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
    for (i = z.neq_v_; i < z.nvsize_; ++i) {
        b[i] *= _nt->_dt;
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
