#pragma once

/*
Starts from Hubert Eichner's modifications but incorporates a
significant refactorization. The best feature of Hubert's original
code is retained. I.e. the user level structures remain unchanged
and the vectors, e.g v_node, are ordered so that each thread uses
a contiguous region. We take this even further by changing rootnodecount
to nrn_global_ncell and ordering the rootnodes so they appear in the proper
place in the lists instead of all at the beginning. This means
most of the user level for i=0,rootnode-1 loops have to be
changed to iterate over all the nrn_thread_t.ncell. But underneath the
VECTORIZE part, most functions are given an ithread argument and none ever
get outside the array portions specified by the nrn_thread_t.
This means that the thread parallelization can be handled at the level
of fadvance() and a network sim can take advantage of the minimum
netcon delay interval

The main caveat with threads is that mod files should not use pointers
that cross thread data boundaries. ie. gap junctions should use the
ParallelContext methods.

*/

/* now included by section.h since this has take over the v_node,
actual_v, etc.
*/

#include "membfunc.h"

#include <cstddef>

typedef struct NrnThreadMembList { /* patterned after CvMembList in cvodeobj.h */
    struct NrnThreadMembList* next;
    Memb_list* ml;
    int index;
} NrnThreadMembList;

typedef struct NrnThreadBAList {
    Memb_list* ml; /* an item in the NrnThreadMembList */
    BAMech* bam;
    struct NrnThreadBAList* next;
} NrnThreadBAList;

struct hoc_Item;
using hoc_List = hoc_Item;
struct Object;

/**
 * \class NrnThread
 * \brief Represent main neuron object computed by single thread
 *
 * NrnThread represent collection of cells or part of a cell computed
 * by single thread within NEURON process.
 *
 * @warning The constructor/destructor of this struct are not called.
 */
struct NrnThread {
    double _t;
    double _dt;
    double cj;
    NrnThreadMembList* tml;
    Memb_list** _ml_list;
    int ncell;            /* analogous to old rootnodecount */
    int end;              /* 1 + position of last in v_node array. Now v_node_count. */
    int id;               /* this is nrn_threads[id] */
    int _stop_stepping;   /* delivered an all thread HocEvent */
    int _ecell_child_cnt; /* see _ecell_children below */

    /** @brief Offset in the global node data where this NrnThread's values start.
     */
    std::size_t _node_data_offset{};

    [[nodiscard]] double* node_a_storage();
    [[nodiscard]] double* node_area_storage();
    [[nodiscard]] double* node_b_storage();
    [[nodiscard]] double* node_d_storage();
    [[nodiscard]] double* node_rhs_storage();
    [[nodiscard]] double* node_sav_d_storage();
    [[nodiscard]] double* node_sav_rhs_storage();
    [[nodiscard]] double* node_voltage_storage();
    [[nodiscard]] double& actual_d(std::size_t row) {
        return node_d_storage()[row];
    }
    [[nodiscard]] double& actual_rhs(std::size_t row) {
        return node_rhs_storage()[row];
    }

    int* _v_parent_index;
    Node** _v_node;
    Node** _v_parent;
    double* _sp13_rhs;           /* rhs matrix for sparse13 solver. updates to and from this vector
                                    need to be transfered to actual_rhs */
    char* _sp13mat;              /* handle to general sparse matrix */
    Memb_list* _ecell_memb_list; /* normally nullptr */
    Node** _ecell_children;      /* nodes with no extcell but parent has it */
    void* _vcv;                  /* replaces old cvode_instance and nrn_cvode_ */

#if 1
    double _ctime; /* computation time in seconds (using nrnmpi_wtime) */
#endif

    NrnThreadBAList* tbl[BEFORE_AFTER_SIZE]; /* wasteful since almost all empty */
    hoc_List* roots;                         /* ncell of these */
    Object* userpart; /* the SectionList if this is a user defined partition */
};


extern int nrn_nthread;
extern NrnThread* nrn_threads;
void nrn_threads_create(int n, bool parallel);
extern void nrn_thread_error(const char*);
using worker_job_t = void* (*) (NrnThread*);
using worker_job_with_token_t = void (*)(neuron::model_sorted_token const&, NrnThread&);
void nrn_multithread_job(worker_job_t);
void nrn_multithread_job(neuron::model_sorted_token const&, worker_job_with_token_t);
extern void nrn_onethread_job(int, void* (*) (NrnThread*) );
extern void nrn_wait_for_threads();
void nrn_thread_table_check(neuron::model_sorted_token const&);
extern void nrn_threads_free();
extern int nrn_user_partition();
extern void reorder_secorder();
extern void nrn_thread_memblist_setup();
extern std::size_t nof_worker_threads();

#define FOR_THREADS(nt) for (nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt)

// olupton 2022-01-31: could add a _NrnThread typedef here for .mod file
//                     backwards compatibility if needed.
