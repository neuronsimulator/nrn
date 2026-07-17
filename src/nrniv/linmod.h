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

  private:
    void f_(Vect& y, Vect& yprime, int size);
    MatrixMap* jacobian_(Vect& y);
    double jacobian_multiplier_();
    void alloc_(int size, int start, int nnode, Node** nodes, int* elayer);

    MatrixMap* g_;
    Vect& b_;
    Object* f_callable_;
};
