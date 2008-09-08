#include <../../nrnconf.h>
// linear model whose equations are solved simultaneously with the
// voltage equations.
// The c*dy/dt + g*y = b equations are added to the node equations
// and the policy is that the the list of nodes pertains to the first
// equations.

// this has only the essential info with regard to solving equations and
// nothing with regard to parameterization.

// the matrices are assumed to be constant during a simulation run.
// and there is no provision here for changing bvec.

// MatrixMap gives fast copying of linear model matrix to main tree matrix

// In DASPK, the equation order for voltage equations is the same as for
// the fixed step method (see nrncvode/occcvode.cpp Cvode::daspk_)
// This is a different order than that of cvode in which cap nodes are first
// followed by no-cap nodes.
// The parallel extends to the additional equations in these linear mechanisms
// along with extracellular nodes.
// Therefore bmap_ can be used directly for the map to the
// daspk equation indices.

#include <stdio.h>
#include "linmod.h"
#include "nrnoc2iv.h"

extern "C" {
#include "membfunc.h"
#include "spmatrix.h"
extern void linmod_alloc();
extern int linmod_extra_eqn_count();
extern void linmod_init();
extern void linmod_rhs(); // relative to c*dy/dt = -g*y + b
extern void linmod_lhs();
extern void linmod_dkmap(double**, double**);
extern void linmod_dkres(double*, double*, double*);
extern void linmod_dkpsol(double);
extern void linmod_update();
extern void nrn_matrix_node_free();
extern int cvode_active_;
extern int nrn_use_daspk_;
extern int secondorder;
}



implementPtrList(LinmodPtrList, LinearModelAddition)

static LinmodPtrList* linmod_list;

int linmod_extra_eqn_count() {
	int i, cnt, neqn;
	neqn = 0;
	if (!linmod_list){
		linmod_list = new LinmodPtrList();
	}
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		neqn += m->extra_eqn_count();
	}
	return neqn;
}

void linmod_alloc() {
	int i, cnt, neqn;
	NrnThread* _nt = nrn_threads;
	nrn_thread_error("LinearMechanism only one thread allowed");
	neqn = _nt->end;
	if (_nt->_ecell_memb_list) {
		neqn += _nt->_ecell_memb_list->nodecount * nlayer;
	}
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		m->alloc(neqn+1);
		neqn += m->extra_eqn_count();
	}
}

void linmod_init() {
	int i, cnt;
	cnt = linmod_list->count();
	if (cnt > 0 && (secondorder > 0 || (cvode_active_ > 0) && (nrn_use_daspk_ == 0))) {
hoc_execerror("LinearMechanisms only work with secondorder==0 or daspk",0);
	}
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		m->init();		
	}
}

void linmod_rhs() {
	int i, cnt;
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		m->rhs();
	}
}

void linmod_lhs() {
	int i, cnt;
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		m->lhs();
	}
}

void linmod_dkmap(double** pv, double** pvdot) {
	int i, cnt;
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		m->dkmap(pv, pvdot);
	}
}

void linmod_dkres(double* y, double* yprime, double* delta) {
	// c*y' + g*y = b so
	// delta = c*y' + g*y - b
	int i, cnt;
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		m->dkres(y, yprime, delta);
	}
}

void linmod_dkpsol(double cj) {
	// c*y' + g*y = b and we need
	// A*x = b where A = dG/dY + CJ*dG/dYPRIME and
	// 0 = G(y, y', t) which is equal to our delta above
	// so we help to set up here the matrix equation portion
	// (cj*c + g)*x = 0
	int i, cnt;
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		m->dkpsol(cj);
	}
}

void linmod_update() {
	int i, cnt;
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		LinearModelAddition* m = linmod_list->item(i);
		m->update();		
	}
}

LinearModelAddition::LinearModelAddition(Matrix* cmat, Matrix* gmat,
	Vect* yvec, Vect* y0, Vect* bvec, int nnode, Node** nodes, Vect* elayer) {
//printf("LinearModelAddition %lx\n", (long)this);
	int i;
	nnode_ = nnode;
	nodes_ = nodes;
	if (nnode_ > 0) {
		elayer_ = new int[nnode_];
		if (elayer) {
			for (i=0; i < nnode_; ++i) {
				elayer_[i] = int(elayer->elem(i));
			}
		}else{
			for (i=0; i < nnode_; ++i) {
				elayer_[i] = 0;
			}
		}
	}else{
		elayer_ = nil;
	}
	c_ = new MatrixMap(cmat);
	g_ = new MatrixMap(gmat);
	y_ = yvec;
	y0_ = y0;
	b_ = bvec;
	gy_ = new Vect((Object*)nil);
	cyp_ = new Vect((Object*)nil);
	yptmp_ = new Vect((Object*)nil);
	ytmp_ = new Vect((Object*)nil);
	bmap_ = new int[1];
	if (linmod_list == nil) {
		linmod_list = new LinmodPtrList();
	}
	linmod_list->append(this);
//	use_sparse13 = 1;
	nrn_matrix_node_free();
}

LinearModelAddition::~LinearModelAddition() {
//printf("~LinearModelAddition %lx\n", (long)this);
	int i, cnt;
	cnt = linmod_list->count();
	for (i=0; i < cnt; ++i) {
		if (linmod_list->item(i) == this) {
			linmod_list->remove(i);
			break;
		}
	}
	lmafree();
	delete gy_;
	delete cyp_;
	delete yptmp_;
	delete ytmp_;
	delete g_;
	delete c_;
	if (elayer_) {
		delete [] elayer_;
	}
//	if (linmod_list->count() == 0) {
//		use_sparse13 = 0;
//	}
	nrn_matrix_node_free();
}

void LinearModelAddition::lmafree() {
//printf("LinearModelAddition::free %lx\n", (long)this);
	delete [] bmap_;
	g_->mmfree();
	c_->mmfree();
}

int LinearModelAddition::extra_eqn_count() {
//printf("LinearModelAddition::extra_eqn_count %lx\n", (long)this);
//printf("  nnode_=%d g_->m_->nrow()=%d\n", nnode_, g_->m_->nrow());
	return g_->m_->nrow() - nnode_;
}

void LinearModelAddition::alloc(int start_index) {
//printf("LinearModelAddition::alloc %lx\n", (long)this);
	int i;
	size_ = b_->capacity();
	assert(y_->capacity() == size_);
	if (y0_) {
		assert(y0_->capacity() == size_);
	}
	assert(c_->m_->nrow() == size_ && c_->m_->ncol() == size_);
	assert(g_->m_->nrow() == size_ && g_->m_->ncol() == size_);
	gy_->resize(size_);
	cyp_->resize(size_);
	ytmp_->resize(size_);
	yptmp_->resize(size_);
	start_ = start_index;
//printf("start=%d size=%d\n", start_, size_);
	delete [] bmap_;
	bmap_ = new int[size_];
	for (i = 0; i < size_; ++i) {
		if (i < nnode_) {
			bmap_[i] = nodes_[i]->eqn_index_ + elayer_[i];
			if (elayer_[i] > 0 && !nodes_[i]->extnode) {
//hoc_execerror(secname(nodes_[i]->sec), "LinearModelAddition: Referring to an extracellular layer but\nextracellular is not inserted.");
// instead treat as though connected to ground.
				bmap_[i] = 0;
			}
		}else{
			bmap_[i] = start_ + i - nnode_;
		}
	}
//printf("g_->alloc start=%d, nnode=%d\n", start_, nnode_);
	g_->alloc(start_, nnode_, nodes_, elayer_);
//printf("c_->alloc start=%d, nnode=%d\n", start_, nnode_);
	c_->alloc(start_, nnode_, nodes_, elayer_);
}

void LinearModelAddition::init() {
//printf("LinearModelAddition::init %lx\n", (long)this);
//printf("init size_=%d %d %d %d\n", size_, y_->capacity(), y0_->capacity(), b_->capacity());
	int i;
	v2y();
	if (y0_) {
		for (i=nnode_; i < size_; ++i) {
			y_->elem(i) = y0_->elem(i);
		}
	}else{
		for (i=nnode_; i < size_; ++i) {
			y_->elem(i) = 0.;
		}
	}
//for (i=0; i < nnode_; ++i) printf(" i=%d y_[i]=%g\n", i, y_->elem(i));
}

void LinearModelAddition::v2y() {
	// vm,vext may be reinitialized between fixed steps and certainly
	// have been adjusted by daspk
	int i;
	for (i=0; i < nnode_; ++i) {
		Node* nd = nodes_[i];
		//Note elayer_[0] refers to internal.
		if (elayer_[i] == 0) {
			y_->elem(i) = NODEV(nd);
			if (nd->extnode) {
				y_->elem(i) += nd->extnode->v[0];
			}
		}else if (nd->extnode){
			y_->elem(i) = nd->extnode->v[elayer_[i] - 1];
		}
	}
}

void LinearModelAddition::rhs() {
//printf("LinearModelAddition::rhs %lx\n", (long)this);
	//right side portion of (c/dt +g)*[dy] =  -g*y + b
	int i;
	NrnThread* _nt = nrn_threads;
	// vm,vext may be reinitialized between fixed steps and certainly
	// has been adjusted by daspk
	v2y();
	g_->m_->mulv(y_, gy_);
	for (i = 0; i < size_; ++i) {
		_nt->_actual_rhs[bmap_[i]] += b_->elem(i) - gy_->elem(i);
	}
}

void LinearModelAddition::lhs() {
//printf("LinearModelAddition::lhs %lx\n", (long)this);
	//left side portion of (c/dt +g)*[dy] =  -g*y + b
	c_->add(nrn_threads[0].cj);
	g_->add(1.);
}

void LinearModelAddition::dkmap(double** pv, double** pvdot) {
//printf("LinearModelAddition::dkmap\n");
	int i;
	NrnThread* _nt = nrn_threads;
	for (i=nnode_; i < size_; ++i) {
//printf("bmap_[%d] = %d\n", i, bmap_[i]);
		pv[bmap_[i]-1] = y_->vec() + i;
		pvdot[bmap_[i]-1] = _nt->_actual_rhs + bmap_[i];
	}
}

static void prm(const char* s, int n, Matrix* m) {
	int i, j;
	printf("%s\n", s);
	for (i=0; i < n; ++ i) {
		for (j=0; j < n; ++j) {
			printf(" %g", m->getval(i, j));
		}
		printf("\n");
	}
}

static void prv(const char* s, int n, Vect* v) {
	int i;
	printf("%s\n", s);
	for (i=0; i < n; ++ i) {
		printf(" %g", v->elem(i));
	}
	printf("\n");
}

void LinearModelAddition::dkres(double* y, double* yprime, double* delta) {
//printf("LinearModelAddition::dkres %lx\n", (long)this);
	// delta is already -g*y + b
	// now subtract c*y'
	// The problem is the map between y and yprime and the local
	// representation of the y and yprime
	int i;
	
	for (i=0; i < size_; ++i) {
//printf("%d %d %g %g\n", i, bmap_[i]-1, y[bmap_[i]-1], yprime[bmap_[i]-1]);
		yptmp_->elem(i) = yprime[bmap_[i]-1];
	}	
#if 0
	prv("b", size_, b_);
	prv("yp", size_, yptmp_);
	prm("c", size_, c_->m_);
#endif
	c_->m_->mulv(yptmp_, cyp_); // mulv cannot multiply in place
#if 0
	prv("c*yp", size_, cyp_);
#endif
	for (i=0; i < size_; ++i) {
		delta[bmap_[i]-1] -= cyp_->elem(i);
	}
}

void LinearModelAddition::dkpsol(double cj) {
//printf("LinearModelAddition::dpsol %lx\n", (long)this);
	// delta = c*y' + g*y - b
	// add c*cj + g to the matrix
	c_->add(cj);
	g_->add(1.);
}

void LinearModelAddition::update() {
//printf("LinearModelAddition::update %lx\n", (long)this);
	int i;
	NrnThread* _nt = nrn_threads;
	// note that the following is correct also for states that refer
	// to the internal potential of a segment. i.e rhs is v + vext[0]
	for (i = 0; i < size_; ++i) {
		y_->elem(i) += _nt->_actual_rhs[bmap_[i]];
	}
//for (i=0; i < size_; ++i) printf(" i=%d bmap_[i]=%d y_[i]=%g\n", i, bmap_[i], y_->elem(i));
}

MatrixMap::MatrixMap(Matrix* mat) {
	m_ = mat;
	plen_ = 0;
	ptree_ = new double*[1];
	pm_ = new double*[1];
}

MatrixMap::~MatrixMap() {
	mmfree();
}

void MatrixMap::mmfree() {
	if (ptree_) {
		delete [] ptree_;
		delete [] pm_;
		ptree_ = nil;
		pm_ = nil;
	}
}

void MatrixMap::add(double fac) {
	int i;
	for (i=0 ; i < plen_; ++i) {
//printf("i=%d %g += %g * %g\n", i, *ptree_[i], fac, *pm_[i]);
		*ptree_[i] += fac * (*pm_[i]);
	}
}

void MatrixMap::alloc(int start, int nnode, Node** nodes, int* layer) {
	NrnThread* _nt = nrn_threads;
	mmfree();
	// how many elements
	int nrow = m_->nrow();
	int ncol = m_->ncol();
//printf("MatrixMap::alloc nrow=%d ncol=%d\n", nrow, ncol);
	int i, j, it, jt;
	plen_ = 0;
	for (i = 0; i < nrow; ++i) {
		for (j = 0; j < ncol; ++j) {
			if (m_->getval(i, j) != 0.0) {
				++plen_;
			}
		}
	}
	pm_ = new double*[plen_];
	ptree_ = new double*[plen_];
	plen_ = 0;
	for (i = 0; i < nrow; ++i) {
		if (i < nnode) {
			it = nodes[i]->eqn_index_ + layer[i];
//printf("i=%d it=%d area=%g\n", i, it, nodes[i]->area);
			if (layer[i]>0 && !nodes[i]->extnode) {
				it = 0;
			}
		}else{
			it = start + i - nnode;
		}
		for (j = 0; j < ncol; ++j) {
			if (m_->getval(i, j) != 0.0) {
				pm_[plen_] = m_->mep(i, j);
				if (j < nnode) {
					jt = nodes[j]->eqn_index_ + layer[j];
					if (layer[j]>0 && !nodes[j]->extnode) {
						jt = 0;
					}
				}else{
					jt = start + j - nnode;
				}
//printf("MatrixMap::alloc getElement(%d,%d)\n", it, jt);
				ptree_[plen_] = spGetElement(_nt->_sp13mat, it, jt);
				++plen_;
			}
		}
	}
}
