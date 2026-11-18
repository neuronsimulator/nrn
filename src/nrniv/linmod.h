#pragma once

#include "ocmatrix.h"
#include "ivocvect.h"
#include "nrnoc2iv.h"
#include "matrixmap.h"
#include "nrndae.h"


class LinearModelAddition: public NrnDAE {
  public:
    LinearModelAddition(Matrix* c,
                        Matrix* g,
                        Vect* y,
                        Vect* y0,
                        Vect* b,
                        int nnode = 0,
                        Node** nodes = NULL,
                        Vect* elayer = NULL,
                        Object* f_callable = NULL);
    virtual ~LinearModelAddition();

    /**
     * Spike: consistent IC by replacing capacitors with voltage sources that
     * hold branch Δv = y_i - y_j (battery replacement). Solves the augmented
     * algebraic system [G, B; B^T, 0] [y; i_s] = [b; hold] and writes y back.
     * @return 0 on success, nonzero on failure / nothing to do.
     */
    int battery_ic_project();

    /**
     * After y is fixed and a particular C*yp ≈ b-Gy is seeded, adjust yp in
     * null(C) so differentiated algebraic constraints hold: Z^T G yp = Z^T bdot
     * for left-null vectors Z of C (e.g. common mode of a floating capacitor).
     * bdot is db/dt from forcing t+ (same layout as b_). yp_global is the full
     * IDA y' vector (bmap_ applied inside).
     */
    void complete_yp_from_bdot(const double* bdot, double* yp_global);

    /** True if play_target points at b_[i]; used to map Vector.play → b'. */
    bool b_element_is(int i, double* play_target) const;

    int size() const {
        return size_;
    }

    /**
     * A4: optional db/dt for IC free y'.
     * @param dforce_callable  if non-null, executed at IC (e.g. Python) before reading bdot
     * @param bdot  vector same length as b; holds db/dt (not owned)
     * Either or both may be set. Clear with set_dforce(nullptr, nullptr).
     */
    void set_dforce(Object* dforce_callable, Vect* bdot);

    /** Call dforce (if any) and fill out[0..size) with bdot; FD fallback if needed. */
    bool fill_bdot_for_ic(double tt, double* out, double fd_h = 1e-8);

    Object* f_callable() const {
        return f_callable_;
    }
    Object* dforce_callable() const {
        return dforce_callable_;
    }
    Vect* bdot_vec() const {
        return bdot_;
    }

  private:
    void f_(Vect& y, Vect& yprime, int size);
    MatrixMap* jacobian_(Vect& y);
    double jacobian_multiplier_();
    void alloc_(int size, int start, int nnode, Node** nodes, int* elayer);

    MatrixMap* g_;
    Vect& b_;
    Object* f_callable_;
    Object* dforce_callable_;  // A4: optional analytic db/dt provider
    Vect* bdot_;               // A4: user vector for db/dt (not owned)
};
