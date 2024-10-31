#pragma once

#include <memory>
#include <utility>
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

    // This function is deprecated and should not be used!
    // mep stands for 'matrix element pointer'
    inline double* mep(int i, int j) {
        return &coeff(i, j);
    }

    inline double operator()(int i, int j) const {
        return getval(i, j);
    };

    virtual double& coeff(int i, int j) {
        static double zero = 0.0;
        unimp();
        return zero;
    }

    inline double& operator()(int i, int j) {
        return coeff(i, j);
    };

    virtual double getval(int i, int j) const {
        unimp();
        return 0.;
    }
    virtual int nrow() const {
        unimp();
        return 0;
    }
    virtual int ncol() const {
        unimp();
        return 0;
    }
    virtual void resize(int, int) {
        unimp();
    }

    virtual std::vector<std::pair<int, int>> nonzeros() const;

    OcFullMatrix* full();

    inline void mulv(Vect& in, Vect& out) const {
        mulv(&in, &out);
    };
    virtual void mulv(Vect* in, Vect* out) const {
        unimp();
    }
    virtual void mulm(Matrix* in, Matrix* out) const {
        unimp();
    }
    virtual void muls(double, Matrix* out) const {
        unimp();
    }
    virtual void add(Matrix*, Matrix* out) const {
        unimp();
    }
    virtual void getrow(int, Vect* out) const {
        unimp();
    }
    virtual void getcol(int, Vect* out) const {
        unimp();
    }
    virtual void getdiag(int, Vect* out) const {
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
    virtual void exp(Matrix* out) const {
        unimp();
    }
    virtual void pow(int, Matrix* out) const {
        unimp();
    }
    virtual void inverse(Matrix* out) const {
        unimp();
    }
    virtual void solv(Vect* vin, Vect* vout, bool use_lu) {
        unimp();
    }
    virtual void copy(Matrix* out) const {
        unimp();
    }
    virtual void bcopy(Matrix* mout, int i0, int j0, int n0, int m0, int i1, int j1) const {
        unimp();
    }
    virtual void transpose(Matrix* out) {
        unimp();
    }
    virtual void symmeigen(Matrix* mout, Vect* vout) const {
        unimp();
    }
    virtual void svd1(Matrix* u, Matrix* v, Vect* d) const {
        unimp();
    }
    virtual double det(int* e) const {
        unimp();
        return 0.0;
    }
    virtual int sprowlen(int) const {
        unimp();
        return 0;
    }
    virtual double spgetrowval(int i, int jindx, int* j) const {
        unimp();
        return 0.;
    }

    void unimp() const;

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

    double& coeff(int, int) override;
    double getval(int i, int j) const override;
    int nrow() const override;
    int ncol() const override;
    void resize(int, int) override;

    void mulv(Vect* in, Vect* out) const override;
    void mulm(Matrix* in, Matrix* out) const override;
    void muls(double, Matrix* out) const override;
    void add(Matrix*, Matrix* out) const override;
    void getrow(int, Vect* out) const override;
    void getcol(int, Vect* out) const override;
    void getdiag(int, Vect* out) const override;
    void setrow(int, Vect* in) override;
    void setcol(int, Vect* in) override;
    void setdiag(int, Vect* in) override;
    void setrow(int, double in) override;
    void setcol(int, double in) override;
    void setdiag(int, double in) override;
    void zero() override;
    void ident() override;
    void exp(Matrix* out) const override;
    void pow(int, Matrix* out) const override;
    void inverse(Matrix* out) const override;
    void solv(Vect* vin, Vect* vout, bool use_lu) override;
    void copy(Matrix* out) const override;
    void bcopy(Matrix* mout, int i0, int j0, int n0, int m0, int i1, int j1) const override;
    void transpose(Matrix* out) override;
    void symmeigen(Matrix* mout, Vect* vout) const override;
    void svd1(Matrix* u, Matrix* v, Vect* d) const override;
    double det(int* exponent) const override;

  private:
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> m_{};
    std::unique_ptr<Eigen::FullPivLU<decltype(m_)>> lu_{};
};

class OcSparseMatrix final: public OcMatrix {  // type 2
  public:
    OcSparseMatrix(int, int);
    ~OcSparseMatrix() override = default;

    double& coeff(int, int) override;
    int nrow() const override;
    int ncol() const override;
    double getval(int, int) const override;
    void ident() override;
    void mulv(Vect* in, Vect* out) const override;
    void solv(Vect* vin, Vect* vout, bool use_lu) override;

    void setrow(int, Vect* in) override;
    void setcol(int, Vect* in) override;
    void setdiag(int, Vect* in) override;
    void setrow(int, double in) override;
    void setcol(int, double in) override;
    void setdiag(int, double in) override;

    std::vector<std::pair<int, int>> nonzeros() const override;

    int sprowlen(int) const override;  // how many elements in row
    double spgetrowval(int i, int jindx, int* j) const override;

    void zero() override;

  private:
    Eigen::SparseMatrix<double, Eigen::RowMajor> m_{};
    std::unique_ptr<Eigen::SparseLU<decltype(m_)>> lu_{};
};
