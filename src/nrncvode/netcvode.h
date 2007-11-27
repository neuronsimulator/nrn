#ifndef netcvode_h
#define netcvode_h

#define PRINT_EVENT 1

#include "mymath.h"
#include "tqueue.h"

class PreSyn;
class HocDataPaths;
class PreSynTable;
class NetCon;
class DiscreteEvent;
class hoc_Item;
class PlayRecord;
class PlayRecList;
class IvocVect;
class BAMechList;
class MaxStateTable;
class HTList;
class HTListList;
struct BAMech;
struct Section;

class NetCvode {
public:
	NetCvode(boolean single=true);
	virtual ~NetCvode();
	int solve(double t);
	void statistics(int);
	void spike_stat();
	void re_init(double t0 = 0.);
	int cellindex();
	void states();
	void dstates();
	void error_weights();
	void acor();
	const char* statename(int, int style=1);
	void localstep(boolean); boolean localstep();
	void use_daspk(boolean); boolean use_daspk();
	void move_event(TQItem*, double);
	void remove_event(TQItem*);
	TQItem* event(double tdeliver, DiscreteEvent*);
#if BBTQ == 3 || BBTQ == 4
	TQItem* fifo_event(double tdeliver, DiscreteEvent*);
#endif
#if BBTQ == 5
	TQItem* bin_event(double tdeliver, DiscreteEvent*);
#endif
	void null_event(double);
	void tstop_event(double);
	void handle_tstop_event(double);
	void hoc_event(double, const char* hoc_stmt);
	NetCon* install_deliver(double* psrc, Section* ssrc, Object* osrc,
		Object* target,	double threshold, double delay,
		double weight
	);
	void presyn_disconnect(PreSyn*);
	void deliver_net_events(); // for default staggered time step method
	void deliver_events(double til); // for initialization events
	void solver_prepare();
	void clear_events();
	void init_events();
	void print_event_queue();
	void event_queue_info();
	void vec_event_store();
	void retreat(double, Cvode*);
	Object** netconlist();
	PlayRecord* playrec_uses(void*);
	void playrec_add(PlayRecord*);
	void playrec_remove(PlayRecord*);
	int playrec_item(PlayRecord*);
	PlayRecord* playrec_item(int);
	PlayRecList*  playrec_list() { return prl_;}
	void simgraph_remove();
	// fixed step continuous play and record
	PlayRecList* fixed_play_;
	PlayRecList* fixed_record_;
	void vecrecord_add(); // hoc interface functions
	void vec_remove();
	void record_init();
	void play_init();
	void fixed_record_continuous();
	void fixed_play_continuous();
	void stelist_change();
	void ste_check(); // for fixed step;
	static double eps(double x) { return eps_*Math::abs(x); }
	int condition_order() { return condition_order_; }
	void condition_order(int i) { condition_order_ = i; }
	TQueue* event_queue() { return tqe_; }
	void psl_append(PreSyn*);
	void recalc_ptrs(int, double**, double*);
public:
	void rtol(double); double rtol(){return rtol_;}
	void atol(double); double atol(){return atol_;}
	double rtol_, atol_;
	void stiff(int); int stiff(){return stiff_;} // 0 nothing stiff, 1 voltage, 2 mechanisms
	void maxorder(int); int maxorder(){return maxorder_;}
	int order(int);
	void minstep(double); double minstep(){return minstep_;}
	void maxstep(double); double maxstep(){return maxstep_;}
	void jacobian(int); int jacobian(){return jacobian_;}
	void structure_change();
	int unreffed_event_cnt_;
	int print_event_;
	int nlist() { return nlist_; }
	Cvode* list() { return list_; }
	boolean initialized_; // for global step solve.
	void consist_sec_pd(const char*, Section*, double*);
	double state_magnitudes();
	Symbol* name2sym(const char*);
	const char* sym2name(Symbol*);
	int pgvts(double tstop);
private:
	static double eps_;
	int local_microstep();
	int global_microstep();
	void deliver_least_event();
	void evaluate_conditions();
	int condition_order_;
	
	int pgvts_event(double& tt);
	DiscreteEvent* pgvts_least(double& tt, int& op, int& init);
	int pgvts_cvode(double tt, int op);
	
	boolean init_global();
	void delete_list();
	void alloc_list();
	void del_cv_memb_list();
	void distribute_dinfo(int*);
	void playrec_setup();
	void fill_global_ba(int, BAMechList**);
	void fill_local_ba(int*);
	void fill_local_ba_cnt(int, int*);
	void fill_local_ba_alloc();
	void fill_local_ba_indices(int, int*);
	BAMechList* cvbml(int, BAMech*, Cvode*);
	void maxstate_analyse();
	void fornetcon_prepare();
	int fornetcon_change_cnt_;
	double maxstate_analyse(Symbol*, double*);
	MaxStateTable* mst_;
private:
	int maxorder_, jacobian_, stiff_;
	double maxstep_, minstep_;
	
	int structure_change_cnt_;
	int matrix_change_cnt_;
	boolean single_;
	int nlist_;
	Cvode* list_;
	TQueue* tq_;
	TQueue* tqe_;
	TQueue* tqr_;
	PreSynTable* pst_;
	int pst_cnt_;
	int playrec_change_cnt_;
	PlayRecList* prl_;
	IvocVect* vec_event_store_;
	HocDataPaths* hdp_;
public:
	hoc_Item* psl_; //actually a hoc_List
	hoc_Item* psl_th_; //for presyns with fixed step threshold checking
	HTListList* wl_list_; // for faster deliver_net_events when many cvode
};	
	
#endif
