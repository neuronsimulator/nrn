/* included by treeset.cpp */
#include <nrnmpi.h>


/*
Now that threads have taken over the actual_v, v_node, etc, it might
be a good time to regularize the method of freeing, allocating, and
updating those arrays. To recapitulate the history, Node used to
be the structure that held direct values for v, area, d, rhs, etc.
That continued to hold for the cray vectorization project which
introduced v_node, v_parent, memb_list. Cache efficiency introduced
actual_v, actual_area, actual_d, etc and the Node started pointing
into those arrays. Additional nodes after allocation required updating
pointers to v and area since those arrays were freed and reallocated.
Now, the threads hold all these arrays and we want to update them
properly under the circumstances of changing topology, changing
number of threads, and changing distribution of cells on threads.
Note there are no longer global versions of any of these arrays.
We do not want to update merely due to a change in area. Recently
we have dealt with diam, area, ri on a section basis. We generally
desire an update just before a simulation when the efficient
structures are necessary. This is reasonably well handled by the
v_structure_change flag which historically freed and reallocated
v_node and v_parent and, just before this comment,
ended up setting the NrnThread tml. This makes most of the old
memb_list vestigial and we now got rid of it except for
the artificial cells (and it is possibly not really necessary there).
Switching between sparse and tree matrix just cause freeing and
reallocation of actual_rhs.

If we can get the freeing, reallocation, and pointer update correct
for _actual_v, I am guessing everything else can be dragged along with
it. We have two major cases, call to pc.nthread and change in
model structure. We want to use Node* as much as possible and defer
the handling of v_structure_change as long as possible.
*/

#include "nmodlmutex.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#define CACHELINE_ALLOC(name, type, size) \
    name = (type*) nrn_cacheline_alloc((void**) &name, size * sizeof(type))
#define CACHELINE_CALLOC(name, type, size) \
    name = (type*) nrn_cacheline_calloc((void**) &name, size, sizeof(type))

int nrn_nthread;
NrnThread* nrn_threads;
void (*nrn_mk_transfer_thread_data_)();

static int busywait_;
static int busywait_main_;
extern void nrn_thread_error(const char*);
extern void nrn_threads_free();
extern void nrn_old_thread_save();
extern double nrn_timeus();

static int nrn_thread_parallel_;
void nrn_mk_table_check();
static int table_check_cnt_;
static Datum* table_check_;
static int allow_busywait_;

static void* nulljob(NrnThread* nt) {
    return nullptr;
}

int nrn_inthread_;
#if NRN_ENABLE_THREADS
static int interpreter_locked;
static std::unique_ptr<std::mutex> _interpreter_lock;

namespace nrn {
std::unique_ptr<std::mutex> nmodlmutex;
}

namespace {
// With C++17 and alignment-aware allocators we could do something like
// alignas(std::hardware_destructive_interference_size) here and then use a
// regular vector.
struct worker_conf_t {
    int flag{};
    int thread_id{};
    /* for nrn_solve etc.*/
    void* (*job)(NrnThread*) = nullptr;
};
}  // namespace

namespace {
std::unique_ptr<std::condition_variable[]> cond;
std::unique_ptr<std::mutex[]> mut;
std::vector<std::thread> worker_threads;
worker_conf_t* wc{};
}  // namespace

static void wait_for_workers() {
    for (int i = 1; i < nrn_nthread; ++i) {
        if (busywait_main_) {
            while (wc[i].flag != 0) {
                ;
            }
        } else {
            std::unique_lock<std::mutex> lock{mut[i]};
            cond[i].wait(lock, [&wc_i = wc[i]] { return wc_i.flag == 0; });
        }
    }
}

static void wait_for_workers_timeit() {
    wait_for_workers();
}

static void send_job_to_slave(int i, void* (*job)(NrnThread*) ) {
    {
        std::lock_guard<std::mutex> _{mut[i]};
        wc[i].job = job;
        wc[i].flag = 1;
    }
    cond[i].notify_one();
}

static void worker_main(worker_conf_t* my_wc) {
    auto& my_mut = mut[my_wc->thread_id];
    auto& my_cond = cond[my_wc->thread_id];
    for (;;) {
        if (busywait_) {
            while (my_wc->flag == 0) {
                ;
            }
            if (my_wc->flag == 1) {
                (*my_wc->job)(nrn_threads + my_wc->thread_id);
            } else {
                return;
            }
            my_wc->flag = 0;
            my_cond.notify_one();
        } else {
            {
                std::unique_lock<std::mutex> lock{my_mut};
                my_cond.wait(lock, [my_wc] { return my_wc->flag != 0; });
            }
            my_mut.lock();
            if (my_wc->flag == 1) {
                my_mut.unlock();
                (*my_wc->job)(nrn_threads + my_wc->thread_id);
            } else {
                my_mut.unlock();
                return;
            }
            {
                std::lock_guard<std::mutex> _{my_mut};
                my_wc->flag = 0;
            }
            my_cond.notify_one();
        }
    }
    return;
}

static void threads_create() {
#if NRNMPI
    if (nrn_nthread > 1 && nrnmpi_numprocs > 1 && nrn_cannot_use_threads_and_mpi == 1) {
        if (nrnmpi_myid == 0) {
            printf("This MPI is not threadsafe so threads are disabled.\n");
        }
        nrn_thread_parallel_ = 0;
        return;
    }
#endif
    if (nrn_nthread > 1) {
        CACHELINE_ALLOC(wc, worker_conf_t, nrn_nthread);
        // Cannot easily use std::vector because std::condition_variable is not
        // moveable.
        cond = std::make_unique<std::condition_variable[]>(nrn_nthread);
        // Cannot easily use std::vector because std::mutex is not moveable.
        mut = std::make_unique<std::mutex[]>(nrn_nthread);
        worker_threads.reserve(nrn_nthread);
        // worker_threads[0] does not appear to be used
        worker_threads.emplace_back();
        for (int i = 1; i < nrn_nthread; ++i) {
            wc[i].flag = 0;
            wc[i].thread_id = i;
            worker_threads.emplace_back(worker_main, &(wc[i]));
        }
        if (!_interpreter_lock) {
            interpreter_locked = 0;
            _interpreter_lock = std::make_unique<std::mutex>();
        }
        if (!nrn::nmodlmutex) {
            nrn::nmodlmutex = std::make_unique<std::mutex>();
        }
        nrn_thread_parallel_ = 1;
    } else {
        nrn_thread_parallel_ = 0;
    }
}

static void threads_free() {
    if (!worker_threads.empty()) {
        wait_for_workers();
        for (int i = 1; i < nrn_nthread; ++i) {
            {
                std::lock_guard<std::mutex> _{mut[i]};
                wc[i].flag = -1;
            }
            cond[i].notify_one();
            worker_threads[i].join();
        }
        cond.reset();
        mut.reset();
        worker_threads.clear();
        free(std::exchange(wc, nullptr));
    }
    if (_interpreter_lock) {
        _interpreter_lock.reset();
        interpreter_locked = 0;
    }
    nrn::nmodlmutex.reset();
    nrn_thread_parallel_ = 0;
}

#else  /* NRN_ENABLE_THREADS */

static void threads_create() {
    nrn_thread_parallel_ = 0;
}
static void threads_free() {
    nrn_thread_parallel_ = 0;
}
#endif /* !NRN_ENABLE_THREADS */

void nrn_thread_error(const char* s) {
    if (nrn_nthread != 1) {
        hoc_execerror(s, (char*) 0);
    }
}

void nrn_threads_create(int n, int parallel) {
    int i, j;
    NrnThread* nt;
    if (nrn_nthread != n) {
        /*printf("sizeof(NrnThread)=%d   sizeof(Memb_list)=%d\n", sizeof(NrnThread),
         * sizeof(Memb_list));*/
        nrn_threads_free();
        for (i = 0; i < nrn_nthread; ++i) {
            nt = nrn_threads + i;
            if (nt->userpart) {
                hoc_obj_unref(nt->userpart);
            }
        }
        free(std::exchange(nrn_threads, nullptr));
        nrn_nthread = n;
        if (n > 0) {
            CACHELINE_ALLOC(nrn_threads, NrnThread, n);
            for (i = 0; i < n; ++i) {
                nt = nrn_threads + i;
                nt->_t = 0.;
                nt->_dt = -1e9;
                nt->id = i;
                nt->_stop_stepping = 0;
                nt->tml = (NrnThreadMembList*) 0;
                nt->_ml_list = NULL;
                nt->roots = (hoc_List*) 0;
                nt->userpart = 0;
                nt->ncell = 0;
                nt->end = 0;
                for (j = 0; j < BEFORE_AFTER_SIZE; ++j) {
                    nt->tbl[j] = (NrnThreadBAList*) 0;
                }
                nt->_actual_rhs = 0;
                nt->_actual_d = 0;
                nt->_actual_a = 0;
                nt->_actual_b = 0;
                nt->_actual_v = 0;
                nt->_actual_area = 0;
                nt->_v_parent_index = 0;
                nt->_v_node = 0;
                nt->_v_parent = 0;
                nt->_ecell_memb_list = 0;
                nt->_ecell_child_cnt = 0;
                nt->_ecell_children = NULL;
                nt->_sp13mat = 0;
                nt->_ctime = 0.0;
                nt->_vcv = 0;
                nt->_nrn_fast_imem = 0;
            }
        }
        v_structure_change = 1;
        diam_changed = 1;
    }
    if (nrn_thread_parallel_ != parallel) {
        threads_free();
        if (parallel) {
            threads_create();
        }
    }
    /*printf("nrn_threads_create %d %d\n", nrn_nthread, nrn_thread_parallel_);*/
}

/*
Avoid invalidating pointers to i_membrane_ unless the number of compartments
in a thread has changed.
*/
static int fast_imem_nthread_ = 0;
static int* fast_imem_size_ = NULL;
static _nrn_Fast_Imem* fast_imem_;

static void fast_imem_free() {
    int i;
    for (i = 0; i < nrn_nthread; ++i) {
        nrn_threads[i]._nrn_fast_imem = NULL;
    }
    for (i = 0; i < fast_imem_nthread_; ++i) {
        if (fast_imem_size_[i] > 0) {
            free(fast_imem_[i]._nrn_sav_rhs);
            free(fast_imem_[i]._nrn_sav_d);
        }
    }
    if (fast_imem_nthread_) {
        free(fast_imem_size_);
        free(fast_imem_);
        fast_imem_nthread_ = 0;
        fast_imem_size_ = NULL;
        fast_imem_ = NULL;
    }
}

static void fast_imem_alloc() {
    int i;
    if (fast_imem_nthread_ != nrn_nthread) {
        fast_imem_free();
        fast_imem_nthread_ = nrn_nthread;
        fast_imem_size_ = static_cast<int*>(ecalloc(nrn_nthread, sizeof(int)));
        fast_imem_ = (_nrn_Fast_Imem*) ecalloc(nrn_nthread, sizeof(_nrn_Fast_Imem));
    }
    for (i = 0; i < nrn_nthread; ++i) {
        NrnThread* nt = nrn_threads + i;
        int n = nt->end;
        _nrn_Fast_Imem* fi = fast_imem_ + i;
        if (n != fast_imem_size_[i]) {
            if (fast_imem_size_[i] > 0) {
                free(fi->_nrn_sav_rhs);
                free(fi->_nrn_sav_d);
            }
            if (n > 0) {
                CACHELINE_CALLOC(fi->_nrn_sav_rhs, double, n);
                CACHELINE_CALLOC(fi->_nrn_sav_d, double, n);
            }
            fast_imem_size_[i] = n;
        }
    }
}

void nrn_fast_imem_alloc() {
    if (nrn_use_fast_imem) {
        int i;
        fast_imem_alloc();
        for (i = 0; i < nrn_nthread; ++i) {
            nrn_threads[i]._nrn_fast_imem = fast_imem_ + i;
        }
    } else {
        fast_imem_free();
    }
}

void nrn_threads_free() {
    threads_free();
    int it, i;
    for (it = 0; it < nrn_nthread; ++it) {
        NrnThread* nt = nrn_threads + it;
        NrnThreadMembList *tml, *tml2;
        for (tml = nt->tml; tml; tml = tml2) {
            Memb_list* ml = tml->ml;
            tml2 = tml->next;
            free((char*) ml->nodelist);
            free((char*) ml->nodeindices);
            if (memb_func[tml->index].hoc_mech) {
                free((char*) ml->prop);
            } else {
                free((char*) ml->data);
                free((char*) ml->pdata);
            }
            if (ml->_thread) {
                if (memb_func[tml->index].thread_cleanup_) {
                    (*memb_func[tml->index].thread_cleanup_)(ml->_thread);
                }
                free((char*) ml->_thread);
            }
            free((char*) ml);
            free((char*) tml);
        }
        if (nt->_ml_list) {
            free((char*) nt->_ml_list);
            nt->_ml_list = NULL;
        }
        for (i = 0; i < BEFORE_AFTER_SIZE; ++i) {
            NrnThreadBAList *tbl, *tbl2;
            for (tbl = nt->tbl[i]; tbl; tbl = tbl2) {
                tbl2 = tbl->next;
                free((char*) tbl);
            }
            nt->tbl[i] = (NrnThreadBAList*) 0;
        }
        nt->tml = (NrnThreadMembList*) 0;
        if (nt->userpart == 0 && nt->roots) {
            hoc_l_freelist(&nt->roots);
            nt->ncell = 0;
        }
        if (nt->_actual_rhs) {
            free((char*) nt->_actual_rhs);
            nt->_actual_rhs = 0;
        }
        if (nt->_actual_d) {
            free((char*) nt->_actual_d);
            nt->_actual_d = 0;
        }
        if (nt->_actual_a) {
            free((char*) nt->_actual_a);
            nt->_actual_a = 0;
        }
        if (nt->_actual_b) {
            free((char*) nt->_actual_b);
            nt->_actual_b = 0;
        }
        if (nt->_v_parent_index) {
            free((char*) nt->_v_parent_index);
            nt->_v_parent_index = 0;
        }
        if (nt->_v_node) {
            free((char*) nt->_v_node);
            nt->_v_node = 0;
        }
        if (nt->_v_parent) {
            free((char*) nt->_v_parent);
            nt->_v_parent = 0;
        }
        nt->_ecell_memb_list = 0;
        if (nt->_ecell_children) {
            nt->_ecell_child_cnt = 0;
            free(nt->_ecell_children);
            nt->_ecell_children = NULL;
        }
        if (nt->_sp13mat) {
            spDestroy(nt->_sp13mat);
            nt->_sp13mat = 0;
        }
        nt->_nrn_fast_imem = NULL;
        /* following freed by nrn_recalc_node_ptrs */
        nrn_old_thread_save();
        nt->_actual_v = 0;
        nt->_actual_area = 0;
        nt->end = 0;
        nt->ncell = 0;
        nt->_vcv = 0;
    }
}

/* be careful to make the tml list in proper memb_order. This is important */
/* for correct finitialize where mechanisms that write concentrations must be */
/* after ions and before mechanisms that read concentrations. */

static void thread_memblist_setup(NrnThread* _nt, int* mlcnt, void** vmap) {
    int i, ii;
    Node* nd;
    Prop* p;
    NrnThreadMembList *tml, **ptml;
    Memb_list** mlmap = (Memb_list**) vmap;
    BAMech** bamap = (BAMech**) vmap;
#if 0
printf("thread_memblist_setup %lx v_node_count=%d ncell=%d end=%d\n", (long)nth, v_node_count, nth->ncell, nth->end);
#endif
    for (i = 0; i < n_memb_func; ++i) {
        mlcnt[i] = 0;
    }

    /* count */
    for (i = 0; i < _nt->end; ++i) {
        nd = _nt->_v_node[i];
        for (p = nd->prop; p; p = p->next) {
            if (memb_func[p->type].current || memb_func[p->type].state ||
                memb_func[p->type].initialize) {
                ++mlcnt[p->type];
            }
        }
    }
    /* allocate */
    ptml = &_nt->tml;
    for (ii = 0; ii < n_memb_func; ++ii) {
        i = memb_order_[ii];
        if (mlcnt[i]) {
            if (_nt->id > 0 && memb_func[i].vectorized == 0) {
                hoc_execerror(memb_func[i].sym->name, "is not thread safe");
            }
            /*printf("thread_memblist_setup %lx type=%d cnt=%d\n", (long)nth, i, mlcnt[i]);*/
            CACHELINE_ALLOC(tml, NrnThreadMembList, 1);
            tml->index = i;
            tml->next = (NrnThreadMembList*) 0;
            *ptml = tml;
            ptml = &tml->next;
            CACHELINE_ALLOC(tml->ml, Memb_list, 1);
            if (i == EXTRACELL) {
                _nt->_ecell_memb_list = tml->ml;
            }
            mlmap[i] = tml->ml;
            CACHELINE_ALLOC(tml->ml->nodelist, Node*, mlcnt[i]);
            CACHELINE_ALLOC(tml->ml->nodeindices, int, mlcnt[i]);
            if (memb_func[i].hoc_mech) {
                tml->ml->prop = (Prop**) emalloc(mlcnt[i] * sizeof(Prop*));
            } else {
                CACHELINE_ALLOC(tml->ml->data, double*, mlcnt[i]);
                CACHELINE_ALLOC(tml->ml->pdata, Datum*, mlcnt[i]);
            }
            tml->ml->_thread = (Datum*) 0;
            if (memb_func[i].thread_size_) {
                tml->ml->_thread = (Datum*) ecalloc(memb_func[i].thread_size_, sizeof(Datum));
                if (memb_func[tml->index].thread_mem_init_) {
                    (*memb_func[tml->index].thread_mem_init_)(tml->ml->_thread);
                }
            }
            tml->ml->nodecount = 0; /* counted again below */
        }
    }
    CACHELINE_CALLOC(_nt->_ml_list, Memb_list*, n_memb_func);
    for (tml = _nt->tml; tml; tml = tml->next) {
        _nt->_ml_list[tml->index] = tml->ml;
    }

    /* fill */
    for (i = 0; i < _nt->end; ++i) {
        nd = _nt->_v_node[i];
        for (p = nd->prop; p; p = p->next) {
            if (memb_func[p->type].current || memb_func[p->type].state ||
                memb_func[p->type].initialize) {
                Memb_list* ml = mlmap[p->type];
                ml->nodelist[ml->nodecount] = nd;
                ml->nodeindices[ml->nodecount] = nd->v_node_index;
                if (memb_func[p->type].hoc_mech) {
                    ml->prop[ml->nodecount] = p;
                } else {
                    ml->data[ml->nodecount] = p->param;
                    ml->pdata[ml->nodecount] = p->dparam;
                }
                ++ml->nodecount;
            }
        }
    }
    /* count and store any Node* with the property
       nd->extnode == NULL && nd->pnd != NULL && nd->pnd->extcell != NULL
    */
    if (_nt->_ecell_memb_list) {
        Node* pnd;
        int cnt = 0;
        for (i = 0; i < _nt->end; ++i) {
            nd = _nt->_v_node[i];
            pnd = _nt->_v_parent[i];
            if (nd->extnode == NULL && pnd && pnd->extnode) {
                ++cnt;
            }
        }
        if (cnt) {
            Node** p;
            CACHELINE_ALLOC(_nt->_ecell_children, Node*, cnt);
            _nt->_ecell_child_cnt = cnt;
            p = _nt->_ecell_children;
            cnt = 0;
            for (i = 0; i < _nt->end; ++i) {
                nd = _nt->_v_node[i];
                pnd = _nt->_v_parent[i];
                if (nd->extnode == NULL && pnd && pnd->extnode) {
                    p[cnt++] = nd;
                }
            }
        }
    }
#if 0
	for (i=0; i < n_memb_func; ++i) {
		if (mlcnt[i]) {assert(mlcnt[i] = mlmap[i]->nodecount);}
	}
	for (tml = _nt->tml; tml; tml = tml->next) {
		assert(mlcnt[tml->index] == tml->ml->nodecount);
	}
#endif
    /* fill the ba lists */
    /* need map from ml type to BA type. Reuse vmap */
    for (i = 0; i < BEFORE_AFTER_SIZE; ++i) {
        BAMech* bam;
        NrnThreadBAList *tbl, **ptbl;
        for (ii = 0; ii < n_memb_func; ++ii) {
            bamap[ii] = (BAMech*) 0;
        }
        for (bam = bamech_[i]; bam; bam = bam->next) {
            // Save first before-after block only.  In case of multiple
            // before-after blocks with the same mech type, we will get
            // subsequent ones using linked list below.
            if (!bamap[bam->type]) {
                bamap[bam->type] = bam;
            }
        }
        // necessary to keep in order wrt multiple BAMech with same mech type
        ptbl = _nt->tbl + i;
        for (tml = _nt->tml; tml; tml = tml->next) {
            if (bamap[tml->index]) {
                int mtype = tml->index;
                Memb_list* ml = tml->ml;
                for (bam = bamap[mtype]; bam && bam->type == mtype; bam = bam->next) {
                    tbl = (NrnThreadBAList*) emalloc(sizeof(NrnThreadBAList));
                    *ptbl = tbl;
                    tbl->next = (NrnThreadBAList*) 0;
                    tbl->bam = bam;
                    tbl->ml = ml;
                    ptbl = &(tbl->next);
                }
            }
        }
    }
    /* fill in the Point_process._vnt value. */
    /* artificial cells done in v_setup_vectors() */
    for (tml = _nt->tml; tml; tml = tml->next)
        if (memb_func[tml->index].is_point) {
            for (i = 0; i < tml->ml->nodecount; ++i) {
                Point_process* pnt = (Point_process*) tml->ml->pdata[i][1]._pvoid;
                pnt->_vnt = (void*) _nt;
            }
        }
}

static void nrn_thread_memblist_setup() {
    int it, *mlcnt;
    void** vmap;
    mlcnt = (int*) emalloc(n_memb_func * sizeof(int));
    vmap = (void**) emalloc(n_memb_func * sizeof(void*));
    for (it = 0; it < nrn_nthread; ++it) {
        thread_memblist_setup(nrn_threads + it, mlcnt, vmap);
    }
    nrn_fast_imem_alloc();
    free((char*) vmap);
    free((char*) mlcnt);
    nrn_mk_table_check();
    if (nrn_mk_transfer_thread_data_) {
        (*nrn_mk_transfer_thread_data_)();
    }
}

/* secorder needs to correspond to cells in NrnThread with roots */
/* at the beginning of each thread region */
/* this differs from original secorder where all roots are at the beginning */
/* in passing, also set start and end indices. */

static void reorder_secorder() {
    NrnThread* _nt;
    Section *sec, *ch;
    Node* nd;
    hoc_Item* qsec;
    hoc_List* sl;
    int order, isec, i, j, inode;
    /* count and allocate */
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        sec->order = -1;
    }
    order = 0;
    FOR_THREADS(_nt) {
        /* roots of this thread */
        sl = _nt->roots;
        inode = 0;
        ITERATE(qsec, sl) {
            sec = hocSEC(qsec);
            assert(sec->order == -1);
            secorder[order] = sec;
            sec->order = order;
            ++order;
            nd = sec->parentnode;
            nd->_nt = _nt;
            inode += 1;
        }
        /* all children of what is already in secorder */
        for (isec = order - _nt->ncell; isec < order; ++isec) {
            sec = secorder[isec];
            /* to make it easy to fill in PreSyn.nt_*/
            sec->prop->dparam[9]._pvoid = (void*) _nt;
            for (j = 0; j < sec->nnode; ++j) {
                nd = sec->pnode[j];
                nd->_nt = _nt;
                inode += 1;
            }
            for (ch = sec->child; ch; ch = ch->sibling) {
                assert(ch->order == -1);
                secorder[order] = ch;
                ch->order = order;
                ++order;
            }
        }
        _nt->end = inode;
        CACHELINE_CALLOC(_nt->_actual_rhs, double, inode);
        CACHELINE_CALLOC(_nt->_actual_d, double, inode);
        CACHELINE_CALLOC(_nt->_actual_a, double, inode);
        CACHELINE_CALLOC(_nt->_actual_b, double, inode);
        CACHELINE_CALLOC(_nt->_v_node, Node*, inode);
        CACHELINE_CALLOC(_nt->_v_parent, Node*, inode);
        CACHELINE_CALLOC(_nt->_v_parent_index, int, inode);
    }
    /* do it again and fill _v_node and _v_parent */
    /* index each cell section in relative order. Do offset later */
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        sec->order = -1;
    }
    order = 0;
    FOR_THREADS(_nt) {
        /* roots of this thread */
        sl = _nt->roots;
        inode = 0;
        ITERATE(qsec, sl) {
            sec = hocSEC(qsec);
            assert(sec->order == -1);
            secorder[order] = sec;
            sec->order = order;
            ++order;
            nd = sec->parentnode;
            nd->_nt = _nt;
            _nt->_v_node[inode] = nd;
            _nt->_v_parent[inode] = (Node*) 0;
            _nt->_v_node[inode]->v_node_index = inode;
            inode += 1;
        }
        /* all children of what is already in secorder */
        for (isec = order - _nt->ncell; isec < order; ++isec) {
            sec = secorder[isec];
            /* to make it easy to fill in PreSyn.nt_*/
            sec->prop->dparam[9]._pvoid = (void*) _nt;
            for (j = 0; j < sec->nnode; ++j) {
                nd = sec->pnode[j];
                nd->_nt = _nt;
                _nt->_v_node[inode] = nd;
                if (j) {
                    _nt->_v_parent[inode] = sec->pnode[j - 1];
                } else {
                    _nt->_v_parent[inode] = sec->parentnode;
                }
                _nt->_v_node[inode]->v_node_index = inode;
                inode += 1;
            }
            for (ch = sec->child; ch; ch = ch->sibling) {
                assert(ch->order == -1);
                secorder[order] = ch;
                ch->order = order;
                ++order;
            }
        }
        _nt->end = inode;
    }
    assert(order == section_count);
    /*assert(inode == v_node_count);*/
    /* not missing any */
    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        assert(sec->order != -1);
    }

    /* here is where multisplit reorders the nodes. Afterwards
      in either case, we can then point to v, d, rhs in proper
      node order
    */
    FOR_THREADS(_nt) for (inode = 0; inode < _nt->end; ++inode) {
        _nt->_v_node[inode]->_classical_parent = _nt->_v_parent[inode];
    }
    if (nrn_multisplit_setup_) {
        /* classical order abandoned */
        (*nrn_multisplit_setup_)();
    }
    /* make the Nodes point to the proper d, rhs */
    FOR_THREADS(_nt) {
        for (j = 0; j < _nt->end; ++j) {
            Node* nd = _nt->_v_node[j];
            nd->_d = _nt->_actual_d + j;
            nd->_rhs = _nt->_actual_rhs + j;
        }
    }
    /* because the d,rhs changed, if multisplit is used we need to update
      the reduced tree gather/scatter pointers
    */
    if (nrn_multisplit_setup_) {
        nrn_multisplit_ptr_update();
    }
}


void nrn_mk_table_check() {
    int i, id, index;
    int* ix;
    if (table_check_) {
        free((void*) table_check_);
        table_check_ = (Datum*) 0;
    }
    ix = (int*) emalloc(n_memb_func * sizeof(int));
    for (i = 0; i < n_memb_func; ++i) {
        ix[i] = -1;
    }
    table_check_cnt_ = 0;
    for (id = 0; id < nrn_nthread; ++id) {
        NrnThread* nt = nrn_threads + id;
        NrnThreadMembList* tml;
        for (tml = nt->tml; tml; tml = tml->next) {
            index = tml->index;
            if (memb_func[index].thread_table_check_ && ix[index] == -1) {
                ix[index] = id;
                table_check_cnt_ += 2;
            }
        }
    }
    if (table_check_cnt_) {
        table_check_ = (Datum*) emalloc(table_check_cnt_ * sizeof(Datum));
    }
    i = 0;
    for (id = 0; id < nrn_nthread; ++id) {
        NrnThread* nt = nrn_threads + id;
        NrnThreadMembList* tml;
        for (tml = nt->tml; tml; tml = tml->next) {
            index = tml->index;
            if (memb_func[index].thread_table_check_ && ix[index] == id) {
                table_check_[i++].i = id;
                table_check_[i++]._pvoid = (void*) tml;
            }
        }
    }
    free((void*) ix);
}

void nrn_thread_table_check() {
    int i;
    for (i = 0; i < table_check_cnt_; i += 2) {
        NrnThread* nt = nrn_threads + table_check_[i].i;
        NrnThreadMembList* tml = (NrnThreadMembList*) table_check_[i + 1]._pvoid;
        Memb_list* ml = tml->ml;
        (*memb_func[tml->index].thread_table_check_)(
            ml->data[0], ml->pdata[0], ml->_thread, nt, tml->index);
    }
}

/* if it is possible for more than one thread to get into the
   interpreter, lock it. */
void nrn_hoc_lock() {
#if NRN_ENABLE_THREADS
    if (nrn_inthread_) {
        _interpreter_lock->lock();
        interpreter_locked = 1;
    }
#endif
}
void nrn_hoc_unlock() {
#if NRN_ENABLE_THREADS
    if (interpreter_locked) {
        interpreter_locked = 0;
        _interpreter_lock->unlock();
    }
#endif
}

void nrn_multithread_job(void* (*job)(NrnThread*) ) {
    int i;
#if NRN_ENABLE_THREADS
    if (nrn_thread_parallel_) {
        nrn_inthread_ = 1;
        for (i = 1; i < nrn_nthread; ++i) {
            send_job_to_slave(i, job);
        }
        (*job)(nrn_threads);
        wait_for_workers();
        nrn_inthread_ = 0;
    } else { /* sequential */
#else
    {
#endif
        for (i = 1; i < nrn_nthread; ++i) {
            (*job)(nrn_threads + i);
        }
        (*job)(nrn_threads);
    }
}


void nrn_onethread_job(int i, void* (*job)(NrnThread*) ) {
    assert(i >= 0 && i < nrn_nthread);
#if NRN_ENABLE_THREADS
    if (nrn_thread_parallel_) {
        if (i > 0) {
            send_job_to_slave(i, job);
            wait_for_workers();
        } else {
            (*job)(nrn_threads);
        }
    } else {
#else
    {
#endif
        (*job)(nrn_threads + i);
    }
}

void nrn_wait_for_threads() {
#if NRN_ENABLE_THREADS
    if (nrn_thread_parallel_) {
        wait_for_workers();
    }
#endif
}

void nrn_thread_partition(int it, Object* sl) {
    NrnThread* nt;
    assert(it >= 0 && it < nrn_nthread);
    nt = nrn_threads + it;
    if (nt->userpart == nullptr && nt->roots) {
        hoc_l_freelist(&nt->roots);
    }
    if (sl) {
        hoc_obj_ref(sl);
    }
    if (nt->userpart) {
        hoc_obj_unref(nt->userpart);
        nt->userpart = nullptr;
        nt->roots = (hoc_List*) 0;
    }
    if (sl) {
        nt->userpart = sl; /* already reffed above */
        nt->roots = (hoc_List*) sl->u.this_pointer;
    }
    v_structure_change = 1;
}

int nrn_user_partition() {
    int i, it, b, n;
    hoc_Item* qsec;
    hoc_List* sl;
    char buf[256];
    Section* sec;
    NrnThread* nt;
    /* all one or all the other*/
    b = (nrn_threads[0].userpart != nullptr);
    for (it = 1; it < nrn_nthread; ++it) {
        if ((nrn_threads[it].userpart != nullptr) != b) {
            hoc_execerror("some threads have a user defined partition", "and some do not");
        }
    }
    if (!b) {
        return 0;
    }

    /* discard partition if any section mentioned has been deleted. The
        model has changed */
    FOR_THREADS(nt) {
        sl = nt->roots;
        ITERATE(qsec, sl) {
            sec = hocSEC(qsec);
            if (!sec->prop) {
                for (i = 0; i < nrn_nthread; ++i) {
                    nrn_thread_partition(i, nullptr);
                }
                return 0;
            }
        }
    }

    // ForAllSections(sec)
    ITERATE(qsec, section_list) {
        Section* sec = hocSEC(qsec);
        sec->volatile_mark = 0;
    }
    /* fill in ncell and verify consistency */
    n = 0;
    for (it = 0; it < nrn_nthread; ++it) {
        nt = nrn_threads + it;
        sl = nt->roots;
        nt->ncell = 0;
        ITERATE(qsec, sl) {
            sec = hocSEC(qsec);
            ++nt->ncell;
            ++n;
            if (sec->parentsec) {
                sprintf(buf, "in thread partition %d is not a root section", it);
                hoc_execerror(secname(sec), buf);
            }
            if (sec->volatile_mark) {
                sprintf(buf, "appeared again in partition %d", it);
                hoc_execerror(secname(sec), buf);
            }
            sec->volatile_mark = 1;
        }
    }
    if (n != nrn_global_ncell) {
        sprintf(buf,
                "The total number of cells, %d, is different than the number of user partition "
                "cells, %d\n",
                nrn_global_ncell,
                n);
        hoc_execerror(buf, (char*) 0);
    }
    return 1;
}

void nrn_use_busywait(int b) {
#if NRN_ENABLE_THREADS
    if (allow_busywait_ && nrn_thread_parallel_) {
        if (b == 0 && busywait_main_ == 1) {
            busywait_ = 0;
            nrn_multithread_job(nulljob);
            busywait_main_ = 0;
        } else if (b == 1 && busywait_main_ == 0) {
            busywait_main_ = 1;
            wait_for_workers();
            busywait_ = 1;
            nrn_multithread_job(nulljob);
        }
    } else {
        if (busywait_main_ == 1) {
            busywait_ = 0;
            nrn_multithread_job(nulljob);
            busywait_main_ = 0;
        }
    }
#endif
}

int nrn_allow_busywait(int b) {
    int old = allow_busywait_;
    allow_busywait_ = b;
    return old;
}

int nrn_how_many_processors() {
#if NRN_ENABLE_THREADS
    // For machines with hyperthreading this probably returns the number of
    // logical, not physical, cores.
    return std::thread::hardware_concurrency();
#else
    return 1;
#endif
}
