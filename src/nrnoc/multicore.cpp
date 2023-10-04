/* included by treeset.cpp */
#include <nrnmpi.h>

#include "hoclist.h"
#include "section.h"
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

#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <variant>
#include <vector>
#include <iostream>

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
extern double nrn_timeus();
extern void (*nrn_multisplit_setup_)();
extern int v_structure_change;
extern int diam_changed;
extern Section** secorder;
extern int section_count;
extern void spDestroy(char*);

void nrn_mk_table_check();
static std::vector<std::pair<int, NrnThreadMembList*>> table_check_;
static int allow_busywait_;

static void* nulljob(NrnThread* nt) {
    return nullptr;
}

int nrn_inthread_;

namespace nrn {
std::unique_ptr<std::mutex> nmodlmutex;
}

namespace {
bool interpreter_locked{false};
std::unique_ptr<std::mutex> interpreter_lock;

enum struct worker_flag { execute_job, exit, wait };

// With C++17 and alignment-aware allocators we could do something like
// alignas(std::hardware_destructive_interference_size) here and then use a
// regular vector. https://en.cppreference.com/w/cpp/compiler_support/17 shows
// that std::hardware_destructive_interference_size is not very well supported.
struct worker_conf_t {
    /* for nrn_solve etc.*/
    std::variant<std::monostate,
                 worker_job_t,
                 std::pair<worker_job_with_token_t, neuron::model_sorted_token const*>>
        job{};
    std::size_t thread_id{};
    worker_flag flag{worker_flag::wait};
    friend bool operator==(worker_conf_t const& lhs, worker_conf_t const& rhs) {
        return lhs.flag == rhs.flag && lhs.thread_id == rhs.thread_id && lhs.job == rhs.job;
    }
};

struct worker_kernel {
    worker_kernel(std::size_t thread_id)
        : m_thread_id{thread_id} {}
    void operator()(std::monostate const&) const {
        throw std::runtime_error("worker_kernel");
    }
    void operator()(worker_job_t job) const {
        job(nrn_threads + m_thread_id);
    }
    void operator()(
        std::pair<worker_job_with_token_t, neuron::model_sorted_token const*> const& pair) const {
        auto const& [job, token_ptr] = pair;
        job(*token_ptr, nrn_threads[m_thread_id]);
    }

  private:
    std::size_t m_thread_id{};
};

void worker_main(worker_conf_t* my_wc_ptr,
                 std::condition_variable* my_cond_ptr,
                 std::mutex* my_mut_ptr) {
    assert(my_cond_ptr);
    assert(my_mut_ptr);
    assert(my_wc_ptr);
    auto& cond{*my_cond_ptr};
    auto& mut{*my_mut_ptr};
    auto& wc{*my_wc_ptr};
    for (;;) {
        if (busywait_) {
            // WARNING: this branch has not been extensively tested after the
            // std::thread migration.
            while (wc.flag == worker_flag::wait) {
                ;
            }
            if (wc.flag == worker_flag::exit) {
                return;
            }
            assert(wc.flag == worker_flag::execute_job);
            std::visit(worker_kernel{wc.thread_id}, wc.job);
            wc.flag = worker_flag::wait;
            wc.job = std::monostate{};
            cond.notify_one();
        } else {
            worker_conf_t conf{};
            {
                // Wait until we have a job to execute or we have been told to
                // shut down.
                std::unique_lock<std::mutex> lock{mut};
                cond.wait(lock, [&wc] { return wc.flag != worker_flag::wait; });
                // Received instructions from the coordinator thread.
                assert(wc.flag == worker_flag::execute_job || wc.flag == worker_flag::exit);
                if (wc.flag == worker_flag::exit) {
                    return;
                }
                assert(wc.flag == worker_flag::execute_job);
                // Save the workload + argument to local variables before
                // releasing the mutex.
                conf = wc;
            }
            // Execute the workload without keeping the mutex
            std::visit(worker_kernel{conf.thread_id}, conf.job);
            // Signal that the work is completed and this thread is becoming
            // idle
            {
                std::lock_guard<std::mutex> _{mut};
                // Make sure we don't accidentally overwrite an exit signal from
                // the coordinating thread.
                if (wc.flag == worker_flag::execute_job) {
                    assert(wc == conf);
                    wc.flag = worker_flag::wait;
                    wc.job = std::monostate{};
                }
            }
            // Notify the coordinating thread.
            cond.notify_one();
        }
    }
}

// Using an instance of a custom type allows us to manage the teardown process
// more easily. TODO: remove the pointless zeroth entry in the vectors/arrays.
struct worker_threads_t {
    worker_threads_t()
        : m_cond{std::make_unique<std::condition_variable[]>(nrn_nthread)}
        , m_mut{std::make_unique<std::mutex[]>(nrn_nthread)} {
        // Note that this does not call the worker_conf_t constructor.
        CACHELINE_ALLOC(m_wc, worker_conf_t, nrn_nthread);
        m_worker_threads.reserve(nrn_nthread);
        // worker_threads[0] does not appear to be used
        m_worker_threads.emplace_back();
        for (std::size_t i = 1; i < nrn_nthread; ++i) {
            new (m_wc + i) worker_conf_t{};
            m_wc[i].thread_id = i;
            m_worker_threads.emplace_back(worker_main, &(m_wc[i]), &(m_cond[i]), &(m_mut[i]));
        }
        if (!interpreter_lock) {
            interpreter_locked = false;
            interpreter_lock = std::make_unique<std::mutex>();
        }
        if (!nrn::nmodlmutex) {
            nrn::nmodlmutex = std::make_unique<std::mutex>();
        }
    }

    ~worker_threads_t() {
        assert(m_worker_threads.size() == nrn_nthread);
        wait();
        for (std::size_t i = 1; i < nrn_nthread; ++i) {
            {
                std::lock_guard<std::mutex> _{m_mut[i]};
                m_wc[i].flag = worker_flag::exit;
            }
            m_cond[i].notify_one();
            m_worker_threads[i].join();
        }
        if (interpreter_lock) {
            interpreter_lock.reset();
            interpreter_locked = 0;
        }
        nrn::nmodlmutex.reset();
        free(std::exchange(m_wc, nullptr));
    }

    void assign_job(std::size_t worker, worker_job_t job) {
        assert(worker > 0);
        auto& cond = m_cond[worker];
        auto& wc = m_wc[worker];
        {
            std::unique_lock<std::mutex> lock{m_mut[worker]};
            // Wait until the worker is idle.
            cond.wait(lock, [&wc] { return wc.flag == worker_flag::wait; });
            assert(std::holds_alternative<std::monostate>(wc.job));
            assert(wc.thread_id == worker);
            wc.job = job;
            wc.flag = worker_flag::execute_job;
        }
        // Notify the worker that it has new work to do.
        cond.notify_one();
    }

    void assign_job(std::size_t worker,
                    neuron::model_sorted_token const& cache_token,
                    worker_job_with_token_t job) {
        assert(worker > 0);
        auto& cond = m_cond[worker];
        auto& wc = m_wc[worker];
        {
            std::unique_lock<std::mutex> lock{m_mut[worker]};
            // Wait until the worker is idle.
            cond.wait(lock, [&wc] { return wc.flag == worker_flag::wait; });
            assert(std::holds_alternative<std::monostate>(wc.job));
            assert(wc.thread_id == worker);
            wc.job = std::make_pair(job, &cache_token);
            wc.flag = worker_flag::execute_job;
        }
        // Notify the worker that it has new work to do.
        cond.notify_one();
    }

    // Wait until all worker threads are waiting
    void wait() const {
        for (std::size_t i = 1; i < nrn_nthread; ++i) {
            auto& wc{m_wc[i]};
            if (busywait_main_) {
                while (wc.flag != worker_flag::wait) {
                    ;
                }
            } else {
                std::unique_lock<std::mutex> lock{m_mut[i]};
                m_cond[i].wait(lock, [&wc] { return wc.flag == worker_flag::wait; });
            }
        }
    }
    std::size_t num_workers() const {
        return m_worker_threads.size();
    }

  private:
    // Cannot easily use std::vector because std::condition_variable is not moveable.
    std::unique_ptr<std::condition_variable[]> m_cond;
    // Cannot easily use std::vector because std::mutex is not moveable.
    std::unique_ptr<std::mutex[]> m_mut;
    std::vector<std::thread> m_worker_threads;
    worker_conf_t* m_wc{};
};
std::unique_ptr<worker_threads_t> worker_threads{};
}  // namespace

void nrn_thread_error(const char* s) {
    if (nrn_nthread != 1) {
        hoc_execerror(s, (char*) 0);
    }
}

void nrn_threads_create(int n, bool parallel) {
    int i, j;
    NrnThread* nt;
    if (nrn_nthread != n) {
        worker_threads.reset();
        // If the number of threads changes then the node storage data is
        // implicitly no longer sorted, as "sorted" includes being partitioned
        // by NrnThread. Similarly for the mechanism data then "sorted" includes
        // being partitioned by thread.
        // TODO: consider if we can be smarter about how/when we call
        // mark_as_unsorted() for different containers.
        neuron::model().node_data().mark_as_unsorted();
        neuron::model().apply_to_mechanisms([](auto& mech_data) { mech_data.mark_as_unsorted(); });
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
                nt->_sp13_rhs = nullptr;
                nt->_v_parent_index = 0;
                nt->_v_node = 0;
                nt->_v_parent = 0;
                nt->_ecell_memb_list = 0;
                nt->_ecell_child_cnt = 0;
                nt->_ecell_children = NULL;
                nt->_sp13mat = 0;
                nt->_ctime = 0.0;
                nt->_vcv = 0;
                nt->_node_data_offset = 0;
            }
        }
        v_structure_change = 1;
        diam_changed = 1;
    }
#if NRN_ENABLE_THREADS
    // Check if we are enabling/disabling parallelisation over threads
    if (parallel != static_cast<bool>(worker_threads)) {
        worker_threads.reset();
#if NRNMPI
        if (nrn_nthread > 1 && nrnmpi_numprocs > 1 && nrn_cannot_use_threads_and_mpi == 1) {
            if (nrnmpi_myid == 0) {
                printf("This MPI is not threadsafe so threads are disabled.\n");
            }
            return;
        }
#endif
        if (parallel && nrn_nthread > 1) {
            worker_threads = std::make_unique<worker_threads_t>();
        }
    }
#endif
}

void nrn_fast_imem_alloc() {
    // Make sure that storage for the fast_imem calculation exists/is destroyed according to
    // nrn_use_fast_imem
    neuron::model()
        .node_data()
        .set_field_status<neuron::container::Node::field::FastIMemSavD,
                          neuron::container::Node::field::FastIMemSavRHS>(nrn_use_fast_imem);
}

void nrn_threads_free() {
    int it, i;
    for (it = 0; it < nrn_nthread; ++it) {
        NrnThread* nt = nrn_threads + it;
        NrnThreadMembList *tml, *tml2;
        for (tml = nt->tml; tml; tml = tml2) {
            Memb_list* ml = tml->ml;
            tml2 = tml->next;
            free((char*) ml->nodelist);
            free((char*) ml->nodeindices);
            delete[] ml->prop;
            if (!memb_func[tml->index].hoc_mech) {
                free((char*) ml->pdata);
            }
            if (ml->_thread) {
                if (memb_func[tml->index].thread_cleanup_) {
                    (*memb_func[tml->index].thread_cleanup_)(ml->_thread);
                }
                delete[] ml->_thread;
            }
            delete ml;
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
            if (memb_func[p->_type].current || memb_func[p->_type].state ||
                memb_func[p->_type].has_initialize()) {
                ++mlcnt[p->_type];
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
            tml->ml = new Memb_list{i};
            if (i == EXTRACELL) {
                _nt->_ecell_memb_list = tml->ml;
            }
            mlmap[i] = tml->ml;
            CACHELINE_ALLOC(tml->ml->nodelist, Node*, mlcnt[i]);
            CACHELINE_ALLOC(tml->ml->nodeindices, int, mlcnt[i]);
            tml->ml->prop = new Prop*[mlcnt[i]];  // used for ode_map
            if (!memb_func[i].hoc_mech) {
                CACHELINE_ALLOC(tml->ml->pdata, Datum*, mlcnt[i]);
            }
            tml->ml->_thread = (Datum*) 0;
            if (memb_func[i].thread_size_) {
                tml->ml->_thread = new Datum[memb_func[i].thread_size_]{};
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
            if (memb_func[p->_type].current || memb_func[p->_type].state ||
                memb_func[p->_type].has_initialize()) {
                Memb_list* ml = mlmap[p->_type];
                ml->nodelist[ml->nodecount] = nd;
                ml->nodeindices[ml->nodecount] = nd->v_node_index;
                ml->prop[ml->nodecount] = p;
                if (!memb_func[p->_type].hoc_mech) {
                    // ml->_data[ml->nodecount] = p->param;
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
                auto* pnt = tml->ml->pdata[i][1].get<Point_process*>();
                pnt->_vnt = _nt;
            }
        }
}

void nrn_thread_memblist_setup() {
    int it, *mlcnt;
    void** vmap;
    mlcnt = (int*) emalloc(n_memb_func * sizeof(int));
    vmap = (void**) emalloc(n_memb_func * sizeof(void*));
    for (it = 0; it < nrn_nthread; ++it) {
        thread_memblist_setup(nrn_threads + it, mlcnt, vmap);
    }
    // Right now the sorting method updates the storage offsets inside the
    // Memb_list* structures owned by NrnThreads. This is a bit of a design
    // failure, as those offsets should have a lifetime linked to the sorted
    // status of the underlying storage, i.e. they should be part of a cache
    // structure. In any case, because we have just created new Memb_list then
    // their offsets are empty, so we need to trigger a re-sort before they are
    // used.
    neuron::model().apply_to_mechanisms([](auto& mech_data) { mech_data.mark_as_unsorted(); });
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

void reorder_secorder() {
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
            sec->prop->dparam[9] = {neuron::container::do_not_search, _nt};
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
            _nt->_v_parent[inode] = nullptr;  // because this is a root node
            _nt->_v_node[inode]->v_node_index = inode;
            ++inode;
        }
        /* all children of what is already in secorder */
        for (isec = order - _nt->ncell; isec < order; ++isec) {
            sec = secorder[isec];
            /* to make it easy to fill in PreSyn.nt_*/
            sec->prop->dparam[9] = {neuron::container::do_not_search, _nt};
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
    /* because the d,rhs changed, if multisplit is used we need to update
      the reduced tree gather/scatter pointers
    */
    if (nrn_multisplit_setup_) {
        nrn_multisplit_ptr_update();
    }
}


void nrn_mk_table_check() {
    std::size_t table_check_cnt_{};
    std::vector<int> ix(n_memb_func, -1);
    for (int id = 0; id < nrn_nthread; ++id) {
        NrnThread* nt = nrn_threads + id;
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            int index = tml->index;
            if (memb_func[index].thread_table_check_ && ix[index] == -1) {
                ix[index] = id;
                ++table_check_cnt_;
            }
        }
    }
    table_check_.clear();
    table_check_.reserve(table_check_cnt_);
    for (int id = 0; id < nrn_nthread; ++id) {
        NrnThread* nt = nrn_threads + id;
        for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
            int index = tml->index;
            if (memb_func[index].thread_table_check_ && ix[index] == id) {
                table_check_.emplace_back(id, tml);
            }
        }
    }
}

void nrn_thread_table_check(neuron::model_sorted_token const& sorted_token) {
    for (auto [id, tml]: table_check_) {
        Memb_list* ml = tml->ml;
        memb_func[tml->index].thread_table_check_(
            ml, 0, ml->pdata[0], ml->_thread, nrn_threads + id, tml->index, sorted_token);
    }
}

/* if it is possible for more than one thread to get into the
   interpreter, lock it. */
void nrn_hoc_lock() {
#if NRN_ENABLE_THREADS
    if (nrn_inthread_) {
        interpreter_lock->lock();
        interpreter_locked = true;
    }
#endif
}
void nrn_hoc_unlock() {
#if NRN_ENABLE_THREADS
    if (interpreter_locked) {
        interpreter_locked = false;
        interpreter_lock->unlock();
    }
#endif
}

void nrn_multithread_job(worker_job_t job) {
#if NRN_ENABLE_THREADS
    if (worker_threads) {
        nrn_inthread_ = 1;
        for (std::size_t i = 1; i < nrn_nthread; ++i) {
            worker_threads->assign_job(i, job);
        }
        (*job)(nrn_threads);
        worker_threads->wait();
        nrn_inthread_ = 0;
        return;
    }
#endif
    for (std::size_t i = 1; i < nrn_nthread; ++i) {
        (*job)(nrn_threads + i);
    }
    (*job)(nrn_threads);
}

void nrn_multithread_job(neuron::model_sorted_token const& cache_token,
                         worker_job_with_token_t job) {
#if NRN_ENABLE_THREADS
    if (worker_threads) {
        nrn_inthread_ = 1;
        for (std::size_t i = 1; i < nrn_nthread; ++i) {
            worker_threads->assign_job(i, cache_token, job);
        }
        job(cache_token, nrn_threads[0]);
        worker_threads->wait();
        nrn_inthread_ = 0;
        return;
    }
#endif
    for (std::size_t i = 1; i < nrn_nthread; ++i) {
        job(cache_token, nrn_threads[i]);
    }
    job(cache_token, nrn_threads[0]);
}

void nrn_onethread_job(int i, void* (*job)(NrnThread*) ) {
    assert(i >= 0 && i < nrn_nthread);
#if NRN_ENABLE_THREADS
    if (worker_threads) {
        if (i > 0) {
            worker_threads->assign_job(i, job);
            worker_threads->wait();
        } else {
            (*job)(nrn_threads);
        }
        return;
    }
#endif
    (*job)(nrn_threads + i);
}

void nrn_wait_for_threads() {
#if NRN_ENABLE_THREADS
    if (worker_threads) {
        worker_threads->wait();
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

Object** nrn_get_thread_partition(int it) {
    assert(it >= 0 && it < nrn_nthread);
    NrnThread* nt = nrn_threads + it;
    if (!nt->roots) {
        v_setup_vectors();
    }
    // nt->roots is a hoc_List of Section*. Create a new SectionList and copy
    // those Section* into it and ref them.
    hoc_List* sl = hoc_l_newlist();
    Object** po = hoc_temp_objvar(hoc_lookup("SectionList"), sl);
    hoc_Item* qsec;
    ITERATE(qsec, nt->roots) {
        Section* sec = hocSEC(qsec);
        section_ref(sec);
        hoc_l_lappendsec(sl, sec);
    }
    return po;
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
                Sprintf(buf, "in thread partition %d is not a root section", it);
                hoc_execerror(secname(sec), buf);
            }
            if (sec->volatile_mark) {
                Sprintf(buf, "appeared again in partition %d", it);
                hoc_execerror(secname(sec), buf);
            }
            sec->volatile_mark = 1;
        }
    }
    if (n != nrn_global_ncell) {
        Sprintf(buf,
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
    if (allow_busywait_ && worker_threads) {
        if (b == 0 && busywait_main_ == 1) {
            busywait_ = 0;
            nrn_multithread_job(nulljob);
            busywait_main_ = 0;
        } else if (b == 1 && busywait_main_ == 0) {
            busywait_main_ = 1;
            worker_threads->wait();
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

std::size_t nof_worker_threads() {
    return worker_threads.get() ? worker_threads->num_workers() : 0;
}

// Need to be able to use these methods while the model is frozen, so avoid calling the
// zero-parameter get().
double* NrnThread::node_a_storage() {
    return &neuron::model().node_data().get<neuron::container::Node::field::AboveDiagonal>(
        _node_data_offset);
}

double* NrnThread::node_area_storage() {
    return &neuron::model().node_data().get<neuron::container::Node::field::Area>(
        _node_data_offset);
}

double* NrnThread::node_b_storage() {
    return &neuron::model().node_data().get<neuron::container::Node::field::BelowDiagonal>(
        _node_data_offset);
}

double* NrnThread::node_d_storage() {
    return &neuron::model().node_data().get<neuron::container::Node::field::Diagonal>(
        _node_data_offset);
}

double* NrnThread::node_rhs_storage() {
    return &neuron::model().node_data().get<neuron::container::Node::field::RHS>(_node_data_offset);
}

double* NrnThread::node_sav_d_storage() {
    auto& node_data = neuron::model().node_data();
    using Tag = neuron::container::Node::field::FastIMemSavD;
    if (node_data.field_active<Tag>()) {
        return &node_data.get<Tag>(_node_data_offset);
    } else {
        return nullptr;
    }
}

double* NrnThread::node_sav_rhs_storage() {
    auto& node_data = neuron::model().node_data();
    using Tag = neuron::container::Node::field::FastIMemSavRHS;
    if (node_data.field_active<Tag>()) {
        return &node_data.get<Tag>(_node_data_offset);
    } else {
        return nullptr;
    }
}

double* NrnThread::node_voltage_storage() {
    return &neuron::model().node_data().get<neuron::container::Node::field::Voltage>(
        _node_data_offset);
}
