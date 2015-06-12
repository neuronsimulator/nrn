#include <../../nrnconf.h>

#include <nrnmpi.h>
#include <nrnrt.h>
#include <errno.h>
#include "neuron.h"
#include "section.h"
#include "nrniv_mf.h"
#include "multisplit.h"
#define nrnoc_fadvance_c
#include "nonvintblock.h"
#include "nrncvode.h"
#include "spmatrix.h"

/*
 after an fadvance from t-dt to t, v is defined at t
 states that depend on v are defined at t+dt/2
 states, such as concentration that depend on current are defined at t
 and currents are defined at t-dt/2.
 */
/*
 fcurrent is used to set up all assigned variables  without changing any
 states.  It assumes all states have their values at time t which is
 only first order correct. It determines the nonvint assignments first
 so that ena, etc. are correct for the current determinations. We
 demand that nonvint assignments can be done without changing states when
 dt is temporarily set to 0.

 It turned out to be a bad idea to do this. Many methods (KINETIC, PROCEDURE)
 for solving are not even called when dt =0. Also it plays havoc with tables
 that are recomputed several times when dt is changed then changed back.
 Therefore we are no longer trying to initialize assigned variables within
 SOLVE'd blocks. Instead we are making initialization more convenient,
 at least within the normal situations, by introducing a finitialize()
 with an optional argument vinit. finitialize will call the INITIALIZE block
 for all mechanisms in all segments. (Default initialization sets all states
 to state0.) The user can set things up specially in a models INITIAL
 block. INITIAL blocks can make use of v. With care they can make use of
 ionic concentrations just like breakpoint and solve blocks. When the argument
 is present, v for all segments are set to that value.
 */
 
#if !defined(NRNMPI) || NRNMPI == 0
extern double nrnmpi_wtime();
#endif
extern double* nrn_mech_wtime_;
extern double t, dt;
extern double chkarg();
extern void nrn_fixed_step();
extern void nrn_fixed_step_group(int);
static void* nrn_fixed_step_thread(NrnThread*);
static void* nrn_fixed_step_group_thread(NrnThread* nth);
static void* nrn_fixed_step_lastpart(NrnThread*);
extern void* setup_tree_matrix(NrnThread*);
extern void nrn_solve(NrnThread*);
extern void nonvint(NrnThread* nt);
extern void nrncvode_set_t(double t);

static void* nrn_ms_treeset_through_triang(NrnThread*);
static void* nrn_ms_reduce_solve(NrnThread*);
static void* nrn_ms_bksub(NrnThread*);
static void* nrn_ms_bksub_through_triang(NrnThread*);
extern void* nrn_multisplit_triang(NrnThread*);
extern void* nrn_multisplit_reduce_solve(NrnThread*);
extern void* nrn_multisplit_bksub(NrnThread*);
extern void (*nrn_multisplit_setup_)();
void (*nrn_allthread_handle)();

extern int tree_changed;
extern int diam_changed;
extern int state_discon_allowed_;
extern double hoc_epsilon;

static void update(NrnThread*);

#if 0
/*
  1 to save space, but must worry about other uses of the memb_list
  such as in netcvode.cpp in fornetcon_prepare
*/
extern short* nrn_is_artificial_;
extern Template** nrn_pnt_template_;
#endif
#if NRN_DAQ
extern void nrn_daq_scanstart();
extern void nrn_daq_ai();
extern void nrn_daq_ao();
#endif

#define NRNCTIME 1
#if NRNCTIME
#define CTBEGIN double wt = nrnmpi_wtime();
#define CTADD nth->_ctime += nrnmpi_wtime() - wt;
#else
#define CTBEGIN /**/
#define CTADD /**/
#endif

#define ELIMINATE_T_ROUNDOFF 0
#if ELIMINATE_T_ROUNDOFF
/* in order to simplify and as much as possible avoid the handling
of round-off error due to repeated t += dt, we use
t = nrn_tbase_ + nrn_ndt_ * nrn_dt_;
to advance time. The problems that must be overcome are when the
user changes dt in the middle of a run or switches from cvode or
abruptly and arbitrarily sets a new value of t and continues from
there (hence nrn_tbase_)
*/
double nrn_ndt_, nrn_tbase_, nrn_dt_;
void nrn_chk_ndt() {
	if (dt != nrn_dt_ || t != nrn_tbase_ + nrn_ndt_ * nrn_dt_) {
if (nrnmpi_myid == 0) printf("nrn_chk_ndt t=%g dt=%g old nrn_tbase_=%g nrn_ndt_=%g nrn_dt_=%g\n",
t, dt, nrn_tbase_, nrn_ndt_, nrn_dt_);
		nrn_dt_ = dt;
		nrn_tbase_ = t;
		nrn_ndt_ = 0.;
	}
}
#endif /* ELIMINATE_T_ROUNDOFF */

/*
There are (too) many variants of nrn_fixed_step depending on
nrnmpi_numprocs 1 or > 1, nrn_nthread 1 or > 1,
nrnmpi_v_transfer nil or callable, nrn_multisplit_setup nil or callable,
and whether one step with fadvance
or possibly many with ParallelContext.psolve before synchronizing with
NetParEvent. The combination of simultaneous nrnmpi_numprocs > 1 and
nrn_nthread > 1 with parallel transfer
requires some refactoring of the use of the old
(*nrnmpi_v_transfer_)() from within nonvint(nt) which handled either mpi
or threads but cannot handle both simultaneously. (Unless the first
thread that arrives in nrnmpi_v_transfer is the one that accomplishes
the mpi transfer). Instead, we replace with
nrnthread_v_transfer(nt) to handle the per thread copying of source
data into the target space owned by the thread. Between update (called by
nrn_fixed_step_thread and nrn_ms_bksub), and nonvint (called by
nrn_fixed_step_lastpart, we need to have thread 0 do the interprocessor
transfer of source voltages to a either a staging area for later
copying to the target by a thread (or directly to the target if only one
thread). This will happen with (*nrnmpi_v_transfer_)(). (Look Ma, no threads).
This deals properly with the necessary mpi synchronization. And leaves
thread handling where it was before. Also, writing to the staging area
is only done by thread 0. Fixed step and global variable step
logic is limited to the case where an nrnmpi_v_transfer requires
existence of nrnthread_v_transfer (even if one thread).
*/
#if 1 || PARANEURON
void (*nrnmpi_v_transfer_)(); /* called by thread 0 */
void (*nrnthread_v_transfer_)(NrnThread* nt);
/* if at least one gap junction has a source voltage with extracellular inserted */
void (*nrnthread_vi_compute_)(NrnThread* nt);
#endif

#if VECTORIZE
extern int v_structure_change;
#endif

#if CVODE
int cvode_active_;
#endif

int stoprun;
int nrn_use_fast_imem;

#define PROFILE 0
#include "profile.h"

void fadvance(void)
{
	tstopunset;
#if CVODE
	if (cvode_active_) {
		cvode_fadvance(-1.);
		tstopunset;
		hoc_retpushx(1.);
		return;
	}
#endif
	if (tree_changed) {
		setup_topology();
	}
	if (v_structure_change) {
		v_setup_vectors();
	}
	if (diam_changed) {
		recalc_diam();
	}
	nrn_fixed_step();
	tstopunset;
	hoc_retpushx(1.);
}

/*
  batch_save() initializes list of variables
  batch_save(&varname, ...) adds variable names to list for saving
  batch_save("varname", ...) adds variable names to the list and name
  				will appear in header.
  batch_run(tstop, tstep, "file") saves variables in file every tstep
*/

static void batch_out(), batch_open(), batch_close();
void batch_run(void) /* avoid interpreter overhead */
{
	double tstop, tstep, tnext;
	char* filename;
	char* comment;

	tstopunset;
	tstop = chkarg(1,0.,1e20);
	tstep = chkarg(2, 0., 1e20);
	if (ifarg(3)) {
		filename = gargstr(3);
	}else{
		filename = 0;
	}
	if (ifarg(4)) {
		comment = gargstr(4);
	}else{
		comment = "";
	}
	
	if (tree_changed) {
		setup_topology();
	}
#if VECTORIZE
	if (v_structure_change) {
		v_setup_vectors();
	}
#endif
	batch_open(filename, tstop, tstep, comment);
	batch_out();
	if (cvode_active_) {
		while (t < tstop) {
			cvode_fadvance(t+tstep);
			batch_out();
		}
	}else{
		tstep -= dt/4.;
		tstop -= dt/4.;
		tnext = t + tstep;
		while (t < tstop) {
			nrn_fixed_step();
			if (t > tnext) {
				batch_out();
				tnext = t + tstep;
			}
			if (stoprun) { tstopunset; break; }
		}
	}
	batch_close();
	hoc_retpushx(1.);
}

static void dt2thread(double adt) {
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

static int _upd;
static void* daspk_init_step_thread(NrnThread* nt) {
	setup_tree_matrix(nt);
	nrn_solve(nt);
	if (_upd) {
		update(nt);
	}
	return (void*)0;
}

void nrn_daspk_init_step(double tt, double dteps, int upd){
	int i;
	double dtsav = nrn_threads->_dt;
	int so = secondorder;
	dt = dteps;
	t = tt;
	secondorder = 0;
	dt2thread(dteps);
	nrn_thread_table_check();
	_upd = upd;
	nrn_multithread_job(daspk_init_step_thread);
	dt = dtsav;
	secondorder = so;
	dt2thread(dtsav);
	nrn_thread_table_check();
}

void nrn_fixed_step() {
	int i;
#if ELIMINATE_T_ROUNDOFF
	nrn_chk_ndt();
#endif
	if (t != nrn_threads->_t) {
		dt2thread(-1.);
	}else{
		dt2thread(dt);
	}
	nrn_thread_table_check();
	if (nrn_multisplit_setup_) {
		nrn_multithread_job(nrn_ms_treeset_through_triang);
		// remove to avoid possible deadlock where some ranks do a
		// v transfer and others do a spike exchange.
		// i.e. must complete the full multisplit time step.
		//if (!nrn_allthread_handle) {
			nrn_multithread_job(nrn_ms_reduce_solve);
			nrn_multithread_job(nrn_ms_bksub);
			/* see comment below */
			if (nrnthread_v_transfer_) {
				if (nrnmpi_v_transfer_) {
					(*nrnmpi_v_transfer_)();
				}
				nrn_multithread_job(nrn_fixed_step_lastpart);
			}
		//}
	}else{
		nrn_multithread_job(nrn_fixed_step_thread);
/* if there is no nrnthread_v_transfer then there cannot be
   a nrnmpi_v_transfer and lastpart
   will be done in above call.
*/
		if (nrnthread_v_transfer_) {
			if (nrnmpi_v_transfer_) {
				(*nrnmpi_v_transfer_)();
			}
			nrn_multithread_job(nrn_fixed_step_lastpart);
		}
	}
	t = nrn_threads[0]._t;
	if (nrn_allthread_handle) { (*nrn_allthread_handle)(); }
}

/* better cache efficiency since a thread can do an entire minimum delay
integration interval before joining
*/
static int step_group_n;
static int step_group_begin;
static int step_group_end;

void nrn_fixed_step_group(int n) {
	int i;
#if ELIMINATE_T_ROUNDOFF
	nrn_chk_ndt();
#endif
	dt2thread(dt);
	nrn_thread_table_check();
	if (nrn_multisplit_setup_) {
		int b = 0;
		nrn_multithread_job(nrn_ms_treeset_through_triang);
		step_group_n = 0; /* abort at bksub flag */
		for (i=1; i < n; ++i) {
			nrn_multithread_job(nrn_ms_reduce_solve);
			nrn_multithread_job(nrn_ms_bksub_through_triang);
			if (step_group_n) {
				step_group_n = 0;
				if (nrn_allthread_handle) {
					(*nrn_allthread_handle)();
				}
				/* aborted step at bksub, so if not stopped
				   must do the triang*/
				b = 1;
				if (!stoprun) {
					nrn_multithread_job(nrn_ms_treeset_through_triang);
				}
			}
			if (stoprun) { break; }
			b = 0;
		}
		if (!b) {
			nrn_multithread_job(nrn_ms_reduce_solve);
			nrn_multithread_job(nrn_ms_bksub);
		}
		if (nrn_allthread_handle) { (*nrn_allthread_handle)(); }
	}else{
		step_group_n = n;
		step_group_begin = 0;
		step_group_end = 0;
		while(step_group_end < step_group_n) {
/*printf("step_group_end=%d step_group_n=%d\n", step_group_end, step_group_n);*/
			nrn_multithread_job(nrn_fixed_step_group_thread);
			if (nrn_allthread_handle) { (*nrn_allthread_handle)(); }
			if (stoprun) { break; }
			step_group_begin = step_group_end;
		}
	}
	t = nrn_threads[0]._t;
}

void* nrn_fixed_step_group_thread(NrnThread* nth) {
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

void* nrn_fixed_step_thread(NrnThread* nth) {
	double wt;
	deliver_net_events(nth);
	wt = nrnmpi_wtime();
	nrn_random_play(nth);
#if ELIMINATE_T_ROUNDOFF
	nth->nrn_ndt_ += .5;
	nth->_t = nrn_tbase_ + nth->nrn_ndt_ * nrn_dt_;
#else
	nth->_t += .5 * nth->_dt;
#endif
	fixed_play_continuous(nth);
	setup_tree_matrix(nth);
	nrn_solve(nth);
	second_order_cur(nth);
	update(nth);
	CTADD
/*
  To simplify the logic,
  if there is no nrnthread_v_transfer then there cannot be an nrnmpi_v_transfer.
*/
	if (!nrnthread_v_transfer_) {
		nrn_fixed_step_lastpart(nth);
	}
	return (void*)0;
}

extern void nrn_extra_scatter_gather(int direction, int tid);
extern void nrn_ba(NrnThread*, int);

void* nrn_fixed_step_lastpart(NrnThread* nth) {
	CTBEGIN
#if NRN_DAQ
	nrn_daq_ao();
#endif
#if ELIMINATE_T_ROUNDOFF
	nth->nrn_ndt_ += .5;
	nth->_t = nrn_tbase_ + nth->nrn_ndt_ * nrn_dt_;
#else
	nth->_t += .5 * nth->_dt;
#endif
	fixed_play_continuous(nth);
#if NRN_DAQ
	nrn_daq_scanstart();
#endif
	nrn_extra_scatter_gather(0, nth->id);
	nonvint(nth);
	nrn_ba(nth, AFTER_SOLVE);
#if NRN_DAQ
	nrn_daq_ai();
#endif
	fixed_record_continuous(nth);
	CTADD
	nrn_deliver_events(nth) ; /* up to but not past texit */
	return (void*)0;
}

/* nrn_fixed_step_thread is split into three pieces */

void* nrn_ms_treeset_through_triang(NrnThread* nth) {
	double wt;
	deliver_net_events(nth);
	wt = nrnmpi_wtime();
	nrn_random_play(nth);
#if ELIMINATE_T_ROUNDOFF
	nth->nrn_ndt_ += .5;
	nth->_t = nrn_tbase_ + nth->nrn_ndt_ * nrn_dt_;
#else
	nth->_t += .5 * nth->_dt;
#endif
	fixed_play_continuous(nth);
	setup_tree_matrix(nth);
	nrn_multisplit_triang(nth);
	CTADD
	return (void*)0;
}
void* nrn_ms_reduce_solve(NrnThread* nth) {
	nrn_multisplit_reduce_solve(nth);
	return (void*)0;
}
void* nrn_ms_bksub(NrnThread* nth) {
	CTBEGIN
	nrn_multisplit_bksub(nth);
	second_order_cur(nth);
	update(nth);
	CTADD
/* see above comment in nrn_fixed_step_thread */
	if (!nrnthread_v_transfer_) {
		nrn_fixed_step_lastpart(nth);
	}
	return (void*)0;
}
void* nrn_ms_bksub_through_triang(NrnThread* nth) {
	nrn_ms_bksub(nth);
	if (nth->_stop_stepping) {
		nth->_stop_stepping = 0;
		if (nth == nrn_threads) {
			step_group_n = 1;
		}
		return (void*)0;
	}
	nrn_ms_treeset_through_triang(nth);
	return (void*)0;
}

#if NRN_REALTIME
void nrn_fake_step() { /* get as much into cache as possible */
	/* if we could do a full nrn_fixed_step we would save about 10 us */

	/*Not nearly enough. This only saving a few */
	setup_tree_matrix();
	nrn_solve();

#if 0
	nonvint(); /* this is an important one, 2 more us,  but ... */
#endif
}
#endif

static void update(NrnThread* _nt)
{
	int i, i1, i2;
	i1 = 0;
	i2 = _nt->end;
#if CACHEVEC
    if (use_cachevec) {
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
    }else
#endif
    {	/* use original non-vectorized update */
 	if (secondorder) {
#if _CRAY
#pragma _CRI ivdep
#endif
		for (i=i1; i < i2; ++i) {
			NODEV(_nt->_v_node[i]) += 2.*NODERHS(_nt->_v_node[i]);
		}
	}else{
#if _CRAY
#pragma _CRI ivdep
#endif
		for (i=i1; i < i2; ++i) {
			NODEV(_nt->_v_node[i]) += NODERHS(_nt->_v_node[i]);
		}
		if (use_sparse13) {
			nrndae_update();
		}
	}
    } /* end of non-vectorized update */

#if EXTRACELLULAR
	nrn_update_2d(_nt);
#endif
	if (nrnthread_vi_compute_) { (*nrnthread_vi_compute_)(_nt); }
#if I_MEMBRANE
	if (_nt->tml) {
		assert(_nt->tml->index == CAP);
		nrn_capacity_current(_nt, _nt->tml->ml);
	}
#endif
	if (nrn_use_fast_imem) { nrn_calc_fast_imem(_nt); }
}

void nrn_calc_fast_imem(NrnThread* _nt) {
	int i;
	int i1 = 0;
	int i3 = _nt->end;
	double* pd = _nt->_nrn_fast_imem->_nrn_sav_d;
	double* prhs = _nt->_nrn_fast_imem->_nrn_sav_rhs;
    if (use_cachevec) {
	for (i = i1; i < i3 ; ++i) {
		prhs[i] = (pd[i]*VEC_RHS(i) + prhs[i])*VEC_AREA(i)*0.01;
	}
    }else{
	for (i = i1; i < i3 ; ++i) {
		Node* nd = _nt->_v_node[i];
		prhs[i] = (pd[i]*NODERHS(nd) + prhs[i])*NODEAREA(nd)*0.01;
	}
    }
}

void fcurrent(void)
{
	int i;
	if (tree_changed) {
		setup_topology();
	}
	if (v_structure_change) {
		v_setup_vectors();
	}
	if (diam_changed) {
		recalc_diam();
	}

	dt2thread(-1.);
	nrn_thread_table_check();
	state_discon_allowed_ = 0;
	nrn_multithread_job(setup_tree_matrix);
	state_discon_allowed_ = 1;
	hoc_retpushx(1.);
}

void nrn_print_matrix(NrnThread* _nt) {
	extern int section_count;
	extern Section** secorder;
	int isec, inode;
	Section* sec;
	Node* nd;
    if (use_sparse13) {
	if(ifarg(1) && chkarg(1, 0., 1.) == 0.) {
		spPrint(_nt->_sp13mat, 1, 0, 1);
	}else{
		int i, n = spGetSize(_nt->_sp13mat, 0);
		spPrint(_nt->_sp13mat, 1, 1, 1);
		for (i=1; i <= n; ++i) {
			printf("%d %g\n", i, _nt->_actual_rhs[i]);
		}
	}
    }else if (_nt) {
	for (inode = 0; inode <  _nt->end; ++inode) {
		nd = _nt->_v_node[inode];
printf("%d %g %g %g %g\n", inode, ClassicalNODEB(nd), ClassicalNODEA(nd), NODED(nd), NODERHS(nd));
	}		
    }else{
	for (isec = 0; isec < section_count; ++isec) {
		sec = secorder[isec];
		for (inode = 0; inode < sec->nnode; ++inode) {
			nd = sec->pnode[inode];
printf("%d %d %g %g %g %g\n", isec, inode, ClassicalNODEB(nd), ClassicalNODEA(nd), NODED(nd), NODERHS(nd));
		}
	}
    }
}

void fmatrix(void) {
	if (ifarg(1)) {
		extern Node* node_exact(Section*, double);
		double x = chkarg(1, 0., 1.);
		int id = (int)chkarg(2, 1., 4.);
		Node* nd = node_exact(chk_access(), x);
		NrnThread* _nt = nd->_nt;
		switch (id) {
		case 1: hoc_retpushx(NODEA(nd)); break;
		case 2: hoc_retpushx(NODED(nd)); break;
		case 3: hoc_retpushx(NODEB(nd)); break;
		case 4: hoc_retpushx(NODERHS(nd)); break;
		}
		return;
	}
	nrn_print_matrix(nrn_threads);
	hoc_retpushx(1.);
	return;
}

void nonvint(NrnThread* _nt)
{
#if VECTORIZE
	int i;
	double w;
	int measure = 0;
	NrnThreadMembList* tml;
#if 1 || PARANEURON
	/* nrnmpi_v_transfer if needed was done earlier */
	if (nrnthread_v_transfer_) {(*nrnthread_v_transfer_)(_nt);}
#endif
	if (_nt->id == 0 && nrn_mech_wtime_) { measure = 1; }
	errno = 0;
	for (tml = _nt->tml; tml; tml = tml->next) if (memb_func[tml->index].state) {
		Pvmi s = memb_func[tml->index].state;
		if (measure) { w = nrnmpi_wtime(); }
		(*s)(_nt, tml->ml, tml->index);
		if (measure) { nrn_mech_wtime_[tml->index] += nrnmpi_wtime() - w; }
		if (errno) {
			if (nrn_errno_check(i)) {
hoc_warning("errno set during calculation of states", (char*)0);
			}
		}
	}
	long_difus_solve(0, _nt); /* if any longitudinal diffusion */
	nrn_nonvint_block_fixed_step_solve(_nt->id);
#endif
}

#if VECTORIZE
int nrn_errno_check(int i)
{
	int ierr;
	ierr = hoc_errno_check();
	if (ierr) {
		fprintf(stderr, 
"%d errno=%d at t=%g during call to mechanism %s\n", nrnmpi_myid, ierr, t, memb_func[i].sym->name);
	}
	return ierr;
}
#else
int nrn_errno_check(Prop* p, int inode, Section* sec)
{
	int ierr;
	char* secname();
	ierr = hoc_errno_check();
	if (ierr) {
		fprintf(stderr, 
"%d errno set at t=%g during call to mechanism %s at node %d in section %s\n",
nrnmpi_myid, t, memb_func[p->type].sym->name, inode, secname(sec));
	}
	return ierr;
}
#endif

void frecord_init(void) { /* useful when changing states after an finitialize() */
	int i;
	dt2thread(-1);
	nrn_record_init();
	if (!cvode_active_) {
		for (i=0; i < nrn_nthread; ++i) {
			fixed_record_continuous(nrn_threads + i);
		}
	}
	hoc_retpushx(1.);
}

void verify_structure(void) {
	if (tree_changed) {
		setup_topology();
	}
	if (v_structure_change) {
		v_setup_vectors();
	}
	if (diam_changed) {
		recalc_diam();
	}
	nrn_solver_prepare(); /* cvode ready to be used */
}
	
void nrn_finitialize(int setv, double v) {
	int iord, i;
	NrnThread* _nt;
	extern int _ninits;
	extern short* nrn_is_artificial_;
	++_ninits;

	nrn_fihexec(3); /* model structure changes can be made */
	verify_structure();
#if ELIMINATE_T_ROUNDOFF
	nrn_ndt_ = 0.; nrn_dt_ = dt; nrn_tbase_ = 0.;
#else
	t = 0.;
	dt2thread(-1.);
#endif
	if (cvode_active_) {
		nrncvode_set_t(t);
	}
	nrn_thread_table_check();
	clear_event_queue();
	nrn_spike_exchange_init();
	nrn_random_play(_nt);
#if VECTORIZE
	nrn_play_init(); /* Vector.play */
	for (i=0; i < nrn_nthread; ++i) {
		nrn_deliver_events(nrn_threads + i); /* The play events at t=0 */
	}
	if (setv) {
#if _CRAY
#pragma _CRI ivdep
#endif
		FOR_THREADS(_nt) for (i=0; i < _nt->end; ++i) {
			NODEV(_nt->_v_node[i]) = v;
		}
	}
#if 1 || PARANEURON
	if (nrnthread_vi_compute_) FOR_THREADS(_nt){
		(*nrnthread_vi_compute_)(_nt);
	}
	if (nrnmpi_v_transfer_) {
		(nrnmpi_v_transfer_)();
	}
	if (nrnthread_v_transfer_) FOR_THREADS(_nt){
		(*nrnthread_v_transfer_)(_nt);
	}
#endif
	nrn_fihexec(0); /* after v is set but before INITIAL blocks are called*/
	for (i=0; i < nrn_nthread; ++i) {
		nrn_ba(nrn_threads + i, BEFORE_INITIAL);
	}
#if _CRAY
	cray_node_init();
#endif
	/* the INITIAL blocks are ordered so that mechanisms that write
	   concentrations are after ions and before mechanisms that read
	   concentrations.
	*/
	/* the memblist list in NrnThread is already so ordered */
#if MULTICORE
	for (i=0; i < nrn_nthread; ++i) {
		NrnThread* nt = nrn_threads + i;
		nrn_nonvint_block_init(nt->id);
		NrnThreadMembList* tml;
		for (tml = nt->tml; tml; tml = tml->next) {
			Pvmi s = memb_func[tml->index].initialize;
			if (s) {
				(*s)(nt, tml->ml, tml->index);
			}
		}
	}
#endif
	for (iord=0; iord < n_memb_func; ++iord) {
	  i = memb_order_[iord];
	  /* first clause due to MULTICORE */
	  if (nrn_is_artificial_[i]) if (memb_func[i].initialize) {
	    Pvmi s = memb_func[i].initialize;
#if 0
	    if (nrn_is_artificial_[i]) {
		/*
	    	 I hope the space saving of the memb_list arrays is worth
	    	 doing this specifically. And if art cells are needed
	    	 in the memb_list anywhere else we will have do do something
	    	 similar to this. This gives up vectorization but this is only
	    	 initialization and all the other use of artcell should be
	    	 event driven.
		*/
		Prop* p;
		hoc_Item* q;
		hoc_List* list = nrn_pnt_template_[i]->olist;
		ITERATE(q, list) {
			Object* obj = OBJ(q);
			Prop* p = ((Point_process*)obj->u.this_pointer)->prop;
			(*s)((Node*)0, p->param, p->dparam);
		}
	    }else if (memb_list[i].nodecount){
#else
	    if (memb_list[i].nodecount){
#endif
		(*s)(nrn_threads, memb_list + i, i);
	    }
		if (errno) {
			if (nrn_errno_check(i)) {
hoc_warning("errno set during call to INITIAL block", (char*)0);
			}
		}
	  }
	}
#endif
	if (use_sparse13) {
		nrndae_init();
	}

	init_net_events();
	for (i = 0; i < nrn_nthread; ++i) {
		nrn_ba(nrn_threads + i, AFTER_INITIAL);
	}
	nrn_fihexec(1); /* after INITIAL blocks, before fcurrent*/

	for (i=0; i < nrn_nthread; ++i) {
		nrn_deliver_events(nrn_threads + i); /* The INITIAL sent events at t=0 */
	}
	if (cvode_active_) {
		cvode_finitialize(t);
		nrn_record_init();
	}else{
		state_discon_allowed_ = 0;
		for (i=0; i < nrn_nthread; ++i) {
			setup_tree_matrix(nrn_threads + i);
			if (nrn_use_fast_imem) {
				nrn_calc_fast_imem(nrn_threads + i);
			}
		}
		state_discon_allowed_ = 1;
#if 0 && NRN_DAQ
		nrn_daq_ao();
		nrn_daq_scanstart();
		nrn_daq_ai();
#endif
		nrn_record_init();
		for (i=0; i < nrn_nthread; ++i) {
			fixed_record_continuous(nrn_threads + i);
		}
	}
	for (i=0; i < nrn_nthread; ++i) {
		nrn_deliver_events(nrn_threads + i); /* The record events at t=0 */
	}
#if NRNMPI
	nrn_spike_exchange(nrn_threads);
#endif
	if (nrn_allthread_handle) { (*nrn_allthread_handle)(); }
	
	nrn_fihexec(2); /* just before return */
}
	
void finitialize(void) {
	int setv;
	double v = 0.0;
	setv = 0;
	if (ifarg(1)) {
		v = *getarg(1);
		setv = 1;
	}
	tstopunset;
	nrn_finitialize(setv, v);
	tstopunset;
	hoc_retpushx(1.);
}

static FILE* batch_file;
static int batch_size;
static int batch_n;
static double** batch_var;

static void  batch_open(name, tstop, tstep, comment)
	char* name, *comment;
	double tstop, tstep;
{
	if (batch_file) {
		batch_close();
	}
	if (!name) {
		return;
	}
	batch_file = fopen(name, "w");
	if (!batch_file) {
		hoc_execerror("Couldn't open batch file", name);
	}
	fprintf(batch_file, "%s\nbatch_run from t = %g to %g in steps of %g with dt = %g\n",
		comment, t, tstop, tstep, dt);
#if 0
	fprintf(batch_file, "variable names --\n");
	if (!batch_var) {
		batch_var = hoc_newlist();
	}
	count = 0;
	ITERATE(q, batch_varname) {
		fprintf(batch_file, "%s\n", STR(q));
		++count;
	}
	if (count != batch_n) {
		if (batch_var) {
			free(batch_var);
		}
		batch_n = count;
		batch_var = (double**)ecalloc(batch_n, sizeof(double*));
	}
	count = 0;
	ITERATE(q, batch_varname) {
		batch_var[count++] = hoc_val_pointer(STR(q));
	}
#endif
}

static void batch_close() {
	if (batch_file) {
		fclose(batch_file);
		batch_file = 0;
	}
}

static void batch_out() {
	if (batch_file) {
		int i;
		for (i =0; i < batch_n; ++i) {
			fprintf(batch_file, " %g", *batch_var[i]);
		}
		fprintf(batch_file,"\n");
	}
}

void batch_save(void) {
	double* hoc_pgetarg();
	int i;
	if (!ifarg(1)) {
		batch_n = 0;
	}else{
		for (i=1; ifarg(i); ++i) {	
			if ( batch_size <= batch_n){
				batch_size += 20;
				batch_var = (double**)erealloc(batch_var, batch_size*sizeof(double*));
			}
			batch_var[batch_n] = hoc_pgetarg(i);
			++batch_n;
		}
	}
	hoc_retpushx(1.);
}

void nrn_ba(NrnThread* nt, int bat){
	NrnThreadBAList* tbl;
	int i;
	for (tbl = nt->tbl[bat]; tbl; tbl = tbl->next) {
		nrn_bamech_t f = tbl->bam->f;
		int type = tbl->bam->type;
		Memb_list* ml = tbl->ml;
		for (i=0; i < ml->nodecount; ++i) {
			(*f)(ml->nodelist[i], ml->data[i], ml->pdata[i], ml->_thread, nt);
		}
	}
}

int set_nonvint_block(int (*new_nrn_nonvint_block)(int method, int size, double* pd1, double* pd2, int tid)) {
	nrn_nonvint_block = new_nrn_nonvint_block;
	return 0;
}

int nrn_nonvint_block_helper(int method, int size, double* pd1, double* pd2, int tid) {
	int rval = (*nrn_nonvint_block)(method, size, pd1, pd2, tid);
	if (rval == -1) {
		hoc_execerror("nrn_nonvint_block error", 0);
	}
	return rval;
}
