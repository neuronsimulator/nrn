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
    if (z.rootnode_end_index_ == 0) {
        return;
    }
    auto* const vec_rhs = _nt->node_rhs_storage();
    for (int i = z.rootnode_begin_index_; i < z.rootnode_end_index_; ++i) {
        vec_rhs[i] = 0.;
    }
    for (int i = z.vnode_begin_index_; i < z.vnode_end_index_; ++i) {
        vec_rhs[i] = 0.;
    }
    auto const vec_sav_rhs = _nt->node_sav_rhs_storage();
    if (vec_sav_rhs) {
        for (int i = z.rootnode_begin_index_; i < z.rootnode_end_index_; ++i) {
            vec_sav_rhs[i] = 0.;
        }
        for (int i = z.vnode_begin_index_; i < z.vnode_end_index_; ++i) {
            vec_sav_rhs[i] = 0.;
        }
    }

    rhs_memb(sorted_token, z.cv_memb_list_, _nt);
    nrn_nonvint_block_current(_nt->end, vec_rhs, _nt->id);

    if (vec_sav_rhs) {
        for (int i = z.rootnode_begin_index_; i < z.rootnode_end_index_; ++i) {
            vec_sav_rhs[i] -= vec_rhs[i];
        }
        for (int i = z.vnode_begin_index_; i < z.vnode_end_index_; ++i) {
            vec_sav_rhs[i] -= vec_rhs[i];
        }
    }

    /* at this point d contains all the membrane conductances */
    /* now the internal axial currents.
        rhs += ai_j*(vi_j - vi)
    */
    auto const vec_a = _nt->node_a_storage();
    auto const vec_b = _nt->node_b_storage();
    auto const vec_v = _nt->node_voltage_storage();
    auto* const parent_i = _nt->_v_parent_index;
    for (int i = z.vnode_begin_index_; i < z.vnode_end_index_; ++i) {
        auto const pi = parent_i[i];
        auto const dv = vec_v[pi] - vec_v[i];
        // our connection coefficients are negative so
        vec_rhs[i] -= vec_b[i] * dv;
        vec_rhs[pi] += vec_a[i] * dv;
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
    if (z.rootnode_end_index_ == 0) {
        return;
    }
    auto* const vec_d = _nt->node_d_storage();
    for (int i = z.rootnode_begin_index_; i < z.rootnode_end_index_; ++i) {
        vec_d[i] = 0.;
    }
    for (int i = z.vnode_begin_index_; i < z.vnode_end_index_; ++i) {
        vec_d[i] = 0.;
    }

    lhs_memb(sorted_token, z.cv_memb_list_, _nt);
    nrn_nonvint_block_conductance(_nt->end, _nt->node_rhs_storage(), _nt->id);
    for (auto& ml: z.cmlcap_->ml) {
        nrn_cap_jacob(sorted_token, _nt, &ml);
    }

    // fast_imem not needed since exact icap added in nrn_div_capacity

    /* now add the axial currents */
    auto* const vec_a = _nt->node_a_storage();
    auto* const vec_b = _nt->node_b_storage();
    auto* const parent_i = _nt->_v_parent_index;
    for (int i = z.vnode_begin_index_; i < z.vnode_end_index_; ++i) {
        vec_d[i] -= vec_b[i];
        vec_d[parent_i[i]] -= vec_a[i];
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
    CvodeThreadData& z = CTD(_nt->id);

    auto* const vec_a = _nt->node_a_storage();
    auto* const vec_b = _nt->node_b_storage();
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    for (int i = z.vnode_end_index_ - 1; i >= z.vnode_begin_index_; --i) {
        double const p = vec_a[i] / vec_d[i];
        auto const pi = _nt->_v_parent_index[i];
        vec_d[pi] -= p * vec_b[i];
        vec_rhs[pi] -= p * vec_rhs[i];
    }
}

/* back substitution to finish solving the matrix equations */
void Cvode::bksub(NrnThread* _nt) {
    CvodeThreadData& z = CTD(_nt->id);

    auto* const vec_b = _nt->node_b_storage();
    auto* const vec_d = _nt->node_d_storage();
    auto* const vec_rhs = _nt->node_rhs_storage();
    auto* const parent_i = _nt->_v_parent_index;
    for (int i = z.rootnode_begin_index_; i < z.rootnode_end_index_; ++i) {
        vec_rhs[i] /= vec_d[i];
    }
    for (int i = z.vnode_begin_index_; i < z.vnode_end_index_; ++i) {
        vec_rhs[i] -= vec_b[i] * vec_rhs[parent_i[i]];
        vec_rhs[i] /= vec_d[i];
    }
}
