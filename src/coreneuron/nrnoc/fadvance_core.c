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
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrniv/nrn_acc_manager.h"

static void* nrn_fixed_step_thread(NrnThread*);
static void* nrn_fixed_step_lastpart(NrnThread*);
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

void nrn_fixed_step_minimal() { /* not so minimal anymore with gap junctions */
	if (t != nrn_threads->_t) {
		dt2thread(-1.);
	}else{
		dt2thread(dt);
	}
/*printf("nrn_fixed_step_minimal t=%g\n", t);*/
	nrn_thread_table_check();
	nrn_multithread_job(nrn_fixed_step_thread);
	if (nrn_have_gaps) {
		nrnmpi_v_transfer();
		nrn_multithread_job(nrn_fixed_step_lastpart);
	}
	if (nrn_threads[0]._stop_stepping) {
		nrn_spike_exchange(nrn_threads);
	}
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
    int stream_id = _nt->stream_id;

    double *vec_v = &(VEC_V(0));
    double *vec_rhs = &(VEC_RHS(0));

	/* do not need to worry about linmod or extracellular*/
	if (secondorder) {
        #pragma acc parallel loop present(vec_v[0:i2], vec_rhs[0:i2]) if(_nt->compute_gpu) async(stream_id)
		for (i=i1; i < i2; ++i) {
			vec_v[i] += 2.*vec_rhs[i];
		}
	}else{
        #pragma acc parallel loop present(vec_v[0:i2], vec_rhs[0:i2]) if(_nt->compute_gpu) async(stream_id)
		for (i=i1; i < i2; ++i) {
			vec_v[i] += vec_rhs[i];
		}
	}

    //update_matrix_to_gpu(_nt);

	if (_nt->tml) {
		assert(_nt->tml->index == CAP);
		nrn_capacity_current(_nt, _nt->tml->ml);
	}
}

static void nonvint(NrnThread* _nt) {
	NrnThreadMembList* tml;
	if (nrn_have_gaps) {
		nrnthread_v_transfer(_nt);
	}
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
	/* check thresholds and deliver all (including binqueue)
	   events up to t+dt/2 */
	deliver_net_events(nth);
	nth->_t += .5 * nth->_dt;

  if (nth->ncell) {
    int stream_id = nth->stream_id;
    /*@todo: do we need to update nth->_t on GPU: Yes (Michael, but can launch kernel) */
    #pragma acc update device(nth->_t) if(nth->compute_gpu) async(stream_id)
    #pragma acc wait(stream_id)

	fixed_play_continuous(nth);
	setup_tree_matrix_minimal(nth);
	nrn_solve_minimal(nth);
	second_order_cur(nth);
	update(nth);
  }
	if (!nrn_have_gaps) {
		nrn_fixed_step_lastpart(nth);
	}
	return (void*)0;
}

static void* nrn_fixed_step_lastpart(NrnThread* nth) {
    nth->_t += .5 * nth->_dt;

  if (nth->ncell) {
    int stream_id = nth->stream_id;
    /*@todo: do we need to update nth->_t on GPU */
    #pragma acc update device(nth->_t) if(nth->compute_gpu) async(stream_id)
    #pragma acc wait(stream_id)

	fixed_play_continuous(nth);
	nonvint(nth);
	nrn_ba(nth, AFTER_SOLVE);
  }

	nrn_deliver_events(nth) ; /* up to but not past texit */
	return (void*)0;
}
