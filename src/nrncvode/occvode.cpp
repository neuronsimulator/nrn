#include <../../nrnconf.h>
#define VECTORIZE 1
#include <errno.h>
#include <InterViews/resource.h>
#include <OS/string.h>
#include "nrnoc2iv.h"
#include "nrndaspk.h"
#include "cvodeobj.h"
#include "netcvode.h"
#include "ivocvect.h"
#include "vrecitem.h"
#include "membfunc.h"
typedef int (*Pfridot)(...);
extern "C" {
extern void setup_topology(), v_setup_vectors();
extern void nrn_mul_capacity(Memb_list*);
extern void nrn_div_capacity(Memb_list*);
extern int nrnmpi_comm;
extern int nrn_cvode_;
extern int diam_changed;
extern void recalc_diam();
extern int nrn_errno_check(int);
extern double t, dt;
extern void nrn_set_cj(double);
extern void long_difus_solve(int);
extern Symlist *hoc_built_in_symlist;

#include "spmatrix.h"
extern void linmod_dkmap(double**, double**);
extern double* sp13mat;

#if 1 || PARANEURON
extern void (*nrnmpi_v_transfer_)();
#endif
#if PARANEURON
extern void (*nrn_multisplit_solve_)();
extern void nrn_multisplit_nocap_v();
extern void nrn_multisplit_adjust_rhs();
extern int nrnmpi_int_sum_reduce(int in, int comm);
extern void nrnmpi_assert_opstep(int, double, int comm);
#endif
};

static Symbol* vsym; // for absolute tolerance
#define SETUP 1
#define USED 2
/*
CVODE expects dy/dt = f(y) and solve (I - gamma*J)*x = b with approx to
J=df/dy.

The NEURON fixed step method sets up C*dv/dt = F(v)
by first calculating F(v) and storing it on the right hand side of
the matrix equation ( see src/nrnoc/treeset.c nrn_rhs() ).
It then sets up the left hand side of the matrix equation using
nrn_set_cj(1./dt); setup1_tree_matrix(); setup2_tree_matrix();
to form
(C/dt -  J(F))*dv = F(v)
After a nrn_solve() the answer, dv, is stored in the right hand side
vector.

However, one must be aware of the fact that the cvode state vector
y is not the vector y for the fixed step in two ways. 1) the cvode
state vector includes ALL states, including channel states.
2) the cvode state vector does NOT include the zero area nodes
(since the capacitance for those nodes are 0). Furthermore, cvode
cannot work with the extracellular mechanism (both because extracellular
capacitance is often 0 and because  more than one dv/dt is involved
in some of the current balance equations) or LinearMechanism (same reasons).
In that case the current balance equations are of the differential
algebraic form c*dv/dt = f(v) where c is non-diagonal and may have empty rows.
The variable step method for these cases is handled by daspk.

*/

// determine neq_ and vector of pointers to scatter/gather y
// as well as algebraic nodes (no_cap)

boolean Cvode::init_global() {
#if PARANEURON
	if (!use_partrans_ && nrnmpi_numprocs > 1
	    && (nrnmpi_v_transfer_ || nrn_multisplit_solve_)) {
		// could be a lot better.
		use_partrans_ = true;
		mpicomm_ = nrnmpi_comm;
	}else
#endif
	if (!structure_change_) {
		return false;
	}
	if (cv_memb_list_ == nil) {
		neq_ = 0;
		if (use_daspk_) {
			return true;
		}
		return false;
	}
	return true;
}

void Cvode::init_eqn(){
	double vtol;

	CvMembList* cml;
	Memb_list* ml;
	int i, j, neq_v, neq_cap_v;
//printf("Cvode::init_eqn\n");
	cmlcap_ = nil;
	cmlext_ = nil;
	for (cml = cv_memb_list_; cml; cml = cml->next) {
		if (cml->index == CAP) {
			cmlcap_ = cml;
		}
		if (cml->index == EXTRACELL) {
			cmlext_ = cml;
		}
	}
	if (use_daspk_) {
		daspk_init_eqn();
		return;
	}
	// how many ode's are there? First ones are non-zero capacitance
	// nodes with non-zero capacitance
	Memb_func* mf;
	if (cmlcap_) {
		neq_cap_v = cmlcap_->ml->nodecount;
	}else{
		neq_cap_v = 0;
	}
	neq_ = neq_v = neq_cap_v;
	// now add the membrane mechanism ode's to the count
	for (cml = cv_memb_list_; cml; cml = cml->next) {
		Pfridot s = (Pfridot)memb_func[cml->index].ode_count;
		if (s) {
			neq_ += cml->ml->nodecount * (*s)(cml->index);
		}
	}
//printf("Cvode::init_eqn neq_v=%d neq_=%d\n", neq_v, neq_);
#if PARANEURON
	if (use_partrans_) {
		global_neq_ = nrnmpi_int_sum_reduce(neq_, mpicomm_);
	}
#endif
	if (pv_) {
		delete [] pv_;
		delete [] pvdot_;
	}
	pv_ = new double*[neq_];
	pvdot_ = new double*[neq_];
	atolvec_alloc(neq_);
	for (i=0; i < neq_; ++i) {
		atolvec_[i] = ncv_->atol();
	}
	vtol = 1.;
	if (!vsym) {
		vsym = hoc_table_lookup("v", hoc_built_in_symlist);
	}
	if (vsym->extra) {
		double x;
		x = vsym->extra->tolerance;
		if (x != 0 && x < vtol) {
			vtol = x;
		}
	}
	// deal with voltage nodes
	// only cap nodes for cvode
	for (i=0; i < v_node_count_; ++i) {
		//sentinal values for determining no_cap
		NODERHS(v_node_[i]) = 1.;
	}
	for (i=0; i < neq_cap_v; ++i) {
		ml = cmlcap_->ml;
		pv_[i] = &NODEV(ml->nodelist[i]);
		pvdot_[i] = &(NODERHS(ml->nodelist[i]));
		*pvdot_[i] = 0.; // only ones = 1 are no_cap
		atolvec_[i] *= vtol;
	}

	// the remainder are no_cap nodes
	if (no_cap_node) {
		delete [] no_cap_node;
		delete [] no_cap_child;
	}
	no_cap_node = new Node*[v_node_count_ - neq_cap_v];
	no_cap_child = new Node*[v_node_count_ - neq_cap_v];
	no_cap_count = 0;
	j = 0;
	for (i=0; i < v_node_count_; ++i) {
		if (NODERHS(v_node_[i]) > .5) {
			no_cap_node[no_cap_count++] = v_node_[i];
		}
		if (v_parent_[i] && NODERHS(v_parent_[i]) > .5) {
			no_cap_child[j++] = v_node_[i];
		}
	}
	no_cap_child_count = j;

	// use the sentinal values in NODERHS to construct a new no cap membrane list
	new_no_cap_memb();
	
	// map the membrane mechanism ode state and dstate pointers
	int ieq = neq_v;
	for (cml = cv_memb_list_; cml; cml = cml->next) {
		int n;
		ml = cml->ml;
		mf = memb_func + cml->index;
		Pfridot sc = (Pfridot)mf->ode_count;
		if (sc && ( (n = (*sc)(cml->index)) > 0)) {
			Pfridot s = (Pfridot)mf->ode_map;
			if (mf->hoc_mech) {
				for (j=0; j < ml->nodecount; ++j) {
(*s)(ieq, pv_ + ieq, pvdot_ + ieq, ml->prop[j], atolvec_ + ieq);
					ieq += n;
				}
			}else{
				for (j=0; j < ml->nodecount; ++j) {
(*s)(ieq, pv_ + ieq, pvdot_ + ieq, ml->data[j], ml->pdata[j], atolvec_ + ieq, cml->index);
					ieq += n;
				}
			}
		}
	}
	structure_change_ = false;
}

void Cvode::new_no_cap_memb() {
	int i, n;
	CvMembList* cml, *ncm;
	Memb_list* ml;
	delete_memb_list(no_cap_memb_);
	no_cap_memb_ = nil;
	for (cml = cv_memb_list_; cml; cml = cml->next) {
		Memb_list* ml = cml->ml;
		Memb_func* mf = memb_func + cml->index;
		// only point processes with currents are possibilities
		if (!mf->is_point || !mf->current) { continue; }
		// count how many at no cap nodes
		n = 0;
		for (i=0; i < ml->nodecount; ++i) {
			if (NODERHS(ml->nodelist[i]) > .5) {
				++n;
			}
		}
		if (n == 0) continue;
		// keep same order
		if (no_cap_memb_ == nil) {
			no_cap_memb_ = new CvMembList();
			ncm = no_cap_memb_;
		}else{
			ncm->next = new CvMembList();
			ncm = ncm->next;
		}
		ncm->next = nil;
		ncm->index = cml->index;
		ncm->ml->nodecount = n;
		// allocate
		ncm->ml->nodelist = new Node*[n];
#if CACHEVEC
		ncm->ml->nodeindices = new int[n];
#endif
		if (mf->hoc_mech) {
			ncm->ml->prop = new Prop*[n];
		}else{
			ncm->ml->data = new double*[n];
			ncm->ml->pdata = new Datum*[n];
		}
		// fill
		n = 0;
		for (i=0; i < ml->nodecount; ++i) {
			if (NODERHS(ml->nodelist[i]) > .5) {
				ncm->ml->nodelist[n] = ml->nodelist[i];
#if CACHEVEC
				ncm->ml->nodeindices[n] = ml->nodeindices[i];
#endif
				if (mf->hoc_mech) {
					ncm->ml->prop[n] = ml->prop[i];
				}else{
					ncm->ml->data[n] = ml->data[i];
					ncm->ml->pdata[n] = ml->pdata[i];
				}
				++n;
			}
		}
	}
}

void Cvode::daspk_init_eqn(){
	// DASPK equation order is exactly the same order as the
	// fixed step method for current balance (including
	// extracellular nodes) and linear mechanism. Remaining ode
	// equations are same order as for Cvode. Thus, daspk differs from
	// cvode order primarily in that cap and no-cap nodes are not
	// distinguished.
	double vtol;
//printf("Cvode::daspk_init_eqn\n");
	int i, j, in, ie, k, neq_v;

	// how many equations are there?
	Memb_func* mf;
	CvMembList* cml;
	//start with all the equations for the fixed step method.
	if (use_sparse13 == 0 || diam_changed != 0) {
		recalc_diam();
	}
	neq_v_ = spGetSize(sp13mat_, 0);
	neq_ = neq_v_;
	// now add the membrane mechanism ode's to the count
	for (cml = cv_memb_list_; cml; cml = cml->next) {
		Pfridot s = (Pfridot)memb_func[cml->index].ode_count;
		if (s) {
			neq_ += cml->ml->nodecount * (*s)(cml->index);
		}
	}
//printf("Cvode::daspk_init_eqn: neq_v_=%d neq_=%d\n", neq_v_, neq_);
	if (pv_) {
		delete [] pv_;
		delete [] pvdot_;
	}
	pv_ = new double*[neq_];
	pvdot_ = new double*[neq_];
	atolvec_alloc(neq_);
	for (i=0; i < neq_; ++i) {
		atolvec_[i] = ncv_->atol();
	}
	vtol = 1.;
	if (!vsym) {
		vsym = hoc_table_lookup("v", hoc_built_in_symlist);
	}
	if (vsym->extra) {
		double x;
		x = vsym->extra->tolerance;
		if (x != 0 && x < vtol) {
			vtol = x;
		}
	}
	// deal with voltage and extracellular and linear circuit nodes
	// for daspk the order is the same
	if (use_sparse13) {
		for (in = 0; in < v_node_count; ++in) {
			Node* nd; Extnode* nde;
			nd = v_node[in];
			nde = nd->extnode;
			i = nd->eqn_index_ - 1; // the sparse matrix index starts at 1
			pv_[i] = &NODEV(nd);
			pvdot_[i] = nd->_rhs;
			if (nde) {
				for (ie=0; ie < nlayer; ++ie) {
					k = i + ie + 1;
					pv_[k] = nde->v + ie;
					pvdot_[k] = nde->_rhs[ie];
				}
			}
		}
		linmod_dkmap(pv_, pvdot_);
		for (i=0; i < neq_v_; ++i) {
			atolvec_[i] *= vtol;
		}
	}else{
		for (i=0; i < neq_v_; ++i) {
			pv_[i] = &NODEV(v_node[i]);
			pvdot_[i] = v_node[i]->_rhs;
			atolvec_[i] *= vtol;
		}
	}

	// map the membrane mechanism ode state and dstate pointers
	int ieq = neq_v_;
	for (cml = cv_memb_list_; cml; cml = cml->next) {
		int n;
		mf = memb_func + cml->index;
		Pfridot sc = (Pfridot)mf->ode_count;
		if (sc && ( (n = (*sc)(cml->index)) > 0)) {
			Memb_list* ml = cml->ml;
			Pfridot s = (Pfridot)mf->ode_map;
			if (mf->hoc_mech) {
				for (j=0; j < ml->nodecount; ++j) {
(*s)(ieq, pv_ + ieq, pvdot_ + ieq, ml->prop[j], atolvec_ + ieq);
					ieq += n;
				}
			}else{
				for (j=0; j < ml->nodecount; ++j) {
(*s)(ieq, pv_ + ieq, pvdot_ + ieq, ml->data[j], ml->pdata[j], atolvec_ + ieq, cml->index);
					ieq += n;
				}
			}
		}
	}
	structure_change_ = false;
}

void Cvode::scatter_y(double* y){
	int i;
	scattered_y_ = y;
	for (i = 0; i < neq_; ++i) {
		*(pv_[i]) = y[i];
//printf("%d scatter_y %d %g\n", nrnmpi_myid, i,  y[i]);
	}
	CvMembList* cml;
	for (cml = cv_memb_list_; cml; cml = cml->next) {
		Memb_func* mf = memb_func + cml->index;
		if (mf->ode_synonym) {
			Pfridot s = (Pfridot)mf->ode_synonym;
			Memb_list* ml = cml->ml;
			(*s)(ml->nodecount, ml->data, ml->pdata);
		}
	}
}
void Cvode::gather_y(double* y){
	int i;
	for (i = 0; i < neq_; ++i) {
		y[i] = *(pv_[i]);
//printf("gather_y %d %g\n", i,  y[i]);
	}
}
void Cvode::scatter_ydot(double* ydot){
	int i;
	for (i = 0; i < neq_; ++i) {
		*(pvdot_[i]) = ydot[i];
//printf("scatter_ydot %d %g\n", i,  ydot[i]);
	}
}
void Cvode::gather_ydot(double* ydot){
	int i;
    if (ydot){
	for (i = 0; i < neq_; ++i) {
		ydot[i] = *(pvdot_[i]);
//printf("%d gather_ydot %d %g\n", nrnmpi_myid, i,  ydot[i]);
	}
    }
}

int Cvode::setup(double* ypred, double* fpred){
//printf("Cvode::setup\n");
	++jac_calls_;
	return 0;
}

int Cvode::solvex(double* b, double* y){
	++mxb_calls_;
//printf("Cvode::solvex t=%g t_=%g\n", t, t_);
//printf("Cvode::solvex %g\n", gam());
//printf("\tenter b\n");
//for (int i=0; i < neq_; ++i) { printf("\t\t%d %g\n", i, b[i]);}
	if (ncv_->stiff() == 0) { return 0; }
	if (gam() == 0.) { return 0; } // i.e. (I - gamma * J)*x = b means x = b
	int i;
	nrn_cvode_ = 1;
	nrn_set_cj(1./gam());
	dt = gam();
	lhs(); // special version for cvode.
	scatter_ydot(b);
	nrn_mul_capacity(cmlcap_->ml);
	for (i=0; i < no_cap_count; ++i) {
		NODERHS(no_cap_node[i]) = 0.;
	}
	// solve it
#if PARANEURON
	if (nrn_multisplit_solve_) {
		(*nrn_multisplit_solve_)();
	}else
#endif
	{
		triang();
		bksub();
	}
//for (i=0; i < v_node_count; ++i) {
//	printf("%d rhs %d %g t=%g\n", nrnmpi_myid, i, VEC_RHS(i), t);
//}
	if (ncv_->stiff() == 2) {
		solvemem();
	}else{
		// bug here should multiply by gam
	}
	gather_ydot(b);
	nrn_cvode_ = 0;
//printf("\texit b\n");
//for (i=0; i < neq_; ++i) { printf("\t\t%d %g\n", i, b[i]);}
	return 0;
}
	
void Cvode::solvemem() {
	// all the membrane mechanism matrices
	CvMembList* cml;
	for (cml = cv_memb_list_; cml; cml = cml->next) { // probably can start at 6 or hh
		Memb_func* mf = memb_func + cml->index;
		if (mf->ode_matsol) {
			Memb_list* ml = cml->ml;
			Pfridot s = (Pfridot)mf->ode_matsol;
			if (mf->vectorized) {
				(*s)(ml, cml->index);
			}else{
				int j, count;
				count = ml->nodecount;
				if (mf->hoc_mech) {
					for (j = 0; j < count; ++j) {
						Node* nd = ml->nodelist[j];
						(*s)(nd, ml->prop[j]);
					}
				}else{
					for (j = 0; j < count; ++j) {
						Node* nd = ml->nodelist[j];
						(*s)(nd, ml->data[j], ml->pdata[j]);
					}
				}
			}
			if (errno) {
				if (nrn_errno_check(cml->index)) {
hoc_warning("errno set during ode jacobian solve", (char*)0);
				}
			}
		}
	}
	long_difus_solve(2);
}

void Cvode::fun(double tt, double* y, double* ydot){
	++f_calls_;
	nrn_cvode_ = 1;
	t = tt;

	// fix this!!!
	dt = h(); // really does not belong here but dt is needed for events
	if (dt == 0.) { dt = 1e-8; }

//printf("%lx fun %d %.15g %g\n", (long)this, neq_, t, dt);
	play_continuous(tt);
	scatter_y(y);
#if PARANEURON
	if (use_partrans_) {
		nrnmpi_assert_opstep(opmode_, t, mpicomm_);
	}
#endif
	nocap_v();  // vm at nocap nodes consistent with adjacent vm
#if PARANEURON
	if (nrnmpi_v_transfer_) {
		(*nrnmpi_v_transfer_)();
	}
#endif
	before_after(before_breakpoint_);
	rhs(); // similar to nrn_rhs in treeset.c
#if PARANEURON
	if (nrn_multisplit_solve_) { // non-zero area nodes need an adjustment
		nrn_multisplit_adjust_rhs();
	}
#endif
	do_ode();
	// divide by cm and compute capacity current
	nrn_div_capacity(cmlcap_->ml);
	gather_ydot(ydot);
	before_after(after_solve_);
	nrn_cvode_ = 0;
//for (int i=0; i < neq_; ++i) { printf("\t%d %g %g\n", i, y[i], ydot?ydot[i]:-1e99);}
}

void Cvode::before_after(BAMechList* baml) {
	BAMechList* ba;
	int i, j;
	for (ba = baml; ba; ba = ba->next) {
		Pfridot f = (Pfridot)ba->bam->f;
		Memb_list* ml = memb_list + ba->bam->type;
		if (ba->indices) {
			for (j=0; j < ba->cnt; ++j) {
				i = ba->indices[j];
				(*f)(ml->nodelist[i], ml->data[i], ml->pdata[i]);
			}
		}else{
			for (i=0; i < ml->nodecount; ++i) {
				(*f)(ml->nodelist[i], ml->data[i], ml->pdata[i]);
			}
		}
	}
}

/*
v at nodes with capacitance is correct (from scatter v) however
v at no-cap nodes is out of date since the values are from the
previous call. v would merely be the weighted average of
the adjacent v's except for the possibility of membrane
currents at branch points. We thus need to calculate both i(v)
and di/dv at those zero area nodes so that we can solve the
algebraic equation (di/dv + a_j)*vmnew =  - i(vmold) + a_j*v_j.
The simplest case is no membrane current and root or leaf. In that
case vmnew = v_j. The next simplest case is no membrane current.
In that case, vm is the weighted sum (via the axial coefficients)
of v_j.
For now we handle only the general case when there are membrane currents
This was done by constructing a list of membrane mechanisms that
contribute to the membrane current at the nocap nodes.
*/

void Cvode::nocap_v(){
	int i;

	for (i = 0; i < no_cap_count; ++i) { // initialize storage
		Node* nd = no_cap_node[i];
		NODED(nd) = 0;
		NODERHS(nd) = 0;
	}
	// compute the i(vmold) and di/dv
	rhs_memb(no_cap_memb_);
	lhs_memb(no_cap_memb_);

	for (i = 0; i < no_cap_count; ++i) {// parent axial current
		Node* nd = no_cap_node[i];
		// following from global v_parent
		NODERHS(nd) += NODED(nd) * NODEV(nd);
		Node* pnd = v_parent[nd->v_node_index];
		if (pnd) {
			NODERHS(nd) -= NODEB(nd) * NODEV(pnd);
			NODED(nd) -= NODEB(nd);
		}
	}		

	for (i = 0; i < no_cap_child_count; ++i) {// child axial current
		Node* nd = no_cap_child[i];
		// following from global v_parent
		Node* pnd = v_parent[nd->v_node_index];
		NODERHS(pnd) -= NODEA(nd) * NODEV(nd);
		NODED(pnd) -= NODEA(nd);
	}		

#if PARANEURON
	if (nrn_multisplit_solve_) { // add up the multisplit equations
		nrn_multisplit_nocap_v();
	}
#endif

	for (i = 0; i < no_cap_count; ++i) {
		Node* nd = no_cap_node[i];
		NODEV(nd) = NODERHS(nd) / NODED(nd);
//		printf("%d %d %g v=%g\n", nrnmpi_myid, i, t, NODEV(nd));
	}		
	// no_cap v's are now consistent with adjacent v's
}

void Cvode::do_ode(){
	// all the membrane mechanism ode's
	CvMembList* cml;
	Memb_func* mf;
	for (cml = cv_memb_list_; cml; cml = cml->next) { // probably can start at 6 or hh
		mf = memb_func + cml->index;
		if (mf->ode_spec) {
			Pfridot s = (Pfridot)mf->ode_spec;
			Memb_list* ml = cml->ml;
			if (mf->vectorized) {
				(*s)(ml, cml->index);
			}else{
				int j, count;
				count = ml->nodecount;
				if (mf->hoc_mech) {
					for (j = 0; j < count; ++j) {
						Node* nd = ml->nodelist[j];
						(*s)(nd, ml->prop[j]);
					}
				}else{
					for (j = 0; j < count; ++j) {
						Node* nd = ml->nodelist[j];
						(*s)(nd, ml->data[j], ml->pdata[j]);
					}
				}
			}
			if (errno) {
				if (nrn_errno_check(cml->index)) {
hoc_warning("errno set during ode evaluation", (char*)0);
				}
			}
		}
	}
	long_difus_solve(1);
}

void Cvode::do_nonode() { // all the hacked integrators, etc, in SOLVE procedure
//almost a verbatim copy of nonvint in fadvance.c

	CvMembList* cml;
	for (cml = cv_memb_list_; cml; cml = cml->next) {
		Memb_func* mf = memb_func + cml->index;
	 if (mf->state) {
	  Memb_list* ml = cml->ml;
	  if (!mf->ode_spec){
		Pfridot s = (Pfridot)mf->state;
		(*s)(ml, cml->index);
#if 0
		if (errno) {
			if (nrn_errno_check(cml->index)) {
hoc_warning("errno set during calculation of states", (char*)0);
			}
		}
#endif
	  }else if (mf->singchan_) {
		Pfridot s = (Pfridot)mf->singchan_;
		(*s)(ml, cml->index);
	  }
	 }
	}
}

void Cvode::states(double* pd) {
	int i;
	double* s = N_VGetArrayPointer(y_);
	for (i=0; i < neq_; ++i) {
//		pd[i] = *pv_[i];
		pd[i] = s[i];
	}	
}

void Cvode::dstates(double* pd) {
	int i;
	for (i=0; i < neq_; ++i) {
		pd[i] = *pvdot_[i];
	}	
}

void Cvode::error_weights(double* pd) {
	int i;
	double* s = ewtvec();
	for (i=0; i < neq_; ++i) {
		pd[i] = s[i];
	}	
}

void Cvode::acor(double* pd) {
	int i;
	double* s = acorvec();
	for (i=0; i < neq_; ++i) {
		pd[i] = s[i];
	}	
}

void Cvode::delete_prl() {
	if (play_) {
		delete play_;
	}
	play_ = nil;
	if (record_) {
		delete record_;
	}
	record_ = nil;
}

void Cvode::record_add(PlayRecord* pr) {
	if (!record_) {
		record_ = new PlayRecList(1);
	}
	record_->append(pr);
}

void Cvode::record_continuous() {
	if (before_step_) {
		before_after(before_step_);
	}
	if (record_) {
		for (long i=0; i < record_->count(); ++i) {
			record_->item(i)->continuous(t_);
		}
	}
}

void Cvode::play_add(PlayRecord* pr) {
	if (!play_) {
		play_ = new PlayRecList(1);
	}
	play_->append(pr);
}

void Cvode::play_continuous(double tt) {
	if (play_) {
		for (long i=0; i < play_->count(); ++i) {
			play_->item(i)->continuous(tt);
		}
	}
}
