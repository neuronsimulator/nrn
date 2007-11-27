#include <../../nrnconf.h>
// differential algebraic system solver interface to DDASPK

// DDASPK is translated from fortran with f2c. Hence all args are
// call by reference

#include <stdio.h>
#include <math.h>
#include <InterViews/resource.h>
extern "C" {
#include "spmatrix.h"
}
#include "nrnoc2iv.h"
#include "cvodeobj.h"
#include "nrndaspk.h"
#include "netcvode.h"
#include "ida/ida.h"
#include "ida/ida_impl.h"
#include "mymath.h"

// the state of the g - d2/dx2 matrix for voltages
#define INVALID 0
#define NO_CAP 1
#define SETUP 2
#define FACTORED 3
static int solve_state_;

// prototypes

double Daspk::dteps_;

extern "C" {
	
extern void linmod_dkres(double*, double*, double*);
extern void linmod_dkpsol(double);
extern void nrn_rhs();
extern void nrn_lhs();
extern void nrn_solve();
extern void nrn_set_cj(double);
void nrn_daspk_init_step(double, int);
// this is private in ida.c but we want to check if our initialization
// is good. Unfortunately ewt is set on the first call to solve which
// is too late for us.
extern booleantype IDAEwtSet(IDAMem IDA_mem, N_Vector ycur);

extern double t, dt;
extern int nrn_cvode_;

static void daspk_nrn_solve() {
	nrn_solve();
}

static int res(
	realtype t, N_Vector y, N_Vector yp, N_Vector delta,
	void* rdata
);

static int minit(IDAMem);

static int msetup(IDAMem mem, N_Vector y, N_Vector ydot, N_Vector delta,
	N_Vector tempv1, N_Vector tempv2, N_Vector tempv3
);

static int msolve(IDAMem mem, N_Vector b, N_Vector ycur, N_Vector ypcur,
	N_Vector deltacur
);

static int mfree(IDAMem);

}

// residual
static int res(
	realtype t, N_Vector y, N_Vector yp, N_Vector delta,
	void* rdata
){
	Cvode* cv = (Cvode*)rdata;
	int ier = cv->res(t, N_VGetArrayPointer(y), N_VGetArrayPointer(yp), N_VGetArrayPointer(delta));
	return ier;
}

// linear solver specific allocation and initialization
static int minit(IDAMem) {
	return IDA_SUCCESS;
}

// linear solver preparation for subsequent calls to msolve
// approximation to jacobian. Everything necessary for solving P*x = b
static int msetup(IDAMem mem, N_Vector y, N_Vector yp, N_Vector,
	N_Vector, N_Vector, N_Vector
){
	Cvode* cv = (Cvode*)mem->ida_rdata;
	dt = mem->ida_hh;
	int ier = cv->jac(mem->ida_tn, N_VGetArrayPointer(y), N_VGetArrayPointer(yp),
		mem->ida_cj);
	return ier;
}

/* solve P*x = b */
static int msolve(IDAMem mem, N_Vector b, N_Vector w, N_Vector ycur, N_Vector, N_Vector){
	Cvode* cv = (Cvode*)mem->ida_rdata;
	int ier = cv->psol(mem->ida_tn, N_VGetArrayPointer(ycur), N_VGetArrayPointer(b),
		mem->ida_cj);
	return ier;
}

static int mfree(IDAMem) {return IDA_SUCCESS;}

Daspk::Daspk(Cvode* cv, int neq) {
//	printf("Daspk::Daspk\n");
	cv_ = cv;
	yp_ = cv->nvnew(neq);
	delta_ = cv->nvnew(neq);
	parasite_ = cv->nvnew(neq);
	use_parasite_ = false;
	spmat_ = nil;
	mem_ = nil;
}

Daspk::~Daspk() {
//	printf("Daspk::~Daspk\n");
	N_VDestroy(delta_);
	N_VDestroy(yp_);
	if (mem_) {
		IDAFree((IDAMem)mem_);
	}
}

void Daspk::ida_init() {
	int ier;
	if (mem_) {
		ier = IDAReInit(mem_, res, cv_->t_, cv_->y_, yp_,
			IDA_SV, &cv_->ncv_->rtol_, cv_->atolnvec_
		);
		if (ier < 0) {
			hoc_execerror("IDAReInit error", 0);
		}
	}else{
		IDAMem mem = (IDAMem) IDACreate();
		if (!mem) {
			hoc_execerror("IDAMalloc error", 0);
		}
		IDASetRdata(mem, cv_);
		ier = IDAMalloc(mem, res, cv_->t_, cv_->y_, yp_,
			IDA_SV, &cv_->ncv_->rtol_, cv_->atolnvec_
		);
		mem->ida_linit = minit;
		mem->ida_lsetup = msetup;
		mem->ida_lsolve = msolve;
		mem->ida_lfree = mfree;
		mem->ida_setupNonNull = false;
		mem_ = mem;
	}
}

void Daspk::info() {
}


// last two bits, 0 error, 1 warning, 2 apply parasitic
// if init_failure_style & 010, then use the original method
int Daspk::init_failure_style_;
int Daspk::init_try_again_;
int Daspk::first_try_init_failures_;

int Daspk::init() {
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

	t = cv_->t_;
	double dtinv = 1./dteps_;
	double* yp;
    if (init_failure_style_ & 010) {
	cv_->play_continuous(t);
	nrn_daspk_init_step(dteps_, 1);
	nrn_daspk_init_step(dteps_, 1);
	yp = N_VGetArrayPointer(yp_);
	cv_->daspk_gather_y(yp);
	t = cv_->t_ + dteps_;
	cv_->play_continuous(t);
	nrn_daspk_init_step(dteps_, 1);
	cv_->daspk_gather_y(N_VGetArrayPointer(cv_->y_));
	N_VLinearSum(dtinv, cv_->y_, -dtinv, yp_, yp_);
    }else{
#if 0
	cv_->play_continuous(t);
	nrn_daspk_init_step(dteps_, 1);
	cv_->daspk_gather_y(N_VGetArrayPointer(cv_->y_));
	t = cv_->t_ + dteps_;
	cv_->play_continuous(t);
	nrn_daspk_init_step(dteps_, 1);
	yp = N_VGetArrayPointer(yp_);
	cv_->daspk_gather_y(yp);
	N_VLinearSum(dtinv, yp_, -dtinv, cv_->y_, yp_);
	cv_->daspk_scatter_y(N_VGetArrayPointer(cv_->y_));
#else
	cv_->play_continuous(t);
	nrn_daspk_init_step(dteps_, 1); // first approx to y (and maybe good enough)
	nrn_daspk_init_step(dteps_, 1); // 2nd approx to y (trouble with 2sramp.hoc)
	cv_->daspk_gather_y(N_VGetArrayPointer(cv_->y_));
	t = cv_->t_ + dteps_;
	cv_->play_continuous(t);
	nrn_daspk_init_step(dteps_, 0); // rhs contains delta y (for v, vext, linmod
	cv_->gather_ydot(N_VGetArrayPointer(yp_));
	N_VScale(dtinv, yp_, yp_);
	yp = N_VGetArrayPointer(yp_);
#endif
    }
	t=cv_->t_;
	cv_->do_ode();
	for (i=cv_->neq_v_; i < cv_->neq_; ++i) {
		yp[i] = *(cv_->pvdot_[i]);
	}
	ida_init();
#if 1
	// test
//printf("test\n");
	if (!IDAEwtSet((IDAMem)mem_, cv_->y_)) {
		hoc_execerror("Bad Ida error weight vector", 0);
	}
	use_parasite_ = false;
	cv_->res(t, N_VGetArrayPointer(cv_->y_), N_VGetArrayPointer(yp_), N_VGetArrayPointer(parasite_));
	double norm = N_VWrmsNorm(parasite_, ((IDAMem)mem_)->ida_ewt);
//printf("norm=%g at t=%g\n", norm, t);
	if (norm > 1.) {
	    switch (init_failure_style_ & 03) {
	    case 0:
		printf("IDA initialization failure, weighted norm of residual=%g\n", norm);
		return IDA_ERR_FAIL;
		break;
	    case 1:
		printf("IDA initialization warning, weighted norm of residual=%g\n", norm);
		break;
	    case 2:
		printf("IDA initialization warning, weighted norm of residual=%g\n", norm);
		use_parasite_ = true;
		t_parasite_ = t;
		printf("  subtracting (for next 1e-6 ms): f(y', y, %g)*exp(-1e7*(t-%g))\n", t, t);
		break;
	    }
#if 0
for (i=0; i < cv_->neq_; ++i) {
	printf("   %d %g %g %g %g\n", i, t, N_VGetArrayPointer(cv_->y_)[i], N_VGetArrayPointer(yp_)[i], N_VGetArrayPointer(delta_)[i]);
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
//printf("Daspk:solve %s tin=%g tstop=%g\n", (info_[0] == 0)?"initial":"continue", *pt_, rwork_[0]);
	double tn = cv_->tn_;
	IDASetStopTime(mem_, tstop);
	int ier = IDASolve(mem_, tstop, &cv_->t_, cv_->y_, yp_, IDA_ONE_STEP_TSTOP);
	if (ier < 0) {
		//printf("DASPK advance_tn error\n");
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
	cv_->res(cv_->t_, N_VGetArrayPointer(cv_->y_), N_VGetArrayPointer(yp_), N_VGetArrayPointer(delta_));
#endif
	cv_->t0_ = tn;
	cv_->tn_ = cv_->t_;
//printf("Daspk:solve %s tout=%g tstop-tout=%g idid=%d\n", (info_[0] == 0)?"initial":"continue", *pt_, rwork_[0] - *pt_, idid_);
	return ier;
}

int Daspk::interpolate(double tt) {
//printf("Daspk::interpolate %.15g\n", tt);
	assert (tt >= cv_->t0_ && tt <= cv_->tn_);
	IDASetStopTime(mem_, tt);
	int ier = IDASolve(mem_, tt, &cv_->t_, cv_->y_, yp_, IDA_NORMAL);
	if (ier < 0) {
		printf("DASPK interpolate error\n");
		return ier;
	}
	assert(MyMath::eq(tt, cv_->t_, NetCvode::eps(cv_->t_)));
	// interpolation does not call res. So we have to.
	cv_->res(cv_->t_, N_VGetArrayPointer(cv_->y_), N_VGetArrayPointer(yp_), N_VGetArrayPointer(delta_));
//if(MyMath::eq(t, cv_->t_, NetCvode::eps(cv_->t_))) {
//printf("t=%.15g t_=%.15g\n", t, cv_->t_);
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
		printf("   %d First try Initialization failures\n", first_try_init_failures_);
	}
}

void Cvode::daspk_scatter_y(double* y) {
	// the dependent variables in daspk are vi,vx,etc
	// whereas in the node structure we need vm, vx
	// note that a corresponding transformation for gather_ydot is
	// not needed since the matrix solve is already with respect to vi,vx
	// in all cases. (i.e. the solution vector is in the right hand side
	// and refers to vi, vx.
	scatter_y(y);
	// transform the vm+vext to vm
	if (cmlext_) {
		Memb_list* ml = cmlext_->ml;
		int i, n = ml->nodecount;
		for (i=0; i < n; ++i) {
			Node* nd = ml->nodelist[i];
			NODEV(nd) -= nd->extnode->v[0];
		}
	}
}
void Cvode::daspk_gather_y(double* y) {
	gather_y(y);
	// transform vm to vm+vext
	if (cmlext_) {
		Memb_list* ml = cmlext_->ml;
		int i, n = ml->nodecount;
		for (i=0; i < n; ++i) {
			Node* nd = ml->nodelist[i];
			int j = nd->eqn_index_;
			y[j-1] += y[j];
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

int Cvode::res(double tt, double* y, double* yprime, double* delta) {
	++f_calls_;
	static int res_;
	int i;
	t = tt;
	res_++;

#if 0
printf("Cvode::res enter tt=%g\n", tt);
for (i=0; i < neq_; ++i) {
	printf("   %d %g %g %g\n", i, y[i], yprime[i], delta[i]);
}
#endif
	nrn_cvode_ = 1; // some models may need to know this
	daspk_scatter_y(y); // vi, vext, channel states, linmod non-node y.
	// rhs of cy' = f(y)
	play_continuous(tt);
	nrn_rhs();
	do_ode();
	// accumulate into delta
	gather_ydot(delta);

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
for (i=0; i < neq_; ++i) {
	printf("   %d %g %g %g\n", i, y[i], yprime[i], delta[i]);
}
#endif
	// the cap nodes : see nrnoc/capac.c for location of cm, etc.
	// these are not in same order as for cvode but are in
	// spmatrix order mixed with nocap nodes and extracellular
	// therefore we use the Node.eqn_index to calculate the delta index.
//	assert(use_sparse13 == true && nlayer <= 1);
	assert(use_sparse13 == true);
	if (cmlcap_) {
		Memb_list* ml = cmlcap_->ml;
		int n = ml->nodecount;
		for (i=0; i < n; ++i) {
			double* cd = ml->data[i];
			Node* nd = ml->nodelist[i];
			int j = nd->eqn_index_ - 1;
			Extnode* nde = nd->extnode;
			if (nde) {
				double cdvm = 1e-3 * cd[0] * (yprime[j] - yprime[j+1]);
				delta[j] -= cdvm;
				delta[j+1] += cdvm;
				// i_cap
				cd[1] = cdvm;
#if I_MEMBRANE
				// add i_cap to i_ion which is in sav_g
				// this will be copied to i_membrane below
				nde->param[3+3*nlayer] += cdvm;
#endif
			}else{
				double cdvm = 1e-3 * cd[0] * yprime[j];
				delta[j] -= cdvm;
				cd[1] = cdvm;
			}
		}
	}
	// See nrnoc/excelln.c for location of cx.
	if (cmlext_) {
		Memb_list* ml = cmlext_->ml;
		int n = ml->nodecount;
		for (i=0; i < n; ++i) {
			double* cd = ml->data[i];
			Node* nd = ml->nodelist[i];
			int j = nd->eqn_index_;
#if I_MEMBRANE
			// i_membrane = sav_rhs --- even for zero area nodes
			cd[1+3*nlayer] = cd[3+3*nlayer];
#endif
#if EXTRACELLULAR == 1
			// only works for one layer
			// otherwise loop over layer,
			// xc is (pd + 2*(nlayer))[layer]
			// and deal with yprime[i+layer]-yprime[i+layer+1]
			delta[j] -= 1e-3 * cd[2] * yprime[j];
#else
			int k, jj;
			double x;
			k = nlayer-1;
			jj = j+k;
			delta[jj] -= 1e-3*cd[2*nlayer+k]*(yprime[jj]);
			for (k=nlayer-2; k >= 0; --k) {
				// k=0 refers to stuff between layer 0 and 1
				// j is for vext[0]				
				jj = j+k;
				x = 1e-3*cd[2*nlayer+k]*(yprime[jj] - yprime[jj+1]);
				delta[jj] -= x;
				delta[jj+1] += x; // last one in iteration is nlayer-1
			}
			
#endif
		}
	}

	linmod_dkres(y, yprime, delta);

	// the ode's
	for (i=neq_v_; i < neq_; ++i) {
		delta[i] -= yprime[i];
	}

	for (i=0; i < neq_; ++i) {
		delta[i] *= -1.;
	}
	if (daspk_->use_parasite_ && tt - daspk_->t_parasite_ < 1e-6) {
		double fac = exp(1e7*(daspk_->t_parasite_ - tt));
		double* tps = N_VGetArrayPointer(daspk_->parasite_);
		for (i=0; i < neq_; ++i) {
			delta[i] -= tps[i]*fac;
		}
	}
	before_after(after_solve_);
#if 0
printf("Cvode::res exit res_=%d tt=%20.12g\n", res_, tt);
for (i=0; i < neq_; ++i) {
	printf("   %d %g %g %g\n", i, y[i], yprime[i], delta[i]);
}
#endif
	nrn_cvode_ = 0;
#if 0
double e = 0;
for (i=0; i < neq_; ++i) {
	e += delta[i]*delta[i];
}
printf("Cvode::res %d e=%g t=%.15g\n", res_, e, tt);
#endif
	return 0;
}

int Cvode::jac(double tt, double* y, double* yprime, double cj) {
//printf("Cvode::jac is not used\n");
	++jac_calls_;
	return 0;
}

int Cvode::psol(double tt, double* y, double* b, double cj) {
	++mxb_calls_;
	int i;
	t = tt;

#if 0
printf("Cvode::psol tt=%g solvestate=%d \n", tt, solve_state_);
for (i=0; i < neq_; ++i) {
printf(" %g", b[i]);
}
printf("\n");
#endif

	nrn_set_cj(cj);
	dt = 1./cj;

	nrn_cvode_ = 1;
	daspk_scatter_y(y); // I'm not sure this is necessary.
	if (solve_state_ == INVALID) {
		nrn_lhs(); // designed to setup M*[dvm+dvext, dvext, dy] = ...
		solve_state_ = SETUP;
	}
	if (solve_state_ == SETUP) {
		// if using sparse 13 then should factor
		solve_state_ = FACTORED;
	}
	scatter_ydot(b);
#if 0
printf("before nrn_solve matrix cj=%g\n", cj);
spPrint((char*)sp13mat_, 1,1,1);
printf("before nrn_solve actual_rhs=\n");
for (i=0; i < neq_v_; ++i) {
	printf("%d %g\n", i+1, actual_rhs[i+1]);
}
#endif
	daspk_nrn_solve(); // not the cvode one
#if 0
//printf("after nrn_solve matrix\n");
//spPrint((char*)sp13mat_, 1,1,1);
printf("after nrn_solve actual_rhs=\n");
for (i=0; i < neq_v_; ++i) {
	printf("%d %g\n", i+1, actual_rhs[i+1]);
}
#endif
	solve_state_ = INVALID; // but not if using sparse13
	solvemem();
	gather_ydot(b);
	// the ode's of the form m' = (minf - m)/mtau in model descriptions compute
	// b = b/(1 + dt*mtau) since cvode required J = 1 - gam*df/dy
	// so we need to scale those results by 1/cj.
	for (i=neq_v_; i < neq_; ++i) {
		b[i] *= dt;
	}
#if 0
for (i=0; i < neq_; ++i) {
printf(" %g", b[i]);
}
printf("\n");
#endif
	nrn_cvode_ = 0;
	return 0;
}

double* Daspk::ewtvec() {
	return N_VGetArrayPointer(((IDAMem)mem_)->ida_ewt);
}

double* Daspk::acorvec() {
	return N_VGetArrayPointer(((IDAMem)mem_)->ida_delta);
}


