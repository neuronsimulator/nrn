#ifndef ocmatrix_h
#define ocmatrix_h

#include <memory>
#include <vector>

#include <Eigen/Eigen>
#include <Eigen/Sparse>
#include <Eigen/LU>

struct Object;
class IvocVect;
class OcMatrix;
using Matrix = OcMatrix;
class OcFullMatrix;
using Vect = IvocVect;

class OcMatrix {
  public:
    enum { MFULL = 1, MSPARSE, MBAND };
    static OcMatrix* instance(int nrow, int ncol, int type = MFULL);
    virtual ~OcMatrix() = default;

    virtual double* mep(int i, int j) {
        unimp();
        return nullptr;
    }  // matrix element pointer
    inline double& operator()(int i, int j) {
        return *mep(i, j);
    };

    virtual double getval(int i, int j) {
        unimp();
        return 0.;
    }
    virtual int nrow() {
        unimp();
        return 0;
    }
    virtual int ncol() {
        unimp();
        return 0;
    }
    virtual void resize(int, int) {
        unimp();
    }

    virtual void nonzeros(std::vector<int>& m, std::vector<int>& n);

    OcFullMatrix* full();

    inline void mulv(Vect& in, Vect& out) {
        mulv(&in, &out);
    };
    virtual void mulv(Vect* in, Vect* out) {
        unimp();
    }
    virtual void mulm(Matrix* in, Matrix* out) {
        unimp();
    }
    virtual void muls(double, Matrix* out) {
        unimp();
    }
    virtual void add(Matrix*, Matrix* out) {
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
    virtual void setrow(int, double in) {
        unimp();
    }
    virtual void setcol(int, double in) {
        unimp();
    }
    virtual void setdiag(int, double in) {
        unimp();
    }
    virtual void zero() {
        unimp();
    }
    virtual void ident() {
        unimp();
    }
    virtual void exp(Matrix* out) {
        unimp();
    }
    virtual void pow(int, Matrix* out) {
        unimp();
    }
    virtual void inverse(Matrix* out) {
        unimp();
    }
    virtual void solv(Vect* vin, Vect* vout, bool use_lu) {
        unimp();
    }
    virtual void copy(Matrix* out) {
        unimp();
    }
    virtual void bcopy(Matrix* mout, int i0, int j0, int n0, int m0, int i1, int j1) {
        unimp();
    }
    virtual void transpose(Matrix* out) {
        unimp();
    }
    virtual void symmeigen(Matrix* mout, Vect* vout) {
        unimp();
    }
    virtual void svd1(Matrix* u, Matrix* v, Vect* d) {
        unimp();
    }
    virtual double det(int* e) {
        unimp();
        return 0.0;
    }
    virtual int sprowlen(int) {
        unimp();
        return 0;
    }
    virtual double spgetrowval(int i, int jindx, int* j) {
        unimp();
        return 0.;
    }

    void unimp();

  protected:
    OcMatrix(int type);

  public:
    Object* obj_{};

  private:
    int type_{};
};

extern Matrix* matrix_arg(int);

class OcFullMatrix final: public OcMatrix {  // type 1
  public:
    OcFullMatrix(int, int);
    ~OcFullMatrix() override = default;

    double* mep(int, int) override;
    double getval(int i, int j) override;
    int nrow() override;
    int ncol() override;
    void resize(int, int) override;

    void mulv(Vect* in, Vect* out) override;
    void mulm(Matrix* in, Matrix* out) override;
    void muls(double, Matrix* out) override;
    void add(Matrix*, Matrix* out) override;
    void getrow(int, Vect* out) override;
    void getcol(int, Vect* out) override;
    void getdiag(int, Vect* out) override;
    void setrow(int, Vect* in) override;
    void setcol(int, Vect* in) override;
    void setdiag(int, Vect* in) override;
    void setrow(int, double in) override;
    void setcol(int, double in) override;
    void setdiag(int, double in) override;
    void zero() override;
    void ident() override;
    void exp(Matrix* out) override;
    void pow(int, Matrix* out) override;
    void inverse(Matrix* out) override;
    void solv(Vect* vin, Vect* vout, bool use_lu) override;
    void copy(Matrix* out) override;
    void bcopy(Matrix* mout, int i0, int j0, int n0, int m0, int i1, int j1) override;
    void transpose(Matrix* out) override;
    void symmeigen(Matrix* mout, Vect* vout) override;
    void svd1(Matrix* u, Matrix* v, Vect* d) override;
    double det(int* exponent) override;

  private:
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> m_{};
    std::unique_ptr<Eigen::FullPivLU<decltype(m_)>> lu_{};
};

class OcSparseMatrix final: public OcMatrix {  // type 2
  public:
    OcSparseMatrix(int, int);
    ~OcSparseMatrix() override = default;

    double* mep(int, int) override;
    int nrow() override;
    int ncol() override;
    double getval(int, int) override;
    void ident(void) override;
    void mulv(Vect* in, Vect* out) override;
    void solv(Vect* vin, Vect* vout, bool use_lu) override;

    void setrow(int, Vect* in) override;
    void setcol(int, Vect* in) override;
    void setdiag(int, Vect* in) override;
    void setrow(int, double in) override;
    void setcol(int, double in) override;
    void setdiag(int, double in) override;

    void nonzeros(std::vector<int>& m, std::vector<int>& n) override;

    int sprowlen(int) override;  // how many elements in row
    double spgetrowval(int i, int jindx, int* j) override;

    void zero() override;

  private:
    Eigen::SparseMatrix<double, Eigen::RowMajor> m_{};
    std::unique_ptr<Eigen::SparseLU<decltype(m_)>> lu_{};
};

#endif
