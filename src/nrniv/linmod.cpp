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

#include <cstdio>
#include <cmath>
#include <vector>
#include "linmod.h"
#include "nrnpy.h"
#include "ocmatrix.h"
#include "nrn_ansi.h"  // for extern t if needed

LinearModelAddition::LinearModelAddition(Matrix* cmat,
                                         Matrix* gmat,
                                         Vect* yvec,
                                         Vect* y0,
                                         Vect* bvec,
                                         int nnode,
                                         Node** nodes,
                                         Vect* elayer,
                                         Object* f_callable)
    : NrnDAE(cmat, yvec, y0, nnode, nodes, elayer)
    , b_(*bvec)
    , f_callable_(f_callable)
    , dforce_callable_(nullptr)
    , bdot_(nullptr) {
    // printf("LinearModelAddition %p\n", this);
    g_ = new MatrixMap(gmat);
}

LinearModelAddition::~LinearModelAddition() {
    // printf("~LinearModelAddition %p\n", this);
    if (dforce_callable_) {
        hoc_obj_unref(dforce_callable_);
        dforce_callable_ = nullptr;
    }
    delete g_;
}

void LinearModelAddition::set_dforce(Object* dforce_callable, Vect* bdot) {
    if (dforce_callable && !bdot) {
        hoc_execerror("LinearMechanism.dforce: callable requires a bdot Vector", 0);
    }
    if (dforce_callable_) {
        hoc_obj_unref(dforce_callable_);
        dforce_callable_ = nullptr;
    }
    if (dforce_callable) {
        dforce_callable_ = dforce_callable;
        hoc_obj_ref(dforce_callable_);
    }
    bdot_ = bdot;
    // size_ may still be 0 before first matrix alloc; compare to b_
    if (bdot_ && bdot_->size() != b_.size()) {
        hoc_execerror("LinearMechanism.dforce: bdot size must match b", 0);
    }
}

bool LinearModelAddition::fill_bdot_for_ic(double tt, double* out, double fd_h) {
    if (!out || size_ <= 0) {
        return false;
    }
    for (int i = 0; i < size_; ++i) {
        out[i] = 0.;
    }
    // Analytic / user vector path
    if (dforce_callable_) {
        extern double t;
        const double t_sav = t;
        t = tt;
        if (nrn_threads) {
            nrn_threads->_t = tt;
        }
        if (!neuron::python::methods.hoccommand_exec ||
            !neuron::python::methods.hoccommand_exec(dforce_callable_)) {
            t = t_sav;
            if (nrn_threads) {
                nrn_threads->_t = t_sav;
            }
            hoc_execerror("LinearMechanism.dforce callable failed", 0);
        }
        t = t_sav;
        if (nrn_threads) {
            nrn_threads->_t = t_sav;
        }
    }
    if (bdot_) {
        for (int i = 0; i < size_; ++i) {
            out[i] = bdot_->elem(i);
        }
        return true;
    }
    // FD fallback: only when f_callable updates b and no bdot vector was given
    if (!f_callable_ || fd_h <= 0.) {
        return false;
    }
    extern double t;
    const double t_sav = t;
    std::vector<double> b0(size_);
    // Ensure b is current at tt
    t = tt;
    if (nrn_threads) {
        nrn_threads->_t = tt;
    }
    f_(y_, yptmp_, size_);
    for (int i = 0; i < size_; ++i) {
        b0[i] = b_[i];
    }
    t = tt + fd_h;
    if (nrn_threads) {
        nrn_threads->_t = t;
    }
    f_(y_, yptmp_, size_);
    for (int i = 0; i < size_; ++i) {
        out[i] = (b_[i] - b0[i]) / fd_h;
        b_[i] = b0[i];  // restore
    }
    t = t_sav;
    if (nrn_threads) {
        nrn_threads->_t = t_sav;
    }
    // re-sync b via f_ at original t
    f_(y_, yptmp_, size_);
    return true;
}

void LinearModelAddition::alloc_(int size, int start, int nnode, Node** nodes, int* elayer) {
    // printf("LinearModelAddition::alloc_ %p\n", this);
    assert(b_.size() == size);
    assert(g_->nrow() == size && g_->ncol() == size);
    // printf("g_->alloc start=%d, nnode=%d\n", start_, nnode_);
    g_->alloc(start, nnode, nodes, elayer);
}

void LinearModelAddition::f_(Vect& y, Vect& yprime, int size) {
    // printf("LinearModelAddition::f_ %p\n", this);
    // right side portion of (c/dt +g)*[dy] =  -g*y + b
    // given y, returns y'
    // vm,vext may be reinitialized between fixed steps and certainly
    // has been adjusted by daspk
    // size is the number of equations
    if (f_callable_) {
        if (!neuron::python::methods.hoccommand_exec(f_callable_)) {
            hoc_execerror("LinearModelAddition runtime error", 0);
        }
    }
    g_->mulv(y, yprime);
    for (int i = 0; i < size; ++i) {
        yprime[i] = b_[i] - yprime[i];
    }
}

// indicates that the returned Jacobian must be multiplied by -1 to be
// true value
double LinearModelAddition::jacobian_multiplier_() {
    return -1;
}

MatrixMap* LinearModelAddition::jacobian_(Vect& y) {
    return g_;
}

int LinearModelAddition::battery_ic_project() {
    // Consistent IC: hold continuous content, solve algebraics.
    //
    // 1) Floating capacitors (both Cij and Cji nonzero): replace with voltage
    //    sources yi - yj = hold, free battery current (classic C→battery).
    // 2) Diagonal C_ii only (inductor current state): hold yi, drop eqn i
    //    (L→current source of value yi).
    // 3) One-sided C_row,col (op-amp lag tau*vk' in row o): hold y_col (vk),
    //    drop dynamic equation row (not a floating two-terminal cap).
    //
    // Solves augmented [G stamps + constraints] for y.
    if (assumed_identity_) {
        return 0;
    }
    const int n = size_;
    if (n <= 0) {
        return -1;
    }

    constexpr double ctol = 1e-18;

    struct FloatingCap {
        int i;
        int j;
        double hold;
    };
    struct StateHold {
        int var;       // y index held continuous
        int drop_eqn;  // dynamic residual row replaced by hold
        double hold;
    };

    std::vector<FloatingCap> caps;
    std::vector<char> row_used(n, 0);
    std::vector<char> col_in_floating(n, 0);

    // --- floating capacitors: mutual off-diagonal pair ---
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const double cij = (*c_)(i, j);
            const double cji = (*c_)(j, i);
            if (std::fabs(cij) > ctol && std::fabs(cji) > ctol) {
                FloatingCap cap;
                cap.i = i;
                cap.j = j;
                cap.hold = y_[i] - y_[j];
                caps.push_back(cap);
                row_used[i] = 1;
                row_used[j] = 1;
                col_in_floating[i] = 1;
                col_in_floating[j] = 1;
            }
        }
    }

    // --- remaining C rows: inductor (diag) or op-amp lag (one-sided) ---
    std::vector<StateHold> holds;
    for (int r = 0; r < n; ++r) {
        if (row_used[r]) {
            continue;
        }
        int nz_cols[8];
        int nnz = 0;
        for (int j = 0; j < n && nnz < 8; ++j) {
            if (std::fabs((*c_)(r, j)) > ctol) {
                nz_cols[nnz++] = j;
            }
        }
        if (nnz == 0) {
            continue;
        }
        StateHold h;
        h.drop_eqn = r;
        if (nnz == 1 && nz_cols[0] == r) {
            // Diagonal mass: inductor current (or similar) — hold y_r
            h.var = r;
            h.hold = y_[r];
        } else {
            // Prefer off-diagonal column (variable being differentiated), e.g.
            // OpAmp: C[o][k]=tau ⇒ hold output voltage y_k, drop eqn o.
            int jhold = nz_cols[0];
            for (int k = 0; k < nnz; ++k) {
                if (nz_cols[k] != r) {
                    jhold = nz_cols[k];
                    break;
                }
            }
            h.var = jhold;
            h.hold = y_[jhold];
        }
        holds.push_back(h);
        row_used[r] = 1;
    }

    const int nc = static_cast<int>(caps.size());
    const int nh = static_cast<int>(holds.size());
    const int m = n + nc;  // unknowns: y[0..n) and is[0..nc)

    // Equations: (n - nh) KCL/alg rows + nc floating constraints + nh holds = n+nc
    Matrix* A = Matrix::instance(m, m);
    Vect rhs(m);
    Vect sol(m);
    for (int i = 0; i < m; ++i) {
        rhs[i] = 0.;
        for (int j = 0; j < m; ++j) {
            (*A)(i, j) = 0.;
        }
    }

    std::vector<char> drop(n, 0);
    for (const auto& h: holds) {
        drop[h.drop_eqn] = 1;
    }

    int eq = 0;
    // Keep algebraic/KCL rows (not dropped dynamic equations)
    for (int r = 0; r < n; ++r) {
        if (drop[r]) {
            continue;
        }
        for (int j = 0; j < n; ++j) {
            (*A)(eq, j) = (*g_)(r, j);
        }
        // battery currents on floating caps
        for (int k = 0; k < nc; ++k) {
            const int i = caps[k].i;
            const int j = caps[k].j;
            if (r == i) {
                (*A)(eq, n + k) = 1.;
            } else if (r == j) {
                (*A)(eq, n + k) = -1.;
            }
        }
        rhs[eq] = b_[r];
        ++eq;
    }
    // Floating capacitor constraints
    for (int k = 0; k < nc; ++k) {
        (*A)(eq, caps[k].i) = 1.;
        (*A)(eq, caps[k].j) = -1.;
        rhs[eq] = caps[k].hold;
        ++eq;
    }
    // State holds (inductor current, op-amp output voltage, …)
    for (int k = 0; k < nh; ++k) {
        (*A)(eq, holds[k].var) = 1.;
        rhs[eq] = holds[k].hold;
        ++eq;
    }
    if (eq != m) {
        // Structure mismatch — refuse rather than solve a bad system
        delete A;
        return -1;
    }

    A->solv(&rhs, &sol, true);
    for (int i = 0; i < n; ++i) {
        y_[i] = sol[i];
    }
    delete A;
    return 0;
}

bool LinearModelAddition::b_element_is(int i, double* play_target) const {
    if (!play_target || i < 0 || i >= size_) {
        return false;
    }
    // Vect stores contiguous doubles; play into b._ref_x[i] targets &b[i]
    return play_target == &b_[i];
}

void LinearModelAddition::complete_yp_from_bdot(const double* bdot, double* yp_global) {
    // C y' = b - G y is underdetermined when C is singular. Existing seed sets a
    // particular solution; free components live in null(C). Differentiated
    // left-null constraints Z^T G y' = Z^T b' fix those when G has a component
    // on the free mode (e.g. R to ground on a floating C–R series pair).
    if (!bdot || !yp_global || size_ <= 0) {
        return;
    }
    if (assumed_identity_) {
        return;  // C = I, no free y'
    }
    if (!c_ || !g_) {
        return;
    }
    const int n = size_;
    constexpr double ctol = 1e-12;
    constexpr double gtol = 1e-18;

    std::vector<double> yp(n);
    for (int i = 0; i < n; ++i) {
        yp[i] = yp_global[bmap_[i] - 1];
    }

    // Floating capacitor pairs: right null n = e_i + e_j, left null Z = e_i + e_j
    // when C is a pure mutual difference stamp on (i,j).
    std::vector<char> used(n, 0);
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const double cij = (*c_)(i, j);
            const double cji = (*c_)(j, i);
            if (std::fabs(cij) <= ctol || std::fabs(cji) <= ctol) {
                continue;
            }
            // Confirm Z = e_i+e_j is ~left null of C
            bool z_null = true;
            for (int k = 0; k < n; ++k) {
                const double ztc = (*c_)(i, k) + (*c_)(j, k);
                if (std::fabs(ztc) >
                    ctol * (1. + std::fabs((*c_)(i, k)) + std::fabs((*c_)(j, k)))) {
                    z_null = false;
                    break;
                }
            }
            if (!z_null) {
                continue;
            }
            // Z^T G yp and Z^T G n with n = e_i + e_j
            double ztg_yp = 0.;
            for (int k = 0; k < n; ++k) {
                ztg_yp += ((*g_)(i, k) + (*g_)(j, k)) * yp[k];
            }
            const double ztg_n = (*g_)(i, i) + (*g_)(i, j) + (*g_)(j, i) + (*g_)(j, j);
            const double ztbdot = bdot[i] + bdot[j];
            if (std::fabs(ztg_n) < gtol) {
                continue;  // free mode invisible to G — leave yp
            }
            const double alpha = (ztbdot - ztg_yp) / ztg_n;
            yp[i] += alpha;
            yp[j] += alpha;
            used[i] = 1;
            used[j] = 1;
        }
    }

    // Pure algebraic rows (C row ~ 0): enforce (G y')_r = bdot[r] by adjusting
    // free algebraic variables where possible (simple diagonal G_rr).
    for (int r = 0; r < n; ++r) {
        bool crow0 = true;
        for (int k = 0; k < n; ++k) {
            if (std::fabs((*c_)(r, k)) > ctol) {
                crow0 = false;
                break;
            }
        }
        if (!crow0) {
            continue;
        }
        const double grr = (*g_)(r, r);
        if (std::fabs(grr) < gtol) {
            continue;
        }
        // residual of differentiated row: bdot[r] - sum_k G_rk yp[k]
        double gyp = 0.;
        for (int k = 0; k < n; ++k) {
            gyp += (*g_)(r, k) * yp[k];
        }
        // Adjust yp[r] only (local diagonal completion)
        yp[r] += (bdot[r] - gyp) / grr;
    }

    for (int i = 0; i < n; ++i) {
        yp_global[bmap_[i] - 1] = yp[i];
    }
}
