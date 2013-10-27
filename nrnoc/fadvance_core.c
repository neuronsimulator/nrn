#include <nrnconf.h>
#include <nrnmpi.h>

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
	int i;
	if (t != nrn_threads->_t) {
		dt2thread(-1.);
	}else{
		dt2thread(dt);
	}
/*printf("nrn_fixed_step_minimal t=%g\n", t);*/
	nrn_thread_table_check();
	nrn_multithread_job(nrn_fixed_step_thread);
	t = nrn_threads[0]._t;
}

/* better cache efficiency since a thread can do an entire minimum delay
integration interval before joining
*/
static int step_group_n;
static int step_group_begin;
static int step_group_end;

void nrn_fixed_step_group_minimal(int n) {
	int i;
	dt2thread(dt);
	nrn_thread_table_check();
	step_group_n = n;
	step_group_begin = 0;
	step_group_end = 0;
	while(step_group_end < step_group_n) {
/*printf("step_group_end=%d step_group_n=%d\n", step_group_end, step_group_n);*/
		nrn_multithread_job(nrn_fixed_step_group_thread);
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
#if I_MEMBRANE
	if (_nt->tml) {
		assert(_nt->tml->index == CAP);
		nrn_capacity_current(_nt, _nt->tml->ml);
	}
#endif

}

static void nonvint(NrnThread* _nt) {
	int i;
	double w;
	int measure = 0;
	NrnThreadMembList* tml;
	errno = 0;
	for (tml = _nt->tml; tml; tml = tml->next) if (memb_func[tml->index].state) {
		mod_f_t s = memb_func[tml->index].state;
		(*s)(_nt, tml->ml, tml->index);
		if (errno) {
hoc_warning("errno set during calculation of states", (char*)0);
		}
	}
}

static void* nrn_fixed_step_thread(NrnThread* nth) {
	double wt;
	deliver_net_events(nth);
	wt = nrnmpi_wtime();
	nrn_random_play(nth);
	nth->_t += .5 * nth->_dt;
	fixed_play_continuous(nth);
	setup_tree_matrix_minimal(nth);
	nrn_solve_minimal(nth);
	second_order_cur(nth);
	update(nth);
	nth->_t += .5 * nth->_dt;
	fixed_play_continuous(nth);
	nonvint(nth);
	nrn_ba(nth, AFTER_SOLVE);
	fixed_record_continuous(nth);
	nrn_deliver_events(nth) ; /* up to but not past texit */
	return (void*)0;
}

