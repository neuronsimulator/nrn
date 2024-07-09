/** @file sparse_thread.hpp
 *  @copyright (c) 1989-90 Duke University
 *
 *  4/23/93 converted to object so many models can use it
 *  Jan 2008 thread safe
 */
#pragma once
#include "hocdec.h"  // emalloc
#include "errcodes.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// avoid including nrniv_mf.h becausee of #define hackery
void* nrn_pool_alloc(void* pool);
void* nrn_pool_create(long count, int itemsize);
void nrn_pool_delete(void* pool);
void nrn_pool_freeall(void* pool);

namespace neuron::scopmath {
// lots of helpers in this header; hide them in another namespace
namespace detail::sparse_thread {
struct Elm {
    unsigned row; /* Row location */
    unsigned col; /* Column location */
    double value; /* The value */
    Elm* r_up;    /* Link to element in same column */
    Elm* r_down;  /* 	in solution order */
    Elm* c_left;  /* Link to left element in same row */
    Elm* c_right; /*	in solution order (see getelm) */
};
struct Item {
    Elm* elm;
    unsigned norder; /* order of a row */
    Item* next;
    Item* prev;
};
using List = Item; /* list of mixed items */
}  // namespace detail::sparse_thread
struct SparseObj {                      /* all the state information */
    detail::sparse_thread::Elm** rowst; /* link to first element in row (solution order)*/
    detail::sparse_thread::Elm** diag;  /* link to pivot element in row (solution order)*/
    void* elmpool;                      /* no interthread cache line sharing for elements */
    unsigned neqn;                      /* number of equations */
    unsigned* varord;                   /* row and column order for pivots */
    double* rhs;                        /* initially- right hand side	finally - answer */
    void* oldfun;      /* not completely portable as a way of storing function pointers
                        */
    unsigned ngetcall; /* counter for number of calls to _wgetelm */
    int phase;         /* 0-solution phase; 1-count phase; 2-build list phase */
    int numop;
    double** coef_list; /* pointer to value in _getelm order */
    /* don't really need the rest */
    int nroworder;                          /* just for freeing */
    detail::sparse_thread::Item** roworder; /* roworder[i] is pointer to order
                 item for row i. Does not have to be in orderlist */
    detail::sparse_thread::List* orderlist; /* list of rows sorted by norder
                 that haven't been used */
    int do_flag;
};
namespace detail::sparse_thread {
/* note: solution order refers to the following
    diag[varord[row]]->row = row = diag[varord[row]]->col
    rowst[varord[row]]->row = row
    varord[el->row] < varord[el->c_right->row]
    varord[el->col] < varord[el->r_down->col]
*/

/* sparse matrix dynamic allocation:
create_coef_list makes a list for fast setup, does minimum ordering and
ensures all elements needed are present */
/* this could easily be made recursive but it isn't right now */

inline __attribute__((always_inline)) void subrow(SparseObj* so, Elm* pivot, Elm* rowsub) {
    double r;
    Elm* el;

    r = rowsub->value / pivot->value;
    so->rhs[rowsub->row] -= so->rhs[pivot->row] * r;
    so->numop++;
    for (el = pivot->c_right; el; el = el->c_right) {
        for (rowsub = rowsub->c_right; rowsub->col != el->col; rowsub = rowsub->c_right) {
            ;
        }
        rowsub->value -= el->value * r;
        so->numop++;
    }
}

inline __attribute__((always_inline)) void bksub(SparseObj* so) {
    unsigned i;
    Elm* el;

    for (i = so->neqn; i >= 1; i--) {
        for (el = so->diag[i]->c_right; el; el = el->c_right) {
            so->rhs[el->row] -= el->value * so->rhs[el->col];
            so->numop++;
        }
        so->rhs[so->diag[i]->row] /= so->diag[i]->value;
        so->numop++;
    }
}

inline __attribute__((always_inline)) int matsol(SparseObj* so) {
    Elm *pivot, *el;
    unsigned i;

    /* Upper triangularization */
    so->numop = 0;
    for (i = 1; i <= so->neqn; i++) {
        if (fabs((pivot = so->diag[i])->value) <= ROUNDOFF) {
            return SINGULAR;
        }
        /* Eliminate all elements in pivot column */
        for (el = pivot->r_down; el; el = el->r_down) {
            subrow(so, pivot, el);
        }
    }
    bksub(so);
    return (SUCCESS);
}

inline void prmat(SparseObj* so) {
    unsigned i, j;
    Elm* el;

    printf("\n        ");
    for (i = 10; i <= so->neqn; i += 10)
        printf("         %1d", (i % 100) / 10);
    printf("\n        ");
    for (i = 1; i <= so->neqn; i++)
        printf("%1d", i % 10);
    printf("\n\n");
    for (i = 1; i <= so->neqn; i++) {
        printf("%3d %3d ", so->diag[i]->row, i);
        j = 0;
        for (el = so->rowst[i]; el; el = el->c_right) {
            for (j++; j < so->varord[el->col]; j++)
                printf(" ");
            printf("*");
        }
        printf("\n");
    }
    fflush(stdin);
}

inline void free_elm(SparseObj* so) {
    unsigned i;
    Elm *el, *elnext;

    /* free all elements */
    nrn_pool_freeall(so->elmpool);
    for (i = 1; i <= so->neqn; i++) {
        so->rowst[i] = nullptr;
        so->diag[i] = nullptr;
    }
}

/* reallocate space for matrix */
inline void initeqn(SparseObj* so, unsigned maxeqn) {
    unsigned i;

    if (maxeqn == so->neqn)
        return;
    free_elm(so);
    if (so->rowst)
        free(so->rowst);
    if (so->diag)
        free(so->diag);
    if (so->varord)
        free(so->varord);
    if (so->rhs)
        free(so->rhs);
    so->rowst = so->diag = (Elm**) 0;
    so->varord = (unsigned*) 0;
    so->rowst = (Elm**) emalloc((maxeqn + 1) * sizeof(Elm*));
    so->diag = (Elm**) emalloc((maxeqn + 1) * sizeof(Elm*));
    so->varord = (unsigned*) emalloc((maxeqn + 1) * sizeof(unsigned));
    so->rhs = (double*) emalloc((maxeqn + 1) * sizeof(double));
    for (i = 1; i <= maxeqn; i++) {
        so->varord[i] = i;
        so->diag[i] = (Elm*) nrn_pool_alloc(so->elmpool);
        so->rowst[i] = so->diag[i];
        so->diag[i]->row = i;
        so->diag[i]->col = i;
        so->diag[i]->r_down = so->diag[i]->r_up = nullptr;
        so->diag[i]->c_right = so->diag[i]->c_left = nullptr;
        so->diag[i]->value = 0.;
        so->rhs[i] = 0.;
    }
    so->neqn = maxeqn;
}

inline void deletion(Item* item) {
    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->prev = nullptr;
    item->next = nullptr;
}

/*link i before item*/
inline void linkitem(Item* item, Item* i) {
    i->prev = item->prev;
    i->next = item;
    item->prev = i;
    i->prev->next = i;
}

inline void insert(SparseObj* so, Item* item) {
    Item* i;

    for (i = so->orderlist->next; i != so->orderlist; i = i->next) {
        if (i->norder >= item->norder) {
            break;
        }
    }
    linkitem(i, item);
}

inline void increase_order(SparseObj* so, unsigned row) {
    /* order of row increases by 1. Maintain the orderlist. */
    Item* order;

    if (!so->do_flag)
        return;
    order = so->roworder[row];
    deletion(order);
    order->norder++;
    insert(so, order);
}

/* see check_assert in minorder for info about how this matrix is supposed
to look.  In new is nonzero and an element would otherwise be created, new
is used instead. This is because linking an element is highly nontrivial
The biggest difference is that elements are no longer removed and this
saves much time allocating and freeing during the solve phase
*/

/* return pointer to row col element maintaining order in rows */
inline __attribute__((always_inline)) Elm* getelm(SparseObj* so,
                                                  unsigned row,
                                                  unsigned col,
                                                  Elm* newElm) {
    Elm *el, *elnext;
    unsigned vrow, vcol;

    vrow = so->varord[row];
    vcol = so->varord[col];

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
        if (!newElm) {
            newElm = (Elm*) nrn_pool_alloc(so->elmpool);
            newElm->value = 0.;
            increase_order(so, row);
        }
        newElm->r_down = el->r_down;
        el->r_down = newElm;
        newElm->r_up = el;
        if (newElm->r_down) {
            newElm->r_down->r_up = newElm;
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
        newElm->c_left = el->c_left;
        el->c_left = newElm;
        newElm->c_right = el;
        if (newElm->c_left) {
            newElm->c_left->c_right = newElm;
        } else {
            so->rowst[vrow] = newElm;
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
        if (!newElm) {
            newElm = (Elm*) nrn_pool_alloc(so->elmpool);
            newElm->value = 0.;
            increase_order(so, row);
        }
        newElm->r_up = el->r_up;
        el->r_up = newElm;
        newElm->r_down = el;
        if (newElm->r_up) {
            newElm->r_up->r_down = newElm;
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
        newElm->c_right = el->c_right;
        el->c_right = newElm;
        newElm->c_left = el;
        if (newElm->c_right) {
            newElm->c_right->c_left = newElm;
        }
    }
    newElm->row = row;
    newElm->col = col;
    return newElm;
}

inline void check_assert(SparseObj* so) {
    /* check that all links are consistent */
    unsigned i;
    Elm* el;

    for (i = 1; i <= so->neqn; i++) {
        assert(so->diag[i]);
        assert(so->diag[i]->row == so->diag[i]->col);
        assert(so->varord[so->diag[i]->row] == i);
        assert(so->rowst[i]->row == so->diag[i]->row);
        for (el = so->rowst[i]; el; el = el->c_right) {
            if (el == so->rowst[i]) {
                assert(el->c_left == nullptr);
            } else {
                assert(el->c_left->c_right == el);
                assert(so->varord[el->c_left->col] < so->varord[el->col]);
            }
        }
        for (el = so->diag[i]->r_down; el; el = el->r_down) {
            assert(el->r_up->r_down == el);
            assert(so->varord[el->r_up->row] < so->varord[el->row]);
        }
        for (el = so->diag[i]->r_up; el; el = el->r_up) {
            assert(el->r_down->r_up == el);
            assert(so->varord[el->r_down->row] > so->varord[el->row]);
        }
    }
}

/* The following routines support the concept of a list.
modified from modl
*/

/* Implementation
  The list is a doubly linked list. A special item with element 0 is
  always at the tail of the list and is denoted as the List pointer itself.
  list->next point to the first item in the list and
  list->prev points to the last item in the list.
    i.e. the list is circular
  Note that in an empty list next and prev points to itself.

It is intended that this implementation be hidden from the user via the
following function calls.
*/

inline Item* newitem() {
    Item* i;
    i = (Item*) emalloc(sizeof(Item));
    i->prev = nullptr;
    i->next = nullptr;
    i->norder = 0;
    i->elm = (Elm*) 0;
    return i;
}

inline List* newlist() {
    Item* i;
    i = newitem();
    i->prev = i;
    i->next = i;
    return (List*) i;
}

/*free the list but not the elements*/
inline void freelist(List* list) {
    Item *i1, *i2;
    for (i1 = list->next; i1 != list; i1 = i2) {
        i2 = i1->next;
        free(i1);
    }
    free(list);
}

inline void init_minorder(SparseObj* so) {
    /* matrix has been set up. Construct the orderlist and orderfind
       vector.
    */
    unsigned i, j;
    Elm* el;

    so->do_flag = 1;
    if (so->roworder) {
        for (i = 1; i <= so->nroworder; ++i) {
            free(so->roworder[i]);
        }
        free(so->roworder);
    }
    so->roworder = (Item**) emalloc((so->neqn + 1) * sizeof(Item*));
    so->nroworder = so->neqn;
    if (so->orderlist)
        freelist(so->orderlist);
    so->orderlist = newlist();
    for (i = 1; i <= so->neqn; i++) {
        so->roworder[i] = newitem();
    }
    for (i = 1; i <= so->neqn; i++) {
        for (j = 0, el = so->rowst[i]; el; el = el->c_right) {
            j++;
        }
        so->roworder[so->diag[i]->row]->elm = so->diag[i];
        so->roworder[so->diag[i]->row]->norder = j;
        insert(so, so->roworder[so->diag[i]->row]);
    }
}

/* at this point row links are out of order for diag[i]->col
   and col links are out of order for diag[i]->row */
inline void re_link(SparseObj* so, unsigned i) {
    Elm *el, *dright, *dleft, *dup, *ddown, *elnext;

    for (el = so->rowst[i]; el; el = el->c_right) {
        /* repair hole */
        if (el->r_up)
            el->r_up->r_down = el->r_down;
        if (el->r_down)
            el->r_down->r_up = el->r_up;
    }

    for (el = so->diag[i]->r_down; el; el = el->r_down) {
        /* repair hole */
        if (el->c_right)
            el->c_right->c_left = el->c_left;
        if (el->c_left)
            el->c_left->c_right = el->c_right;
        else
            so->rowst[so->varord[el->row]] = el->c_right;
    }

    for (el = so->diag[i]->r_up; el; el = el->r_up) {
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
    so->rowst[i] = so->diag[i];
    dright = so->diag[i]->c_right;
    dleft = so->diag[i]->c_left;
    dup = so->diag[i]->r_up;
    ddown = so->diag[i]->r_down;
    so->diag[i]->c_right = so->diag[i]->c_left = nullptr;
    so->diag[i]->r_up = so->diag[i]->r_down = nullptr;
    for (el = dright; el; el = elnext) {
        elnext = el->c_right;
        getelm(so, el->row, el->col, el);
    }
    for (el = dleft; el; el = elnext) {
        elnext = el->c_left;
        getelm(so, el->row, el->col, el);
    }
    for (el = dup; el; el = elnext) {
        elnext = el->r_up;
        getelm(so, el->row, el->col, el);
    }
    for (el = ddown; el; el = elnext) {
        elnext = el->r_down;
        getelm(so, el->row, el->col, el);
    }
}

inline void reduce_order(SparseObj* so, unsigned row) {
    /* order of row decreases by 1. Maintain the orderlist. */
    Item* order;

    if (!so->do_flag)
        return;
    order = so->roworder[row];
    deletion(order);
    order->norder--;
    insert(so, order);
}

inline __attribute__((always_inline)) void get_next_pivot(SparseObj* so, unsigned i) {
    /* get varord[i], etc. from the head of the orderlist. */
    Item* order;
    Elm *pivot, *el;
    unsigned j;

    order = so->orderlist->next;
    assert(order != so->orderlist);

    if ((j = so->varord[order->elm->row]) != i) {
        /* push order lists down by 1 and put new diag in empty slot */
        assert(j > i);
        el = so->rowst[j];
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
    for (el = so->diag[i]->r_down; el; el = el->r_down) {
        for (pivot = so->diag[i]->c_right; pivot; pivot = pivot->c_right) {
            getelm(so, el->row, pivot->col, nullptr);
        }
        reduce_order(so, el->row);
    }
    deletion(order);
}

/* Minimum ordering algorithm to determine the order
            that the matrix should be solved. Also make sure
            all needed elements are present.
            This does not mess up the matrix
        */
inline void spar_minorder(SparseObj* so) {
    unsigned i;

    check_assert(so);
    init_minorder(so);
    for (i = 1; i <= so->neqn; i++) {
        get_next_pivot(so, i);
    }
    so->do_flag = 0;
    check_assert(so);
}

template <typename Callable, typename... Args>
void create_coef_list(SparseObj* so, int n, Callable fun, Args&&... args) {
    initeqn(so, (unsigned) n);
    so->phase = 1;
    so->ngetcall = 0;
    fun(so, so->rhs, args...);
    if (so->coef_list) {
        free(so->coef_list);
    }
    so->coef_list = (double**) emalloc(so->ngetcall * sizeof(double*));
    spar_minorder(so);
    so->phase = 2;
    so->ngetcall = 0;
    fun(so, so->rhs, std::forward<Args>(args)...);
    so->phase = 0;
}

inline __attribute__((always_inline)) void init_coef_list(SparseObj* so) {
    unsigned i;
    Elm* el;

    so->ngetcall = 0;
    for (i = 1; i <= so->neqn; i++) {
        for (el = so->rowst[i]; el; el = el->c_right) {
            el->value = 0.;
        }
    }
}

inline SparseObj* create_sparseobj() {
    SparseObj* so;

    so = (SparseObj*) emalloc(sizeof(SparseObj));
    so->elmpool = nrn_pool_create(100, sizeof(Elm));
    so->rowst = 0;
    so->diag = 0;
    so->neqn = 0;
    so->varord = 0;
    so->rhs = 0;
    so->oldfun = 0;
    so->ngetcall = 0;
    so->phase = 0;
    so->coef_list = 0;
    so->roworder = 0;
    so->nroworder = 0;
    so->orderlist = 0;
    so->do_flag = 0;

    return so;
}
}  // namespace detail::sparse_thread

/**
 * This is an experimental numerical method for SCoP-3 which integrates kinetic
 * rate equations. It is intended to be used only by models generated by MODL,
 * and its identity is meant to be concealed from the user.
 *
 * @param n number of state variables
 * @param s array of pointers to the state variables
 * @param d array of pointers to the derivatives of states
 * @param t pointer to the independent variable
 * @param dt the time step
 * @param fun pointer to the function corresponding to the kinetic block
 * equations
 * @param prhs pointer to right hand side vector (answer on return) does not
 * have to be allocated by caller.
 * @param linflag solve as linear equations when nonlinear, all states are
 * forced >= 0
 */
template <typename Array, typename Callable, typename IndexArray, typename... Args>
int sparse_thread(void** v,
                  int n,
                  IndexArray s,
                  IndexArray d,
                  Array p,
                  double* t,
                  double dt,
                  Callable fun,
                  int linflag,
                  Args&&... args) {
    auto const s_ = [&p, &s](auto arg) -> auto& {
        return p[s[arg]];
    };
    auto const d_ = [&p, &d](auto arg) -> auto& {
        return p[d[arg]];
    };
    int i, j, ierr;
    double err;
    auto* so = static_cast<SparseObj*>(*v);
    if (!so) {
        so = detail::sparse_thread::create_sparseobj();
        *v = so;
    }
    static_assert(sizeof(void*) == sizeof(fun), "This code assumes function pointers fit in void*");
    if (so->oldfun != reinterpret_cast<void*>(fun)) {
        so->oldfun = reinterpret_cast<void*>(fun);
        // calls fun twice
        detail::sparse_thread::create_coef_list(so, n, fun, args...);
    }
    for (i = 0; i < n; i++) { /*save old state*/
        d_(i) = s_(i);
    }
    for (err = 1, j = 0; err > CONVERGE; j++) {
        detail::sparse_thread::init_coef_list(so);
        fun(so, so->rhs, args...);
        if ((ierr = detail::sparse_thread::matsol(so))) {
            return ierr;
        }
        for (err = 0., i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
            s_(i - 1) += so->rhs[i];
            /* stability of nonlinear kinetic schemes sometimes requires this */
            if (!linflag && s_(i - 1) < 0.) {
                s_(i - 1) = 0.;
            }
            err += fabs(so->rhs[i]);
        }
        if (j > MAXSTEPS) {
            return EXCEED_ITERS;
        }
        if (linflag)
            break;
    }
    detail::sparse_thread::init_coef_list(so);
    fun(so, so->rhs, std::forward<Args>(args)...);
    for (i = 0; i < n; i++) { /*restore Dstate at t+dt*/
        d_(i) = (s_(i) - d_(i)) / dt;
    }
    return SUCCESS;
}

/* for solving ax=b */
template <typename Array, typename Callable, typename IndexArray, typename... Args>
int _cvode_sparse_thread(void** v, int n, IndexArray x, Array p, Callable fun, Args&&... args) {
    auto const x_ = [&p, &x](auto arg) -> auto& {
        return p[x[arg]];
    };
    int i, j, ierr;
    SparseObj* so;

    so = (SparseObj*) (*v);
    if (!so) {
        so = detail::sparse_thread::create_sparseobj();
        *v = (void*) so;
    }
    static_assert(sizeof(void*) == sizeof(fun), "This code assumes function pointers fit in void*");
    if (so->oldfun != reinterpret_cast<void*>(fun)) {
        so->oldfun = reinterpret_cast<void*>(fun);
        // calls fun twice
        detail::sparse_thread::create_coef_list(so, n, fun, args...);
    }
    detail::sparse_thread::init_coef_list(so);
    fun(so, so->rhs, std::forward<Args>(args)...);
    if ((ierr = detail::sparse_thread::matsol(so))) {
        return ierr;
    }
    for (i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
        x_(i - 1) = so->rhs[i];
    }
    return SUCCESS;
}

inline __attribute__((always_inline)) double* _nrn_thread_getelm(SparseObj* so, int row, int col) {
    detail::sparse_thread::Elm* el;
    if (!so->phase) {
        return so->coef_list[so->ngetcall++];
    }
    el = detail::sparse_thread::getelm(so, (unsigned) row, (unsigned) col, nullptr);
    if (so->phase == 1) {
        so->ngetcall++;
    } else {
        so->coef_list[so->ngetcall++] = &el->value;
    }
    return &el->value;
}
inline void _nrn_destroy_sparseobj_thread(SparseObj* so) {
    int i;
    if (!so) {
        return;
    }
    nrn_pool_delete(so->elmpool);
    if (so->rowst)
        free(so->rowst);
    if (so->diag)
        free(so->diag);
    if (so->varord)
        free(so->varord);
    if (so->rhs)
        free(so->rhs);
    if (so->coef_list)
        free(so->coef_list);
    if (so->roworder) {
        for (i = 1; i <= so->nroworder; ++i) {
            free(so->roworder[i]);
        }
        free(so->roworder);
    }
    if (so->orderlist)
        freelist(so->orderlist);
    free(so);
}
}  // namespace neuron::scopmath
using neuron::scopmath::_cvode_sparse_thread;
using neuron::scopmath::_nrn_destroy_sparseobj_thread;
using neuron::scopmath::_nrn_thread_getelm;
using neuron::scopmath::sparse_thread;
using neuron::scopmath::SparseObj;
