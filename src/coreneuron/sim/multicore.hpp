/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include "coreneuron/nrnconf.h"
#include "coreneuron/mechanism/membfunc.hpp"
#include "coreneuron/utils/memory.h"
#include "coreneuron/mpi/nrnmpi.h"
#include <vector>

namespace coreneuron {
class NetCon;
class PreSyn;

extern bool use_solve_interleave;

/*
   Point_process._presyn, used only if its NET_RECEIVE sends a net_event, is
   eliminated. Needed only by net_event function. Replaced by
   PreSyn* = nt->presyns + nt->pnt2presyn_ix[pnttype2presyn[pnt->_type]][pnt->_i_instance];
*/

struct NrnThreadMembList { /* patterned after CvMembList in cvodeobj.h */
    NrnThreadMembList* next;
    Memb_list* ml;
    int index;
    int* dependencies; /* list of mechanism types that this mechanism depends on*/
    int ndependencies; /* for scheduling we need to know the dependency count */
};

struct NrnThreadBAList {
    Memb_list* ml; /* an item in the NrnThreadMembList */
    BAMech* bam;
    NrnThreadBAList* next;
};

struct NrnFastImem {
    double* nrn_sav_rhs;
    double* nrn_sav_d;
};

struct TrajectoryRequests {
    void** vpr;       /* PlayRecord Objects known by NEURON */
    double** scatter; /* if bsize == 0, each time step */
    double** varrays; /* if bsize > 0, the Vector data pointers. */
    double** gather;  /* pointers to values that get scattered to NEURON */
    int n_pr;         /* number of PlayRecord instances */
    int n_trajec;     /* number of trajectories requested */
    int bsize;        /* buffer size of the Vector data */
    int vsize;        /* number of elements in varrays so far */
};

/* for OpenACC, in order to avoid an error while update PreSyn, with virtual base
 * class, we are adding helper with flag variable which could be updated on GPU
 */
struct PreSynHelper {
    int flag_;
};

struct NrnThread : public MemoryManaged {
    double _t = 0;
    double _dt = -1e9;
    double cj = 0.0;

    NrnThreadMembList* tml = nullptr;
    Memb_list** _ml_list = nullptr;
    Point_process* pntprocs = nullptr;  // synapses and artificial cells with and without gid
    PreSyn* presyns = nullptr;          // all the output PreSyn with and without gid
    PreSynHelper* presyns_helper = nullptr;
    int** pnt2presyn_ix = nullptr;  // eliminates Point_process._presyn used only by net_event sender.
    NetCon* netcons = nullptr;
    double* weights = nullptr;  // size n_weight. NetCon.weight_ points into this array.

    int n_pntproc = 0;
    int n_weight = 0;
    int n_netcon = 0;
    int n_input_presyn = 0;
    int n_presyn = 0;  // only for model_size

    int ncell = 0; /* analogous to old rootnodecount */
    int end = 0;   /* 1 + position of last in v_node array. Now v_node_count. */
    int id = 0;    /* this is nrn_threads[id] */
    int _stop_stepping = 0;
    int n_vecplay = 0; /* number of instances of VecPlayContinuous */

    size_t _ndata = 0;
    size_t _nvdata = 0;
    size_t _nidata = 0;                            /* sizes */
    double* _data = nullptr;                   /* all the other double* and Datum to doubles point into here*/
    int* _idata = nullptr;                     /* all the Datum to ints index into here */
    void** _vdata = nullptr;                   /* all the Datum to pointers index into here */
    void** _vecplay = nullptr;                 /* array of instances of VecPlayContinuous */

    double* _actual_rhs = nullptr;
    double* _actual_d = nullptr;
    double* _actual_a = nullptr;
    double* _actual_b = nullptr;
    double* _actual_v = nullptr;
    double* _actual_area = nullptr;
    double* _actual_diam = nullptr; /* nullptr if no mechanism has dparam with diam semantics */
    double* _shadow_rhs = nullptr;  /* Not pointer into _data. Avoid race for multiple POINT_PROCESS in same
                             compartment */
    double* _shadow_d = nullptr;    /* Not pointer into _data. Avoid race for multiple POINT_PROCESS in same
                             compartment */

    /* Fast membrane current calculation struct */
    NrnFastImem* nrn_fast_imem = nullptr;

    int* _v_parent_index = nullptr;
    int* _permute = nullptr;
    char* _sp13mat = nullptr;              /* handle to general sparse matrix */
    Memb_list* _ecell_memb_list = nullptr; /* normally nullptr */

    double _ctime = 0.0; /* computation time in seconds (using nrnmpi_wtime) */

    NrnThreadBAList* tbl[BEFORE_AFTER_SIZE]; /* wasteful since almost all empty */

    int shadow_rhs_cnt = 0; /* added to facilitate the NrnThread transfer to GPU */
    int compute_gpu = 0;    /* define whether to compute with gpus */
    int stream_id = 0;      /* define where the kernel will be launched on GPU stream */
    int _net_send_buffer_size = 0;
    int _net_send_buffer_cnt = 0;
    int* _net_send_buffer = nullptr;

    int* _watch_types = nullptr;                   /* nullptr or 0 terminated array of integers */
    void* mapping = nullptr;                       /* section to segment mapping information */
    TrajectoryRequests* trajec_requests = nullptr; /* per time step values returned to NEURON */

    /* Needed in case there are FOR_NETCON statements in use. */
    std::vector<size_t> _fornetcon_perm_indices; /* displacement like list of indices */
    std::vector<size_t> _fornetcon_weight_perm;  /* permutation indices into weight */
};

extern void nrn_threads_create(int n);
extern int nrn_nthread;
extern NrnThread* nrn_threads;
template<typename F, typename... Args>
void nrn_multithread_job(F&& job, Args&&... args) {
    int i;
// clang-format off
    #pragma omp parallel for private(i) shared(nrn_threads, job, nrn_nthread, \
                                           nrnmpi_myid) schedule(static, 1)
    for (i = 0; i < nrn_nthread; ++i) {
        job(nrn_threads + i, std::forward<Args>(args)...);
    }
// clang-format on
}

extern void nrn_thread_table_check(void);

extern void nrn_threads_free(void);

extern bool _nrn_skip_initmodel;


extern void dt2thread(double);
extern void clear_event_queue(void);
extern void nrn_ba(NrnThread*, int);
extern void* nrn_fixed_step_lastpart(NrnThread*);
extern void nrn_solve_minimal(NrnThread*);
extern void nrncore2nrn_send_init();
extern void* setup_tree_matrix_minimal(NrnThread*);
extern void nrncore2nrn_send_values(NrnThread*);
extern void nrn_fixed_step_group_minimal(int total_sim_steps);
extern void nrn_fixed_single_steps_minimal(int total_sim_steps, double tstop);
extern void nrn_fixed_step_minimal(void);
extern void nrn_finitialize(int setv, double v);
extern void nrn_mk_table_check(void);
extern void nonvint(NrnThread* _nt);
extern void update(NrnThread*);


}  // namespace coreneuron
