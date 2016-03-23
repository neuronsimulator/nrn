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

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/nrnpthread.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnmpi/nrnmpi.h"

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

#define CACHELINE_ALLOC(name,type,size) name = (type*)nrn_cacheline_alloc((void**)&name, size*sizeof(type))

int nrn_nthread;
NrnThread* nrn_threads;
void (*nrn_mk_transfer_thread_data_)();

extern int v_structure_change;
extern int diam_changed;

#if PERMANENT /*|| USE_PTHREAD*/
static int busywait_;
static int busywait_main_;
#endif


extern void nrn_threads_free();
extern void nrn_old_thread_save();
extern double nrn_timeus();

static int nrn_thread_parallel_;
static int table_check_cnt_;
static ThreadDatum* table_check_;

/* linux specfic for performance testing */
/* eventually will be removed */
#define BENCHMARKING 0
#if BENCHMARKING
/* for rdtscll() */
#include <asm/msr.h>
#define BENCHDECLARE unsigned long t1;
#define BENCHBEGIN(arg) if (t_[arg] < t1_[arg] + BSIZE) {rdtscl(t1); *(t_[arg]++) = t1;}
#define BENCHADD(arg) BENCHBEGIN(arg)
#define WAIT wait_for_workers_timeit
#define CPU_MHZ 3192
#define BSIZE 200000
#define BS 10
static unsigned long bcnt_, bcnt1_;
static unsigned long t1_[BS][BSIZE], *t_[BS];
#else
#define BENCHDECLARE /**/
#define BENCHBEGIN(arg) /**/
#define BENCHADD(arg) /**/
#define WAIT wait_for_workers
#define BS 0
#endif

#if 0 && USE_PTHREAD
static void* nulljob(NrnThread* nt) {
	(void)nt; /* unused */
	return (void*)0;
}
#endif

int nrn_inthread_;
#if USE_PTHREAD

#include <pthread.h>
//#include <sched.h> /* for sched_setaffinity */

/* abort if using threads and a call to malloc is unprotected */
#define use_malloc_hook 0
#if use_malloc_hook
#include <malloc.h>

static int nrn_malloc_protected_;
static void my_init_hook();
static void *(*old_malloc_hook) (size_t, const void*);
static void *(*old_memalign_hook) (size_t, size_t, const void*);
static void *(*old_realloc_hook) (void*, size_t, const void*);
static void (*old_free_hook) (void*, const void*);
static void *my_malloc_hook (size_t, const void*);
static void *my_memalign_hook (size_t, size_t, const void*);
static void *my_realloc_hook (void*, size_t, const void*);
static void my_free_hook (void*, const void*);
void (*__malloc_initialize_hook)(void) = my_init_hook;

static void* my_malloc_hook(size_t size, const void* caller) {
	void* result;
	if (nrn_inthread_ && !nrn_malloc_protected_) {
		abort();
	}
	__malloc_hook = old_malloc_hook;
	__memalign_hook = old_memalign_hook;
	__realloc_hook = old_realloc_hook;
	__free_hook = old_free_hook;
	result = malloc(size);
	old_malloc_hook = __malloc_hook;
	old_memalign_hook = __memalign_hook;
	old_realloc_hook = __realloc_hook;
	old_free_hook = __free_hook;
	__malloc_hook = my_malloc_hook;
	__memalign_hook = my_memalign_hook;
	__realloc_hook = my_realloc_hook;
	__free_hook = my_free_hook;
	return result;
}
static void* my_memalign_hook(size_t alignment, size_t size, const void* caller) {
	void* result;
	if (nrn_inthread_ && !nrn_malloc_protected_) {
		abort();
	}
	__malloc_hook = old_malloc_hook;
	__memalign_hook = old_memalign_hook;
	__realloc_hook = old_realloc_hook;
	__free_hook = old_free_hook;
	result = memalign(alignment, size);
	old_malloc_hook = __malloc_hook;
	old_memalign_hook = __memalign_hook;
	old_realloc_hook = __realloc_hook;
	old_free_hook = __free_hook;
	__malloc_hook = my_malloc_hook;
	__memalign_hook = my_memalign_hook;
	__realloc_hook = my_realloc_hook;
	__free_hook = my_free_hook;
	return result;
}
static void* my_realloc_hook(void* ptr, size_t size, const void* caller) {
	void* result;
	if (nrn_inthread_ && !nrn_malloc_protected_) {
		abort();
	}
	__malloc_hook = old_malloc_hook;
	__memalign_hook = old_memalign_hook;
	__realloc_hook = old_realloc_hook;
	__free_hook = old_free_hook;
	result = realloc(ptr, size);
	old_malloc_hook = __malloc_hook;
	old_memalign_hook = __memalign_hook;
	old_realloc_hook = __realloc_hook;
	old_free_hook = __free_hook;
	__malloc_hook = my_malloc_hook;
	__memalign_hook = my_memalign_hook;
	__realloc_hook = my_realloc_hook;
	__free_hook = my_free_hook;
	return result;
}
static void my_free_hook(void* ptr, const void* caller) {
	if (nrn_inthread_ && !nrn_malloc_protected_) {
		abort();
	}
	__malloc_hook = old_malloc_hook;
	__memalign_hook = old_memalign_hook;
	__realloc_hook = old_realloc_hook;
	__free_hook = old_free_hook;
	free(ptr);
	old_malloc_hook = __malloc_hook;
	old_memalign_hook = __memalign_hook;
	old_realloc_hook = __realloc_hook;
	old_free_hook = __free_hook;
	__malloc_hook = my_malloc_hook;
	__memalign_hook = my_memalign_hook;
	__realloc_hook = my_realloc_hook;
	__free_hook = my_free_hook;
}
static void my_init_hook() {
	static int installed = 0;
	if (installed) { return; }
	installed = 1;
	old_malloc_hook = __malloc_hook;
	__malloc_hook = my_malloc_hook;
	old_memalign_hook = __memalign_hook;
	__memalign_hook = my_memalign_hook;
	old_realloc_hook = __realloc_hook;
	__realloc_hook = my_realloc_hook;
	old_free_hook = __free_hook;
	__free_hook = my_free_hook;
}
#endif

static int interpreter_locked;
static pthread_mutex_t interpreter_lock_;
static pthread_mutex_t* _interpreter_lock;

static pthread_mutex_t nmodlmutex_;
pthread_mutex_t* _nmodlmutex;

static pthread_mutex_t nrn_malloc_mutex_;
static pthread_mutex_t* _nrn_malloc_mutex;


/* when PERMANENT is 0, we avoid false warnings with helgrind, but a bit slower */
/* when 0, create/join instead of wait on condition. */
#if !defined(NDEBUG)
#define PERMANENT 0
#endif
#ifndef PERMANENT
#define PERMANENT 1
#endif

typedef volatile struct {
        int flag;
	int thread_id;
        /* for nrn_solve etc.*/
        void* (*job)(NrnThread*);
} slave_conf_t;

static pthread_t* slave_threads;
#if PERMANENT
static pthread_cond_t* cond;
static pthread_mutex_t* mut;
static slave_conf_t* wc;
#endif

static void wait_for_workers() {
	int i;
	for (i=1; i < nrn_nthread; ++i) {
#if PERMANENT
	    if (busywait_main_) {
		while (wc[i].flag != 0){;}
	    }else{
		pthread_mutex_lock(mut + i);
		while (wc[i].flag != 0) {
			pthread_cond_wait(cond + i, mut + i);
		}
		pthread_mutex_unlock(mut + i);
	    }
#else
		pthread_join(slave_threads[i], (void*)0);
#endif
	}
}

#if BENCHMARKING
static void wait_for_workers_timeit() {
	BENCHDECLARE
	BENCHBEGIN(BS-2)
	wait_for_workers();
	BENCHADD(BS-1)
}
#endif

static void send_job_to_slave(int i, void* (*job)(NrnThread*)) {
#if PERMANENT
	pthread_mutex_lock(mut + i);
	wc[i].job = job;
	wc[i].flag = 1;
	pthread_cond_signal(cond + i);
	pthread_mutex_unlock(mut + i);
#else
	pthread_create(slave_threads + i, (void*)0, (void*(*)(void*))job, (void*)(nrn_threads + i));
#endif
}


#if PERMANENT
static void* slave_main(void* arg) {
	slave_conf_t* my_wc = (slave_conf_t*)arg;
	pthread_mutex_t *my_mut = mut + my_wc->thread_id;
	pthread_cond_t *my_cond = cond + my_wc->thread_id;
	BENCHDECLARE
#if BENCHMARKING
	unsigned long* t_[BS];
	int a1, a2;
	a1 = my_wc->thread_id;
	a2 = my_wc->thread_id + nrn_nthread;
	t_[a1] = t1_[a1];
	t_[a2] = t1_[a2];
#endif

	for(;;) {
	    if (busywait_) {
			while(my_wc->flag == 0) {;}
			if (my_wc->flag == 1) {
				BENCHBEGIN(a1)
				(*my_wc->job)(nrn_threads + my_wc->thread_id);
				BENCHADD(a2)
			}else{
				return (void*)0;
			}
			my_wc->flag = 0;
			pthread_cond_signal(my_cond);
	    }else{
		pthread_mutex_lock(my_mut);
		while (my_wc->flag == 0) {
			pthread_cond_wait(my_cond, my_mut);
		}
		pthread_mutex_unlock(my_mut);
		pthread_mutex_lock(my_mut);
		if (my_wc->flag == 1) {
			pthread_mutex_unlock(my_mut);
			BENCHBEGIN(a1)
			(*my_wc->job)(nrn_threads + my_wc->thread_id);
			BENCHADD(a2)
		}else{
			pthread_mutex_unlock(my_mut);
			return (void*)0;
		}
		pthread_mutex_lock(my_mut);
		my_wc->flag = 0;
		pthread_cond_signal(my_cond);
		pthread_mutex_unlock(my_mut);
	    }
	}
	return (void*)0;
}

#endif

static void threads_create_pthread(){
    if (nrn_nthread > 1) {
#if PERMANENT
	int i;
	CACHELINE_ALLOC(wc, slave_conf_t, nrn_nthread);
	slave_threads = (pthread_t *)emalloc(sizeof(pthread_t)*nrn_nthread);
	cond = (pthread_cond_t *)emalloc(sizeof(pthread_cond_t)*nrn_nthread);
	mut = (pthread_mutex_t *)emalloc(sizeof(pthread_mutex_t)*nrn_nthread);
	for (i=1; i < nrn_nthread; ++i) {
		wc[i].flag = 0;
		wc[i].thread_id = i;
		pthread_cond_init(cond + i, (void*)0);
		pthread_mutex_init(mut + i, (void*)0);
		pthread_create(slave_threads + i, (void*)0, slave_main, (void*)(wc+i));
	}
#else
	slave_threads = (pthread_t *)emalloc(sizeof(pthread_t)*nrn_nthread);
#endif /* PERMANENT */
	if (!_interpreter_lock) {
		interpreter_locked = 0;
		_interpreter_lock = &interpreter_lock_;
		pthread_mutex_init(_interpreter_lock, (void*)0);
	}
	if (!_nmodlmutex) {
		_nmodlmutex = &nmodlmutex_;
		pthread_mutex_init(_nmodlmutex, (void*)0);
	}
	if (!_nrn_malloc_mutex) {
		_nrn_malloc_mutex = &nrn_malloc_mutex_;
		pthread_mutex_init(_nrn_malloc_mutex, (void*)0);
	}
	nrn_thread_parallel_ = 1;
    }else{
    	nrn_thread_parallel_ = 0;
    }
}

static void threads_free_pthread(){
	if (slave_threads) {
#if PERMANENT
		int i;
		wait_for_workers();
		for (i=1; i < nrn_nthread; ++i) {
			pthread_mutex_lock(mut + i);
			wc[i].flag = -1;
			pthread_cond_signal(cond + i);
			pthread_mutex_unlock(mut + i);
			pthread_join(slave_threads[i], (void*)0);
			pthread_cond_destroy(cond + i);
			pthread_mutex_destroy(mut + i);
		}
		free((char*)slave_threads);
		free((char*)cond);
		free((char*)mut);
		free((char*)wc);
		slave_threads = (pthread_t*)0;
		cond = (pthread_cond_t*)0;
		mut = (pthread_mutex_t*)0;
		wc = (slave_conf_t*)0;
#else
		free((char*)slave_threads);
		slave_threads = (pthread_t*)0;
#endif /*PERMANENT*/
	}
	if (_interpreter_lock) {
		pthread_mutex_destroy(_interpreter_lock);
		_interpreter_lock = (pthread_mutex_t*)0;
		interpreter_locked = 0;
	}
	if (_nmodlmutex) {
		pthread_mutex_destroy(_nmodlmutex);
		_nmodlmutex = (pthread_mutex_t*)0;
	}
	if (_nrn_malloc_mutex) {
		pthread_mutex_destroy(_nrn_malloc_mutex);
		_nrn_malloc_mutex = (pthread_mutex_t*)0;
	}
	nrn_thread_parallel_ = 0;
}

#else /* USE_PTHREAD */


static void threads_create_pthread(){
	nrn_thread_parallel_ = 0;
}
static void threads_free_pthread(){
	nrn_thread_parallel_ = 0;
}
#endif /* !USE_PTHREAD */

void nrn_thread_stat() {
#if BENCHMARKING
	FILE* f;
	long i, j, n;
	char buf[50];
	sprintf(buf, "bench.%d.dat", nrnmpi_myid);
	f = fopen(buf, "w");
	n = (t_[0] - t1_[0]);
	for (i=1; i < nrn_nthread; ++i) {
		t_[i] = t1_[i] + n;
		t_[i+nrn_nthread] = t1_[i+nrn_nthread] + n;
	}
	n = 0;
	for (i=0; i < BS; ++i) {
		n += t_[i] - t1_[i];	
	}
	fprintf(f, "%ld\n", n);
	n = 0;
	for (j=0; j < BS; ++j) {
		n = t_[j] - t1_[j];
		for (i=0; i < n; ++i) {
			fprintf(f, "%ld %d\n", t1_[j][i], j*nrnmpi_numprocs + nrnmpi_myid);
		}
	}
	fclose(f);
#endif /*BENCHMARKING*/
}

void nrn_threads_create(int n, int parallel) {
    int i, j;
	NrnThread* nt;
	if (nrn_nthread != n) {
/*printf("sizeof(NrnThread)=%d   sizeof(Memb_list)=%d\n", sizeof(NrnThread), sizeof(Memb_list));*/
#if BENCHMARKING
#endif
		nrn_threads = (NrnThread*)0;
		nrn_nthread = n;
		if (n > 0) {
			CACHELINE_ALLOC(nrn_threads, NrnThread, n);
#if BENCHMARKING
			for (i=0; i < BS; ++i) {
				t_[i] = t1_[i];
			}
#endif
			for (i=0; i < n; ++i) {
				nt = nrn_threads + i;
				nt->_t = 0.;
				nt->_dt = -1e9;
				nt->id = i;
				nt->_stop_stepping = 0;
				nt->n_vecplay = 0;
				nt->_vecplay = NULL;
				nt->tml = (NrnThreadMembList*)0;
				nt->_ml_list = (Memb_list**)0;
				nt->pntprocs = (Point_process*)0;
				nt->presyns = (PreSyn*)0;
				nt->netcons = (NetCon*)0;
				nt->weights = (double*)0;
				nt->n_pntproc = nt->n_presyn = nt->n_netcon = 0;
				nt->n_weight = 0;
				nt->_ndata = nt->_nidata = nt->_nvdata = 0;
				nt->_data = (double*)0;
				nt->_idata = (int*)0;
				nt->_vdata = (void**)0;
				nt->ncell = 0;
				nt->end = 0;
                                for (j=0; j < BEFORE_AFTER_SIZE; ++j) {
                                        nt->tbl[j] = (NrnThreadBAList*)0;
                                }
				nt->_actual_rhs = 0;
				nt->_actual_d = 0;
				nt->_actual_a = 0;
				nt->_actual_b = 0;
				nt->_actual_v = 0;
				nt->_actual_area = 0;
				nt->_v_parent_index = 0;
				nt->_permute = 0;
                                nt->_shadow_rhs = 0;
                                nt->_shadow_d = 0;
				nt->_ecell_memb_list = 0;
				nt->_sp13mat = 0;
				nt->_ctime = 0.0;
			}
		}
		v_structure_change = 1;
		diam_changed = 1;
	}
	if (nrn_thread_parallel_ != parallel) {
		threads_free_pthread();
		if (parallel) {
			threads_create_pthread();
		}
	}
	/*printf("nrn_threads_create %d %d\n", nrn_nthread, nrn_thread_parallel_);*/
}

void nrn_threads_free() {
  if(nrn_nthread)
  {
    threads_free_pthread();
    free((char*)nrn_threads);
    nrn_threads = 0;
    nrn_nthread = 0;
  }
}

void nrn_mk_table_check() {
	int i, id, index;
	int* ix;
	if (table_check_) {
		free((void*)table_check_);
		table_check_ = (ThreadDatum*)0;
	}

    /// Allocate int array of size of mechanism types
	ix = (int*)emalloc(n_memb_func*sizeof(int));
	for (i=0; i < n_memb_func; ++i) {
		ix[i] = -1;
	}
	table_check_cnt_ = 0;
	for (id=0; id < nrn_nthread; ++id) {
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
		table_check_ = (ThreadDatum*)emalloc(table_check_cnt_*sizeof(ThreadDatum));
	}
	i=0;
	for (id=0; id < nrn_nthread; ++id) {
		NrnThread* nt = nrn_threads + id;
		NrnThreadMembList* tml;
		for (tml = nt->tml; tml; tml = tml->next) {
			index = tml->index;
			if (memb_func[index].thread_table_check_ && ix[index] == id) {
				table_check_[i++].i = id;
				table_check_[i++]._pvoid = (void*)tml;
			}
		}
	}
	free((void*)ix);
}

void nrn_thread_table_check() {
	int i;
	for (i=0; i < table_check_cnt_; i += 2) {
		NrnThread* nt = nrn_threads + table_check_[i].i;
		NrnThreadMembList* tml = (NrnThreadMembList*)table_check_[i+1]._pvoid;
		Memb_list* ml = tml->ml;
		(*memb_func[tml->index].thread_table_check_)(
			0, ml->nodecount, ml->data, ml->pdata, ml->_thread, nt, tml->index
		);
	}
}


void nrn_multithread_job(void*(*job)(NrnThread*)) {
	int i;
#if defined(_OPENMP)

    // default(none) removed to avoid issue with opari2
	#pragma omp parallel for private(i) \
	shared(nrn_threads, job, nrn_nthread, nrnmpi_myid) schedule(static, 1)
	for(i=0; i < nrn_nthread; ++i) {
		(*job)(nrn_threads + i);
	}
#else

/* old implementation */
#if USE_PTHREAD
	BENCHDECLARE
	if (nrn_thread_parallel_) {
		nrn_inthread_ = 1;
		for (i=1; i < nrn_nthread; ++i) {
			send_job_to_slave(i, job);
		}
		BENCHBEGIN(0)
		(*job)(nrn_threads);
		BENCHADD(nrn_nthread)
		WAIT();
		nrn_inthread_ = 0;
	}else{ /* sequential */
#else
	{
#endif
		for (i=1; i < nrn_nthread; ++i) {
			BENCHBEGIN(i)
			(*job)(nrn_threads + i);
			BENCHADD(i+nrn_nthread)
		}
		BENCHBEGIN(0)
		(*job)(nrn_threads);
		BENCHADD(nrn_nthread)
	}
#endif

}
