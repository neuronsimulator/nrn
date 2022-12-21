/** @file sparse.hpp
 *  @copyright (c) 1989-90 Duke University
 *
 *  4/23/93 converted to object so many models can use it
 */
#pragma once
#include "errcodes.hpp"
#include "hocdec.h"
#include "scoplib.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>


namespace neuron::scopmath {
// This file has a lot of helper functions, so nest them deeper than the other stuff
namespace detail::sparse {
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
using List = Item;  // list of mixed items

struct SparseObj { /* all the state information */
    Elm** rowst;
    Elm** diag;
    unsigned neqn;
    unsigned* varord;
    int (*oldfun)();
    unsigned ngetcall;
    int phase;
    double** coef_list;
    /* don't really need the rest */
    int nroworder;
    Item** roworder;
    List* orderlist;
    int do_flag;
};

inline SparseObj* old_sparseobj;
inline SparseObj* create_sparseobj() {
    SparseObj* so = (SparseObj*) emalloc(sizeof(SparseObj));
    so->rowst = 0;
    so->diag = 0;
    so->neqn = 0;
    so->varord = 0;
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

inline Elm** rowst;        /* link to first element in row (solution order)*/
inline Elm** diag;         /* link to pivot element in row (solution order)*/
inline unsigned neqn;      /* number of equations */
inline unsigned* varord;   /* row and column order for pivots */
inline unsigned ngetcall;  /* counter for number of calls to _getelm */
inline int phase;          /* 0-solution phase; 1-count phase; 2-build list phase */
inline double** coef_list; /* pointer to value in _getelm order */
inline Item** roworder;    /* roworder[i] is pointer to order item for row i. Does not have to be in
                              orderlist */
inline int nroworder;      /* just for freeing */
inline List* orderlist;    /* list of rows sorted by norder that haven't been used */
inline int do_flag;
inline void sparseobj2local(SparseObj* so) {
    rowst = so->rowst;
    diag = so->diag;
    neqn = so->neqn;
    varord = so->varord;
    ngetcall = so->ngetcall;
    phase = so->phase;
    coef_list = so->coef_list;
    roworder = so->roworder;
    nroworder = so->nroworder;
    orderlist = so->orderlist;
    do_flag = so->do_flag;
}
inline void local2sparseobj(SparseObj* so) {
    so->rowst = rowst;
    so->diag = diag;
    so->neqn = neqn;
    so->varord = varord;
    so->ngetcall = ngetcall;
    so->phase = phase;
    so->coef_list = coef_list;
    so->roworder = roworder;
    so->nroworder = nroworder;
    so->orderlist = orderlist;
    so->do_flag = do_flag;
}
inline double* rhs; /* initially- right hand side	finally - answer */
inline int numop;

inline void subrow(Elm* pivot, Elm* rowsub) {
    double r;
    Elm* el;

    r = rowsub->value / pivot->value;
    rhs[rowsub->row] -= rhs[pivot->row] * r;
    numop++;
    for (el = pivot->c_right; el; el = el->c_right) {
        for (rowsub = rowsub->c_right; rowsub->col != el->col; rowsub = rowsub->c_right) {
            ;
        }
        rowsub->value -= el->value * r;
        numop++;
    }
}
inline void bksub() {
    unsigned i;
    Elm* el;

    for (i = neqn; i >= 1; i--) {
        for (el = diag[i]->c_right; el; el = el->c_right) {
            rhs[el->row] -= el->value * rhs[el->col];
            numop++;
        }
        rhs[diag[i]->row] /= diag[i]->value;
        numop++;
    }
}
inline int matsol() {
    Elm *pivot, *el;
    unsigned i;

    /* Upper triangularization */
    numop = 0;
    for (i = 1; i <= neqn; i++) {
        if (fabs((pivot = diag[i])->value) <= ROUNDOFF) {
            return SINGULAR;
        }
        /* Eliminate all elements in pivot column */
        for (el = pivot->r_down; el; el = el->r_down) {
            subrow(pivot, el);
        }
    }
    bksub();
    return (SUCCESS);
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

inline void insert(Item* item) {
    Item* i;

    for (i = orderlist->next; i != orderlist; i = i->next) {
        if (i->norder >= item->norder) {
            break;
        }
    }
    linkitem(i, item);
}
inline void increase_order(unsigned row) {
    /* order of row increases by 1. Maintain the orderlist. */
    Item* order;

    if (!do_flag)
        return;
    order = roworder[row];
    deletion(order);
    order->norder++;
    insert(order);
}

/* see check_assert in minorder for info about how this matrix is supposed
to look.  In new is nonzero and an element would otherwise be created, new
is used instead. This is because linking an element is highly nontrivial
The biggest difference is that elements are no longer removed and this
saves much time allocating and freeing during the solve phase
*/

/* return pointer to row col element maintaining order in rows */
inline Elm* getelm(unsigned row, unsigned col, Elm* newElm) {
    Elm *el, *elnext;
    unsigned vrow, vcol;

    vrow = varord[row];
    vcol = varord[col];

    if (vrow == vcol) {
        return diag[vrow]; /* a common case */
    }
    if (vrow > vcol) { /* in the lower triangle */
        /* search downward from diag[vcol] */
        for (el = diag[vcol];; el = elnext) {
            elnext = el->r_down;
            if (!elnext) {
                break;
            } else if (elnext->row == row) { /* found it */
                return elnext;
            } else if (varord[elnext->row] > vrow) {
                break;
            }
        }
        /* insert below el */
        if (!newElm) {
            newElm = (Elm*) emalloc(sizeof(Elm));
            newElm->value = 0.;
            increase_order(row);
        }
        newElm->r_down = el->r_down;
        el->r_down = newElm;
        newElm->r_up = el;
        if (newElm->r_down) {
            newElm->r_down->r_up = newElm;
        }
        /* search leftward from diag[vrow] */
        for (el = diag[vrow];; el = elnext) {
            elnext = el->c_left;
            if (!elnext) {
                break;
            } else if (varord[elnext->col] < vcol) {
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
            rowst[vrow] = newElm;
        }
    } else { /* in the upper triangle */
        /* search upward from diag[vcol] */
        for (el = diag[vcol];; el = elnext) {
            elnext = el->r_up;
            if (!elnext) {
                break;
            } else if (elnext->row == row) { /* found it */
                return elnext;
            } else if (varord[elnext->row] < vrow) {
                break;
            }
        }
        /* insert above el */
        if (!newElm) {
            newElm = (Elm*) emalloc(sizeof(Elm));
            newElm->value = 0.;
            increase_order(row);
        }
        newElm->r_up = el->r_up;
        el->r_up = newElm;
        newElm->r_down = el;
        if (newElm->r_up) {
            newElm->r_up->r_down = newElm;
        }
        /* search right from diag[vrow] */
        for (el = diag[vrow];; el = elnext) {
            elnext = el->c_right;
            if (!elnext) {
                break;
            } else if (varord[elnext->col] > vcol) {
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
inline void free_elm() {
    unsigned i;
    Elm* el;

    /* free all elements */
    for (i = 1; i <= neqn; i++) {
        for (el = rowst[i]; el; el = el->c_right)
            free(el);
        rowst[i] = nullptr;
        diag[i] = nullptr;
    }
}
/* reallocate space for matrix */
inline void initeqn(unsigned maxeqn) {
    unsigned i;

    if (maxeqn == neqn)
        return;
    free_elm();
    if (rowst)
        free(rowst);
    if (diag)
        free(diag);
    if (varord)
        free(varord);
    rowst = diag = (Elm**) 0;
    varord = (unsigned*) 0;
    rowst = (Elm**) emalloc((maxeqn + 1) * sizeof(Elm*));
    diag = (Elm**) emalloc((maxeqn + 1) * sizeof(Elm*));
    varord = (unsigned*) emalloc((maxeqn + 1) * sizeof(unsigned));
    for (i = 1; i <= maxeqn; i++) {
        varord[i] = i;
        diag[i] = (Elm*) emalloc(sizeof(Elm));
        rowst[i] = diag[i];
        diag[i]->row = i;
        diag[i]->col = i;
        diag[i]->r_down = diag[i]->r_up = nullptr;
        diag[i]->c_right = diag[i]->c_left = nullptr;
        diag[i]->value = 0.;
        rhs[i] = 0.;
    }
    neqn = maxeqn;
}
inline void check_assert() {
    /* check that all links are consistent */
    unsigned i;
    Elm* el;

    for (i = 1; i <= neqn; i++) {
        assert(diag[i]);
        assert(diag[i]->row == diag[i]->col);
        assert(varord[diag[i]->row] == i);
        assert(rowst[i]->row == diag[i]->row);
        for (el = rowst[i]; el; el = el->c_right) {
            if (el == rowst[i]) {
                assert(el->c_left == nullptr);
            } else {
                assert(el->c_left->c_right == el);
                assert(varord[el->c_left->col] < varord[el->col]);
            }
        }
        for (el = diag[i]->r_down; el; el = el->r_down) {
            assert(el->r_up->r_down == el);
            assert(varord[el->r_up->row] < varord[el->row]);
        }
        for (el = diag[i]->r_up; el; el = el->r_up) {
            assert(el->r_down->r_up == el);
            assert(varord[el->r_down->row] > varord[el->row]);
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
static void init_minorder() {
    /* matrix has been set up. Construct the orderlist and orderfind
       vector.
    */
    unsigned i, j;
    Elm* el;

    do_flag = 1;
    if (roworder) {
        for (i = 1; i <= nroworder; ++i) {
            free(roworder[i]);
        }
        free(roworder);
    }
    roworder = (Item**) emalloc((neqn + 1) * sizeof(Item*));
    nroworder = neqn;
    if (orderlist)
        freelist(orderlist);
    orderlist = newlist();
    for (i = 1; i <= neqn; i++) {
        roworder[i] = newitem();
    }
    for (i = 1; i <= neqn; i++) {
        for (j = 0, el = rowst[i]; el; el = el->c_right) {
            j++;
        }
        roworder[diag[i]->row]->elm = diag[i];
        roworder[diag[i]->row]->norder = j;
        insert(roworder[diag[i]->row]);
    }
}
/* at this point row links are out of order for diag[i]->col
       and col links are out of order for diag[i]->row */
inline void re_link(unsigned i) {
    Elm *el, *dright, *dleft, *dup, *ddown, *elnext;

    for (el = rowst[i]; el; el = el->c_right) {
        /* repair hole */
        if (el->r_up)
            el->r_up->r_down = el->r_down;
        if (el->r_down)
            el->r_down->r_up = el->r_up;
    }

    for (el = diag[i]->r_down; el; el = el->r_down) {
        /* repair hole */
        if (el->c_right)
            el->c_right->c_left = el->c_left;
        if (el->c_left)
            el->c_left->c_right = el->c_right;
        else
            rowst[varord[el->row]] = el->c_right;
    }

    for (el = diag[i]->r_up; el; el = el->r_up) {
        /* repair hole */
        if (el->c_right)
            el->c_right->c_left = el->c_left;
        if (el->c_left)
            el->c_left->c_right = el->c_right;
        else
            rowst[varord[el->row]] = el->c_right;
    }

    /* matrix is consistent except that diagonal row elements are unlinked from
    their columns and the diagonal column elements are unlinked from their
    rows.
    For simplicity discard all knowledge of links and use getelm to relink
    */
    rowst[i] = diag[i];
    dright = diag[i]->c_right;
    dleft = diag[i]->c_left;
    dup = diag[i]->r_up;
    ddown = diag[i]->r_down;
    diag[i]->c_right = diag[i]->c_left = nullptr;
    diag[i]->r_up = diag[i]->r_down = nullptr;
    for (el = dright; el; el = elnext) {
        elnext = el->c_right;
        getelm(el->row, el->col, el);
    }
    for (el = dleft; el; el = elnext) {
        elnext = el->c_left;
        getelm(el->row, el->col, el);
    }
    for (el = dup; el; el = elnext) {
        elnext = el->r_up;
        getelm(el->row, el->col, el);
    }
    for (el = ddown; el; el = elnext) {
        elnext = el->r_down;
        getelm(el->row, el->col, el);
    }
}
inline void reduce_order(unsigned row) {
    /* order of row decreases by 1. Maintain the orderlist. */
    Item* order;

    if (!do_flag)
        return;
    order = roworder[row];
    deletion(order);
    order->norder--;
    insert(order);
}
inline void get_next_pivot(unsigned i) {
    /* get varord[i], etc. from the head of the orderlist. */
    Item* order;
    Elm *pivot, *el;
    unsigned j;

    order = orderlist->next;
    assert(order != orderlist);

    if ((j = varord[order->elm->row]) != i) {
        /* push order lists down by 1 and put new diag in empty slot */
        assert(j > i);
        el = rowst[j];
        for (; j > i; j--) {
            diag[j] = diag[j - 1];
            rowst[j] = rowst[j - 1];
            varord[diag[j]->row] = j;
        }
        diag[i] = order->elm;
        rowst[i] = el;
        varord[diag[i]->row] = i;
        /* at this point row links are out of order for diag[i]->col
           and col links are out of order for diag[i]->row */
        re_link(i);
    }


    /* now make sure all needed elements exist */
    for (el = diag[i]->r_down; el; el = el->r_down) {
        for (pivot = diag[i]->c_right; pivot; pivot = pivot->c_right) {
            getelm(el->row, pivot->col, nullptr);
        }
        reduce_order(el->row);
    }

    deletion(order);
}
/* Minimum ordering algorithm to determine the order
            that the matrix should be solved. Also make sure
            all needed elements are present.
            This does not mess up the matrix
        */
inline void spar_minorder() {
    unsigned i;
    check_assert();
    init_minorder();
    for (i = 1; i <= neqn; i++) {
        get_next_pivot(i);
    }
    do_flag = 0;
    check_assert();
}
inline void create_coef_list(int n, int (*fun)()) {
    initeqn((unsigned) n);
    phase = 1;
    ngetcall = 0;
    (*fun)();
    if (coef_list) {
        free(coef_list);
    }
    coef_list = (double**) emalloc(ngetcall * sizeof(double*));
    spar_minorder();
    phase = 2;
    ngetcall = 0;
    (*fun)();
    phase = 0;
}

inline void init_coef_list() {
    unsigned i;
    Elm* el;

    ngetcall = 0;
    for (i = 1; i <= neqn; i++) {
        for (el = rowst[i]; el; el = el->c_right) {
            el->value = 0.;
        }
    }
}
inline void prmat() {
    unsigned i, j;
    Elm* el;

    printf("\n        ");
    for (i = 10; i <= neqn; i += 10)
        printf("         %1d", (i % 100) / 10);
    printf("\n        ");
    for (i = 1; i <= neqn; i++)
        printf("%1d", i % 10);
    printf("\n\n");
    for (i = 1; i <= neqn; i++) {
        printf("%3d %3d ", diag[i]->row, i);
        j = 0;
        for (el = rowst[i]; el; el = el->c_right) {
            for (j++; j < varord[el->col]; j++)
                printf(" ");
            printf("*");
        }
        printf("\n");
    }
    fflush(stdin);
}
}  // namespace detail::sparse

/* sparse matrix dynamic allocation:
create_coef_list makes a list for fast setup, does minimum ordering and
ensures all elements needed are present */
/* this could easily be made recursive but it isn't right now */

/**
 * This is an experimental numerical method for SCoP-3 which integrates kinetic rate equations. It
 * is intended to be used only by models generated by MODL, and its identity is meant to be
 * concealed from the user.
 *
 * @param v The sparse matrix object (?)
 * @param n Number of state variables
 * @param s Array of pointers to the state variables
 * @param d Array of pointers to the derivatives of states
 * @param t	pointer to the independent variable
 * @param dt the time step
 * @param fun pointer to the function corresponding to the kinetic block equations
 * @param prhs pointer to right hand side vector (answer on return) does not have to be allocated by
 * caller.
 * @param linflag solve as linear equations when nonlinear, all states are forced >= 0
 *
 * note: solution order refers to the following
 *   diag[varord[row]]->row = row = diag[varord[row]]->col
 *   rowst[varord[row]]->row = row
 *   varord[el->row] < varord[el->c_right->row]
 *   varord[el->col] < varord[el->r_down->col]
 */
template <typename Array>
int sparse(void** v,
           int n,
           int* s,
           int* d,
           Array p,
           double* t,
           double dt,
           int (*fun)(),
           double** prhs,
           int linflag) {
    auto const s_ = [&p, s](auto arg) -> auto& {
        return p[s[arg]];
    };
    auto const d_ = [&p, d](auto arg) -> auto& {
        return p[d[arg]];
    };
    int i, j, ierr;
    double err;
    detail::sparse::SparseObj* so;
    if (!*prhs) {
        *prhs = (double*) emalloc((n + 1) * sizeof(double));
    }
    detail::sparse::rhs = *prhs;
    so = (detail::sparse::SparseObj*) (*v);
    if (!so) {
        so = detail::sparse::create_sparseobj();
        *v = (void*) so;
    }
    if (so != detail::sparse::old_sparseobj) {
        detail::sparse::sparseobj2local(so);
    }
    if (so->oldfun != fun) {
        so->oldfun = fun;
        detail::sparse::create_coef_list(n, fun); /* calls fun twice */
        detail::sparse::local2sparseobj(so);
    }
    for (i = 0; i < n; i++) { /*save old state*/
        d_(i) = s_(i);
    }
    for (err = 1, j = 0; err > CONVERGE; j++) {
        detail::sparse::init_coef_list();
        (*fun)();
        if ((ierr = detail::sparse::matsol())) {
            return ierr;
        }
        for (err = 0., i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
            s_(i - 1) += detail::sparse::rhs[i];
            /* stability of nonlinear kinetic schemes sometimes requires this */
            if (!linflag && s_(i - 1) < 0.) {
                s_(i - 1) = 0.;
            }
            err += fabs(detail::sparse::rhs[i]);
        }
        if (j > MAXSTEPS) {
            return EXCEED_ITERS;
        }
        if (linflag)
            break;
    }
    detail::sparse::init_coef_list();
    (*fun)();
    for (i = 0; i < n; i++) { /*restore Dstate at t+dt*/
        d_(i) = (s_(i) - d_(i)) / dt;
    }
    return SUCCESS;
}

/* for solving ax=b */
template <typename Array>
int _cvode_sparse(void** v, int n, int* x, Array p, int (*fun)(), double** prhs) {
    auto const x_ = [&p, x](auto arg) -> auto& {
        return p[x[arg]];
    };
    int i, j, ierr;
    detail::sparse::SparseObj* so;

    if (!*prhs) {
        *prhs = (double*) emalloc((n + 1) * sizeof(double));
    }
    detail::sparse::rhs = *prhs;
    so = (detail::sparse::SparseObj*) (*v);
    if (!so) {
        so = detail::sparse::create_sparseobj();
        *v = (void*) so;
    }
    if (so != detail::sparse::old_sparseobj) {
        detail::sparse::sparseobj2local(so);
    }
    if (so->oldfun != fun) {
        so->oldfun = fun;
        detail::sparse::create_coef_list(n, fun); /* calls fun twice */
        detail::sparse::local2sparseobj(so);
    }
    detail::sparse::init_coef_list();
    (*fun)();
    if ((ierr = detail::sparse::matsol())) {
        return ierr;
    }
    for (i = 1; i <= n; i++) { /* why oh why did I write it from 1 */
        x_(i - 1) = detail::sparse::rhs[i];
    }
    return SUCCESS;
}
inline double* _getelm(int row, int col) {
    detail::sparse::Elm* el;
    if (!detail::sparse::phase) {
        return detail::sparse::coef_list[detail::sparse::ngetcall++];
    }
    el = detail::sparse::getelm((unsigned) row, (unsigned) col, nullptr);
    if (detail::sparse::phase == 1) {
        detail::sparse::ngetcall++;
    } else {
        detail::sparse::coef_list[detail::sparse::ngetcall++] = &el->value;
    }
    return &el->value;
}
}  // namespace neuron::scopmath
using neuron::scopmath::_cvode_sparse;
using neuron::scopmath::_getelm;
using neuron::scopmath::sparse;
