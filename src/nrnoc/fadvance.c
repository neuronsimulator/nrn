#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/fadvance.c,v 1.19 1999/03/24 18:38:48 hines Exp */

#include <nrnmpi.h>
#include <nrnrt.h>
#include <errno.h>
#include "neuron.h"
#include "section.h"
#include "membfunc.h"
#include "multisplit.h"

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
 
extern double chkarg();
extern void setup_tree_matrix(), nrn_solve();
extern int tree_changed;
extern int diam_changed;
extern int state_discon_allowed_;
extern double hoc_epsilon;
extern void nrncvode_set_t(double t);
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

#if 1 || PARANEURON
void (*nrnmpi_v_transfer_)();
#endif

#if VECTORIZE
extern int v_structure_change;
#endif

#if CVODE
int cvode_active_;
extern cvode_fadvance();
extern cvode_finitialize();
#endif

int stoprun;
static update();

#define PROFILE 0
#include "profile.h"

fadvance()
{
PSTART(0)
#if CVODE
	if (cvode_active_) {
		cvode_fadvance(-1.);
PSTOP(0)
		ret(1.);
		return;
	}
#endif
#if ELIMINATE_T_ROUNDOFF
	nrn_chk_ndt();
#endif
	if (tree_changed) {
		setup_topology();
	}
#if VECTORIZE
	if (v_structure_change) {
		v_setup_vectors();
	}
#endif
	deliver_net_events(); /* up to tentry + dt/2 */
	nrn_random_play();
#if ELIMINATE_T_ROUNDOFF
	nrn_ndt_ += .5;
	t = nrn_tbase_ + nrn_ndt_ * nrn_dt_;
#else
	t += .5*dt;
#endif
	fixed_play_continuous();
	setup_tree_matrix(); /* currents ambiguous between t and t+dt/2
				ie. v at tentry and states at tentry+dt/2 */
	nrn_solve();		/* rhs = v(texit) */
	second_order_cur();	/*ion currents adjusted to t(entry)+dt/2
				but individual mechanism currents not adjusted
				 */
	update();		/* v at texit */
#if ELIMINATE_T_ROUNDOFF
	nrn_ndt_ += .5;
	t = nrn_tbase_ + nrn_ndt_ * nrn_dt_;		/* t = texit */
#else
	t += .5*dt;
#endif
	fixed_play_continuous();
	nonvint();	/* m,h,n go to t+dt/2, concentrations go to t */
#if METHOD3
	if (_method3 == 3) {
		method3_axial_current();
	}
#endif
	nrn_ba(AFTER_SOLVE);
	fixed_record_continuous();
	nrn_deliver_events(t) ; /* up to but not past texit */
PSTOP(0)
	ret(1.);
}

/*
  batch_save() initializes list of variables
  batch_save(&varname, ...) adds variable names to list for saving
  batch_save("varname", ...) adds variable names to the list and name
  				will appear in header.
  batch_run(tstop, tstep, "file") saves variables in file every tstep
*/

static batch_out(), batch_open(), batch_close();
batch_run() /* avoid interpreter overhead */
{
	double tstop, tstep, tnext;
	char* filename;
	char* comment;

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
		}
	}
	batch_close();
	ret(1.);
}

nrn_daspk_init_step(dteps, upd) double dteps; int upd;{
	double dtsav = dt;
	int so = secondorder;
	dt = dteps;
	secondorder = 0;
	setup_tree_matrix();
	nrn_solve();
	if (upd) {
		update();
	}
	dt = dtsav;
	secondorder = so;
}

nrn_fixed_step() {
	static double dtsav;
#if ELIMINATE_T_ROUNDOFF
	nrn_chk_ndt();
#endif
	deliver_net_events();
	nrn_random_play();
#if ELIMINATE_T_ROUNDOFF
	nrn_ndt_ += .5;
	t = nrn_tbase_ + nrn_ndt_ * nrn_dt_;
#else
	t += .5*dt;
#endif
	fixed_play_continuous();
	setup_tree_matrix();
	nrn_solve();
	second_order_cur();
	update();
#if NRN_DAQ
	nrn_daq_ao();
#endif
#if ELIMINATE_T_ROUNDOFF
	nrn_ndt_ += .5;
	t = nrn_tbase_ + nrn_ndt_ * nrn_dt_;
#else
	t += .5*dt;
#endif
	fixed_play_continuous();
#if NRN_DAQ
	nrn_daq_scanstart();
#endif
	nonvint();
	nrn_ba(AFTER_SOLVE);
#if NRN_DAQ
	nrn_daq_ai();
#endif
	fixed_record_continuous();
	nrn_deliver_events(t) ; /* up to but not past texit */
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

static update()
{
	int i;

#if CACHEVEC
    if (use_cachevec) {
	/* do not need to worry about linmod or extracellular*/
	if (secondorder) {
		for (i=0; i < v_node_count; ++i) {
			VEC_V(i) += 2.*VEC_RHS(i);
		}
	}else{
		for (i=0; i < v_node_count; ++i) {
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
		for (i=0; i < v_node_count; ++i) {
			NODEV(v_node[i]) += 2.*NODERHS(v_node[i]);
		}
	}else{
#if _CRAY
#pragma _CRI ivdep
#endif
		for (i=0; i < v_node_count; ++i) {
			NODEV(v_node[i]) += NODERHS(v_node[i]);
		}
		if (use_sparse13) {
			linmod_update();
		}
	}
    } /* end of non-vectorized update */

#if EXTRACELLULAR
	nrn_update_2d();
#endif

#if I_MEMBRANE
	nrn_capacity_current(memb_list + CAP);
#endif

}

fcurrent()
{

	if (tree_changed) {
		setup_topology();
	}
#if VECTORIZE
	if (v_structure_change) {
		v_setup_vectors();
	}
#endif

#if 0	/* doesn't work for all possible models. */
	savdt = dt;
	dt = -1e-9;
	nonvint();	/* no states computed when dt <= 0 */
	dt = savdt;
#endif
	state_discon_allowed_ = 0;
	setup_tree_matrix();
	state_discon_allowed_ = 1;
	ret(1.);
}

nrn_print_matrix() {
	extern int section_count;
	extern Section** secorder;
	int isec, inode;
	Section* sec;
	Node* nd;
    if (use_sparse13) {
	if(ifarg(1) && chkarg(1, 0., 1.) == 0.) {
		spPrint(sp13mat_, 1, 0, 1);
	}else{
		int i, n = spGetSize(sp13mat_, 0);
		spPrint(sp13mat_, 1, 1, 1);
		for (i=1; i <= n; ++i) {
			printf("%d %g\n", i, actual_rhs[i]);
		}
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
fmatrix() {
	nrn_print_matrix();
	ret(1.);
}
nonvint()
{
#if VECTORIZE
	int i;
#else
	int isec, inode;
	Section *sec;
	Node *nd;
	Prop *p;
	double vm;
#endif

#if VECTORIZE
#if _CRAY
	cray_node_init();
#endif
#if 1 || PARANEURON
	if (nrnmpi_v_transfer_) {(*nrnmpi_v_transfer_)();}
#endif
	errno = 0;
	for (i=0; i < n_memb_func; ++i) if (memb_func[i].state && memb_list[i].nodecount){
		Pfri s = memb_func[i].state;
		(*s)(memb_list + i, i);
		if (errno) {
			if (nrn_errno_check(i)) {
hoc_warning("errno set during calculation of states", (char*)0);
			}
		}
	}
	long_difus_solve(0); /* if any longitudinal diffusion */
#endif
}

#if VECTORIZE
nrn_errno_check(i)
	int i;
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
nrn_errno_check(p, inode, sec)
	Prop* p;
	int inode;
	Section* sec;
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

frecord_init() { /* useful when changing states after an finitialize() */
	nrn_record_init();
	if (!cvode_active_) {
		fixed_record_continuous();
	}
	ret(1.);
}

verify_structure() {
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
	
finitialize() {
	int iord, i, setv=0;
	double v;
	extern int _ninits;
	++_ninits;

	nrn_fihexec(3); /* model structure changes can be made */
	if (ifarg(1)) {
		v = *getarg(1);
		setv = 1;
	}
#if ELIMINATE_T_ROUNDOFF
	nrn_ndt_ = 0.; nrn_dt_ = dt; nrn_tbase_ = 0.;
#else
	t = 0.;
#endif
	if (cvode_active_) {
		nrncvode_set_t(t);
	}
	clear_event_queue();
#if NRNMPI
	nrn_spike_exchange_init();
#endif
	nrn_random_play();
	verify_structure();
#if VECTORIZE
	nrn_play_init(); /* Vector.play */
	nrn_deliver_events(0.); /* The play events at t=0 */
	if (setv) {
#if _CRAY
#pragma _CRI ivdep
#endif
		for (i=0; i < v_node_count; ++i) {
			NODEV(v_node[i]) = v;
		}
	}
#if 1 || PARANEURON
	if (nrnmpi_v_transfer_) {(*nrnmpi_v_transfer_)();}
#endif
	nrn_fihexec(0); /* after v is set but before INITIAL blocks are called*/
	nrn_ba(BEFORE_INITIAL);
#if _CRAY
	cray_node_init();
#endif
	/* the INITIAL blocks are ordered so that mechanisms that write
	   concentrations are after ions and before mechanisms that read
	   concentrations.
	*/
	for (iord=0; iord < n_memb_func; ++iord) {
	  i = memb_order_[iord];
	  if (memb_func[i].initialize) {
	    Pfri s = memb_func[i].initialize;
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
		(*s)(memb_list + i, i);
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
		linmod_init();
	}

	init_net_events();
	nrn_ba(AFTER_INITIAL);
	nrn_fihexec(1); /* after INITIAL blocks, before fcurrent*/

	nrn_deliver_events(0.); /* The INITIAL sent events at t=0 */

	if (cvode_active_) {
		cvode_finitialize();
		nrn_record_init();
	}else{
		state_discon_allowed_ = 0;
		setup_tree_matrix();
		state_discon_allowed_ = 1;
#if 0 && NRN_DAQ
		nrn_daq_ao();
		nrn_daq_scanstart();
		nrn_daq_ai();
#endif
		nrn_record_init();
		fixed_record_continuous();
	}
	nrn_deliver_events(0.); /* The record events at t=0 */
#if NRNMPI
	nrn_spike_exchange();
#endif
	nrn_fihexec(2); /* just before return */
	ret(1.);
}
	

static FILE* batch_file;
static int batch_size;
static int batch_n;
static double** batch_var;

static batch_open(name, tstop, tstep, comment)
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

static batch_close() {
	if (batch_file) {
		fclose(batch_file);
		batch_file = 0;
	}
}

static batch_out() {
	if (batch_file) {
		int i;
		for (i =0; i < batch_n; ++i) {
			fprintf(batch_file, " %g", *batch_var[i]);
		}
		fprintf(batch_file,"\n");
	}
}

batch_save() {
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
	ret(1.);
}

/* BEFORE and AFTER functions of mechanisms are executed globally */
/* the BEFORE BREAKPOINT, AFTER SOLVE, and BEFORE STEP for the local step method
requires special handling */
nrn_ba(bat) int bat; {
	int i, j;
	BAMech* bam;
	for (bam = bamech_[bat]; bam; bam = bam->next) {
		Memb_list* ml = memb_list + bam->type;
		for (i=0; i < ml->nodecount; ++i) {
			(*bam->f)(ml->nodelist[i], ml->data[i], ml->pdata[i]);
		}
	}
}

