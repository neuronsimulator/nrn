#ifndef ocmatrix_h
#define ocmatrix_h

#ifndef MATRIXH
#define MAT   void
#define SPMAT void
#define PERM  void
#endif

#include <vector>
using std::vector;

class IvocVect;
template <typename T>
class NrnFullMatrix;
#define Vect   IvocVect
#define Matrix OcMatrix

template <typename T>
class NrnMatrix {
  public:
    enum { MFULL = 1, MSPARSE, MBAND };
    static NrnMatrix* instance(int nrow, int ncol, int type = MFULL);
    virtual ~NrnMatrix() = default;

    virtual T* mep(int i, int j) {
        unimp();
        return nullptr;
    }  // matrix element pointer
    inline T& operator()(int i, int j) {
        return *mep(i, j);
    };

    virtual T getval(int i, int j) {
        unimp();
        return {};
    }
    virtual int nrow() {
        unimp();
        return {};
    }
    virtual int ncol() {
        unimp();
        return {};
    }
    virtual void resize(int, int) {
        unimp();
    }

    virtual void nonzeros(vector<int>& m, vector<int>& n);

    NrnFullMatrix<T>* full();

    inline void mulv(Vect& in, Vect& out) {
        mulv(&in, &out);
    };
    virtual void mulv(Vect* in, Vect* out) {
        unimp();
    }
    virtual void mulm(NrnMatrix<T>* in, NrnMatrix<T>* out) {
        unimp();
    }
    virtual void muls(T, NrnMatrix<T>* out) {
        unimp();
    }
    virtual void add(NrnMatrix<T>*, NrnMatrix<T>* out) {
        unimp();
    }
    virtual void getrow(int, Vect* out) {
        unimp();
    }
    virtual void getcol(int, Vect* out) {
        unimp();
    }
    virtual void getdiag(int, Vect* out) {
        unimp();
    }
    virtual void setrow(int, Vect* in) {
        unimp();
    }
    virtual void setcol(int, Vect* in) {
        unimp();
    }
    virtual void setdiag(int, Vect* in) {
        unimp();
    }
    virtual void setrow(int, T in) {
        unimp();
    }
    virtual void setcol(int, T in) {
        unimp();
    }
    virtual void setdiag(int, T in) {
        unimp();
    }
    virtual void zero() {
        unimp();
    }
    virtual void ident() {
        unimp();
    }
    virtual void exp(NrnMatrix<T>* out) {
        unimp();
    }
    virtual void pow(int, NrnMatrix<T>* out) {
        unimp();
    }
    virtual void inverse(NrnMatrix<T>* out) {
        unimp();
    }
    virtual void solv(Vect* vin, Vect* vout, bool use_lu) {
        unimp();
    }
    virtual void copy(NrnMatrix<T>* out) {
        unimp();
    }
    virtual void bcopy(NrnMatrix<T>* mout, int i0, int j0, int n0, int m0, int i1, int j1) {
        unimp();
    }
    virtual void transpose(NrnMatrix<T>* out) {
        unimp();
    }
    virtual void symmeigen(NrnMatrix<T>* mout, Vect* vout) {
        unimp();
    }
    virtual void svd1(NrnMatrix<T>* u, NrnMatrix<T>* v, Vect* d) {
        unimp();
    }
    virtual T det(int* e) {
        unimp();
        return {};
    }
    virtual int sprowlen(int) {
        unimp();
        return {};
    }
    virtual T spgetrowval(int i, int jindx, int* j) {
        unimp();
        return {};
    }

    void unimp();

  protected:
    NrnMatrix(int type);

  public:
    void* obj_;

  private:
    int type_;
};
using OcMatrix = NrnMatrix<double>;

extern Matrix* matrix_arg(int);

template <typename T>
class NrnFullMatrix: public NrnMatrix<T> {  // type 1
  public:
    NrnFullMatrix(int, int);
    virtual ~NrnFullMatrix();

    virtual T* mep(int, int);
    virtual T getval(int i, int j);
    virtual int nrow();
    virtual int ncol();
    virtual void resize(int, int);

    virtual void mulv(Vect* in, Vect* out);
    virtual void mulm(Matrix* in, Matrix* out);
    virtual void muls(T, Matrix* out);
    virtual void add(Matrix*, Matrix* out);
    virtual void getrow(int, Vect* out);
    virtual void getcol(int, Vect* out);
    virtual void getdiag(int, Vect* out);
    virtual void setrow(int, Vect* in);
    virtual void setcol(int, Vect* in);
    virtual void setdiag(int, Vect* in);
    virtual void setrow(int, T in);
    virtual void setcol(int, T in);
    virtual void setdiag(int, T in);
    virtual void zero();
    virtual void ident();
    virtual void exp(Matrix* out);
    virtual void pow(int, Matrix* out);
    virtual void inverse(Matrix* out);
    virtual void solv(Vect* vin, Vect* vout, bool use_lu);
    virtual void copy(Matrix* out);
    virtual void bcopy(Matrix* mout, int i0, int j0, int n0, int m0, int i1, int j1);
    virtual void transpose(Matrix* out);
    virtual void symmeigen(Matrix* mout, Vect* vout);
    virtual void svd1(Matrix* u, Matrix* v, Vect* d);
    virtual T det(int* exponent);

  private:
    MAT* m_;
    MAT* lu_factor_;
    PERM* lu_pivot_;
};
using OcFullMatrix = NrnFullMatrix<double>;

class OcSparseMatrix: public NrnMatrix<double> {  // type 2
  public:
    OcSparseMatrix(int, int);
    virtual ~OcSparseMatrix();

    virtual double* mep(int, int);
    virtual double* pelm(int, int);  // nullptr if element does not exist
    virtual int nrow();
    virtual int ncol();
    virtual double getval(int, int);
    virtual void ident(void);
    virtual void mulv(Vect* in, Vect* out);
    virtual void solv(Vect* vin, Vect* vout, bool use_lu);

    virtual void setrow(int, Vect* in);
    virtual void setcol(int, Vect* in);
    virtual void setdiag(int, Vect* in);
    virtual void setrow(int, double in);
    virtual void setcol(int, double in);
    virtual void setdiag(int, double in);

    virtual void nonzeros(vector<int>& m, vector<int>& n);

    virtual int sprowlen(int);  // how many elements in row
    virtual double spgetrowval(int i, int jindx, int* j);

    virtual void zero();

  private:
    SPMAT* m_;
    SPMAT* lu_factor_;
    PERM* lu_pivot_;
};

#ifndef MATRIXH
#undef MAT
#undef SPMAT
#endif

#endif
