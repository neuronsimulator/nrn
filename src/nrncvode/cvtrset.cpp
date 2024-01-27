#include "nrnconf.h"
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "nrnoc2iv.h"
#include "cvodeobj.h"
#include "nonvintblock.h"

#include "membfunc.h"
#include "neuron.h"

void Cvode::rhs(neuron::model_sorted_token const& sorted_token, NrnThread* _nt) {
    CvodeThreadData& z = CTD(_nt->id);
    if (diam_changed) {
        recalc_diam();
    }
    if (z.v_node_count_ == 0) {
        return;
    }
    for (int i = 0; i < z.v_node_count_; ++i) {
        NODERHS(z.v_node_[i]) = 0.;
    }
    auto const vec_sav_rhs = _nt->node_sav_rhs_storage();
    if (vec_sav_rhs) {
        for (int i = 0; i < z.v_node_count_; ++i) {
            Node* nd = z.v_node_[i];
            vec_sav_rhs[nd->v_node_index] = 0;
        }
    }

    rhs_memb(sorted_token, z.cv_memb_list_, _nt);
    auto const vec_rhs = _nt->node_rhs_storage();
    nrn_nonvint_block_current(_nt->end, vec_rhs, _nt->id);

    if (vec_sav_rhs) {
        for (int i = 0; i < z.v_node_count_; ++i) {
            auto const node_index = z.v_node_[i]->v_node_index;
            vec_sav_rhs[node_index] -= vec_rhs[node_index];
        }
    }

    /* at this point d contains all the membrane conductances */
    /* now the internal axial currents.
        rhs += ai_j*(vi_j - vi)
    */
    auto const vec_a = _nt->node_a_storage();
    auto const vec_b = _nt->node_b_storage();
    auto const vec_v = _nt->node_voltage_storage();
    for (auto i = z.rootnodecount_; i < z.v_node_count_; ++i) {
        auto const node_i = z.v_node_[i]->v_node_index;
        auto const parent_i = z.v_parent_[i]->v_node_index;
        auto const dv = vec_v[parent_i] - vec_v[node_i];
        // our connection coefficients are negative
        vec_rhs[node_i] -= vec_b[node_i] * dv;
        vec_rhs[parent_i] += vec_a[node_i] * dv;
    }
}

void Cvode::rhs_memb(neuron::model_sorted_token const& sorted_token,
                     CvMembList* cmlist,
                     NrnThread* _nt) {
    errno = 0;
    for (CvMembList* cml = cmlist; cml; cml = cml->next) {
        const Memb_func& mf = memb_func[cml->index];
        if (auto const current = mf.current; current) {
            for (auto& ml: cml->ml) {
                current(sorted_token, _nt, &ml, cml->index);
                if (errno && nrn_errno_check(cml->index)) {
                    hoc_warning("errno set during calculation of currents", nullptr);
                }
            }
        }
    }
    activsynapse_rhs();
    activstim_rhs();
    activclamp_rhs();
}

void Cvode::lhs(neuron::model_sorted_token const& sorted_token, NrnThread* _nt) {
    int i;

    CvodeThreadData& z = CTD(_nt->id);
    if (z.v_node_count_ == 0) {
        return;
    }
    for (i = 0; i < z.v_node_count_; ++i) {
        NODED(z.v_node_[i]) = 0.;
    }

    lhs_memb(sorted_token, z.cv_memb_list_, _nt);
    nrn_nonvint_block_conductance(_nt->end, _nt->node_rhs_storage(), _nt->id);
    for (auto& ml: z.cmlcap_->ml) {
        nrn_cap_jacob(sorted_token, _nt, &ml);
    }

    // fast_imem not needed since exact icap added in nrn_div_capacity

    /* now add the axial currents */
    for (i = 0; i < z.v_node_count_; ++i) {
        NODED(z.v_node_[i]) -= NODEB(z.v_node_[i]);
    }
    for (i = z.rootnodecount_; i < z.v_node_count_; ++i) {
        NODED(z.v_parent_[i]) -= NODEA(z.v_node_[i]);
    }
}

void Cvode::lhs_memb(neuron::model_sorted_token const& sorted_token,
                     CvMembList* cmlist,
                     NrnThread* _nt) {
    CvMembList* cml;
    for (cml = cmlist; cml; cml = cml->next) {
        const Memb_func& mf = memb_func[cml->index];
        if (auto const jacob = mf.jacob; jacob) {
            for (auto& ml: cml->ml) {
                jacob(sorted_token, _nt, &ml, cml->index);
                if (errno && nrn_errno_check(cml->index)) {
                    hoc_warning("errno set during calculation of di/dv", nullptr);
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
