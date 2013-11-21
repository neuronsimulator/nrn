#include "simcore/nrnconf.h"
#include "simcore/nrnoc/multicore.h"
#include "simcore/nrnoc/nrnoc_decl.h"
	
void nrn_finitialize(int setv, double v) {
	int iord, i;
	NrnThread* _nt;
	extern int _ninits;
	extern short* nrn_is_artificial_;
	++_ninits;

#if ELIMINATE_T_ROUNDOFF
	nrn_ndt_ = 0.; nrn_dt_ = dt; nrn_tbase_ = 0.;
#else
	t = 0.;
	dt2thread(-1.);
#endif
	nrn_thread_table_check();
	clear_event_queue();
	nrn_spike_exchange_init();
	for (i=0; i < nrn_nthread; ++i) {
		nrn_random_play(nrn_threads + i);
	}
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
			VEC_V(i) = v;
		}
	}
#if 0 /* Alex:: gap junctions maybe someday */
	if (nrnmpi_v_transfer_) {
		(nrnmpi_v_transfer_)();
	}
	if (nrnthread_v_transfer_) FOR_THREADS(_nt){
		(*nrnthread_v_transfer_)(_nt);
	}
#endif
	for (i=0; i < nrn_nthread; ++i) {
		nrn_ba(nrn_threads + i, BEFORE_INITIAL);
	}
	/* the INITIAL blocks are ordered so that mechanisms that write
	   concentrations are after ions and before mechanisms that read
	   concentrations.
	*/
	/* the memblist list in NrnThread is already so ordered */
#if MULTICORE
	for (i=0; i < nrn_nthread; ++i) {
		NrnThread* nt = nrn_threads + i;
		NrnThreadMembList* tml;
		for (tml = nt->tml; tml; tml = tml->next) {
			mod_f_t s = memb_func[tml->index].initialize;
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
	    mod_f_t s = memb_func[i].initialize;
	    if (memb_list[i].nodecount){
		(*s)(nrn_threads, memb_list + i, i);
	    }
		if (errno) {
hoc_warning("errno set during call to INITIAL block", (char*)0);
		}
	  }
	}
#endif

	init_net_events();
	for (i = 0; i < nrn_nthread; ++i) {
		nrn_ba(nrn_threads + i, AFTER_INITIAL);
	}
	for (i=0; i < nrn_nthread; ++i) {
		nrn_deliver_events(nrn_threads + i); /* The INITIAL sent events at t=0 */
	}
	{
		for (i=0; i < nrn_nthread; ++i) {
			setup_tree_matrix(nrn_threads + i);
		}
		nrn_record_init();
		for (i=0; i < nrn_nthread; ++i) {
			fixed_record_continuous(nrn_threads + i);
		}
	}
	for (i=0; i < nrn_nthread; ++i) {
		nrn_deliver_events(nrn_threads + i); /* The record events at t=0 */
	}
#if NRNMPI
	nrn_spike_exchange();
#endif
}
