/*
# =============================================================================
# Originally sparse.c from SCoP library, Copyright (c) 1989-90 Duke University
# =============================================================================
# Subsequent extensive prototype and memory layout changes for CoreNEURON
#
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#pragma once
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"
#include "coreneuron/sim/scopmath/errcodes.h"

namespace coreneuron {
namespace scopmath {
namespace sparse {
// Methods that may be called from offloaded regions are declared inline.
inline void delete_item(Item* item) {
    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->prev = nullptr;
    item->next = nullptr;
}

/*link ii before item*/
inline void linkitem(Item* item, Item* ii) {
    ii->prev = item->prev;
    ii->next = item;
    item->prev = ii;
    ii->prev->next = ii;
}

inline void insert(SparseObj* so, Item* item) {
    Item* ii{};
    for (ii = so->orderlist->next; ii != so->orderlist; ii = ii->next) {
        if (ii->norder >= item->norder) {
            break;
        }
    }
    linkitem(ii, item);
}

/* note: solution order refers to the following
        diag[varord[row]]->row = row = diag[varord[row]]->col
        rowst[varord[row]]->row = row
        varord[el->row] < varord[el->c_right->row]
        varord[el->col] < varord[el->r_down->col]
*/
inline void increase_order(SparseObj* so, unsigned row) {
    /* order of row increases by 1. Maintain the orderlist. */
    if (!so->do_flag)
        return;
    Item* order = so->roworder[row];
    delete_item(order);
    order->norder++;
    insert(so, order);
}

/**
 * Return pointer to (row, col) element maintaining order in rows.
 *
 * See check_assert in minorder for info about how this matrix is supposed to
 * look. If new_elem is nonzero and an element would otherwise be created, new
 * is used instead. This is because linking an element is highly nontrivial. The
 * biggest difference is that elements are no longer removed and this saves much
 * time allocating and freeing during the solve phase.
 */
template <enabled_code code_to_enable = enabled_code::all>
Elm* getelm(SparseObj* so, unsigned row, unsigned col, Elm* new_elem) {
    Elm *el, *elnext;

    unsigned vrow = so->varord[row];
    unsigned vcol = so->varord[col];

    if (vrow == vcol) {
        return so->diag[vrow]; /* a common case */
    }
    if (vrow > vcol) { /* in the lower triangle */
        /* search downward from diag[vcol] */
        for (el = so->diag[vcol];; el = elnext) {
            elnext = el->r_down;
            if (!elnext) {
                break;
            } else if (elnext->row == row) { /* found it */
                return elnext;
            } else if (so->varord[elnext->row] > vrow) {
                break;
            }
        }
        /* insert below el */
        if (!new_elem) {
            if constexpr (code_to_enable == enabled_code::compute_only) {
                // Dynamic allocation should not happen during the compute phase.
                assert(false);
            } else {
                new_elem = new Elm{};
                new_elem->value = new double[so->_cntml_padded];
                increase_order(so, row);
            }
        }
        new_elem->r_down = el->r_down;
        el->r_down = new_elem;
        new_elem->r_up = el;
        if (new_elem->r_down) {
            new_elem->r_down->r_up = new_elem;
        }
        /* search leftward from diag[vrow] */
        for (el = so->diag[vrow];; el = elnext) {
            elnext = el->c_left;
            if (!elnext) {
                break;
            } else if (so->varord[elnext->col] < vcol) {
                break;
            }
        }
        /* insert to left of el */
        new_elem->c_left = el->c_left;
        el->c_left = new_elem;
        new_elem->c_right = el;
        if (new_elem->c_left) {
            new_elem->c_left->c_right = new_elem;
        } else {
            so->rowst[vrow] = new_elem;
        }
    } else { /* in the upper triangle */
        /* search upward from diag[vcol] */
        for (el = so->diag[vcol];; el = elnext) {
            elnext = el->r_up;
            if (!elnext) {
                break;
            } else if (elnext->row == row) { /* found it */
                return elnext;
            } else if (so->varord[elnext->row] < vrow) {
                break;
            }
        }
        /* insert above el */
        if (!new_elem) {
            if constexpr (code_to_enable == enabled_code::compute_only) {
                assert(false);
            } else {
                new_elem = new Elm{};
                new_elem->value = new double[so->_cntml_padded];
                increase_order(so, row);
            }
        }
        new_elem->r_up = el->r_up;
        el->r_up = new_elem;
        new_elem->r_down = el;
        if (new_elem->r_up) {
            new_elem->r_up->r_down = new_elem;
        }
        /* search right from diag[vrow] */
        for (el = so->diag[vrow];; el = elnext) {
            elnext = el->c_right;
            if (!elnext) {
                break;
            } else if (so->varord[elnext->col] > vcol) {
                break;
            }
        }
        /* insert to right of el */
        new_elem->c_right = el->c_right;
        el->c_right = new_elem;
        new_elem->c_left = el;
        if (new_elem->c_right) {
            new_elem->c_right->c_left = new_elem;
        }
    }
    new_elem->row = row;
    new_elem->col = col;
    return new_elem;
}

/**
 * The following routines support the concept of a list. Modified from modl. The
 * list is a doubly linked list. A special item with element 0 is always at the
 * tail of the list and is denoted as the List pointer itself. list->next point
 * to the first item in the list and list->prev points to the last item in the
 * list. i.e. the list is circular. Note that in an empty list next and prev
 * points to itself.
 *
 * It is intended that this implementation be hidden from the user via the
 * following function calls.
 */
inline List* newlist() {
    auto* ii = new Item{};
    ii->prev = ii;
    ii->next = ii;
    return ii;
}

/*free the list but not the elements*/
inline void freelist(List* list) {
    Item* i2;
    for (Item* i1 = list->next; i1 != list; i1 = i2) {
        i2 = i1->next;
        delete i1;
    }
    delete list;
}

inline void check_assert(SparseObj* so) {
    /* check that all links are consistent */
    for (unsigned i = 1; i <= so->neqn; i++) {
        assert(so->diag[i]);
        assert(so->diag[i]->row == so->diag[i]->col);
        assert(so->varord[so->diag[i]->row] == i);
        assert(so->rowst[i]->row == so->diag[i]->row);
        for (Elm* el = so->rowst[i]; el; el = el->c_right) {
            if (el == so->rowst[i]) {
                assert(el->c_left == nullptr);
            } else {
                assert(el->c_left->c_right == el);
                assert(so->varord[el->c_left->col] < so->varord[el->col]);
            }
        }
        for (Elm* el = so->diag[i]->r_down; el; el = el->r_down) {
            assert(el->r_up->r_down == el);
            assert(so->varord[el->r_up->row] < so->varord[el->row]);
        }
        for (Elm* el = so->diag[i]->r_up; el; el = el->r_up) {
            assert(el->r_down->r_up == el);
            assert(so->varord[el->r_down->row] > so->varord[el->row]);
        }
    }
}

/* at this point row links are out of order for diag[i]->col
   and col links are out of order for diag[i]->row */
inline void re_link(SparseObj* so, unsigned i) {
    for (Elm* el = so->rowst[i]; el; el = el->c_right) {
        /* repair hole */
        if (el->r_up)
            el->r_up->r_down = el->r_down;
        if (el->r_down)
            el->r_down->r_up = el->r_up;
    }

    for (Elm* el = so->diag[i]->r_down; el; el = el->r_down) {
        /* repair hole */
        if (el->c_right)
            el->c_right->c_left = el->c_left;
        if (el->c_left)
            el->c_left->c_right = el->c_right;
        else
            so->rowst[so->varord[el->row]] = el->c_right;
    }

    for (Elm* el = so->diag[i]->r_up; el; el = el->r_up) {
        /* repair hole */
        if (el->c_right)
            el->c_right->c_left = el->c_left;
        if (el->c_left)
            el->c_left->c_right = el->c_right;
        else
            so->rowst[so->varord[el->row]] = el->c_right;
    }

    /* matrix is consistent except that diagonal row elements are unlinked from
    their columns and the diagonal column elements are unlinked from their
    rows.
    For simplicity discard all knowledge of links and use getelm to relink
    */
    Elm *dright, *dleft, *dup, *ddown, *elnext;

    so->rowst[i] = so->diag[i];
    dright = so->diag[i]->c_right;
    dleft = so->diag[i]->c_left;
    dup = so->diag[i]->r_up;
    ddown = so->diag[i]->r_down;
    so->diag[i]->c_right = so->diag[i]->c_left = nullptr;
    so->diag[i]->r_up = so->diag[i]->r_down = nullptr;
    for (Elm* el = dright; el; el = elnext) {
        elnext = el->c_right;
        getelm(so, el->row, el->col, el);
    }
    for (Elm* el = dleft; el; el = elnext) {
        elnext = el->c_left;
        getelm(so, el->row, el->col, el);
    }
    for (Elm* el = dup; el; el = elnext) {
        elnext = el->r_up;
        getelm(so, el->row, el->col, el);
    }
    for (Elm* el = ddown; el; el = elnext) {
        elnext = el->r_down;
        getelm(so, el->row, el->col, el);
    }
}

inline void free_elm(SparseObj* so) {
    /* free all elements */
    for (unsigned i = 1; i <= so->neqn; i++) {
        so->rowst[i] = nullptr;
        so->diag[i] = nullptr;
    }
}

inline void init_minorder(SparseObj* so) {
    /* matrix has been set up. Construct the orderlist and orderfind
       vector.
    */

    so->do_flag = 1;
    if (so->roworder) {
        for (unsigned i = 1; i <= so->nroworder; ++i) {
            delete so->roworder[i];
        }
        delete[] so->roworder;
    }
    so->roworder = new Item* [so->neqn + 1] {};
    so->nroworder = so->neqn;
    if (so->orderlist) {
        freelist(so->orderlist);
    }
    so->orderlist = newlist();
    for (unsigned i = 1; i <= so->neqn; i++) {
        so->roworder[i] = new Item{};
    }
    for (unsigned i = 1; i <= so->neqn; i++) {
        unsigned j = 0;
        for (auto el = so->rowst[i]; el; el = el->c_right) {
            j++;
        }
        so->roworder[so->diag[i]->row]->elm = so->diag[i];
        so->roworder[so->diag[i]->row]->norder = j;
        insert(so, so->roworder[so->diag[i]->row]);
    }
}

inline void reduce_order(SparseObj* so, unsigned row) {
    /* order of row decreases by 1. Maintain the orderlist. */

    if (!so->do_flag)
        return;
    Item* order = so->roworder[row];
    delete_item(order);
    order->norder--;
    insert(so, order);
}

inline void get_next_pivot(SparseObj* so, unsigned i) {
    /* get varord[i], etc. from the head of the orderlist. */
    Item* order = so->orderlist->next;
    assert(order != so->orderlist);

    unsigned j;
    if ((j = so->varord[order->elm->row]) != i) {
        /* push order lists down by 1 and put new diag in empty slot */
        assert(j > i);
        Elm* el = so->rowst[j];
        for (; j > i; j--) {
            so->diag[j] = so->diag[j - 1];
            so->rowst[j] = so->rowst[j - 1];
            so->varord[so->diag[j]->row] = j;
        }
        so->diag[i] = order->elm;
        so->rowst[i] = el;
        so->varord[so->diag[i]->row] = i;
        /* at this point row links are out of order for diag[i]->col
           and col links are out of order for diag[i]->row */
        re_link(so, i);
    }

    /* now make sure all needed elements exist */
    for (Elm* el = so->diag[i]->r_down; el; el = el->r_down) {
        for (Elm* pivot = so->diag[i]->c_right; pivot; pivot = pivot->c_right) {
            getelm(so, el->row, pivot->col, nullptr);
        }
        reduce_order(so, el->row);
    }
    delete_item(order);
}

/* reallocate space for matrix */
inline void initeqn(SparseObj* so, unsigned maxeqn) {
    if (maxeqn == so->neqn)
        return;
    free_elm(so);
    so->neqn = maxeqn;
    delete[] so->rowst;
    delete[] so->diag;
    delete[] so->varord;
    delete[] so->rhs;
    delete[] so->ngetcall;
    so->elmpool = nullptr;
    so->rowst = new Elm*[maxeqn + 1];
    so->diag = new Elm*[maxeqn + 1];
    so->varord = new unsigned[maxeqn + 1];
    so->rhs = new double[(maxeqn + 1) * so->_cntml_padded];
    so->ngetcall = new unsigned[so->_cntml_padded];
    for (unsigned i = 1; i <= maxeqn; i++) {
        so->varord[i] = i;
        so->diag[i] = new Elm{};
        so->diag[i]->value = new double[so->_cntml_padded];
        so->rowst[i] = so->diag[i];
        so->diag[i]->row = i;
        so->diag[i]->col = i;
        so->diag[i]->r_down = so->diag[i]->r_up = nullptr;
        so->diag[i]->c_right = so->diag[i]->c_left = nullptr;
    }
    unsigned nn = so->neqn * so->_cntml_padded;
    for (unsigned i = 0; i < nn; ++i) {
        so->rhs[i] = 0.;
    }
}

/**
 * Minimum ordering algorithm to determine the order that the matrix should be
 * solved. Also make sure all needed elements are present. This does not mess up
 * the matrix.
 */
inline void spar_minorder(SparseObj* so) {
    check_assert(so);
    init_minorder(so);
    for (unsigned i = 1; i <= so->neqn; i++) {
        get_next_pivot(so, i);
    }
    so->do_flag = 0;
    check_assert(so);
}

inline void init_coef_list(SparseObj* so, int _iml) {
    so->ngetcall[_iml] = 0;
    for (unsigned i = 1; i <= so->neqn; i++) {
        for (Elm* el = so->rowst[i]; el; el = el->c_right) {
            el->value[_iml] = 0.;
        }
    }
}

#if defined(scopmath_sparse_d) || defined(scopmath_sparse_ix) || defined(scopmath_sparse_s) || \
    defined(scopmath_sparse_x)
#error "naming clash on sparse_thread.hpp-internal macros"
#endif
#define scopmath_sparse_ix(arg) CNRN_FLAT_INDEX_IML_ROW(arg)
inline void subrow(SparseObj* so, Elm* pivot, Elm* rowsub, int _iml) {
    unsigned int const _cntml_padded{so->_cntml_padded};
    double const r{rowsub->value[_iml] / pivot->value[_iml]};
    so->rhs[scopmath_sparse_ix(rowsub->row)] -= so->rhs[scopmath_sparse_ix(pivot->row)] * r;
    so->numop++;
    for (auto el = pivot->c_right; el; el = el->c_right) {
        for (rowsub = rowsub->c_right; rowsub->col != el->col; rowsub = rowsub->c_right) {
        }
        rowsub->value[_iml] -= el->value[_iml] * r;
        so->numop++;
    }
}

inline void bksub(SparseObj* so, int _iml) {
    int _cntml_padded = so->_cntml_padded;
    for (unsigned i = so->neqn; i >= 1; i--) {
        for (Elm* el = so->diag[i]->c_right; el; el = el->c_right) {
            so->rhs[scopmath_sparse_ix(el->row)] -= el->value[_iml] *
                                                    so->rhs[scopmath_sparse_ix(el->col)];
            so->numop++;
        }
        so->rhs[scopmath_sparse_ix(so->diag[i]->row)] /= so->diag[i]->value[_iml];
        so->numop++;
    }
}

inline int matsol(SparseObj* so, int _iml) {
    /* Upper triangularization */
    so->numop = 0;
    for (unsigned i = 1; i <= so->neqn; i++) {
        Elm* pivot{so->diag[i]};
        if (fabs(pivot->value[_iml]) <= ROUNDOFF) {
            return SINGULAR;
        }
        // Eliminate all elements in pivot column. The OpenACC annotation here
        // is to avoid problems with nvc++'s automatic paralellisation; see:
        // https://forums.developer.nvidia.com/t/device-kernel-hangs-at-o-and-above/212733
        nrn_pragma_acc(loop seq)
        for (auto el = pivot->r_down; el; el = el->r_down) {
            subrow(so, pivot, el, _iml);
        }
    }
    bksub(so, _iml);
    return SUCCESS;
}

template <typename SPFUN>
void create_coef_list(SparseObj* so, int n, SPFUN fun, _threadargsproto_) {
    initeqn(so, (unsigned) n);
    so->phase = 1;
    so->ngetcall[0] = 0;
    fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
    if (so->coef_list) {
        free(so->coef_list);
    }
    so->coef_list_size = so->ngetcall[0];
    so->coef_list = new double*[so->coef_list_size];
    spar_minorder(so);
    so->phase = 2;
    so->ngetcall[0] = 0;
    fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
    so->phase = 0;
}

template <enabled_code code_to_enable = enabled_code::all>
double* thread_getelm(SparseObj* so, int row, int col, int _iml) {
    if (!so->phase) {
        return so->coef_list[so->ngetcall[_iml]++];
    }
    Elm* el = scopmath::sparse::getelm<code_to_enable>(so, (unsigned) row, (unsigned) col, nullptr);
    if (so->phase == 1) {
        so->ngetcall[_iml]++;
    } else {
        so->coef_list[so->ngetcall[_iml]++] = el->value;
    }
    return el->value;
}
}  // namespace sparse
}  // namespace scopmath

// Methods that may be called from translated MOD files are kept outside the
// scopmath::sparse namespace.
#define scopmath_sparse_s(arg) _p[scopmath_sparse_ix(s[arg])]
#define scopmath_sparse_d(arg) _p[scopmath_sparse_ix(d[arg])]

/**
 * sparse matrix dynamic allocation: create_coef_list makes a list for fast
 * setup, does minimum ordering and ensures all elements needed are present.
 * This could easily be made recursive but it isn't right now.
 */
template <typename SPFUN>
void* nrn_cons_sparseobj(SPFUN fun, int n, Memb_list* ml, _threadargsproto_) {
    // fill in the unset _threadargsproto_ assuming _iml = 0;
    _iml = 0; /* from _threadargsproto_ */
    _p = ml->data;
    _ppvar = ml->pdata;
    _v = _nt->_actual_v[ml->nodeindices[_iml]];
    SparseObj* so{new SparseObj};
    so->_cntml_padded = _cntml_padded;
    scopmath::sparse::create_coef_list(so, n, fun, _threadargs_);
    nrn_sparseobj_copyto_device(so);
    return so;
}

/**
 * This is an experimental numerical method for SCoP-3 which integrates kinetic
 * rate equations.  It is intended to be used only by models generated by MODL,
 * and its identity is meant to be concealed from the user.
 *
 * @param n number of state variables
 * @param s array of pointers to the state variables
 * @param d array of pointers to the derivatives of states
 * @param t pointer to the independent variable
 * @param dt the time step
 * @param fun callable corresponding to the kinetic block equations
 * @param prhs pointer to right hand side vector (answer on return) does not
 *             have to be allocated by caller. (this is no longer quite right)
 * @param linflag solve as linear equations, when nonlinear, all states are
 *                forced >= 0
 */
template <typename F>
int sparse_thread(SparseObj* so,
                  int n,
                  int* s,
                  int* d,
                  double* t,
                  double dt,
                  F fun,
                  int linflag,
                  _threadargsproto_) {
    int i, j, ierr;
    double err;

    for (i = 0; i < n; i++) { /*save old state*/
        scopmath_sparse_d(i) = scopmath_sparse_s(i);
    }
    for (err = 1, j = 0; err > CONVERGE; j++) {
        scopmath::sparse::init_coef_list(so, _iml);
        fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
        if ((ierr = scopmath::sparse::matsol(so, _iml))) {
            return ierr;
        }
        for (err = 0., i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
            scopmath_sparse_s(i - 1) += so->rhs[scopmath_sparse_ix(i)];
            if (!linflag && scopmath_sparse_s(i - 1) < 0.) {
                scopmath_sparse_s(i - 1) = 0.;
            }
            err += fabs(so->rhs[scopmath_sparse_ix(i)]);
        }
        if (j > MAXSTEPS) {
            return EXCEED_ITERS;
        }
        if (linflag)
            break;
    }
    scopmath::sparse::init_coef_list(so, _iml);
    fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
    for (i = 0; i < n; i++) {        /*restore Dstate at t+dt*/
        scopmath_sparse_d(i) = (scopmath_sparse_s(i) - scopmath_sparse_d(i)) / dt;
    }
    return SUCCESS;
}
#undef scopmath_sparse_d
#undef scopmath_sparse_ix
#undef scopmath_sparse_s
#define scopmath_sparse_x(arg) _p[CNRN_FLAT_INDEX_IML_ROW(x[arg])]
/* for solving ax=b */
template <typename SPFUN>
int _cvode_sparse_thread(void** vpr, int n, int* x, SPFUN fun, _threadargsproto_) {
    SparseObj* so = (SparseObj*) (*vpr);
    if (!so) {
        so = new SparseObj{};
        *vpr = so;
    }
    scopmath::sparse::create_coef_list(so, n, fun, _threadargs_); /* calls fun twice */
    scopmath::sparse::init_coef_list(so, _iml);
    fun(so, so->rhs, _threadargs_);  // std::invoke in C++17
    int ierr;
    if ((ierr = scopmath::sparse::matsol(so, _iml))) {
        return ierr;
    }
    for (int i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
        scopmath_sparse_x(i - 1) = so->rhs[i];
    }
    return SUCCESS;
}
#undef scopmath_sparse_x

inline void _nrn_destroy_sparseobj_thread(SparseObj* so) {
    if (!so) {
        return;
    }
    nrn_sparseobj_delete_from_device(so);
    delete[] so->rowst;
    delete[] so->diag;
    delete[] so->varord;
    delete[] so->rhs;
    delete[] so->coef_list;
    if (so->roworder) {
        for (int ii = 1; ii <= so->nroworder; ++ii) {
            delete so->roworder[ii];
        }
        delete[] so->roworder;
    }
    if (so->orderlist) {
        scopmath::sparse::freelist(so->orderlist);
    }
    delete so;
}
}  // namespace coreneuron
