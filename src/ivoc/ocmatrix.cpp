#include <../../nrnconf.h>
#include <vector>
#include <cmath>
#include <InterViews/resource.h>

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
    OcMatrix* m = (OcMatrix*) vm;
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

OcMatrix::OcMatrix(int type) {
    obj_ = NULL;
    type_ = type;
}
OcMatrix::~OcMatrix() {}

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

void OcMatrix::nonzeros(vector<int>& m, vector<int>& n) {
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

void OcSparseMatrix::nonzeros(vector<int>& m, vector<int>& n) {
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


OcFullMatrix* OcMatrix::full() {
    if (type_ != MFULL) {  // could clone one maybe
        hoc_execerror("Matrix is not a FULL matrix (type 1)", 0);
    }
    return (OcFullMatrix*) this;
}

OcFullMatrix::OcFullMatrix(int nrow, int ncol)
    : OcMatrix(MFULL) {
    lu_factor_ = NULL;
    lu_pivot_ = NULL;
    m_ = m_get(nrow, ncol);
}
OcFullMatrix::~OcFullMatrix() {
    if (lu_factor_) {
        M_FREE(lu_factor_);
        PX_FREE(lu_pivot_);
    }
    M_FREE(m_);
}
double* OcFullMatrix::mep(int i, int j) {
    return &m_->me[i][j];
}
double OcFullMatrix::getval(int i, int j) {
    return m_->me[i][j];
}
int OcFullMatrix::nrow() {
    return m_->m;
}
int OcFullMatrix::ncol() {
    return m_->n;
}

void OcFullMatrix::resize(int i, int j) {
    m_resize(m_, i, j);
}

void OcFullMatrix::mulv(Vect* vin, Vect* vout) {
    VEC v1, v2;
    Vect2VEC(vin, v1);
    Vect2VEC(vout, v2);
    mv_mlt(m_, &v1, &v2);
}

void OcFullMatrix::mulm(Matrix* in, Matrix* out) {
    m_mlt(m_, in->full()->m_, out->full()->m_);
}

void OcFullMatrix::muls(double s, Matrix* out) {
    sm_mlt(s, m_, out->full()->m_);
}

void OcFullMatrix::add(Matrix* in, Matrix* out) {
    m_add(m_, in->full()->m_, out->full()->m_);
}

void OcFullMatrix::copy(Matrix* out) {
    m_copy(m_, out->full()->m_);
}

void OcFullMatrix::bcopy(Matrix* out, int i0, int j0, int n0, int m0, int i1, int j1) {
    m_move(m_, i0, j0, n0, m0, out->full()->m_, i1, j1);
}

void OcFullMatrix::transpose(Matrix* out) {
    m_transp(m_, out->full()->m_);
}

void OcFullMatrix::symmeigen(Matrix* mout, Vect* vout) {
    VEC v1;
    Vect2VEC(vout, v1);
    symmeig(m_, mout->full()->m_, &v1);
}

void OcFullMatrix::svd1(Matrix* u, Matrix* v, Vect* d) {
    VEC v1;
    Vect2VEC(d, v1);
    svd(m_, u ? u->full()->m_ : NULL, v ? v->full()->m_ : NULL, &v1);
}

void OcFullMatrix::getrow(int k, Vect* out) {
    VEC v1;
    Vect2VEC(out, v1);
    get_row(m_, k, &v1);
}

void OcFullMatrix::getcol(int k, Vect* out) {
    VEC v1;
    Vect2VEC(out, v1);
    get_col(m_, k, &v1);
}

void OcFullMatrix::getdiag(int k, Vect* out) {
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
        for (i = -k, j = 0; i < row && j < col; ++i, ++j) {
#ifdef WIN32
            v_elem(out, i) = m_entry(m_, i, j);
#else
            out->elem(i) = m_entry(m_, i, j);
#endif
        }
    }
}

void OcFullMatrix::setrow(int k, Vect* in) {
    VEC v1;
    Vect2VEC(in, v1);
    set_row(m_, k, &v1);
}

void OcFullMatrix::setcol(int k, Vect* in) {
    VEC v1;
    Vect2VEC(in, v1);
    set_col(m_, k, &v1);
}

void OcFullMatrix::setdiag(int k, Vect* in) {
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
        for (i = -k, j = 0; i < row && j < col; ++i, ++j) {
#ifdef WIN32
            m_set_val(m_, i, j, v_elem(in, i));
#else
            m_set_val(m_, i, j, in->elem(i));
#endif
        }
    }
}

void OcFullMatrix::setrow(int k, double in) {
    int i, col = ncol();
    for (i = 0; i < col; ++i) {
        m_set_val(m_, k, i, in);
    }
}

void OcFullMatrix::setcol(int k, double in) {
    int i, row = nrow();
    for (i = 0; i < row; ++i) {
        m_set_val(m_, i, k, in);
    }
}

void OcFullMatrix::setdiag(int k, double in) {
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

void OcFullMatrix::zero() {
    m_zero(m_);
}

void OcFullMatrix::ident() {
    m_ident(m_);
}

void OcFullMatrix::exp(Matrix* out) {
    m_exp(m_, 0., out->full()->m_);
}

void OcFullMatrix::pow(int i, Matrix* out) {
    m_pow(m_, i, out->full()->m_);
}

void OcFullMatrix::inverse(Matrix* out) {
    m_inverse(m_, out->full()->m_);
}

void OcFullMatrix::solv(Vect* in, Vect* out, bool use_lu) {
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

double OcFullMatrix::det(int* e) {
    int n = nrow();
    MAT* lu = m_get(n, n);
    PERM* piv = px_get(n);
    m_copy(m_, lu);
    LUfactor(lu, piv);
#if 0
printf("LU\n");
for (int i = 0; i < n; ++i) {
	for (int j = 0; j < n; ++j) {
		printf(" %g", lu->me[i][j]);
	}
	printf("\t%d\n", piv->pe[i]);
}
#endif
    double m = 1.0;
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
    m *= double(px_sign(piv));
    M_FREE(lu);
    PX_FREE(piv);
    return m;
}

//--------------------------

OcSparseMatrix::OcSparseMatrix(int nrow, int ncol)
    : OcMatrix(MSPARSE) {
    /* sp_get -- get sparse matrix
       -- len is number of elements available for each row without
          allocating further memory */

    int len = 4;
    m_ = sp_get(nrow, ncol, len);
    lu_factor_ = NULL;
    lu_pivot_ = NULL;
}
OcSparseMatrix::~OcSparseMatrix() {
    if (lu_factor_) {
        SP_FREE(lu_factor_);
        PX_FREE(lu_pivot_);
    }
    SP_FREE(m_);
}

// returns pointer to sparse element. NULL if it does not exist.
double* OcSparseMatrix::pelm(int i, int j) {
    SPROW* r = m_->row + i;
    int idx = sprow_idx(r, j);
    if (idx >= 0) {
        return &r->elt[idx].val;
    } else {
        return NULL;
    }
}

double* OcSparseMatrix::mep(int i, int j) {
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

void OcSparseMatrix::zero() {
    sp_zero(m_);
}

double OcSparseMatrix::getval(int i, int j) {
    return sp_get_val(m_, i, j);
}
int OcSparseMatrix::nrow() {
    return m_->m;
}
int OcSparseMatrix::ncol() {
    return m_->n;
}
void OcSparseMatrix::mulv(Vect* vin, Vect* vout) {
    VEC v1, v2;
    Vect2VEC(vin, v1);
    Vect2VEC(vout, v2);
    sp_mv_mlt(m_, &v1, &v2);
}

void OcSparseMatrix::solv(Vect* in, Vect* out, bool use_lu) {
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

void OcSparseMatrix::setrow(int k, Vect* in) {
    VEC v1;
    Vect2VEC(in, v1);
    int i, n = ncol();
    double* p;
    for (i = 0; i < n; ++i) {
        if ((p = pelm(k, i)) != NULL) {
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

void OcSparseMatrix::setcol(int k, Vect* in) {
    VEC v1;
    Vect2VEC(in, v1);
    int i, n = nrow();
    double* p;
    for (i = 0; i < n; ++i) {
        if ((p = pelm(i, k)) != NULL) {
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

void OcSparseMatrix::setdiag(int k, Vect* in) {
    int i, j, row, col;
    row = nrow();
    col = ncol();
    double* p;
    if (k >= 0) {
        for (i = 0, j = k; i < row && j < col; ++i, ++j) {
            if ((p = pelm(i, j)) != NULL) {
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
            if ((p = pelm(i, j)) != NULL) {
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

void OcSparseMatrix::setrow(int k, double in) {
    int i, col = ncol();
    for (i = 0; i < col; ++i) {
        sp_set_val(m_, k, i, in);
    }
}

void OcSparseMatrix::setcol(int k, double in) {
    int i, row = nrow();
    for (i = 0; i < row; ++i) {
        sp_set_val(m_, i, k, in);
    }
}

void OcSparseMatrix::ident(void) {
    setdiag(0, 1);
}

void OcSparseMatrix::setdiag(int k, double in) {
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

int OcSparseMatrix::sprowlen(int i) {
    return m_->row[i].len;
}

double OcSparseMatrix::spgetrowval(int i, int jindx, int* j) {
    *j = m_->row[i].elt[jindx].col;
    return m_->row[i].elt[jindx].val;
}
