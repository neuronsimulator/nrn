/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrniv/nrn_acc_manager.h"

static void* nrn_fixed_step_thread(NrnThread*);
static void* nrn_fixed_step_group_thread(NrnThread*);
static void update(NrnThread*);
static void nonvint(NrnThread*);

void dt2thread(double adt) { /* copied from nrnoc/fadvance.c */
    if (adt != nrn_threads[0]._dt) {
	int i;
	for (i=0; i < nrn_nthread; ++i) {
		NrnThread* nt = nrn_threads + i;
		nt->_t = t;
		nt->_dt = dt;
		if (secondorder) {
			nt->cj = 2.0/dt;
		}else{
			nt->cj = 1.0/dt;
		}
	}
    }
}

void nrn_fixed_step_minimal() {
	if (t != nrn_threads->_t) {
		dt2thread(-1.);
	}else{
		dt2thread(dt);
	}
/*printf("nrn_fixed_step_minimal t=%g\n", t);*/
	nrn_thread_table_check();
	nrn_multithread_job(nrn_fixed_step_thread);
	nrn_spike_exchange(nrn_threads);
	t = nrn_threads[0]._t;
}

/* better cache efficiency since a thread can do an entire minimum delay
integration interval before joining
*/
static int step_group_n;
static int step_group_begin;
static int step_group_end;

void nrn_fixed_step_group_minimal(int n) {
	dt2thread(dt);
	nrn_thread_table_check();
	step_group_n = n;
	step_group_begin = 0;
	step_group_end = 0;
	while(step_group_end < step_group_n) {
/*printf("step_group_end=%d step_group_n=%d\n", step_group_end, step_group_n);*/
		nrn_multithread_job(nrn_fixed_step_group_thread);
		nrn_spike_exchange(nrn_threads);
		if (stoprun) { break; }
		step_group_begin = step_group_end;
	}
	t = nrn_threads[0]._t;
}

static void* nrn_fixed_step_group_thread(NrnThread* nth) {
	int i;
	nth->_stop_stepping = 0;
	for (i = step_group_begin; i < step_group_n; ++i) {
		nrn_fixed_step_thread(nth);
		if (nth->_stop_stepping) {
			if (nth->id == 0) { step_group_end = i + 1; }
			nth->_stop_stepping = 0;
			return (void*)0;
		}
	}
	if (nth->id == 0) { step_group_end = step_group_n; }
	return (void*)0;
}

static void update(NrnThread* _nt){
	int i, i1, i2;
	i1 = 0;
	i2 = _nt->end;
	/* do not need to worry about linmod or extracellular*/
	if (secondorder) {
		for (i=i1; i < i2; ++i) {
			VEC_V(i) += 2.*VEC_RHS(i);
		}
	}else{
		for (i=i1; i < i2; ++i) {
			VEC_V(i) += VEC_RHS(i);
		}
	}

    #pragma acc wait(_nt->stream_id)
     update_matrix_to_gpu(_nt);

	if (_nt->tml) {
		assert(_nt->tml->index == CAP);
		nrn_capacity_current(_nt, _nt->tml->ml);
	}

}

static void nonvint(NrnThread* _nt) {
	NrnThreadMembList* tml;
	errno = 0;

	for (tml = _nt->tml; tml; tml = tml->next) if (memb_func[tml->index].state) {
		mod_f_t s = memb_func[tml->index].state;
		(*s)(_nt, tml->ml, tml->index);
#ifdef DEBUG
		if (errno) {
hoc_warning("errno set during calculation of states", (char*)0);
		}
#endif
	}

}

void nrn_ba(NrnThread* nt, int bat){
	NrnThreadBAList* tbl;
	for (tbl = nt->tbl[bat]; tbl; tbl = tbl->next) {
		mod_f_t f = tbl->bam->f;
		int type = tbl->bam->type;
		Memb_list* ml = tbl->ml;
		(*f)(nt, ml, type);
	}
}

static void* nrn_fixed_step_thread(NrnThread* nth) {
	deliver_net_events(nth);
	nrn_random_play(nth);
	nth->_t += .5 * nth->_dt;

    /*@todo: do we need to update nth->_t on GPU */
    #ifdef _OPENACC
        acc_update_device(&(nth->_t), sizeof(double));
    #endif

	fixed_play_continuous(nth);
	setup_tree_matrix_minimal(nth);
	nrn_solve_minimal(nth);
	second_order_cur(nth);
	update(nth);
	nth->_t += .5 * nth->_dt;

    /*@todo: do we need to update nth->_t on GPU */
    #ifdef _OPENACC
        acc_update_device(&(nth->_t), sizeof(double));
    #endif

	fixed_play_continuous(nth);
	nonvint(nth);
	nrn_ba(nth, AFTER_SOLVE);
	fixed_record_continuous(nth);
	nrn_deliver_events(nth) ; /* up to but not past texit */
	return (void*)0;
}

