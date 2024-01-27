#ifndef linmod_h
#define linmod_h

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

  private:
    void f_(Vect& y, Vect& yprime, int size);
    MatrixMap* jacobian_(Vect& y);
    double jacobian_multiplier_();
    void alloc_(int size, int start, int nnode, Node** nodes, int* elayer);

    MatrixMap* g_;
    Vect& b_;
    Object* f_callable_;
};

#endif
