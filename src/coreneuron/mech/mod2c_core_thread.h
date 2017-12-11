#ifndef mod2c_core_thread_h
#define mod2c_core_thread_h

#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_ml.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(LAYOUT)
/* 1 means AoS, >1 means AoSoA, <= 0 means SOA */
#define LAYOUT 1
#endif
#if LAYOUT >= 1
#define _STRIDE LAYOUT
#else
#define _STRIDE _cntml_padded + _iml
#endif

#define _threadargscomma_ _iml, _cntml_padded, _p, _ppvar, _thread, _nt, _v,
#define _threadargsprotocomma_                                                                    \
    int _iml, int _cntml_padded, double *_p, Datum *_ppvar, ThreadDatum *_thread, NrnThread *_nt, \
        double _v,
#define _threadargs_ _iml, _cntml_padded, _p, _ppvar, _thread, _nt, _v
#define _threadargsproto_                                                                         \
    int _iml, int _cntml_padded, double *_p, Datum *_ppvar, ThreadDatum *_thread, NrnThread *_nt, \
        double _v

#if 0

typedef int (*DIFUN)(_threadargsproto_);
typedef int (*NEWTFUN)(_threadargsproto_);
typedef int (*SPFUN)(struct SparseObj*, double*, _threadargsproto_);
#define difun(arg) (*arg)(_threadargs_);
#define newtfun(arg) (*arg)(_threadargs_);

#else

/**
 * \todo: typedefs like DIFUN can be removed
 * \todo: macros for difun, newtfun, eulerfun are not necessary
 *        and need to be refactored.
 */

typedef int DIFUN;
typedef int NEWTFUN;
typedef int SPFUN;
typedef int EULFUN;
#pragma acc routine seq
extern int nrn_derivimplicit_steer(int, _threadargsproto_);
#define difun(arg) nrn_derivimplicit_steer(arg, _threadargs_);
#pragma acc routine seq
extern int nrn_newton_steer(int, _threadargsproto_);
#define newtfun(arg) nrn_newton_steer(arg, _threadargs_);
#pragma acc routine seq
extern int nrn_euler_steer(int, _threadargsproto_);
#define eulerfun(arg) nrn_euler_steer(arg, _threadargs_);

#endif

typedef struct Elm {
    unsigned row;        /* Row location */
    unsigned col;        /* Column location */
    double* value;       /* The value SOA  _cntml_padded of them*/
    struct Elm* r_up;    /* Link to element in same column */
    struct Elm* r_down;  /*       in solution order */
    struct Elm* c_left;  /* Link to left element in same row */
    struct Elm* c_right; /*       in solution order (see getelm) */
} Elm;
#define ELM0 (Elm*)0

typedef struct Item {
    Elm* elm;
    unsigned norder; /* order of a row */
    struct Item* next;
    struct Item* prev;
} Item;
#define ITEM0 (Item*)0

typedef Item List; /* list of mixed items */

typedef struct SparseObj {  /* all the state information */
    Elm** rowst;            /* link to first element in row (solution order)*/
    Elm** diag;             /* link to pivot element in row (solution order)*/
    void* elmpool;          /* no interthread cache line sharing for elements */
    unsigned neqn;          /* number of equations */
    unsigned _cntml_padded; /* number of instances */
    unsigned* varord;       /* row and column order for pivots */
    double* rhs;            /* initially- right hand side        finally - answer */
    SPFUN oldfun;
    unsigned* ngetcall; /* per instance counter for number of calls to _getelm */
    int phase;          /* 0-solution phase; 1-count phase; 2-build list phase */
    int numop;
    unsigned coef_list_size;
    double** coef_list; /* pointer to (first instance) value in _getelm order */
    /* don't really need the rest */
    int nroworder;   /* just for freeing */
    Item** roworder; /* roworder[i] is pointer to order item for row i.
                             Does not have to be in orderlist */
    List* orderlist; /* list of rows sorted by norder
                             that haven't been used */
    int do_flag;
} SparseObj;

#pragma acc routine seq
extern int nrn_kinetic_steer(int, SparseObj*, double*, _threadargsproto_);
#define spfun(arg1, arg2, arg3) nrn_kinetic_steer(arg1, arg2, arg3, _threadargs_);

#pragma acc routine seq
extern int euler_thread(int, int*, int*, EULFUN, _threadargsproto_);
#pragma acc routine seq
extern int derivimplicit_thread(int, int*, int*, DIFUN, _threadargsproto_);
#pragma acc routine seq
extern int _ss_derivimplicit_thread(int n, int* slist, int* dlist, DIFUN fun, _threadargsproto_);
#pragma acc routine seq
extern int
sparse_thread(SparseObj*, int, int*, int*, double*, double, SPFUN, int, _threadargsproto_);
#pragma acc routine seq
int _ss_sparse_thread(SparseObj*,
                      int n,
                      int* s,
                      int* d,
                      double* t,
                      double dt,
                      SPFUN fun,
                      int linflag,
                      _threadargsproto_);

#pragma acc routine seq
extern double _modl_get_dt_thread(NrnThread*);
#pragma acc routine seq
extern void _modl_set_dt_thread(double, NrnThread*);

void nrn_sparseobj_copyto_device(SparseObj* so);

#if defined(__cplusplus)
}
#endif

#endif
