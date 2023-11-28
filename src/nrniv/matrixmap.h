#ifndef matrixmap_h
#define matrixmap_h

#include "ocmatrix.h"
#include "nrnoc2iv.h"

class MatrixMap {
  public:
    MatrixMap(Matrix*);
    MatrixMap(Matrix&);
    ~MatrixMap();

    void alloc(int, int, Node**, int*);
    void mmfree();
    void add(double fac);

    // passthroughs for accessing m_
    inline void mulv(Vect& in, Vect& out) {
        m_.mulv(in, out);
    };
    inline void mulv(Vect* in, Vect* out) {
        m_.mulv(in, out);
    };
    inline void mulm(Matrix* in, Matrix* out) {
        m_.mulm(in, out);
    };
    inline void muls(double a, Matrix* out) {
        m_.muls(a, out);
    };
    inline void add(Matrix* a, Matrix* out) {
        m_.add(a, out);
    };
    inline void getrow(int r, Vect* out) {
        m_.getrow(r, out);
    };
    inline void getcol(int c, Vect* out) {
        m_.getcol(c, out);
    };
    inline double& operator()(int i, int j) {
        return m_(i, j);
    };
    inline int nrow() {
        return m_.nrow();
    };
    inline int ncol() {
        return m_.ncol();
    };
    inline double* mep(int i, int j) {
        return m_.mep(i, j);
    };
    inline void getdiag(int d, Vect* out) {
        m_.getdiag(d, out);
    }
    inline void setrow(int r, Vect* in) {
        m_.setrow(r, in);
    }
    inline void setcol(int c, Vect* in) {
        m_.setcol(c, in);
    }
    inline void setdiag(int d, Vect* in) {
        m_.setdiag(d, in);
    }
    inline void setrow(int r, double in) {
        m_.setrow(r, in);
    }
    inline void setcol(int c, double in) {
        m_.setcol(c, in);
    }
    inline void setdiag(int d, double in) {
        m_.setdiag(d, in);
    }
    inline void zero() {
        m_.zero();
    }
    inline void ident() {
        m_.ident();
    }
    inline void exp(Matrix* out) {
        m_.exp(out);
    }
    inline void pow(int p, Matrix* out) {
        m_.pow(p, out);
    }
    inline void inverse(Matrix* out) {
        m_.inverse(out);
    }
    inline void solv(Vect* vin, Vect* vout, bool use_lu) {
        m_.solv(vin, vout, use_lu);
    }
    inline void copy(Matrix* out) {
        m_.copy(out);
    }
    inline void bcopy(Matrix* mout, int i0, int j0, int n0, int m0, int i1, int j1) {
        m_.bcopy(mout, i0, j0, n0, m0, i1, j1);
    }
    inline void transpose(Matrix* out) {
        m_.transpose(out);
    }
    inline void symmeigen(Matrix* mout, Vect* vout) {
        m_.symmeigen(mout, vout);
    }
    inline void svd1(Matrix* u, Matrix* v, Vect* d) {
        m_.svd1(u, v, d);
    }
    inline double det(int* e) {
        return m_.det(e);
    }
    inline int sprowlen(int a) {
        return m_.sprowlen(a);
    }
    inline double spgetrowval(int i, int jindx, int* j) {
        return m_.spgetrowval(i, jindx, j);
    }


  private:
    Matrix& m_;

    // the map
    int plen_;
    double** pm_;
    double** ptree_;
};

#endif
