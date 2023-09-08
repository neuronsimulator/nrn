#include <../../nrnconf.h>
#include <vector>
#include <cmath>

#define v_elem(v, i) (*(vector_vec(v) + i))

#include "ivocvect.h"
#include "oc2iv.h"

#undef error

extern "C" {
#undef OUT           /* /usr/x86_64-w64-mingw32/sys-root/mingw/include/windef.h */
#include "matrix.h"  //meschach
#include "matrix2.h"
#include "sparse.h"
#include "sparse2.h"
extern MAT* m_get(int, int);
}  // extern "C"

int nrn_matrix_dim(void*, int);

#include "ocmatrix.h"
using std::vector;

int nrn_matrix_dim(void* vm, int d) {
    NrnMatrix<double>* m = (NrnMatrix<double>*) vm;
    return d ? m->ncol() : m->nrow();
}

static void Vect2VEC(Vect* v1, VEC& v2) {
#ifdef WIN32
    v2.ve = vector_vec(v1);
    v2.dim = vector_capacity(v1);
    v2.max_dim = vector_buffer_size(v1);
#else
    v2.ve = v1->data();
    v2.dim = v1->size();
    v2.max_dim = v1->buffer_size();
#endif
}

template <typename T>
NrnMatrix<T>::NrnMatrix(int type) {
    obj_ = nullptr;
    type_ = type;
}

template <typename T>
NrnMatrix<T>* NrnMatrix<T>::instance(int nrow, int ncol, int type) {
    switch (type) {
    default:
    case MFULL:
        return new NrnFullMatrix<T>(nrow, ncol);
    case MSPARSE:
        return new NrnSparseMatrix<T>(nrow, ncol);
    }
}

template <typename T>
void NrnMatrix<T>::unimp() {
    hoc_execerror("Matrix method not implemented for this type matrix", 0);
}

template <typename T>
void NrnMatrix<T>::nonzeros(vector<int>& m, vector<int>& n) {
    m.clear();
    n.clear();
    for (int i = 0; i < nrow(); i++) {
        for (int j = 0; j < ncol(); j++) {
            if (getval(i, j)) {
                m.push_back(i);
                n.push_back(j);
            }
        }
    }
}
template class NrnMatrix<double>;

template <typename T>
void NrnSparseMatrix<T>::nonzeros(vector<int>& m, vector<int>& n) {
    m.clear();
    n.clear();
    for (int i = 0; i < m_->m; i++) {
        SPROW* const r = m_->row + i;
        row_elt* r_elt = r->elt;
        for (int k = 0; k < r->len; k++) {
            int j = r_elt[k].col;
            m.push_back(i);
            n.push_back(j);
        }
    }
}


template <typename T>
NrnFullMatrix<T>* NrnMatrix<T>::full() {
    if (type_ != MFULL) {  // could clone one maybe
        hoc_execerror("Matrix is not a FULL matrix (type 1)", 0);
    }
    return (NrnFullMatrix<T>*) this;
}

template <typename T>
NrnFullMatrix<T>::NrnFullMatrix(int nrow, int ncol)
    : NrnMatrix<T>(NrnMatrix<T>::MFULL) {
    lu_factor_ = nullptr;
    lu_pivot_ = nullptr;
    m_ = m_get(nrow, ncol);
}
template <typename T>
NrnFullMatrix<T>::~NrnFullMatrix() {
    if (lu_factor_) {
        M_FREE(lu_factor_);
        PX_FREE(lu_pivot_);
    }
    M_FREE(m_);
}
template <typename T>
T* NrnFullMatrix<T>::mep(int i, int j) {
    return &m_->me[i][j];
}
template <typename T>
T NrnFullMatrix<T>::getval(int i, int j) {
    return m_->me[i][j];
}
template <typename T>
int NrnFullMatrix<T>::nrow() {
    return m_->m;
}
template <typename T>
int NrnFullMatrix<T>::ncol() {
    return m_->n;
}

template <typename T>
void NrnFullMatrix<T>::resize(int i, int j) {
    m_resize(m_, i, j);
}

template <typename T>
void NrnFullMatrix<T>::mulv(Vect* vin, Vect* vout) {
    VEC v1, v2;
    Vect2VEC(vin, v1);
    Vect2VEC(vout, v2);
    mv_mlt(m_, &v1, &v2);
}

template <typename T>
void NrnFullMatrix<T>::mulm(Matrix* in, Matrix* out) {
    m_mlt(m_, in->full()->m_, out->full()->m_);
}

template <typename T>
void NrnFullMatrix<T>::muls(T s, Matrix* out) {
    sm_mlt(s, m_, out->full()->m_);
}

template <typename T>
void NrnFullMatrix<T>::add(Matrix* in, Matrix* out) {
    m_add(m_, in->full()->m_, out->full()->m_);
}

template <typename T>
void NrnFullMatrix<T>::copy(Matrix* out) {
    m_copy(m_, out->full()->m_);
}

template <typename T>
void NrnFullMatrix<T>::bcopy(Matrix* out, int i0, int j0, int n0, int m0, int i1, int j1) {
    m_move(m_, i0, j0, n0, m0, out->full()->m_, i1, j1);
}

template <typename T>
void NrnFullMatrix<T>::transpose(Matrix* out) {
    m_transp(m_, out->full()->m_);
}

template <typename T>
void NrnFullMatrix<T>::symmeigen(Matrix* mout, Vect* vout) {
    VEC v1;
    Vect2VEC(vout, v1);
    symmeig(m_, mout->full()->m_, &v1);
}

template <typename T>
void NrnFullMatrix<T>::svd1(Matrix* u, Matrix* v, Vect* d) {
    VEC v1;
    Vect2VEC(d, v1);
    svd(m_, u ? u->full()->m_ : nullptr, v ? v->full()->m_ : nullptr, &v1);
}

template <typename T>
void NrnFullMatrix<T>::getrow(int k, Vect* out) {
    VEC v1;
    Vect2VEC(out, v1);
    get_row(m_, k, &v1);
}

template <typename T>
void NrnFullMatrix<T>::getcol(int k, Vect* out) {
    VEC v1;
    Vect2VEC(out, v1);
    get_col(m_, k, &v1);
}

template <typename T>
void NrnFullMatrix<T>::getdiag(int k, Vect* out) {
    int i, j, row, col;
    row = nrow();
    col = ncol();
    if (k >= 0) {
        for (i = 0, j = k; i < row && j < col; ++i, ++j) {
#ifdef WIN32
            v_elem(out, i) = m_entry(m_, i, j);
#else
            out->elem(i) = m_entry(m_, i, j);
#endif
        }
    } else {
        // Yes for negative diagonal we set the vector from the middle
        // The output vector should ALWAYS be the size of biggest diagonal
        for (i = -k, j = 0; i < row && j < col; ++i, ++j) {
#ifdef WIN32
            v_elem(out, i) = m_entry(m_, i, j);
#else
            out->elem(i) = m_entry(m_, i, j);
#endif
        }
    }
}

template <typename T>
void NrnFullMatrix<T>::setrow(int k, Vect* in) {
    VEC v1;
    Vect2VEC(in, v1);
    set_row(m_, k, &v1);
}

template <typename T>
void NrnFullMatrix<T>::setcol(int k, Vect* in) {
    VEC v1;
    Vect2VEC(in, v1);
    set_col(m_, k, &v1);
}

template <typename T>
void NrnFullMatrix<T>::setdiag(int k, Vect* in) {
    int i, j, row, col;
    row = nrow();
    col = ncol();
    if (k >= 0) {
        for (i = 0, j = k; i < row && j < col; ++i, ++j) {
#ifdef WIN32
            m_set_val(m_, i, j, v_elem(in, i));
#else
            m_set_val(m_, i, j, in->elem(i));
#endif
        }
    } else {
        // Yes for negative diagonal we set the vector from the middle
        // The input vector should ALWAYS have `nrows` elements.
        for (i = -k, j = 0; i < row && j < col; ++i, ++j) {
#ifdef WIN32
            m_set_val(m_, i, j, v_elem(in, i));
#else
            m_set_val(m_, i, j, in->elem(i));
#endif
        }
    }
}

template <typename T>
void NrnFullMatrix<T>::setrow(int k, T in) {
    int i, col = ncol();
    for (i = 0; i < col; ++i) {
        m_set_val(m_, k, i, in);
    }
}

template <typename T>
void NrnFullMatrix<T>::setcol(int k, T in) {
    int i, row = nrow();
    for (i = 0; i < row; ++i) {
        m_set_val(m_, i, k, in);
    }
}

template <typename T>
void NrnFullMatrix<T>::setdiag(int k, T in) {
    int i, j, row, col;
    row = nrow();
    col = ncol();
    if (k >= 0) {
        for (i = 0, j = k; i < row && j < col; ++i, ++j) {
            m_set_val(m_, i, j, in);
        }
    } else {
        for (i = -k, j = 0; i < row && j < col; ++i, ++j) {
            m_set_val(m_, i, j, in);
        }
    }
}

template <typename T>
void NrnFullMatrix<T>::zero() {
    m_zero(m_);
}

template <typename T>
void NrnFullMatrix<T>::ident() {
    m_ident(m_);
}

template <typename T>
void NrnFullMatrix<T>::exp(Matrix* out) {
    m_exp(m_, 0., out->full()->m_);
}

template <typename T>
void NrnFullMatrix<T>::pow(int i, Matrix* out) {
    m_pow(m_, i, out->full()->m_);
}

template <typename T>
void NrnFullMatrix<T>::inverse(Matrix* out) {
    m_inverse(m_, out->full()->m_);
}

template <typename T>
void NrnFullMatrix<T>::solv(Vect* in, Vect* out, bool use_lu) {
    bool call_lufac = true;
    if (!lu_factor_) {
        lu_factor_ = m_get(nrow(), nrow());
        lu_pivot_ = px_get(nrow());
    } else if (use_lu && lu_factor_->m == nrow()) {
        call_lufac = false;
    }
    VEC v1, v2;
    Vect2VEC(in, v1);
    Vect2VEC(out, v2);
    if (call_lufac) {
        m_resize(lu_factor_, nrow(), nrow());
        m_copy(m_, lu_factor_);
        px_resize(lu_pivot_, nrow());
        LUfactor(lu_factor_, lu_pivot_);
    }
    LUsolve(lu_factor_, lu_pivot_, &v1, &v2);
}

template <typename T>
T NrnFullMatrix<T>::det(int* e) {
    int n = nrow();
    MAT* lu = m_get(n, n);
    PERM* piv = px_get(n);
    m_copy(m_, lu);
    LUfactor(lu, piv);
    T m = 1.0;
    *e = 0;
    for (int i = 0; i < n; ++i) {
        m *= lu->me[i][i];
        if (m == 0.0) {
            break;
        }
        while (std::abs(m) >= 1e12) {
            m *= 1e-12;
            *e += 12;
        }
        while (std::abs(m) < 1e-12) {
            m *= 1e12;
            *e -= 12;
        }
    }
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
    m *= T(px_sign(piv));
    M_FREE(lu);
    PX_FREE(piv);
    return m;
}
template class NrnFullMatrix<double>;

//--------------------------

template <typename T>
NrnSparseMatrix<T>::NrnSparseMatrix(int nrow, int ncol)
    : NrnMatrix<T>(NrnMatrix<T>::MSPARSE) {
    /* sp_get -- get sparse matrix
       -- len is number of elements available for each row without
          allocating further memory */

    int len = 4;
    m_ = sp_get(nrow, ncol, len);
    lu_factor_ = nullptr;
    lu_pivot_ = nullptr;
}
template <typename T>
NrnSparseMatrix<T>::~NrnSparseMatrix() {
    if (lu_factor_) {
        SP_FREE(lu_factor_);
        PX_FREE(lu_pivot_);
    }
    SP_FREE(m_);
}

// returns pointer to sparse element. nullptr if it does not exist.
template <typename T>
T* NrnSparseMatrix<T>::pelm(int i, int j) {
    SPROW* r = m_->row + i;
    int idx = sprow_idx(r, j);
    if (idx >= 0) {
        return &r->elt[idx].val;
    } else {
        return nullptr;
    }
}

template <typename T>
T* NrnSparseMatrix<T>::mep(int i, int j) {
    SPROW* r = m_->row + i;
    int idx = sprow_idx(r, j);
    if (idx >= 0) {
        return &r->elt[idx].val;
    }
    // does not exist so create it with a value of 0
    sp_set_val(m_, i, j, 0.);
    // and try again
    idx = sprow_idx(r, j);
    return &r->elt[idx].val;
}

template <typename T>
void NrnSparseMatrix<T>::zero() {
    sp_zero(m_);
}

template <typename T>
T NrnSparseMatrix<T>::getval(int i, int j) {
    return sp_get_val(m_, i, j);
}
template <typename T>
int NrnSparseMatrix<T>::nrow() {
    return m_->m;
}
template <typename T>
int NrnSparseMatrix<T>::ncol() {
    return m_->n;
}
template <typename T>
void NrnSparseMatrix<T>::mulv(Vect* vin, Vect* vout) {
    VEC v1, v2;
    Vect2VEC(vin, v1);
    Vect2VEC(vout, v2);
    sp_mv_mlt(m_, &v1, &v2);
}

template <typename T>
void NrnSparseMatrix<T>::solv(Vect* in, Vect* out, bool use_lu) {
    bool call_lufac = true;
    if (!lu_factor_) {
        lu_factor_ = sp_get(nrow(), nrow(), 4);
        lu_pivot_ = px_get(nrow());
    } else if (use_lu && lu_factor_->m == nrow()) {
        call_lufac = false;
    }
    VEC v1, v2;
    Vect2VEC(in, v1);
    Vect2VEC(out, v2);
    if (call_lufac) {
        sp_resize(lu_factor_, nrow(), nrow());
        sp_copy2(m_, lu_factor_);
        px_resize(lu_pivot_, nrow());
        spLUfactor(lu_factor_, lu_pivot_, .9);
    }
    spLUsolve(lu_factor_, lu_pivot_, &v1, &v2);
}

template <typename T>
void NrnSparseMatrix<T>::setrow(int k, Vect* in) {
    VEC v1;
    Vect2VEC(in, v1);
    int i, n = ncol();
    T* p;
    for (i = 0; i < n; ++i) {
        if ((p = pelm(k, i)) != nullptr) {
#ifdef WIN32
            *p = v_elem(in, i);
        } else if (v_elem(in, i)) {
            sp_set_val(m_, k, i, v_elem(in, i));
#else
            *p = in->elem(i);
        } else if (in->elem(i)) {
            sp_set_val(m_, k, i, in->elem(i));
#endif
        }
    }
}

template <typename T>
void NrnSparseMatrix<T>::setcol(int k, Vect* in) {
    VEC v1;
    Vect2VEC(in, v1);
    int i, n = nrow();
    T* p;
    for (i = 0; i < n; ++i) {
        if ((p = pelm(i, k)) != nullptr) {
#ifdef WIN32
            *p = v_elem(in, i);
        } else if (v_elem(in, i)) {
            sp_set_val(m_, i, k, v_elem(in, i));
#else
            *p = in->elem(i);
        } else if (in->elem(i)) {
            sp_set_val(m_, i, k, in->elem(i));
#endif
        }
    }
}

template <typename T>
void NrnSparseMatrix<T>::setdiag(int k, Vect* in) {
    int i, j, row, col;
    row = nrow();
    col = ncol();
    T* p;
    if (k >= 0) {
        for (i = 0, j = k; i < row && j < col; ++i, ++j) {
            if ((p = pelm(i, j)) != nullptr) {
#ifdef WIN32
                *p = v_elem(in, i);
            } else if (v_elem(in, i)) {
                sp_set_val(m_, i, j, v_elem(in, i));
#else
                *p = in->elem(i);
            } else if (in->elem(i)) {
                sp_set_val(m_, i, j, in->elem(i));
#endif
            }
        }
    } else {
        for (i = -k, j = 0; i < row && j < col; ++i, ++j) {
            // Yes for negative diagonal we set the vector from the middle
            // The input vector should ALWAYS have `nrows` elements.
            if ((p = pelm(i, j)) != nullptr) {
#ifdef WIN32
                *p = v_elem(in, i);
            } else if (v_elem(in, i)) {
                sp_set_val(m_, i, j, v_elem(in, i));
#else
                *p = in->elem(i);
            } else if (in->elem(i)) {
                sp_set_val(m_, i, j, in->elem(i));
#endif
            }
        }
    }
}

template <typename T>
void NrnSparseMatrix<T>::setrow(int k, T in) {
    int i, col = ncol();
    for (i = 0; i < col; ++i) {
        sp_set_val(m_, k, i, in);
    }
}

template <typename T>
void NrnSparseMatrix<T>::setcol(int k, T in) {
    int i, row = nrow();
    for (i = 0; i < row; ++i) {
        sp_set_val(m_, i, k, in);
    }
}

template <typename T>
void NrnSparseMatrix<T>::ident(void) {
    setdiag(0, 1);
}

template <typename T>
void NrnSparseMatrix<T>::setdiag(int k, T in) {
    int i, j, row, col;
    row = nrow();
    col = ncol();
    if (k >= 0) {
        for (i = 0, j = k; i < row && j < col; ++i, ++j) {
            sp_set_val(m_, i, j, in);
        }
    } else {
        for (i = -k, j = 0; i < row && j < col; ++i, ++j) {
            sp_set_val(m_, i, j, in);
        }
    }
}

template <typename T>
int NrnSparseMatrix<T>::sprowlen(int i) {
    return m_->row[i].len;
}

template <typename T>
T NrnSparseMatrix<T>::spgetrowval(int i, int jindx, int* j) {
    *j = m_->row[i].elt[jindx].col;
    return m_->row[i].elt[jindx].val;
}
template class NrnSparseMatrix<double>;
