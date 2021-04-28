#ifndef cvodeobj_h
#define cvodeobj_h

#include "nrnmpi.h"
#include "nrnneosm.h"
//#include "shared/nvector_serial.h"
#include "shared/nvector.h"
#include "membfunc.h"

class NetCvode;
class Daspk;
class TQItem;
class TQueue;
class PreSynList;
struct BAMech;
struct NrnThread;
class PlayRecList;
class PlayRecord;
class STEList;
class HTList;

class CvMembList {
public:
	CvMembList();
	virtual ~CvMembList();
	CvMembList* next;
	Memb_list* ml;
	int index;
};

class BAMechList {
public:
	BAMechList(BAMechList** first);
	BAMechList* next;
	BAMech* bam;
	Memb_list* ml;
	static void destruct(BAMechList** first);
};

#define CTD(i) ctd_[((nctd_ > 1) ? (i) : 0)]
class CvodeThreadData {
public:
	CvodeThreadData();
	virtual ~CvodeThreadData();
	void delete_memb_list(CvMembList*);

	int no_cap_count_; // number of nodes with no capacitance
	int no_cap_child_count_;
	Node** no_cap_node_;
	Node** no_cap_child_; // connected to nodes that have no capacitance
	CvMembList* cv_memb_list_;
	CvMembList* cmlcap_;
	CvMembList* cmlext_; // used only by daspk
	CvMembList* no_cap_memb_; // used only by cvode, point processes in the no cap nodes
	BAMechList* before_breakpoint_;
	BAMechList* after_solve_;
	BAMechList* before_step_;
	int rootnodecount_;
	int v_node_count_;
	Node** v_node_;
	Node** v_parent_;
	PreSynList* psl_th_; // with a threshold
	HTList* watch_list_;
	double** pv_;
	double** pvdot_;
	int nvoffset_; // beginning of this threads states
	int nvsize_; // total number of states for this thread
	int neq_v_; //for daspk, number of voltage states for this thread
	int nonvint_offset_; // synonym for neq_v_. Beginning of this threads nonvint variables.
	int nonvint_extra_offset_; // extra states (probably Python). Not scattered or gathered.
	PlayRecList* record_;
	PlayRecList* play_;
};

class Cvode {
public:
	Cvode(NetCvode*);
	Cvode();
	virtual ~Cvode();
	
	virtual int handle_step(NetCvode*, double);
	virtual int init(double t);
	virtual int advance_tn();
	virtual int interpolate(double t);
	virtual double tn() { return tn_;} // furthest time of advance
	virtual double t0() { return t0_;} // beginning of last real step
	void init_prepare();

	int solve(); // checks event_flag and init or advance_tn
	void statistics();
	double gam();
	double time() const { return t_; }
	void free_cvodemem();
	int order();
	void maxorder(int), minstep(double), maxstep(double);
public:
	double tn_, t0_, t_;
	bool initialize_;
	bool can_retreat_; // only true after an integration step
	// statistics
	void stat_init();
	int advance_calls_, interpolate_calls_, init_calls_;
	int f_calls_, mxb_calls_, jac_calls_, ts_inits_;
private:
	void alloc_cvode();
	void alloc_daspk();
	int cvode_init(double);
	int cvode_advance_tn();
	int cvode_interpolate(double);
	int daspk_init(double);
	int daspk_advance_tn();
	int daspk_interpolate(double);
public:
	N_Vector nvnew(long);
	int setup(N_Vector ypred, N_Vector fpred);
	int solvex_thread(double* b, double* y, NrnThread* nt);
	int solvex_thread_part1(double* b, NrnThread* nt);
	int solvex_thread_part2(NrnThread* nt);
	int solvex_thread_part3(double* b, NrnThread* nt);
	void fun_thread(double t, double* y, double* ydot, NrnThread* nt);
	void fun_thread_transfer_part1(double t, double* y, NrnThread* nt);
	void fun_thread_transfer_part2(double* ydot, NrnThread* nt);
	void fun_thread_ms_part1(double t, double* y, NrnThread* nt);
	void fun_thread_ms_part2(NrnThread* nt);
	void fun_thread_ms_part3(NrnThread* nt);
	void fun_thread_ms_part4(double* ydot, NrnThread* nt);
	void fun_thread_ms_part34(double* ydot, NrnThread* nt);
	bool at_time(double, NrnThread*);
	void set_init_flag();
	void check_deliver(NrnThread* nt = 0);
	void evaluate_conditions(NrnThread* nt = 0);
	void ste_check();
	void states(double*);
	void dstates(double*);
	void error_weights(double*);
	void acor(double*);
	void fill(Cvode* standard);
	// following 7 crucial for local time step recording, also used by global
	void delete_prl();
	void record_add(PlayRecord*);
	void record_continuous();
	void record_continuous_thread(NrnThread*);
	void play_add(PlayRecord*);
	void play_continuous(double t);
	void play_continuous_thread(double t, NrnThread*);
	void do_ode(NrnThread*);
	void do_nonode(NrnThread* nt = 0);
	double* n_vector_data(N_Vector, int);
private:
	void cvode_constructor();
	bool init_global();
	void init_eqn();
	void daspk_init_eqn();
	void matmeth();
	void nocap_v(NrnThread*);
	void nocap_v_part1(NrnThread*);
	void nocap_v_part2(NrnThread*);
	void nocap_v_part3(NrnThread*);
	void solvemem(NrnThread*);
	void atolvec_alloc(int);
	double h();
	N_Vector ewtvec();
	N_Vector acorvec();
	void new_no_cap_memb(CvodeThreadData&, NrnThread*);
	void before_after(BAMechList*, NrnThread*);
public:
	// daspk
	bool use_daspk_;
	Daspk* daspk_;
	int res(double, double*, double*, double*, NrnThread*);
	int psol(double, double*, double*, double, NrnThread*);
	void daspk_scatter_y(N_Vector); // daspk solves vi,vx instead of vm,vx
	void daspk_gather_y(N_Vector);
	void daspk_scatter_y(double*, int);
	void daspk_gather_y(double*, int);
	void scatter_y(double*, int);
	void gather_y(N_Vector);
	void gather_y(double*, int);
	void scatter_ydot(double*, int);
	void gather_ydot(N_Vector);
	void gather_ydot(double*, int);
public:
	void activate_maxstate(bool);
	void maxstate(double*);
	void maxstate(bool, NrnThread* nt = 0);
	void maxacor(double*);
public:
	void* mem_;
	N_Vector y_;
	N_Vector atolnvec_;
	N_Vector maxstate_;
	N_Vector maxacor_;
public:
	bool structure_change_;
#if USENEOSIM
	TQueue* neosim_self_events_;
#endif
public:
	CvodeThreadData* ctd_;
	NrnThread* nth_; // for lvardt
	int nctd_;
	long int* nthsizes_; // N_Vector_NrnThread uses this copy of ctd_[i].nvsize_
	NetCvode* ncv_;
	int neq_;
	int event_flag_;
	double next_at_time_;
	double tstop_;
	double tstop_begin_, tstop_end_;

private:
	void rhs(NrnThread*);
	void rhs_memb(CvMembList*, NrnThread*);
	void lhs(NrnThread*);
	void lhs_memb(CvMembList*, NrnThread*);
	void triang(NrnThread*);
	void bksub(NrnThread*);
private:
	// segregation of old vectorized information to per cell info
	friend class NetCvode;
	bool is_owner(double*); // for play and record in local step context.
	bool local_;
	void daspk_setup1_tree_matrix(); //unused
	void daspk_setup2_tree_matrix(); //unused
	TQItem* tqitem_;
private:
	int prior2init_;
#if PARANEURON
public:
	bool use_partrans_;
	int global_neq_;
	int opmode_; // 1 advance, 2 interpolate, 3 init; for testing
#endif // PARANEURON
};

#endif
