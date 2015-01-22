#include <../../nrnconf.h>
#include <stdio.h>
#include <math.h>
#include <InterViews/resource.h>
#include "nonlinz.h"
#include "nrnoc2iv.h"
extern "C" {
#include "cspmatrix.h"
#include "membfunc.h"
}

typedef int (*Pfridot)(...);

extern "C" {
extern int structure_change_cnt;
extern void v_setup_vectors();
extern void nrn_rhs(NrnThread*);
extern int nrndae_extra_eqn_count();
extern Symlist *hoc_built_in_symlist;
}


class NonLinImpRep {
public:
	NonLinImpRep();
	virtual ~NonLinImpRep();
	void delta(double);
	void didv();
	void dids();
	void dsdv();
	void dsds();

	char* m_;
	int scnt_; // structure_change
	int n_v_, n_ext_, n_lin_, n_ode_, neq_v_, neq_;
	double** pv_;
	double** pvdot_;
	int* v_index_;
	double* rv_;
	double* jv_;
	double** diag_;
	double* deltavec_; // just like cvode.atol*cvode.atolscale for ode's
	double delta_; // slightly more efficient and easier for v.
	void current(int, Memb_list*, int);
	void ode(int, Memb_list*);

	double omega_;
	int iloc_; // current injection site of last ztransfer calculation
	float* vsymtol_;
};

NonLinImp::NonLinImp() {
	rep_ = NULL;
}
NonLinImp::~NonLinImp() {
	if (rep_) {
		delete rep_;
	}
}
double NonLinImp::transfer_amp(int curloc, int vloc) {
	solve(curloc);
	double x = rep_->rv_[vloc];
	double y = rep_->jv_[vloc];
	return sqrt(x*x+y*y);
}
double NonLinImp::input_amp(int curloc) {
	solve(curloc);
	double x = rep_->rv_[curloc];
	double y = rep_->jv_[curloc];
	return sqrt(x*x+y*y);
}
double NonLinImp::transfer_phase(int curloc, int vloc) {
	solve(curloc);
	double x = rep_->rv_[vloc];
	double y = rep_->jv_[vloc];
	return atan2(y, x);
}
double NonLinImp::input_phase(int curloc) {
	solve(curloc);
	double x = rep_->rv_[curloc];
	double y = rep_->jv_[curloc];
	return atan2(y,x);
}
double NonLinImp::ratio_amp(int clmploc, int vloc) {
	solve(clmploc);
	double ax,bx,cx, ay,by,cy,bb;
	ax = rep_->rv_[vloc];
	ay = rep_->jv_[vloc];
	bx = rep_->rv_[clmploc];
	by = rep_->jv_[clmploc];
	bb = bx*bx + by*by;
	cx = (ax*bx + ay*by)/bb;
	cy = (ay*bx - ax*by)/bb;
	return sqrt(cx*cx + cy*cy);
}
void NonLinImp::compute(double omega, double deltafac) {
	v_setup_vectors();
	nrn_rhs(nrn_threads);
	if (rep_ && rep_->scnt_ != structure_change_cnt) {
		delete rep_;
		rep_ = NULL;
	}
	if (!rep_) {
		rep_ = new NonLinImpRep();
	}
	if (nrndae_extra_eqn_count() > 0) {
		hoc_execerror("Impedance calculation with LinearMechanism not implemented", 0);
	}
	if (nrn_threads->_ecell_memb_list) {
		hoc_execerror("Impedance calculation with extracellular not implemented", 0);
	}
	rep_->omega_ = 1000.* omega;
	rep_->delta(deltafac);
	// fill matrix
	cmplx_spClear(rep_->m_);
	rep_->didv();
	rep_->dsds();
#if 1	// when 0 equivalent to standard method
 	rep_->dids();
	rep_->dsdv();
#endif
	
//	cmplx_spPrint(rep_->m_, 0, 1, 1);
//	for (int i=0; i < rep_->neq_; ++i) {
//		printf("i=%d %g %g\n", i, rep_->diag_[i][0], rep_->diag_[i][1]);
//	}
	int e = cmplx_spFactor(rep_->m_);
	switch (e) {
	case spZERO_DIAG:
		hoc_execerror("cmplx_spFactor error:", "Zero Diagonal");
	case spNO_MEMORY:
		hoc_execerror("cmplx_spFactor error:", "No Memory");
	case spSINGULAR:
		hoc_execerror("cmplx_spFactor error:", "Singular");
	}  

	rep_->iloc_ = -1;
}

void NonLinImp::solve(int curloc) {
	NrnThread* _nt = nrn_threads;
	if (!rep_) {
		hoc_execerror("Must call Impedance.compute first", 0);
	}
	if (rep_->iloc_ != curloc) {
		int i;
		for (i=0; i < rep_->neq_; ++i) {
			rep_->rv_[i] = 0;
			rep_->jv_[i] = 0;
		}
		rep_->rv_[curloc] = 1.e2/NODEAREA(_nt->_v_node[curloc]);
		cmplx_spSolve(rep_->m_, rep_->rv_-1, rep_->rv_-1, rep_->jv_-1, rep_->jv_-1);
		rep_->iloc_ = curloc;
	}
}

// too bad it is not easy to reuse the cvode/daspk structures. Most of
// mapping is already done there.

NonLinImpRep::NonLinImpRep() {
	int err;
	int i, j, ieq, cnt;
	NrnThread* _nt = nrn_threads;

	vsymtol_ = NULL;
	Symbol* vsym = hoc_table_lookup("v", hoc_built_in_symlist);
	if (vsym->extra) {
		vsymtol_ = &vsym->extra->tolerance;
	}
	// the equation order is the same order as for the fixed step method
	// for current balance. Remaining ode equation order is
	// defined by the memb_list.

	
	// how many equations
	n_v_ = _nt->end;
	n_ext_ = 0;
	if (_nt->_ecell_memb_list) {
		n_ext_ = _nt->_ecell_memb_list->nodecount*nlayer;
	}
	n_lin_ = nrndae_extra_eqn_count();
	n_ode_ = 0;
	for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
		Memb_list* ml = tml->ml;
		i = tml->index;
		Pfridot s = (Pfridot)memb_func[i].ode_count;
		if (s) {
			cnt = (*s)(i);
			n_ode_ += cnt * ml->nodecount;
		}
	}
	neq_v_ = n_v_ + n_ext_ + n_lin_;
	neq_ = neq_v_ + n_ode_;
	m_ = cmplx_spCreate(neq_, 1, &err);
	assert(err == spOKAY);
	pv_ = new double*[neq_];
	pvdot_ = new double*[neq_];
	v_index_ = new int[n_v_];
	rv_ = new double[neq_+1];
	rv_ += 1;
	jv_ = new double[neq_+1];
	jv_ += 1;
	diag_ = new double*[neq_];
	deltavec_ = new double[neq_];

	for (i=0; i < n_v_; ++i) {
		// utilize nd->eqn_index in case of use_sparse13 later
		Node* nd = _nt->_v_node[i];
		pv_[i] = &NODEV(nd);
		pvdot_[i] = nd->_rhs;
		v_index_[i] = i+1;
	}
	for (i=0; i < n_v_; ++i) {
		diag_[i] = cmplx_spGetElement(m_, v_index_[i], v_index_[i]);
	}
	for (i=neq_v_; i < neq_; ++i) {
		diag_[i] = cmplx_spGetElement(m_, i+1, i+1);
	}
	scnt_ = structure_change_cnt;
}

NonLinImpRep::~NonLinImpRep() {
	cmplx_spDestroy(m_);
	delete [] pv_;
	delete [] pvdot_;
	delete [] v_index_;
	delete [] (rv_ - 1);
	delete [] (jv_ - 1);
	delete [] diag_;
	delete [] deltavec_;
}

void NonLinImpRep::delta(double deltafac){ // also defines pv_,pvdot_ map for ode
	int i, j, nc, cnt, ieq;
	NrnThread* nt = nrn_threads;
	for (i=0; i < neq_; ++i) {
		deltavec_[i] = deltafac; // all v's wasted but no matter.
	}
	ieq = neq_v_;
	for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
		Memb_list* ml = tml->ml;
		i = tml->index;
		nc = ml->nodecount;
		Pfridot s = (Pfridot)memb_func[i].ode_count;
		if (s && (cnt = (*s)(i)) > 0) {
			s = (Pfridot)memb_func[i].ode_map;
			for (j=0; j < nc; ++j) {
(*s)(ieq, pv_ + ieq, pvdot_ + ieq, ml->data[j], ml->pdata[j], deltavec_ + ieq, i);
				ieq += cnt;
			}
		}
	}
	delta_ = (vsymtol_ && (*vsymtol_ != 0.)) ? *vsymtol_ : 1.;
	delta_ *= deltafac;
	
}

void NonLinImpRep::didv() {
	int i, j, ip;
	Node* nd;
	NrnThread* _nt = nrn_threads;
	// d2v/dv2 terms
	for (i=_nt->ncell; i < n_v_; ++i) {
		nd = _nt->_v_node[i];
		ip = _nt->_v_parent[i]->v_node_index;
		double* a = cmplx_spGetElement(m_, v_index_[ip], v_index_[i]);
		double* b = cmplx_spGetElement(m_, v_index_[i], v_index_[ip]);
		*a += NODEA(nd);
		*b += NODEB(nd);
		*diag_[i] -= NODEB(nd);
		*diag_[ip] -= NODEA(nd);
	}
	// jwC term
	Memb_list* mlc = _nt->tml->ml;
	int n = mlc->nodecount;
	for (i=0; i < n; ++i) {
		double* cd = mlc->data[i];
		j = mlc->nodelist[i]->v_node_index;
		diag_[v_index_[j]-1][1] += .001 * cd[0] * omega_;
	}
	// di/dv terms
	// because there may be several point processes of the same type
	// at the same location, we have to be careful to neither increment that
	// nd->v multiple times nor count the rhs multiple times.
	// So we can't take advantage of vectorized point processes.
	// To avoid this we do each mechanism item separately.
	// We assume there is no interaction between
	// separate locations. Note that interactions such as gap junctions
	// would not be handled in any case without computing a full jacobian.
	// i.e. calling nrn_rhs varying every state one at a time (that would
	// give the d2v/dv2 terms as well), but the expense is unwarranted.
	for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
		i = tml->index;
		if (i == CAP) { continue; }
		if (!memb_func[i].current) { continue; }
		Memb_list* ml = tml->ml;
		double* x1 = rv_; // use as temporary storage
		double* x2 = jv_;
		for (j = 0; j < ml->nodecount; ++j) { Node* nd = ml->nodelist[j];
			// zero rhs
			// save v
			NODERHS(nd) = 0;
			x1[j] = NODEV(nd);
			// v+dv
			NODEV(nd) += delta_;
			current(i, ml, j);
			// save rhs
			// zero rhs
			// restore v
			x2[j] = NODERHS(nd);
			NODERHS(nd) = 0;
			NODEV(nd) = x1[j];
			current(i, ml, j);
			// conductance
			// add to matrix
			*diag_[v_index_[nd->v_node_index]-1] -= (x2[j] - NODERHS(nd))/delta_;
		}
	}
}	

void NonLinImpRep::dids() {
	// same strategy as didv but now the innermost loop is over
	// every state but just for that compartment
	// outer loop over ode mechanisms in same order as we created the pv_ map	// so we can eas
	int ieq, i, in, is, iis;
	NrnThread* nt = nrn_threads;
	ieq = neq_ - n_ode_;
	for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
	  Memb_list* ml = tml->ml;
	  i = tml->index;
	  if (memb_func[i].ode_count && ml->nodecount) {
		int nc = ml->nodecount;
		Pfridot s = (Pfridot)memb_func[i].ode_count;
		int cnt = (*s)(i);
	    if (memb_func[i].current) {
		double* x1 = rv_; // use as temporary storage
		double* x2 = jv_;
		for (in = 0; in < ml->nodecount; ++in) { Node* nd = ml->nodelist[in];
			// zero rhs
			NODERHS(nd) = 0;
			// compute rhs
			current(i, ml, in);
			// save rhs
			x2[in] = NODERHS(nd);
			// each state incremented separately and restored
			for (iis = 0; iis < cnt; ++iis) {			
				is = ieq + in*cnt + iis;
				// save s
				x1[is] = *pv_[is];
				// increment s and zero rhs
				*pv_[is] += deltavec_[is];
				NODERHS(nd) = 0;
				current(i, ml, in);
				*pv_[is] = x1[is]; // restore s
				double g = (NODERHS(nd) - x2[in])/deltavec_[is];
				if (g != 0.) {
					double* elm = cmplx_spGetElement(m_, v_index_[nd->v_node_index], is+1);
					elm[0] = -g;
				}
			}
			// don't know if this is necessary but make sure last
			// call with respect to original states
			current(i, ml, in);
		}
	    }
		ieq += cnt*nc;
	  }
	}
}

void NonLinImpRep::dsdv() {
	int ieq, i, in, is, iis;
	NrnThread* nt = nrn_threads;
	ieq = neq_ - n_ode_;
	for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
	  Memb_list* ml = tml->ml;
	  i = tml->index;
	  if (memb_func[i].ode_count && ml->nodecount) {
		int nc = ml->nodecount;
		Pfridot s = (Pfridot)memb_func[i].ode_count;
		int cnt = (*s)(i);
	    if (memb_func[i].current) {
		double* x1 = rv_; // use as temporary storage
		double* x2 = jv_;
		// zero rhs, save v
		for (in = 0; in < ml->nodecount; ++in) { Node* nd = ml->nodelist[in];
			for (is = ieq + in*cnt, iis = 0; iis < cnt; ++iis, ++is) {
				*pvdot_[is] = 0.;
			}
			x1[in] = NODEV(nd);
		}
		// increment v only once in case there are multiple
		// point processes at the same location
		for (in = 0; in < ml->nodecount; ++in) { Node* nd = ml->nodelist[in];
			if (x1[in] == NODEV(nd)) {
				NODEV(nd) += delta_;
			}
		}
		// compute rhs. this is the rhs(v+dv)
		ode(i, ml);
		// save rhs, restore v, and zero rhs
		for (in = 0; in < ml->nodecount; ++in) { Node* nd = ml->nodelist[in];
			for (is = ieq + in*cnt, iis = 0; iis < cnt; ++iis, ++is) {
				x2[is] = *pvdot_[is];
				*pvdot_[is] = 0;
			}
			NODEV(nd) = x1[in];
		}
		// compute the rhs(v)
		ode(i, ml);
		// fill the ds/dv elements
		for (in = 0; in < ml->nodecount; ++in) { Node* nd = ml->nodelist[in];
			for (is = ieq + in*cnt, iis = 0; iis < cnt; ++iis, ++is) {
				double ds = (x2[is] - *pvdot_[is])/delta_;
				if (ds != 0.) {
					double* elm = cmplx_spGetElement(m_, is+1, v_index_[nd->v_node_index]);
					elm[0] = -ds;
				}
			}
		}
	    }
		ieq += cnt*nc;
	  }
	}
}

void NonLinImpRep::dsds() {
	int ieq, i, in, is, iis, ks, kks;
	NrnThread* nt = nrn_threads;
	// jw term
	for (i = neq_v_; i < neq_; ++i) {
		diag_[i][1] += omega_;
	}
	ieq = neq_v_;
	for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
	  Memb_list* ml = tml->ml;
	  i = tml->index;
	  if (memb_func[i].ode_count && ml->nodecount) {
		int nc = ml->nodecount;
		Pfridot s = (Pfridot)memb_func[i].ode_count;
		int cnt = (*s)(i);
		double* x1 = rv_; // use as temporary storage
		double* x2 = jv_;
		// zero rhs, save s
		for (in = 0; in < ml->nodecount; ++in) {
			for (is = ieq + in*cnt, iis = 0; iis < cnt; ++iis, ++is) {
				*pvdot_[is] = 0.;
				x1[is] = *pv_[is];
			}
		}
		// compute rhs. this is the rhs(s)
		ode(i, ml);
		// save rhs
		for (in = 0; in < ml->nodecount; ++in) {
			for (is = ieq + in*cnt, iis = 0; iis < cnt; ++iis, ++is) {
				x2[is] = *pvdot_[is];
			}
		}
	    // iterate over the states
	    for (kks=0; kks < cnt; ++kks) {
		// zero rhs, increment s(ks)
		for (in = 0; in < ml->nodecount; ++in) {
			ks = ieq + in*cnt + kks;
			for (is = ieq + in*cnt, iis = 0; iis < cnt; ++iis, ++is) {
				*pvdot_[is] = 0.;
			}
			*pv_[ks] += deltavec_[ks];
		}
		ode(i, ml);
		// store element and restore s
		// fill the ds/dv elements
		for (in = 0; in < ml->nodecount; ++in) { Node* nd = ml->nodelist[in];
			ks = ieq + in*cnt + kks;
			for (is = ieq + in*cnt, iis = 0; iis < cnt; ++iis, ++is) {
				double ds = ( *pvdot_[is] - x2[is])/deltavec_[is];
				if (ds != 0.) {
					double* elm = cmplx_spGetElement(m_, is+1, ks+1);
					elm[0] = -ds;
				}
				*pv_[ks] = x1[ks];
			}
		}
		// perhaps not necessary but ensures the last computation is with
		// the base states.
		ode(i, ml);
	    }
		ieq += cnt*nc;
	  }
	}
}

void NonLinImpRep::current(int im, Memb_list* ml, int in) { // assume there is in fact a current method
	Pfridot s = (Pfridot)memb_func[im].current;
	// fake a 1 element memb_list
	Memb_list mfake;
#if CACHEVEC != 0
	mfake.nodeindices = ml->nodeindices + in;
#endif
	mfake.nodelist = ml->nodelist+in;
	mfake.data = ml->data + in;
	mfake.pdata = ml->pdata + in;
	mfake.prop = ml->prop + in;
	mfake.nodecount = 1;
	mfake._thread = ml->_thread;
	(*s)(nrn_threads, &mfake, im);
}

void NonLinImpRep::ode(int im, Memb_list* ml) { // assume there is in fact an ode method
	int i, nc;
	Pfridot s = (Pfridot)memb_func[im].ode_spec;
	nc = ml->nodecount;
	if (memb_func[im].hoc_mech) {
		int j, count;
		count = ml->nodecount;
		for (j=0; j < count; ++j) {
			Node* nd = ml->nodelist[j];
			(*s)(nd, ml->prop[j]);
		}
	}else{
		(*s)(nrn_threads, ml, im);
	}
}

