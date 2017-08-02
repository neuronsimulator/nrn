/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef multicore_h
#define multicore_h

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/membfunc.h"

#if defined(__cplusplus)
class NetCon;
class PreSyn;
extern "C" {
#else
typedef void NetCon;
typedef void PreSyn;
#endif

/*
   Point_process._presyn, used only if its NET_RECEIVE sends a net_event, is
   eliminated. Needed only by net_event function. Replaced by
   PreSyn* = nt->presyns + nt->pnt2presyn_ix[pnttype2presyn[pnt->_type]][pnt->_i_instance];
*/
extern int nrn_has_net_event_cnt_; /* how many net_event sender types are there? */
extern int* nrn_has_net_event_;    /* the types that send a net_event */
extern int* pnttype2presyn; /* from the type, which array of pnt2presyn_ix are we talking about. */

typedef struct NrnThreadMembList { /* patterned after CvMembList in cvodeobj.h */
    struct NrnThreadMembList* next;
    struct Memb_list* ml;
    int index;
    int* dependencies; /* list of mechanism types that this mechanism depends on*/
    int ndependencies; /* for scheduling we need to know the dependency count */
} NrnThreadMembList;

typedef struct NrnThreadBAList {
    struct Memb_list* ml; /* an item in the NrnThreadMembList */
    struct BAMech* bam;
    struct NrnThreadBAList* next;
} NrnThreadBAList;

/* for OpenACC, in order to avoid an error while update PreSyn, with virtual base
 * class, we are adding helper with flag variable which could be updated on GPU
 */
typedef struct PreSynHelper { int flag_; } PreSynHelper;

typedef struct NrnThread {
    double _t;
    double _dt;
    double cj;

    NrnThreadMembList* tml;
    Memb_list** _ml_list;
    Point_process* pntprocs;  // synapses and artificial cells with and without gid
    PreSyn* presyns;          // all the output PreSyn with and without gid
    PreSynHelper* presyns_helper;
    int** pnt2presyn_ix;  // eliminates Point_process._presyn used only by net_event sender.
    NetCon* netcons;
    double* weights;  // size n_weight. NetCon.weight_ points into this array.

    int n_pntproc, n_presyn, n_input_presyn, n_netcon, n_weight;  // only for model_size

    int ncell; /* analogous to old rootnodecount */
    int end;   /* 1 + position of last in v_node array. Now v_node_count. */
    int id;    /* this is nrn_threads[id] */
    int _stop_stepping;
    int n_vecplay; /* number of instances of VecPlayContinuous */

    size_t _ndata, _nidata, _nvdata; /* sizes */
    double* _data;                   /* all the other double* and Datum to doubles point into here*/
    int* _idata;                     /* all the Datum to ints index into here */
    void** _vdata;                   /* all the Datum to pointers index into here */
    void** _vecplay;                 /* array of instances of VecPlayContinuous */

    double* _actual_rhs;
    double* _actual_d;
    double* _actual_a;
    double* _actual_b;
    double* _actual_v;
    double* _actual_area;
    double* _actual_diam; /* NULL if no mechanism has dparam with diam semantics */
    double* _shadow_rhs;  /* Not pointer into _data. Avoid race for multiple POINT_PROCESS in same
                             compartment */
    double* _shadow_d;    /* Not pointer into _data. Avoid race for multiple POINT_PROCESS in same
                             compartment */
    int* _v_parent_index;
    int* _permute;
    char* _sp13mat;                     /* handle to general sparse matrix */
    struct Memb_list* _ecell_memb_list; /* normally nil */

    double _ctime; /* computation time in seconds (using nrnmpi_wtime) */

    NrnThreadBAList* tbl[BEFORE_AFTER_SIZE]; /* wasteful since almost all empty */

    int shadow_rhs_cnt; /* added to facilitate the NrnThread transfer to GPU */
    int compute_gpu;    /* define whether to compute with gpus */
    int stream_id;      /* define where the kernel will be launched on GPU stream */
    int _net_send_buffer_size;
    int _net_send_buffer_cnt;
    int* _net_send_buffer;

    int* _watch_types; /* NULL or 0 terminated array of integers */
    void* mapping;     /* section to segment mapping information */

} NrnThread;

extern void nrn_threads_create(int n, int parallel);
extern int nrn_nthread;
extern NrnThread* nrn_threads;
extern void nrn_multithread_job(void* (*)(NrnThread*));
extern void nrn_thread_table_check(void);

extern void nrn_threads_free(void);

#if defined(__cplusplus)
}
#endif

#endif
