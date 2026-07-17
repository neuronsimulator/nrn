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
    , f_callable_(f_callable) {
    // printf("LinearModelAddition %p\n", this);
    g_ = new MatrixMap(gmat);
}

LinearModelAddition::~LinearModelAddition() {
    // printf("~LinearModelAddition %p\n", this);
    delete g_;
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
    // Replace each capacitive coupling (off-diagonal C) with a voltage-source
    // constraint y_i - y_j = hold, current free. Solve
    //   [ G   B ] [ y  ]   [ b    ]
    //   [ B^T 0 ] [ is ] = [ hold ]
    // where B columns are (+1 at i, -1 at j) for each capacitor pair.
    // Spike scope: pure LinearMechanism algebra (works with nnode_==0 tests).
    if (assumed_identity_) {
        // C is identity: all states differential ODE-like; no floating cap graph.
        return 0;
    }
    const int n = size_;
    if (n <= 0) {
        return -1;
    }

    struct Cap {
        int i;
        int j;
        double hold;
    };
    std::vector<Cap> caps;
    constexpr double ctol = 1e-18;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const double cij = (*c_)(i, j);
            const double cji = (*c_)(j, i);
            if (std::fabs(cij) + std::fabs(cji) > ctol) {
                Cap cap;
                cap.i = i;
                cap.j = j;
                cap.hold = y_[i] - y_[j];
                caps.push_back(cap);
            }
        }
    }
    if (caps.empty()) {
        // No capacitors: pure algebraic G y = b if G invertible.
        Matrix* A = Matrix::instance(n, n);
        Vect rhs(n);
        Vect sol(n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                (*A)(i, j) = (*g_)(i, j);
            }
            rhs[i] = b_[i];
        }
        A->solv(&rhs, &sol, true);
        for (int i = 0; i < n; ++i) {
            y_[i] = sol[i];
        }
        delete A;
        return 0;
    }

    const int nc = static_cast<int>(caps.size());
    const int m = n + nc;
    Matrix* A = Matrix::instance(m, m);
    Vect rhs(m);
    Vect sol(m);
    for (int i = 0; i < m; ++i) {
        rhs[i] = 0.;
        for (int j = 0; j < m; ++j) {
            (*A)(i, j) = 0.;
        }
    }
    // G block and b
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            (*A)(i, j) = (*g_)(i, j);
        }
        rhs[i] = b_[i];
    }
    // Voltage-source stamps
    for (int k = 0; k < nc; ++k) {
        const int i = caps[k].i;
        const int j = caps[k].j;
        const int col = n + k;
        // Current is from i to j: +is on KCL i, -is on KCL j
        (*A)(i, col) = 1.;
        (*A)(j, col) = -1.;
        // Constraint y_i - y_j = hold
        (*A)(col, i) = 1.;
        (*A)(col, j) = -1.;
        rhs[col] = caps[k].hold;
    }

    A->solv(&rhs, &sol, true);
    for (int i = 0; i < n; ++i) {
        y_[i] = sol[i];
    }
    delete A;
    return 0;
}
