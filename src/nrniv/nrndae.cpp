#include <../../nrnconf.h>
#include <cstdio>
#include "nrndae.h"
#include "nrndae_c.h"
#include "nrnoc2iv.h"
#include "treeset.h"
#include "utils/enumerate.h"

extern int secondorder;

static NrnDAEPtrList nrndae_list;

int nrndae_list_is_empty() {
    return nrndae_list.empty() ? 1 : 0;
}


void nrndae_register(NrnDAE* n) {
    nrndae_list.push_back(n);
}

void nrndae_deregister(NrnDAE* n) {
    nrndae_list.remove(n);
}

int nrndae_extra_eqn_count() {
    int neqn = 0;
    for (NrnDAE* item: nrndae_list) {
        neqn += item->extra_eqn_count();
    }
    return neqn;
}

void nrndae_update(NrnThread* _nt) {
    update_sp13_rhs_based_on_actual_rhs(_nt);
    for (NrnDAE* item: nrndae_list) {
        item->update();
    }
    update_actual_rhs_based_on_sp13_rhs(_nt);
}

void nrndae_alloc() {
    NrnThread* _nt = nrn_threads;
    nrn_thread_error("NrnDAE only one thread allowed");
    int neqn = _nt->end;
    if (_nt->_ecell_memb_list) {
        neqn += _nt->_ecell_memb_list->nodecount * nlayer;
    }
    for (NrnDAE* item: nrndae_list) {
        item->alloc(neqn + 1);
        neqn += item->extra_eqn_count();
    }
}


void nrndae_init() {
    for (int it = 0; it < nrn_nthread; ++it) {
        auto* const nt = std::next(nrn_threads, it);
        update_sp13_mat_based_on_actual_d(nt);
        update_sp13_rhs_based_on_actual_rhs(nt);
    }
    if ((!nrndae_list.empty()) &&
        (secondorder > 0 || ((cvode_active_ > 0) && (nrn_use_daspk_ == 0)))) {
        hoc_execerror("NrnDAEs only work with secondorder==0 or daspk", 0);
    }
    for (NrnDAE* item: nrndae_list) {
        item->init();
    }
    for (int it = 0; it < nrn_nthread; ++it) {
        auto* const nt = std::next(nrn_threads, it);
        update_actual_d_based_on_sp13_mat(nt);
        update_actual_rhs_based_on_sp13_rhs(nt);
    }
}

void nrndae_rhs(NrnThread* _nt) {
    update_sp13_mat_based_on_actual_d(_nt);
    update_sp13_rhs_based_on_actual_rhs(_nt);
    for (NrnDAE* item: nrndae_list) {
        item->rhs();
    }
    update_actual_d_based_on_sp13_mat(_nt);
    update_actual_rhs_based_on_sp13_rhs(_nt);
}

void nrndae_lhs() {
    for (NrnDAE* item: nrndae_list) {
        item->lhs();
    }
}

void nrndae_dkmap(std::vector<neuron::container::data_handle<double>>& pv,
                  std::vector<neuron::container::data_handle<double>>& pvdot) {
    for (NrnDAE* item: nrndae_list) {
        item->dkmap(pv, pvdot);
    }
}

void nrndae_dkres(double* y, double* yprime, double* delta) {
    // c*y' = f(y) so
    // delta = c*y' - f(y)
    for (NrnDAE* item: nrndae_list) {
        item->dkres(y, yprime, delta);
    }
}

inline void NrnDAE::alloc_(int size, int start, int nnode, Node** nodes, int* elayer) {}

void NrnDAE::alloc(int start_index) {
    // printf("NrnDAE::alloc %lx\n", (long)this);
    size_ = y_.size();
    if (y0_) {
        assert(y0_->size() == size_);
    }
    assert(c_->nrow() == size_ && c_->ncol() == size_);
    cyp_.resize(size_);
    yptmp_.resize(size_);
    start_ = start_index;
    // printf("start=%d size=%d\n", start_, size_);
    delete[] bmap_;
    bmap_ = new int[size_];
    for (int i = 0; i < size_; ++i) {
        if (i < nnode_) {
            bmap_[i] = nodes_[i]->eqn_index_ + elayer_[i];
            if (elayer_[i] > 0 && !nodes_[i]->extnode) {
                // hoc_execerror(secname(nodes_[i]->sec), "NrnDAE: Referring to an extracellular
                // layer but\nextracellular is not inserted.");
                // instead treat as though connected to ground.
                bmap_[i] = 0;
            }
        } else {
            bmap_[i] = start_ + i - nnode_;
        }
    }
    // printf("c_->alloc start=%d, nnode=%d\n", start_, nnode_);
    c_->alloc(start_, nnode_, nodes_, elayer_);

    // allow subclasses to do their own allocations as well
    alloc_(size_, start_, nnode_, nodes_, elayer_);
}

NrnDAE::NrnDAE(Matrix* cmat,
               Vect* const yvec,
               Vect* const y0,
               int nnode,
               Node** const nodes,
               Vect* const elayer,
               void (*f_init)(void* data),
               void* const data)
    : y_(*yvec)
    , yptmp_((Object*) NULL)
    , cyp_((Object*) NULL)
    , f_init_(f_init)
    , data_(data) {
    // printf("NrnDAE %lx\n", (long)this);
    if (cmat) {
        assumed_identity_ = NULL;
    } else {
        const int size = y_.size();
        assumed_identity_ = new OcSparseMatrix(size, size);
        // assumed_identity_->setdiag(0, 1);
        for (int i = 0; i < size; i++)
            (*assumed_identity_)(i, i) = 1;
        cmat = assumed_identity_;
    }
    c_ = new MatrixMap(cmat);
    Vect& elay = *elayer;
    nnode_ = nnode;
    nodes_ = nodes;
    if (nnode_ > 0) {
        elayer_ = new int[nnode_];
        if (elayer) {
            for (int i = 0; i < nnode_; ++i) {
                elayer_[i] = int(elay[i]);
            }
        } else {
            for (int i = 0; i < nnode_; ++i) {
                elayer_[i] = 0;
            }
        }
    } else {
        elayer_ = NULL;
    }
    y0_ = y0;
    bmap_ = new int[1];

    nrndae_register(this);
    //	use_sparse13 = 1;
    nrn_matrix_node_free();
}


NrnDAE::~NrnDAE() {
    nrndae_deregister(this);
    delete[] bmap_;
    delete c_;
    delete assumed_identity_;
    if (elayer_) {
        delete[] elayer_;
    }
    //	if (nrndae_list->count() == 0) {
    //		use_sparse13 = 0;
    //	}
    nrn_matrix_node_free();
}


int NrnDAE::extra_eqn_count() {
    // printf("NrnDAE::extra_eqn_count %lx\n", (long)this);
    // printf("  nnode_=%d g_->nrow()=%d\n", nnode_, g_->nrow());
    return c_->nrow() - nnode_;
}

void NrnDAE::dkmap(std::vector<neuron::container::data_handle<double>>& pv,
                   std::vector<neuron::container::data_handle<double>>& pvdot) {
    // printf("NrnDAE::dkmap\n");
    NrnThread* _nt = nrn_threads;
    for (int i = nnode_; i < size_; ++i) {
        // printf("bmap_[%d] = %d\n", i, bmap_[i]);
        pv[bmap_[i] - 1] = neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                                  y_.data() + i};
        pvdot[bmap_[i] - 1] =
            neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                   _nt->_sp13_rhs + bmap_[i]};
    }
}

void NrnDAE::update() {
    // printf("NrnDAE::update %lx\n", (long)this);
    NrnThread* _nt = nrn_threads;
    // note that the following is correct also for states that refer
    // to the internal potential of a segment. i.e rhs is v + vext[0]
    for (int i = 0; i < size_; ++i) {
        y_[i] += _nt->_sp13_rhs[bmap_[i]];
    }
    // for (int i=0; i < size_; ++i) printf(" i=%d bmap_[i]=%d y_[i]=%g\n", i, bmap_[i],
    // y_->elem(i));
}

void NrnDAE::init() {
    // printf("NrnDAE::init %lx\n", (long)this);
    // printf("init size_=%d %d %d %d\n", size_, y_->size(), y0_->size(), b_->size());
    Vect& y0 = *y0_;

    v2y();
    if (f_init_) {
        f_init_(data_);
    } else {
        if (y0_) {
            for (int i = nnode_; i < size_; ++i) {
                y_[i] = y0[i];
            }
        } else {
            for (int i = nnode_; i < size_; ++i) {
                y_[i] = 0.;
            }
        }
    }
    // for (i=0; i < nnode_; ++i) printf(" i=%d y[i]=%g\n", i, y[i]);
}

void NrnDAE::v2y() {
    // vm,vext may be reinitialized between fixed steps and certainly
    // have been adjusted by daspk

    for (int i = 0; i < nnode_; ++i) {
        Node* nd = nodes_[i];
        // Note elayer_[0] refers to internal.
        if (elayer_[i] == 0) {
            y_[i] = NODEV(nd);
            if (nd->extnode) {
                y_[i] += nd->extnode->v[0];
            }
        } else if (nd->extnode) {
            y_[i] = nd->extnode->v[elayer_[i] - 1];
        }
    }
}


void NrnDAE::dkres(double* y, double* yprime, double* delta) {
    // printf("NrnDAE::dkres %lx\n", (long)this);
    // delta is already f(y)
    // now subtract c*y'
    // The problem is the map between y and yprime and the local
    // representation of the y and yprime

    for (int i = 0; i < size_; ++i) {
        // printf("%d %d %g %g\n", i, bmap_[i]-1, y[bmap_[i]-1], yprime[bmap_[i]-1]);
        yptmp_[i] = yprime[bmap_[i] - 1];
    }
    Vect* cyp;
    if (assumed_identity_) {
        // if c_ is KNOWN to be the identity matrix, then no multiplication to do
        // for now, this happens only when cmat = NULL
        // note that the user might change cmat, so we can't assume that if it's initially
        // the identity that it will stay that way
        cyp = &yptmp_;
    } else {
        c_->mulv(yptmp_, cyp_);  // mulv cannot multiply in place
        cyp = &cyp_;
    }
    for (int i = 0; i < size_; ++i) {
        delta[bmap_[i] - 1] -= (*cyp)[i];
    }
}


void NrnDAE::rhs() {
    NrnThread* _nt = nrn_threads;
    v2y();
    f_(y_, yptmp_, size_);
    for (int i = 0; i < size_; ++i) {
        _nt->_sp13_rhs[bmap_[i]] += yptmp_[i];
    }
}

void NrnDAE::lhs() {
    // printf("NrnDAE::lhs %lx\n", (long)this);
    // printf("  nrn_threads[0].cj = %g\n", nrn_threads[0].cj);
    // left side portion of (c/dt - J)*[dy] =  f(y)
    c_->add(nrn_threads[0].cj);
    v2y();
    jacobian_(y_)->add(jacobian_multiplier_() * -1);
}
