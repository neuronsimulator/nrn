#include <../../nrnconf.h>
#include "classreg.h"

#include <stdio.h>
#include <math.h>
#include "ocmatrix.h"
#include "oc2iv.h"
#include "parse.hpp"
#include "ivocvect.h"

#define EPS hoc_epsilon
Symbol* nrn_matrix_sym;  // also used in oc/hoc_oop.cpp

extern int hoc_return_type_code;

extern double hoc_scan(FILE*);
extern Object** hoc_temp_objptr(Object*);

static void check_domain(int i, int j) {
    if (i > j || i < 0) {
        auto const tmp = "index=" + std::to_string(i) + "  max_index=" + std::to_string(j) + "\n";
        hoc_execerror("Matrix index out of range:", tmp.c_str());
    }
}

static void check_capac(int i, int j) {
    if (i != j) {
        hoc_execerror("wrong size for Matrix or Vector operation", 0);
    }
}

Matrix* matrix_arg(int i) {
    Object* ob = *hoc_objgetarg(i);
    if (!ob || ob->ctemplate != nrn_matrix_sym->u.ctemplate) {
        check_obj_type(ob, "Matrix");
    }
    return (Matrix*) (ob->u.this_pointer);
}

static Object** temp_objvar(Matrix* m) {
    Object** po;
    if (m->obj_) {
        po = hoc_temp_objptr(m->obj_);
    } else {
        po = hoc_temp_objvar(nrn_matrix_sym, (void*) m);
        m->obj_ = *po;
    }
    return po;
}

static double m_nrow(void* v) {
    hoc_return_type_code = 1;  // integer
    Matrix* m = (Matrix*) v;
    return (double) m->nrow();
}

static double m_ncol(void* v) {
    hoc_return_type_code = 1;  // integer
    Matrix* m = (Matrix*) v;
    return (double) m->ncol();
}

static double m_setval(void* v) {
    Matrix* m = (Matrix*) v;
    int i, j;
    double val, *pval;
    i = (int) chkarg(1, 0, m->nrow() - 1);
    j = (int) chkarg(2, 0, m->ncol() - 1);
    val = *getarg(3);
    pval = m->mep(i, j);
    *pval = val;
    return val;
}

static double m_getval(void* v) {
    Matrix* m = (Matrix*) v;
    int i, j;
    i = (int) chkarg(1, 0, m->nrow() - 1);
    j = (int) chkarg(2, 0, m->ncol() - 1);
    return m->getval(i, j);
}

static double m_sprowlen(void* v) {
    Matrix* m = (Matrix*) v;
    int i;
    hoc_return_type_code = 1;  // integer
    i = (int) chkarg(1, 0, m->nrow() - 1);
    return double(m->sprowlen(i));
}

static double m_spgetrowval(void* v) {
    Matrix* m = (Matrix*) v;
    int i, jx, j;
    double x;
    i = (int) chkarg(1, 0, m->nrow() - 1);
    jx = (int) chkarg(2, 0, m->sprowlen(i) - 1);
    x = m->spgetrowval(i, jx, &j);
    if (ifarg(3)) {
        *hoc_pgetarg(3) = double(j);
    }
    return x;
}

static double m_printf(void* v) {
    Matrix* m = (Matrix*) v;
    int i, j, nrow = m->nrow(), ncol = m->ncol();
    const char* f1 = " %-8.3g";
    const char* f2 = "\n";
    if (ifarg(1)) {
        f1 = gargstr(1);
    }
    if (ifarg(2)) {
        f2 = gargstr(2);
    }
    for (i = 0; i < nrow; ++i) {
        for (j = 0; j < ncol; ++j) {
            Printf(f1, m->getval(i, j));
        }
        Printf("%s", f2);
    }
    return 0.;
}

static double m_fprint(void* v) {
    Matrix* m = (Matrix*) v;
    int i, j, nrow = m->nrow(), ncol = m->ncol();
    int ia = 1;
    bool pr_size = true;
    const char* f1 = " %-8.3g";
    const char* f2 = "\n";
    if (hoc_is_double_arg(ia)) {
        pr_size = ((int) chkarg(ia, 0, 1) == 1) ? true : false;
        ++ia;
    }
    FILE* f = hoc_obj_file_arg(ia);
    if (ifarg(ia + 1)) {
        f1 = gargstr(ia + 1);
    }
    if (ifarg(ia + 2)) {
        f2 = gargstr(ia + 2);
    }
    if (pr_size) {
        fprintf(f, "%d %d\n", nrow, ncol);
    }
    for (i = 0; i < nrow; ++i) {
        for (j = 0; j < ncol; ++j) {
            fprintf(f, f1, m->getval(i, j));
        }
        fprintf(f, "%s", f2);
    }
    return 0.;
}

static double m_scanf(void* v) {
    // file assumed to be an array of numbers. Numbers in rows
    // are contiguous in the stream.
    // first two numbers are nrow and ncol unless
    // arguments 2 and 3 specify them
    Matrix* m = (Matrix*) v;
    FILE* f = hoc_obj_file_arg(1);
    int i, j, nrow, ncol;
    if (ifarg(2)) {
        nrow = (int) chkarg(2, 1, 1e9);
        ncol = (int) chkarg(3, 1, 1e9);
    } else {
        nrow = (int) hoc_scan(f);
        ncol = (int) hoc_scan(f);
    }
    m->resize(nrow, ncol);
    for (i = 0; i < nrow; ++i)
        for (j = 0; j < ncol; ++j) {
            *(m->mep(i, j)) = hoc_scan(f);
        }
    return 0.;
}

static Object** m_resize(void* v) {
    Matrix* m = (Matrix*) v;
    m->resize((int) (chkarg(1, 1., 1e9) + EPS), (int) (chkarg(2, 1., 1e9) + EPS));
    return temp_objvar(m);
}

static Object** m_mulv(void* v) {
    Matrix* m = (Matrix*) v;
    Vect* vin = vector_arg(1);
    Vect* vout;
    bool f = false;
    if (ifarg(2)) {
        vout = vector_arg(2);
    } else {
#ifdef WIN32
        vout = vector_new(m->nrow());
#else
        vout = new Vect(m->nrow());
#endif
    }
    if (vin == vout) {
        f = true;
#ifdef WIN32
        vin = vector_new2(vin);
#else
        vin = new Vect(*vin);
#endif
    }
#ifdef WIN32
    check_capac(vector_capacity(vin), m->ncol());
    vector_resize(vout, m->nrow());
#else
    check_capac(vin->size(), m->ncol());
    vout->resize(m->nrow());
#endif
    m->mulv(vin, vout);
    if (f) {
#ifdef WIN32
        vector_delete(vin);
#else
        delete vin;
#endif
    }
#ifdef WIN32
    return vector_temp_objvar(vout);
#else
    return vout->temp_objvar();
#endif
}


static Matrix* get_out_mat(Matrix* mat, int n, int m, int i, const char* mes = NULL);

static Matrix* get_out_mat(Matrix* mat, int n, int m, int i, const char* mes) {
    Matrix* out;
    if (ifarg(i)) {
        out = matrix_arg(i);
    } else {
        out = Matrix::instance(n, m, Matrix::MFULL);
        out->obj_ = NULL;
    }
    if (mat == out && mes) {
        hoc_execerror(mes, " matrix operation cannot be done in place");
    }
    return out;
}

static Matrix* get_out_mat(Matrix* m, int i, const char* mes = NULL) {
    return get_out_mat(m, m->nrow(), m->ncol(), i, mes);
}

static Object** m_add(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix* out;
    out = m;
    if (ifarg(2)) {
        out = matrix_arg(2);
    }
    m->add(matrix_arg(1), out);
    return temp_objvar(out);
}

static Object** m_bcopy(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix* out;
    int i0, j0, m0, n0, i1, j1, i;
    i0 = (int) chkarg(1, 0, m->nrow() - 1);
    j0 = (int) chkarg(2, 0, m->ncol() - 1);
    m0 = (int) chkarg(3, 1, m->nrow() - i0);
    n0 = (int) chkarg(4, 1, m->ncol() - j0);
    if (ifarg(5) && hoc_is_double_arg(5)) {
        i1 = (int) chkarg(5, 0, 1e9);
        j1 = (int) chkarg(6, 0, 1e9);
        i = 7;
    } else {
        i1 = 0;
        j1 = 0;
        i = 5;
    }
    out = get_out_mat(m, m0, n0, i);
    m->bcopy(out, i0, j0, m0, n0, i1, j1);
    return temp_objvar(out);
}

static Object** m_mulm(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix *in, *out;
    in = matrix_arg(1);
    if (ifarg(2)) {
        out = matrix_arg(2);
    } else {
        out = Matrix::instance(m->nrow(), in->ncol(), Matrix::MFULL);
    }
    if (in == out || m == out) {
        hoc_execerror("matrix multiplication cannot be done in place", 0);
    }
    out->resize(m->nrow(), in->ncol());
    check_domain(m->ncol(), in->nrow());
    m->mulm(in, out);
    return temp_objvar(out);
}

static Object** m_c(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix* out = get_out_mat(m, 1);
    m->copy(out);
    return temp_objvar(out);
}

static Object** m_transpose(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix* out = get_out_mat(m, 1);
    out->resize(m->ncol(), m->nrow());
    m->transpose(out);
    return temp_objvar(out);
}

static Object** m_symmeig(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix* out = matrix_arg(1);
    Object** p;
    out->resize(m->nrow(), m->ncol());
    Vect* vout;
#ifdef WIN32
    vout = vector_new(m->nrow());
    p = vector_temp_objvar(vout);
#else
    vout = new Vect(m->nrow());
    p = vout->temp_objvar();
#endif
    m->symmeigen(out, vout);
    return p;
}

static Object** m_svd(void* vv) {
    Matrix* m = (Matrix*) vv;
    Matrix *u = NULL, *v = NULL;
    if (ifarg(2)) {
        u = matrix_arg(1);
        v = matrix_arg(2);
        u->resize(m->nrow(), m->nrow());
        v->resize(m->ncol(), m->ncol());
    }
    Object** p;
    Vect* d;
    int dsize = m->nrow() < m->ncol() ? m->nrow() : m->ncol();
#ifdef WIN32
    d = vector_new(dsize);
    p = vector_temp_objvar(d);
#else
    d = new Vect(dsize);
    p = d->temp_objvar();
#endif
    m->svd1(u, v, d);
    return p;
}

static Object** m_muls(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix* out;
    out = m;
    if (ifarg(2)) {
        out = matrix_arg(2);
    }
    m->muls(*getarg(1), out);
    return temp_objvar(out);
}

static Object** m_getrow(void* v) {
    Matrix* m = (Matrix*) v;
    int k = (int) chkarg(1, 0, m->nrow() - 1);
    Vect* vout;
    if (ifarg(2)) {
        vout = vector_arg(2);
#ifdef WIN32
        vector_resize(vout, m->ncol());
#else
        vout->resize(m->ncol());
#endif
    } else {
#ifdef WIN32
        vout = vector_new(m->ncol());
#else
        vout = new Vect(m->ncol());
#endif
    }
    m->getrow(k, vout);
#ifdef WIN32
    return vector_temp_objvar(vout);
#else
    return vout->temp_objvar();
#endif
}

static Object** m_getcol(void* v) {
    Matrix* m = (Matrix*) v;
    int k = (int) chkarg(1, 0, m->ncol() - 1);
    Vect* vout;
    if (ifarg(2)) {
        vout = vector_arg(2);
#ifdef WIN32
        vector_resize(vout, m->nrow());
#else
        vout->resize(m->nrow());
#endif
    } else {
#ifdef WIN32
        vout = vector_new(m->nrow());
#else
        vout = new Vect(m->nrow());
#endif
    }
    m->getcol(k, vout);
#ifdef WIN32
    return vector_temp_objvar(vout);
#else
    return vout->temp_objvar();
#endif
}

static Object** m_setrow(void* v) {
    Matrix* m = (Matrix*) v;
    int k = (int) chkarg(1, 0, m->nrow() - 1);
    if (hoc_is_double_arg(2)) {
        m->setrow(k, *getarg(2));
    } else {
        Vect* in = vector_arg(2);
#ifdef WIN32
        check_domain(vector_capacity(in), m->ncol());
#else
        check_domain(in->size(), m->ncol());
#endif
        m->setrow(k, in);
    }
    return temp_objvar(m);
}

static Object** m_setcol(void* v) {
    Matrix* m = (Matrix*) v;
    int k = (int) chkarg(1, 0, m->ncol() - 1);
    if (hoc_is_double_arg(2)) {
        m->setcol(k, *getarg(2));
    } else {
        Vect* in = vector_arg(2);
#ifdef WIN32
        check_domain(vector_capacity(in), m->nrow());
#else
        check_domain(in->size(), m->nrow());
#endif
        m->setcol(k, in);
    }
    return temp_objvar(m);
}

static Object** m_setdiag(void* v) {
    Matrix* m = (Matrix*) v;
    int k = (int) chkarg(1, -(m->nrow() - 1), m->ncol() - 1);
    if (hoc_is_double_arg(2)) {
        m->setdiag(k, *getarg(2));
    } else {
        Vect* in = vector_arg(2);
#ifdef WIN32
        check_domain(vector_capacity(in), m->nrow());
#else
        check_domain(in->size(), m->nrow());
#endif
        m->setdiag(k, in);
    }
    return temp_objvar(m);
}

static Object** m_getdiag(void* v) {
    Matrix* m = (Matrix*) v;
    int k = (int) chkarg(1, -(m->nrow() - 1), m->ncol() - 1);
    Vect* vout;
    if (ifarg(2)) {
        vout = vector_arg(2);
#ifdef WIN32
        vector_resize(vout, m->nrow());
#else
        vout->resize(m->nrow());
#endif
    } else {
#ifdef WIN32
        vout = vector_new(m->nrow());
#else
        vout = new Vect(m->nrow());
#endif
    }
    m->getdiag(k, vout);
#ifdef WIN32
    return vector_temp_objvar(vout);
#else
    return vout->temp_objvar();
#endif
}

static Object** m_zero(void* v) {
    Matrix* m = (Matrix*) v;
    m->zero();
    return temp_objvar(m);
}

static Object** m_ident(void* v) {
    Matrix* m = (Matrix*) v;
    m->ident();
    return temp_objvar(m);
}

static Object** m_exp(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix* out = get_out_mat(m, 1, "exponentiation");
    m->exp(out);
    return temp_objvar(out);
}

static Object** m_pow(void* v) {
    Matrix* m = (Matrix*) v;
    int k = (int) chkarg(1, 0., 100.);
    Matrix* out = get_out_mat(m, 2, "raising to a power");
    m->pow(k, out);
    return temp_objvar(out);
}

static Object** m_inverse(void* v) {
    Matrix* m = (Matrix*) v;
    Matrix* out = get_out_mat(m, 1);
    m->inverse(out);
    return temp_objvar(out);
}

static double m_det(void* v) {
    Matrix* m = (Matrix*) v;
    int e;
    double a = m->det(&e);
    double* pe = hoc_pgetarg(1);
    *pe = double(e);
    return a;
}

static Object** m_solv(void* v) {
    Matrix* m = (Matrix*) v;
    check_capac(m->nrow(), m->ncol());
    Vect* vin = vector_arg(1);
#ifdef WIN32
    check_capac(vector_capacity(vin), m->ncol());
#else
    check_capac(vin->size(), m->ncol());
#endif
    Vect* vout = NULL;
    bool f = false;
    bool use_lu = false;
    // args 2 and 3 are optional [vout, use previous LU factorization]
    // and in either order
    for (int i = 2; i <= 3; ++i) {
        if (ifarg(i)) {
            if (hoc_is_object_arg(i)) {
                if (vout) {
                }
                vout = vector_arg(i);
            } else {
                use_lu = ((int) (*getarg(i))) ? true : false;
            }
        }
    }
    if (!vout) {
#ifdef WIN32
        vout = vector_new(m->nrow());
#else
        vout = new Vect(m->nrow());
#endif
    }
#ifdef WIN32
    vector_resize(vout, m->ncol());
#else
    vout->resize(m->ncol());
#endif
    if (vin == vout) {
        f = true;
#ifdef WIN32
        vin = vector_new2(vin);
#else
        vin = new Vect(*vin);
#endif
    }
    m->solv(vin, vout, use_lu);
    if (f) {
#ifdef WIN32
        vector_delete(vin);
#else
        delete vin;
#endif
    }
#ifdef WIN32
    return vector_temp_objvar(vout);
#else
    return vout->temp_objvar();
#endif
}

static Object** m_set(void* v) {
    Matrix* m = (Matrix*) v;
    int i, j, nrow = m->nrow(), ncol = m->ncol();
    int k;
    for (k = 0, i = 0; i < nrow; ++i) {
        for (j = 0; j < ncol; ++j) {
            *(m->mep(i, j)) = *getarg(++k);
        }
    }
    return temp_objvar(m);
}

static Object** m_to_vector(void* v) {
    Matrix* m = (Matrix*) v;
    Vect* vout;
    int i, j, k;
    int nrow = m->nrow();
    int ncol = m->ncol();
    if (ifarg(1)) {
        vout = vector_arg(1);
        vector_resize(vout, nrow * ncol);
    } else {
        vout = vector_new(nrow * ncol);
    }
    k = 0;
    double* ve = vector_vec(vout);
    for (j = 0; j < ncol; ++j)
        for (i = 0; i < nrow; ++i) {
            ve[k++] = m->getval(i, j);
        }
    return vector_temp_objvar(vout);
}

static Object** m_from_vector(void* v) {
    Matrix* m = (Matrix*) v;
    Vect* vout;
    int i, j, k;
    int nrow = m->nrow();
    int ncol = m->ncol();
    vout = vector_arg(1);
    check_capac(nrow * ncol, vector_capacity(vout));
    k = 0;
    double* ve = vector_vec(vout);
    for (j = 0; j < ncol; ++j)
        for (i = 0; i < nrow; ++i) {
            *(m->mep(i, j)) = ve[k++];
        }
    return temp_objvar(m);
}

static Member_func m_members[] = {
    // returns double scalar
    {"x", m_nrow},  // will be changed below
    {"nrow", m_nrow},
    {"ncol", m_ncol},
    {"getval", m_getval},
    {"setval", m_setval},
    {"sprowlen", m_sprowlen},
    {"spgetrowval", m_spgetrowval},
    {"det", m_det},

    {"printf", m_printf},
    {"fprint", m_fprint},
    {"scanf", m_scanf},
    {0, 0}};

static Member_ret_obj_func m_retobj_members[] = {
    // returns Vector
    {"mulv", m_mulv},
    {"getrow", m_getrow},
    {"getcol", m_getcol},
    {"getdiag", m_getdiag},
    {"solv", m_solv},
    {"symmeig", m_symmeig},
    {"svd", m_svd},
    // returns Matrix
    {"c", m_c},
    {"add", m_add},
    {"bcopy", m_bcopy},
    {"resize", m_resize},
    {"mulm", m_mulm},
    {"muls", m_muls},
    {"setrow", m_setrow},
    {"setcol", m_setcol},
    {"setdiag", m_setdiag},
    {"zero", m_zero},
    {"ident", m_ident},
    {"exp", m_exp},
    {"pow", m_pow},
    {"inverse", m_inverse},
    {"transpose", m_transpose},
    {"set", m_set},
    {"to_vector", m_to_vector},
    {"from_vector", m_from_vector},
    {0, 0}};

static void* m_cons(Object* o) {
    int i = 1, j = 1, storage_type = Matrix::MFULL;
    if (ifarg(1))
        i = int(chkarg(1, 1, 1e10) + EPS);
    if (ifarg(2))
        j = int(chkarg(2, 1, 1e10) + EPS);
    if (ifarg(3))
        storage_type = int(chkarg(3, 1, 3));
    Matrix* m = Matrix::instance(i, j, storage_type);
    m->obj_ = o;
    return m;
}

static void m_destruct(void* v) {
    // supposed to notify freed val array here.
    // printf("Matrix deleted\n");
    delete (Matrix*) v;
}

static void steer_x(void* v) {
    Matrix* m = (Matrix*) v;
    int i1, i2;
    Symbol* s = hoc_spop();
    if (!hoc_stack_type_is_ndim()) {
        hoc_execerr_ext("Array dimension of Matrix.x is 2");
    }
    hoc_pop_ndim();
    i2 = (int) (hoc_xpop() + EPS);
    i1 = (int) (hoc_xpop() + EPS);
    check_domain(i1, m->nrow() - 1);
    check_domain(i2, m->ncol() - 1);
    hoc_pushpx(m->mep(i1, i2));
}


#if WIN32 && !USEMATRIX
void Matrix_reg();
#endif

void Matrix_reg() {
    class2oc("Matrix", m_cons, m_destruct, m_members, NULL, m_retobj_members, NULL);
    nrn_matrix_sym = hoc_lookup("Matrix");
    // now make the x variable an actual double
    Symbol* sx = hoc_table_lookup("x", nrn_matrix_sym->u.ctemplate->symtable);
    sx->type = VAR;
    sx->arayinfo = (Arrayinfo*) hoc_Emalloc(sizeof(Arrayinfo) + 2 * sizeof(int));
    sx->arayinfo->refcount = 1;
    sx->arayinfo->a_varn = NULL;
    sx->arayinfo->nsub = 2;
    sx->arayinfo->sub[0] = 1;
    sx->arayinfo->sub[1] = 1;
    nrn_matrix_sym->u.ctemplate->steer = steer_x;
}
