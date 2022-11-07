#include "nrnconf.h"
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <InterViews/resource.h>
#include "nrnoc2iv.h"
#include "cvodeobj.h"
#include "nonvintblock.h"

#include "membfunc.h"
#include "neuron.h"

void Cvode::rhs(NrnThread* _nt) {
    int i;

    CvodeThreadData& z = CTD(_nt->id);
    if (diam_changed) {
        recalc_diam();
    }
    if (z.v_node_count_ == 0) {
        return;
    }
    for (i = 0; i < z.v_node_count_; ++i) {
        NODERHS(z.v_node_[i]) = 0.;
    }
    if (_nt->_nrn_fast_imem) {
        double* p = _nt->_nrn_fast_imem->_nrn_sav_rhs;
        for (i = 0; i < z.v_node_count_; ++i) {
            Node* nd = z.v_node_[i];
            p[nd->v_node_index] = 0;
        }
    }

    rhs_memb(z.cv_memb_list_, _nt);
    nrn_nonvint_block_current(_nt->end, _nt->_actual_rhs, _nt->id);

    if (_nt->_nrn_fast_imem) {
        double* p = _nt->_nrn_fast_imem->_nrn_sav_rhs;
        for (i = 0; i < z.v_node_count_; ++i) {
            Node* nd = z.v_node_[i];
            p[nd->v_node_index] -= NODERHS(nd);
        }
    }

    /* at this point d contains all the membrane conductances */
    /* now the internal axial currents.
        rhs += ai_j*(vi_j - vi)
    */
    for (i = z.rootnodecount_; i < z.v_node_count_; ++i) {
        Node* nd = z.v_node_[i];
        Node* pnd = z.v_parent_[i];
        double dv = NODEV(pnd) - NODEV(nd);
        /* our connection coefficients are negative so */
        NODERHS(nd) -= NODEB(nd) * dv;
        NODERHS(pnd) += NODEA(nd) * dv;
    }
}

void Cvode::rhs_memb(CvMembList* cmlist, NrnThread* _nt) {
    errno = 0;
    for (CvMembList* cml = cmlist; cml; cml = cml->next) {
        Memb_func* mf = memb_func + cml->index;
        Pvmi s = mf->current;
        if (s) {
            Memb_list* ml = cml->ml;
            (*s)(_nt, ml, cml->index);
            if (errno) {
                if (nrn_errno_check(cml->index)) {
                    hoc_warning("errno set during calculation of currents", (char*) 0);
                }
            }
        }
    }
    activsynapse_rhs();
    activstim_rhs();
    activclamp_rhs();
}

void Cvode::lhs(NrnThread* _nt) {
    int i;

    CvodeThreadData& z = CTD(_nt->id);
    if (z.v_node_count_ == 0) {
        return;
    }
    for (i = 0; i < z.v_node_count_; ++i) {
        NODED(z.v_node_[i]) = 0.;
    }

    lhs_memb(z.cv_memb_list_, _nt);
    nrn_nonvint_block_conductance(_nt->end, _nt->_actual_rhs, _nt->id);
    nrn_cap_jacob(_nt, z.cmlcap_->ml);

    // _nrn_fast_imem not needed since exact icap added in nrn_div_capacity

    /* now add the axial currents */
    for (i = 0; i < z.v_node_count_; ++i) {
        NODED(z.v_node_[i]) -= NODEB(z.v_node_[i]);
    }
    for (i = z.rootnodecount_; i < z.v_node_count_; ++i) {
        NODED(z.v_parent_[i]) -= NODEA(z.v_node_[i]);
    }
}

void Cvode::lhs_memb(CvMembList* cmlist, NrnThread* _nt) {
    CvMembList* cml;
    for (cml = cmlist; cml; cml = cml->next) {
        Memb_func* mf = memb_func + cml->index;
        Memb_list* ml = cml->ml;
        Pvmi s = mf->jacob;
        if (s) {
            Pvmi s = mf->jacob;
            (*s)(_nt, ml, cml->index);
            if (errno) {
                if (nrn_errno_check(cml->index)) {
                    hoc_warning("errno set during calculation of di/dv", (char*) 0);
                }
            }
        }
    }
    activsynapse_lhs();
    activclamp_lhs();
}

/* triangularization of the matrix equations */
void Cvode::triang(NrnThread* _nt) {
    Node *nd, *pnd;
    double p;
    int i;
    CvodeThreadData& z = CTD(_nt->id);

    for (i = z.v_node_count_ - 1; i >= z.rootnodecount_; --i) {
        nd = z.v_node_[i];
        pnd = z.v_parent_[i];
        p = NODEA(nd) / NODED(nd);
        NODED(pnd) -= p * NODEB(nd);
        NODERHS(pnd) -= p * NODERHS(nd);
    }
}

/* back substitution to finish solving the matrix equations */
void Cvode::bksub(NrnThread* _nt) {
    Node *nd, *cnd;
    int i;
    CvodeThreadData& z = CTD(_nt->id);

    for (i = 0; i < z.rootnodecount_; ++i) {
        NODERHS(z.v_node_[i]) /= NODED(z.v_node_[i]);
    }
    for (i = z.rootnodecount_; i < z.v_node_count_; ++i) {
        cnd = z.v_node_[i];
        nd = z.v_parent_[i];
        NODERHS(cnd) -= NODEB(cnd) * NODERHS(nd);
        NODERHS(cnd) /= NODED(cnd);
    }
}
