#ifndef netcvode_h
#define netcvode_h

#define PRINT_EVENT 1

#include "mymath.h"
#include "tqueue.h"

struct NrnThread;
class PreSyn;
class PreSynTable;
class NetCon;
class DiscreteEvent;
class TQItemPool;
class SelfEventPool;
class SelfEvent;
class hoc_Item;
class PlayRecord;
class PlayRecList;
class IvocVect;
class BAMechList;
class MaxStateTable;
class HTList;
class HTListList;
class NetCvode;
class MaxStateItem;
struct BAMech;
struct InterThreadEvent;

class NetCvodeThreadData {
public:
	NetCvodeThreadData();
	virtual ~NetCvodeThreadData();
	void interthread_send(double, DiscreteEvent*, NrnThread*);
	void enqueue(NetCvode*, NrnThread*);
	TQueue* tq_; // for lvardt
	Cvode* lcv_; // for lvardt
	TQueue* tqe_;
	hoc_Item* psl_thr_; //for presyns with fixed step threshold checking
	SelfEventPool* sepool_;
	TQItemPool* tpool_;
	InterThreadEvent* inter_thread_events_;
	SelfQueue* selfqueue_;
	MUTDEC
	int nlcv_;
	int ite_cnt_;
	int ite_size_;
	int unreffed_event_cnt_;
	double immediate_deliver_;
};

class NetCvode {
public:
	NetCvode(bool single=true);
	virtual ~NetCvode();
	void statistics(int);
	void spike_stat();
	void move_event(TQItem*, double, NrnThread*);
	void remove_event(TQItem*, int threadid);
	TQItem* event(double tdeliver, DiscreteEvent*, NrnThread*);
#if BBTQ == 5
	TQItem* bin_event(double tdeliver, DiscreteEvent*, NrnThread*);
#endif
	void send2thread(double, DiscreteEvent*, NrnThread*);
	void null_event(double);
	void tstop_event(double);
	void handle_tstop_event(double, NrnThread* nt);
	void hoc_event(double, const char* hoc_stmt, Object* ppobj = nil, int reinit = 0, Object* pyact=nil);
	void check_thresh(NrnThread*);
	void deliver_net_events(NrnThread*); // for default staggered time step method
	void deliver_events(double til, NrnThread*); // for initialization events
	void clear_events();
	void init_events();
	Object** netconlist();
	int owned_by_thread(double*);
	PlayRecord* playrec_uses(void*);
	void playrec_add(PlayRecord*);
	void playrec_remove(PlayRecord*);
	int playrec_item(PlayRecord*);
	PlayRecord* playrec_item(int);
	PlayRecList*  playrec_list() { return prl_;}
	// fixed step continuous play and record
	PlayRecList* fixed_play_;
	PlayRecList* fixed_record_;
	void vecrecord_add(); // hoc interface functions
	void vec_remove();
	void record_init();
	void play_init();
	void fixed_record_continuous(NrnThread*);
	void fixed_play_continuous(NrnThread*);
	void stelist_change();
	void ste_check(); // for fixed step;
	static double eps(double x) { return eps_*Math::abs(x); }
	TQueue* event_queue(NrnThread* nt);
	void psl_append(PreSyn*);
public:
	int print_event_;
	void point_receive(int, Point_process*, double*, double);
	bool deliver_event(double til, NrnThread*); //uses TQueue atomically
	bool empty_;
	void delete_list();
	void delete_list(Cvode*);
//private:
public:
	static double eps_;
	void deliver_least_event(NrnThread*);

	void alloc_list();
	void distribute_dinfo(int*, int);
	void playrec_setup();
	void fornetcon_prepare();
	int fornetcon_change_cnt_;
	void p_construct(int);
	void ps_thread_link(PreSyn*);
private:
	PreSynTable* pst_;
	int pst_cnt_;
	int playrec_change_cnt_;
	PlayRecList* prl_;
	IvocVect* vec_event_store_;
	HocDataPaths* hdp_;
public:
	Cvode* gcv_;
	void set_CVRhsFn();
	bool use_partrans();
	hoc_Item* psl_; //actually a hoc_List
	HTListList* wl_list_; // for faster deliver_net_events when many cvode
	int pcnt_;
	NetCvodeThreadData* p;
	int enqueueing_;
public:
	double allthread_least_t(int& tid);
	int solve_when_threads(double);
	void deliver_events_when_threads(double);
};	
	
#endif
