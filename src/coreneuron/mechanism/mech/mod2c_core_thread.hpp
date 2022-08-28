/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mechanism/mechanism.hpp"
#include "coreneuron/utils/offload.hpp"

namespace coreneuron {

#define _STRIDE _cntml_padded + _iml

#define _threadargscomma_ _iml, _cntml_padded, _p, _ppvar, _thread, _nt, _ml, _v,
#define _threadargsprotocomma_                                                                    \
    int _iml, int _cntml_padded, double *_p, Datum *_ppvar, ThreadDatum *_thread, NrnThread *_nt, \
        Memb_list *_ml, double _v,
#define _threadargs_ _iml, _cntml_padded, _p, _ppvar, _thread, _nt, _ml, _v
#define _threadargsproto_                                                                         \
    int _iml, int _cntml_padded, double *_p, Datum *_ppvar, ThreadDatum *_thread, NrnThread *_nt, \
        Memb_list *_ml, double _v

struct Elm {
    unsigned row;        /* Row location */
    unsigned col;        /* Column location */
    double* value;       /* The value SOA  _cntml_padded of them*/
    struct Elm* r_up;    /* Link to element in same column */
    struct Elm* r_down;  /*       in solution order */
    struct Elm* c_left;  /* Link to left element in same row */
    struct Elm* c_right; /*       in solution order (see getelm) */
};

struct Item {
    Elm* elm{};
    unsigned norder{}; /* order of a row */
    Item* next{};
    Item* prev{};
};

using List = Item; /* list of mixed items */

struct SparseObj {            /* all the state information */
    Elm** rowst{};            /* link to first element in row (solution order)*/
    Elm** diag{};             /* link to pivot element in row (solution order)*/
    void* elmpool{};          /* no interthread cache line sharing for elements */
    unsigned neqn{};          /* number of equations */
    unsigned _cntml_padded{}; /* number of instances */
    unsigned* varord{};       /* row and column order for pivots */
    double* rhs{};            /* initially- right hand side        finally - answer */
    unsigned* ngetcall{};     /* per instance counter for number of calls to _getelm */
    int phase{};              /* 0-solution phase; 1-count phase; 2-build list phase */
    int numop{};
    unsigned coef_list_size{};
    double** coef_list{}; /* pointer to (first instance) value in _getelm order */
    /* don't really need the rest */
    int nroworder{};   /* just for freeing */
    Item** roworder{}; /* roworder[i] is pointer to order item for row i.
                             Does not have to be in orderlist */
    List* orderlist{}; /* list of rows sorted by norder
                             that haven't been used */
    int do_flag{};
};

extern void _nrn_destroy_sparseobj_thread(SparseObj* so);

// derived from nrn/src/scopmath/euler.c
// updated for aos/soa layout index
template <typename F>
int euler_thread(int neqn, int* var, int* der, F fun, _threadargsproto_) {
    double const dt{_nt->_dt};
    /* calculate the derivatives */
    fun(_threadargs_);  // std::invoke in C++17
    /* update dependent variables */
    for (int i = 0; i < neqn; i++) {
        _p[var[i] * _STRIDE] += dt * (_p[der[i] * _STRIDE]);
    }
    return 0;
}

template <typename F>
int derivimplicit_thread(int n, int* slist, int* dlist, F fun, _threadargsproto_) {
    fun(_threadargs_);  // std::invoke in C++17
    return 0;
}

void nrn_sparseobj_copyto_device(SparseObj* so);
void nrn_sparseobj_delete_from_device(SparseObj* so);

}  // namespace coreneuron
