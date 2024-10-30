#include <../../nrnconf.h>
#include <vector>
#include <cmath>

#define v_elem(v, i) (*(vector_vec(v) + i))

#include <Eigen/Eigen>
#include <unsupported/Eigen/MatrixFunctions>

#include "ivocvect.h"
#include "oc_ansi.h"

#include "ocmatrix.h"

int nrn_matrix_dim(void* vm, int d) {
    OcMatrix* m = (OcMatrix*) vm;
    return d ? m->ncol() : m->nrow();
}

static Eigen::Map<Eigen::VectorXd> Vect2VEC(Vect* v1) {
    return Eigen::Map<Eigen::VectorXd>(v1->data(), v1->size());
}

OcMatrix::OcMatrix(int type)
    : type_(type) {}

OcMatrix* OcMatrix::instance(int nrow, int ncol, int type) {
    switch (type) {
    default:
    case MFULL:
        return new OcFullMatrix(nrow, ncol);
    case MSPARSE:
        return new OcSparseMatrix(nrow, ncol);
    }
}

void OcMatrix::unimp() {
    hoc_execerror("Matrix method not implemented for this type matrix", 0);
}

void OcMatrix::nonzeros(std::vector<int>& m, std::vector<int>& n) {
    m.clear();
    n.clear();
    for (int i = 0; i < nrow(); i++) {
        for (int j = 0; j < ncol(); j++) {
            if (getval(i, j) != 0) {
                m.push_back(i);
                n.push_back(j);
            }
        }
    }
}

OcFullMatrix* OcMatrix::full() {
    if (type_ != MFULL) {  // could clone one maybe
        hoc_execerror("Matrix is not a FULL matrix (type 1)", 0);
    }
    return (OcFullMatrix*) this;
}

OcFullMatrix::OcFullMatrix(int nrow, int ncol)
    : OcMatrix(MFULL)
    , m_(nrow, ncol) {
    // The constructor of Eigen::Matrix does not initialize values
    m_.setZero();
}

double* OcFullMatrix::mep(int i, int j) {
    return &m_(i, j);
}
double OcFullMatrix::getval(int i, int j) {
    return m_(i, j);
}
int OcFullMatrix::nrow() {
    return m_.rows();
}
int OcFullMatrix::ncol() {
    return m_.cols();
}

void OcFullMatrix::resize(int i, int j) {
    // This is here because we want that new values are initialized to 0
    auto v = Eigen::MatrixXd::Zero(i, j);
    m_.conservativeResizeLike(v);
}

void OcFullMatrix::mulv(Vect* vin, Vect* vout) {
    auto v1 = Vect2VEC(vin);
    auto v2 = Vect2VEC(vout);
    v2 = m_ * v1;
}

void OcFullMatrix::mulm(Matrix* in, Matrix* out) {
    out->full()->m_ = m_ * in->full()->m_;
}

void OcFullMatrix::muls(double s, Matrix* out) {
    out->full()->m_ = s * m_;
}

void OcFullMatrix::add(Matrix* in, Matrix* out) {
    out->full()->m_ = m_ + in->full()->m_;
}

void OcFullMatrix::copy(Matrix* out) {
    out->full()->m_ = m_;
}

void OcFullMatrix::bcopy(Matrix* out, int i0, int j0, int n0, int m0, int i1, int j1) {
    out->full()->m_.block(i1, j1, n0, m0) = m_.block(i0, j0, n0, m0);
}

void OcFullMatrix::transpose(Matrix* out) {
    if (out->full()->m_ == m_) {
        m_.transposeInPlace();
    } else {
        out->full()->m_ = m_.transpose();
    }
}

// As only symmetric matrix are accepted, eigenvalues are not complex
void OcFullMatrix::symmeigen(Matrix* mout, Vect* vout) {
    auto v1 = Vect2VEC(vout);
    Eigen::EigenSolver<Eigen::MatrixXd> es(m_);
    v1 = es.eigenvalues().real();
    mout->full()->m_ = es.eigenvectors().real();
}

void OcFullMatrix::svd1(Matrix* u, Matrix* v, Vect* d) {
    auto v1 = Vect2VEC(d);
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(m_, Eigen::ComputeFullU | Eigen::ComputeFullV);
    v1 = svd.singularValues();
    if (u) {
        u->full()->m_ = svd.matrixU().transpose();
    }
    if (v) {
        v->full()->m_ = svd.matrixV().transpose();
    }
}

void OcFullMatrix::getrow(int k, Vect* out) {
    auto v1 = Vect2VEC(out);
    v1 = m_.row(k);
}

void OcFullMatrix::getcol(int k, Vect* out) {
    auto v1 = Vect2VEC(out);
    v1 = m_.col(k);
}

void OcFullMatrix::getdiag(int k, Vect* out) {
    auto vout = m_.diagonal(k);
    if (k >= 0) {
        for (int i = 0, j = k; i < nrow() && j < ncol(); ++i, ++j) {
            out->elem(i) = vout(i);
        }
    } else {
        for (int i = -k, j = 0; i < nrow() && j < ncol(); ++i, ++j) {
            out->elem(i) = vout(j);
        }
    }
}

void OcFullMatrix::setrow(int k, Vect* in) {
    auto v1 = Vect2VEC(in);
    m_.block(k, 0, 1, v1.size()) = v1.transpose();
}

void OcFullMatrix::setcol(int k, Vect* in) {
    auto v1 = Vect2VEC(in);
    m_.block(0, k, v1.size(), 1) = v1;
}

void OcFullMatrix::setdiag(int k, Vect* in) {
    auto out = m_.diagonal(k);
    if (k >= 0) {
        for (int i = 0, j = k; i < nrow() && j < ncol() && i < in->size(); ++i, ++j) {
            out(i) = in->elem(i);
        }
    } else {
        for (int i = -k, j = 0; i < nrow() && j < ncol() && i < in->size(); ++i, ++j) {
            out(j) = in->elem(i);
        }
    }
    m_.diagonal(k) = out;
}

void OcFullMatrix::setrow(int k, double in) {
    m_.row(k).fill(in);
}

void OcFullMatrix::setcol(int k, double in) {
    m_.col(k).fill(in);
}

void OcFullMatrix::setdiag(int k, double in) {
    m_.diagonal(k).fill(in);
}

void OcFullMatrix::zero() {
    m_.setZero();
}

void OcFullMatrix::ident() {
    m_.setIdentity();
}

void OcFullMatrix::exp(Matrix* out) {
    out->full()->m_ = m_.exp();
}

void OcFullMatrix::pow(int i, Matrix* out) {
    out->full()->m_ = m_.pow(i).eval();
}

void OcFullMatrix::inverse(Matrix* out) {
    out->full()->m_ = m_.inverse();
}

void OcFullMatrix::solv(Vect* in, Vect* out, bool use_lu) {
    if (!lu_ || !use_lu || lu_->rows() != m_.rows()) {
        lu_ = std::make_unique<Eigen::FullPivLU<decltype(m_)>>(m_);
    }
    auto v1 = Vect2VEC(in);
    auto v2 = Vect2VEC(out);
    v2 = lu_->solve(v1);
}

double OcFullMatrix::det(int* e) {
    *e = 0;
    double m = m_.determinant();
    if (m) {
        while (std::abs(m) >= 10.0) {
            m *= 0.1;
            *e += 1;
        }
        while (std::abs(m) < 1.0) {
            m *= 10.0;
            *e -= 1;
        }
    }
    return m;
}

//--------------------------

OcSparseMatrix::OcSparseMatrix(int nrow, int ncol)
    : OcMatrix(MSPARSE)
    , m_(nrow, ncol) {}

double* OcSparseMatrix::mep(int i, int j) {
    return &m_.coeffRef(i, j);
}

void OcSparseMatrix::zero() {
    for (int k = 0; k < m_.outerSize(); ++k) {
        for (decltype(m_)::InnerIterator it(m_, k); it; ++it) {
            it.valueRef() = 0.;
        }
    }
}

double OcSparseMatrix::getval(int i, int j) {
    return m_.coeff(i, j);
}

int OcSparseMatrix::nrow() {
    return m_.rows();
}

int OcSparseMatrix::ncol() {
    return m_.cols();
}

void OcSparseMatrix::mulv(Vect* vin, Vect* vout) {
    auto v1 = Vect2VEC(vin);
    auto v2 = Vect2VEC(vout);
    v2 = m_ * v1;
}

void OcSparseMatrix::solv(Vect* in, Vect* out, bool use_lu) {
    if (!lu_ || !use_lu || lu_->rows() != m_.rows()) {
        m_.makeCompressed();
        lu_ = std::make_unique<Eigen::SparseLU<decltype(m_)>>(m_);
    }
    auto v1 = Vect2VEC(in);
    auto v2 = Vect2VEC(out);
    v2 = lu_->solve(v1);
}

void OcSparseMatrix::setrow(int k, Vect* in) {
    int col = m_.cols();
    for (int i = 0; i < col; ++i) {
        m_.coeffRef(k, i) = in->elem(i);
    }
}

void OcSparseMatrix::setcol(int k, Vect* in) {
    int row = m_.rows();
    for (int i = 0; i < row; ++i) {
        m_.coeffRef(i, k) = in->elem(i);
    }
}

void OcSparseMatrix::setdiag(int k, Vect* in) {
    int row = m_.rows();
    int col = m_.cols();
    if (k >= 0) {
        for (int i = 0, j = k; i < row && j < col; ++i, ++j) {
            m_.coeffRef(i, j) = in->elem(i);
        }
    } else {
        for (int i = -k, j = 0; i < row && j < col; ++i, ++j) {
            m_.coeffRef(i, j) = in->elem(i);
        }
    }
}

void OcSparseMatrix::setrow(int k, double in) {
    int col = m_.cols();
    for (int i = 0; i < col; ++i) {
        m_.coeffRef(k, i) = in;
    }
}

void OcSparseMatrix::setcol(int k, double in) {
    int row = m_.rows();
    for (int i = 0; i < row; ++i) {
        m_.coeffRef(i, k) = in;
    }
}

void OcSparseMatrix::ident(void) {
    m_.setIdentity();
}

void OcSparseMatrix::setdiag(int k, double in) {
    int row = m_.rows();
    int col = m_.cols();
    if (k >= 0) {
        for (int i = 0, j = k; i < row && j < col; ++i, ++j) {
            m_.coeffRef(i, j) = in;
        }
    } else {
        for (int i = -k, j = 0; i < row && j < col; ++i, ++j) {
            m_.coeffRef(i, j) = in;
        }
    }
}

int OcSparseMatrix::sprowlen(int i) {
    int acc = 0;
    for (decltype(m_)::InnerIterator it(m_, i); it; ++it) {
        acc += 1;
    }
    return acc;
}

double OcSparseMatrix::spgetrowval(int i, int jindx, int* j) {
    int acc = 0;
    for (decltype(m_)::InnerIterator it(m_, i); it; ++it) {
        if (acc == jindx) {
            *j = it.col();
            return it.value();
        }
        acc += 1;
    }
    return 0;
}

void OcSparseMatrix::nonzeros(std::vector<int>& m, std::vector<int>& n) {
    m.clear();
    n.clear();
    for (int k = 0; k < m_.outerSize(); ++k) {
        for (decltype(m_)::InnerIterator it(m_, k); it; ++it) {
            m.push_back(it.row());
            n.push_back(it.col());
        }
    }
}
