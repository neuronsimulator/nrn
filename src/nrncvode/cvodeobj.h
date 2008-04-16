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
	int* indices; // if nil then all in memb_list of bam->type
	int cnt; // number of indices if non nil
	static void destruct(BAMechList** first);
	static void alloc(BAMechList* first);
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
	boolean initialize_;
	boolean can_retreat_; // only true after an integration step
	// statistics
	void stat_init();
	int advance_calls_, interpolate_calls_, init_calls_;
	int f_calls_, mxb_calls_, jac_calls_, ts_inits_;
	HTList* watch_list_;
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
	int setup(double* ypred, double* fpred);
	int solvex(double* b, double* y);
	void fun(double t, double* y, double* ydot);
	boolean at_time(double);
	void set_init_flag();
	void check_deliver();
	void evaluate_conditions();
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
	void play_add(PlayRecord*);
	void play_continuous(double t);
	void delete_memb_list(CvMembList*);
	void do_ode();
private:
	void cvode_constructor();
	boolean init_global();
	void init_eqn();
	void daspk_init_eqn();
	void matmeth();
	void nocap_v();
	void do_nonode();
	void solvemem();
	void atolvec_alloc(int);
	double h();
	double* ewtvec();
	double* acorvec();
	void new_no_cap_memb();
	void before_after(BAMechList*);
public:
	// daspk
	boolean use_daspk_;
	Daspk* daspk_;
	int res(double, double*, double*, double*);
	int jac(double, double*, double*, double);
	int psol(double, double*, double*, double);
	void daspk_scatter_y(double*); // daspk solves vi,vx instead of vm,vx
	void daspk_gather_y(double*);
	void scatter_y(double*);
	void gather_y(double*);
	void scatter_ydot(double*);
	void gather_ydot(double*);
public:
	void activate_maxstate(boolean);
	void maxstate(double*);
	void maxstate(boolean);
	void maxacor(double*);
public:
	void* mem_;
	N_Vector y_;
	N_Vector atolnvec_;
	double* maxstate_;
	double* maxacor_;
public:
	double* atolvec_; // lives in e_->atolnvec_
	boolean structure_change_;
#if USENEOSIM
	TQueue* neosim_self_events_;
#endif
public:
	NetCvode* ncv_;
	int neq_;
	int neq_v_; //for daspk
	int event_flag_;
	double next_at_time_;
	double tstop_;
	double tstop_begin_, tstop_end_;

	double** pv_;
	double** pvdot_;
	double* scattered_y_;
private:
	int no_cap_count; // number of nodes with no capacitance
	int no_cap_child_count;
	Node** no_cap_node;
	Node** no_cap_child; // connected to nodes that have no capacitance
private:
	void rhs();
	void rhs_memb(CvMembList*);
	void lhs();
	void lhs_memb(CvMembList*);
	void triang();
	void bksub();
private:
	// segregation of old vectorized information to per cell info
	friend class NetCvode;
	boolean is_owner(double*); // for play and record in local step context.
	boolean local_;
	void daspk_setup1_tree_matrix(); //unused
	void daspk_setup2_tree_matrix(); //unused
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
	TQItem* tqitem_;
	PreSynList* psl_th_; // with a threshold
	STEList* ste_list_;
private:
	PlayRecList* record_;
	PlayRecList* play_;
	int prior2init_;
#if PARANEURON
public:
	boolean use_partrans_;
	int global_neq_;
	int mpicomm_;
	int opmode_; // 1 advance, 2 interpolate, 3 init; for testing
#endif // PARANEURON
};

#endif
