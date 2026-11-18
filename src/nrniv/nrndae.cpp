#include <../../nrnconf.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "nrndae.h"
#include "nrndae_c.h"
#include "nrnoc2iv.h"
#include "treeset.h"
#include "utils/enumerate.h"
#include "linmod.h"
#include "netcvode.h"
#include "vrecitem.h"
#include "vecplay_tplus.h"

extern NetCvode* net_cvode_instance;

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

void nrndae_dkmap(std::vector<double*>& pv, std::vector<double*>& pvdot) {
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

int nrndae_battery_ic_project() {
    int err = 0;
    for (NrnDAE* item: nrndae_list) {
        auto* lm = dynamic_cast<LinearModelAddition*>(item);
        if (!lm) {
            continue;
        }
        const int e = lm->battery_ic_project();
        if (e != 0) {
            err = e;
        }
    }
    return err;
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
    nnode_ = nnode;
    nodes_ = nodes;
    if (nnode_ > 0) {
        elayer_ = new int[nnode_];
        if (elayer) {
            for (int i = 0; i < nnode_; ++i) {
                elayer_[i] = int((*elayer)[i]);
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

// Switch back from data_handle to double*
void NrnDAE::dkmap(std::vector<double*>& pv, std::vector<double*>& pvdot) {
    // printf("NrnDAE::dkmap\n");
    NrnThread* _nt = nrn_threads;
    for (int i = nnode_; i < size_; ++i) {
        // printf("bmap_[%d] = %d\n", i, bmap_[i]);
        pv[bmap_[i] - 1] = y_.data() + i;
        pvdot[bmap_[i] - 1] = _nt->_sp13_rhs + bmap_[i];
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

    v2y();
    if (f_init_) {
        f_init_(data_);
    } else if (y0_) {
        for (int i = nnode_; i < size_; ++i) {
            y_[i] = (*y0_)[i];
        }
    } else {
        for (int i = nnode_; i < size_; ++i) {
            y_[i] = 0.;
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

void NrnDAE::seed_yp_from_f(double* f, double* yp) {
    // Set yp on mapped equations so C*yp ≈ f for simple mass structure.
    // Residual uses delta -= C*yp with the same map (see dkres).
    constexpr double ctol = 1e-18;
    if (assumed_identity_) {
        for (int i = 0; i < size_; ++i) {
            yp[bmap_[i] - 1] = f[bmap_[i] - 1];
        }
        return;
    }
    if (!c_ || size_ <= 0) {
        return;
    }
    for (int r = 0; r < size_; ++r) {
        int jnz[8];
        double cval[8];
        int nnz = 0;
        for (int j = 0; j < size_ && nnz < 8; ++j) {
            const double cij = (*c_)(r, j);
            if (std::fabs(cij) > ctol) {
                jnz[nnz] = j;
                cval[nnz] = cij;
                ++nnz;
            }
        }
        if (nnz == 0) {
            continue;  // algebraic row: yp free / leave existing
        }
        if (nnz == 1) {
            // Diagonal mass or one-sided lag: c_rj * yp_j = f_r
            yp[bmap_[jnz[0]] - 1] = f[bmap_[r] - 1] / cval[0];
            continue;
        }
        if (nnz == 2 && std::fabs(cval[0] + cval[1]) < ctol * (1. + std::fabs(cval[0]))) {
            // Pure difference stamp C*(yp_a - yp_b) = f_r (floating capacitor row).
            // Gauge: leave the more negative column's yp unchanged (often 0), set the other.
            const int ja = jnz[0];
            const int jb = jnz[1];
            // c_ra * yp_a + c_rb * yp_b = f_r with c_rb ≈ -c_ra
            // => yp_a - yp_b = f_r / c_ra
            const double scale = cval[0];
            const double dyp = f[bmap_[r] - 1] / scale;
            // Keep yp[jb] as is (0 unless set by another row), set yp[ja]
            yp[bmap_[ja] - 1] = yp[bmap_[jb] - 1] + dyp;
        }
        // denser rows: leave yp; residual check / heuristic fallback may apply
    }
}

void nrndae_seed_yp_from_f(double* f, double* yp) {
    for (NrnDAE* item: nrndae_list) {
        item->seed_yp_from_f(f, yp);
    }
}

int nrndae_complete_yp_from_forcing(double* yp, const std::vector<NrnForcingTPlus>& forcing) {
    int flags = 0;
    if (!yp) {
        return 0;
    }
    std::vector<PlayRecord*>* prl = net_cvode_instance ? net_cvode_instance->playrec_list()
                                                       : nullptr;
    extern double t;

    for (NrnDAE* item: nrndae_list) {
        auto* lm = dynamic_cast<LinearModelAddition*>(item);
        if (!lm) {
            continue;
        }
        const int n = lm->size();
        if (n <= 0) {
            continue;
        }
        std::vector<double> bdot(n, 0.);
        bool have_bdot = false;
        int src = 0;

        // A1/A2: continuous Vector.play → components of b
        if (prl && !forcing.empty()) {
            for (const auto& e: forcing) {
                if (e.playrec_index < 0 || e.playrec_index >= (int) prl->size()) {
                    continue;
                }
                PlayRecord* pr = (*prl)[e.playrec_index];
                if (!pr || pr->type() != VecPlayContinuousType) {
                    continue;
                }
                auto* vpc = static_cast<VecPlayContinuous*>(pr);
                double* target = nullptr;
                if (vpc->pd_) {
                    target = static_cast<double*>(vpc->pd_);
                }
                if (!target) {
                    continue;
                }
                for (int i = 0; i < n; ++i) {
                    if (lm->b_element_is(i, target)) {
                        bdot[i] = e.deriv;
                        have_bdot = true;
                        src |= NRN_IC_FORCING_PLAY;
                    }
                }
            }
        }

        // A4: dforce / bdot vector (overrides play); else FD if f_callable and no play
        const bool have_dforce = lm->bdot_vec() || lm->dforce_callable();
        if (have_dforce || (lm->f_callable() && !have_bdot)) {
            std::vector<double> bdot_df(n, 0.);
            if (lm->fill_bdot_for_ic(t, bdot_df.data())) {
                for (int i = 0; i < n; ++i) {
                    bdot[i] = bdot_df[i];
                }
                have_bdot = true;
                if (have_dforce) {
                    src |= NRN_IC_FORCING_DFORCE;
                } else {
                    src |= NRN_IC_FORCING_FD;
                }
            }
        }

        if (have_bdot) {
            lm->complete_yp_from_bdot(bdot.data(), yp);
            flags |= src | NRN_IC_FORCING_APPLIED;
        }
    }
    return flags;
}

void nrndae_append_dforce_to_forcing_list(double tt, std::vector<NrnForcingTPlus>& out) {
    int lm_i = 0;
    for (NrnDAE* item: nrndae_list) {
        auto* lm = dynamic_cast<LinearModelAddition*>(item);
        if (!lm || lm->size() <= 0) {
            ++lm_i;
            continue;
        }
        if (!lm->bdot_vec() && !lm->dforce_callable() && !lm->f_callable()) {
            ++lm_i;
            continue;
        }
        std::vector<double> bdot(lm->size(), 0.);
        if (!lm->fill_bdot_for_ic(tt, bdot.data())) {
            ++lm_i;
            continue;
        }
        for (int i = 0; i < lm->size(); ++i) {
            NrnForcingTPlus e{};
            e.deriv = bdot[i];
            e.playrec_index = -1 - lm_i;
            e.ubound_index = i;
            if (lm->dforce_callable()) {
                std::snprintf(e.label, sizeof e.label, "LM[%d].dforce b'[%d]", lm_i, i);
            } else if (lm->bdot_vec()) {
                std::snprintf(e.label, sizeof e.label, "LM[%d].bdot[%d]", lm_i, i);
            } else {
                std::snprintf(e.label, sizeof e.label, "LM[%d].bdot_fd[%d]", lm_i, i);
            }
            out.push_back(e);
        }
        ++lm_i;
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
