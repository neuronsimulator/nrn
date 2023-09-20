#ifndef netcvode_h
#define netcvode_h

#define PRINT_EVENT 1

#include "mymath.h"

#include "cvodeobj.h"
#include "neuron/container/data_handle.hpp"
#include "tqueue.h"

#include <cmath>
#include <vector>
#include <unordered_map>

struct NrnThread;
class PreSyn;
class HocDataPaths;
using PreSynTable = std::unordered_map<neuron::container::data_handle<double>, PreSyn*>;
class NetCon;
class DiscreteEvent;
class SelfEvent;
using SelfEventPool = MutexPool<SelfEvent>;
struct hoc_Item;
class PlayRecord;
class IvocVect;
struct BAMechList;
class HTList;
// nrn_nthread vectors of HTList* for fixed step method
// Thread segregated HTList* of all the CVode.CvodeThreadData.HTList*
// Interior vector needed because of the chance of local variable time step.
//   Practically it will always have length <= 1.
using HTListList = std::vector<std::vector<HTList*>>;
class NetCvode;
class MaxStateItem;
typedef std::unordered_map<void*, MaxStateItem*> MaxStateTable;
class CvodeThreadData;
class HocEvent;
typedef std::vector<HocEvent*> HocEventList;
struct BAMech;
struct Section;
struct InterThreadEvent;

class NetCvodeThreadData {
  public:
    NetCvodeThreadData();
    virtual ~NetCvodeThreadData();
    void interthread_send(double, DiscreteEvent*, NrnThread*);
    void enqueue(NetCvode*, NrnThread*);
    TQueue* tq_;  // for lvardt
    Cvode* lcv_;  // for lvardt
    TQueue* tqe_;
    hoc_Item* psl_thr_;  // for presyns with fixed step threshold checking
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
    NetCvode(bool single = true);
    virtual ~NetCvode();
    int solve(double t);
    void statistics(int);
    void spike_stat();
    void re_init(double t0 = 0.);
    int cellindex();
    void states();
    void dstates();
    int fun(double t, double* y, double* ydot);
    void error_weights();
    void acor();
    std::string statename(int, int style = 1);
    void localstep(bool);
    bool localstep();
    bool is_local();
    void use_daspk(bool);
    bool use_daspk();
    void move_event(TQItem*, double, NrnThread*);
    void remove_event(TQItem*, int threadid);
    TQItem* event(double tdeliver, DiscreteEvent*, NrnThread*);
#if BBTQ == 4
    TQItem* fifo_event(double tdeliver, DiscreteEvent*, NrnThread*);
#endif
#if BBTQ == 5
    TQItem* bin_event(double tdeliver, DiscreteEvent*, NrnThread*);
#endif
    void send2thread(double, DiscreteEvent*, NrnThread*);
    void null_event(double);
    void tstop_event(double);
    void hoc_event(double,
                   const char* hoc_stmt,
                   Object* ppobj = nullptr,
                   int reinit = 0,
                   Object* pyact = nullptr);
    NetCon* install_deliver(neuron::container::data_handle<double> psrc,
                            Section* ssrc,
                            Object* osrc,
                            Object* target,
                            double threshold,
                            double delay,
                            double weight);
    void presyn_disconnect(PreSyn*);
    void check_thresh(NrnThread*);
    void deliver_net_events(NrnThread*);          // for default staggered time step method
    void deliver_events(double til, NrnThread*);  // for initialization events
    void solver_prepare();
    void clear_events();
    void free_event_pools();
    void init_events();
    void print_event_queue();
    void event_queue_info();
    void vec_event_store();
    void local_retreat(double, Cvode*);
    void retreat(double, Cvode*);
    Object** netconlist();
    int owned_by_thread(neuron::container::data_handle<double> const&);
    PlayRecord* playrec_uses(void*);
    void playrec_add(PlayRecord*);
    void playrec_remove(PlayRecord*);
    int playrec_item(PlayRecord*);
    PlayRecord* playrec_item(int);
    std::vector<PlayRecord*>* playrec_list() {
        return prl_;
    }
    void simgraph_remove();
    // fixed step continuous play and record
    std::vector<PlayRecord*>* fixed_play_;
    std::vector<PlayRecord*>* fixed_record_;
    void vecrecord_add();  // hoc interface functions
    void vec_remove();
    void record_init();
    void play_init();
    void fixed_record_continuous(neuron::model_sorted_token const&, NrnThread& nt);
    void fixed_play_continuous(NrnThread*);
    static double eps(double x) {
        return eps_ * std::abs(x);
    }
    int condition_order() {
        return condition_order_;
    }
    void condition_order(int i) {
        condition_order_ = i;
    }
    TQueue* event_queue(NrnThread* nt);
    void psl_append(PreSyn*);

  public:
    void rtol(double);
    double rtol() {
        return rtol_;
    }
    void atol(double);
    double atol() {
        return atol_;
    }
    double rtol_, atol_;
    void stiff(int);
    int stiff() {
        return stiff_;
    }  // 0 nothing stiff, 1 voltage, 2 mechanisms
    void maxorder(int);
    int maxorder() {
        return maxorder_;
    }
    int order(int);
    void minstep(double);
    double minstep() {
        return minstep_;
    }
    void maxstep(double);
    double maxstep() {
        return maxstep_;
    }
    void jacobian(int);
    int jacobian() {
        return jacobian_;
    }
    void structure_change();
    int print_event_;
    //	int nlist() { return nlist_; }
    //	Cvode* list() { return list_; }
    bool initialized_;  // for global step solve.
    void consist_sec_pd(const char*, Section*, neuron::container::data_handle<double> const&);
    double state_magnitudes();
    Symbol* name2sym(const char*);
    const char* sym2name(Symbol*);
    int pgvts(double tstop);
    void update_ps2nt();
    void point_receive(int, Point_process*, double*, double);
    bool deliver_event(double til, NrnThread*);  // uses TQueue atomically
    bool empty_;
    void delete_list();
    void delete_list(Cvode*);
    // private:
  public:
    static double eps_;
    int local_microstep(neuron::model_sorted_token const&, NrnThread&);
    int global_microstep();
    void deliver_least_event(NrnThread*);
    void evaluate_conditions();
    int condition_order_;

    int pgvts_event(double& tt);
    DiscreteEvent* pgvts_least(double& tt, int& op, int& init);
    int pgvts_cvode(double tt, int op);

    bool init_global();
    void alloc_list();
    void del_cv_memb_list();
    void del_cv_memb_list(Cvode*);
    void distribute_dinfo(int*, int);
    void playrec_setup();
    void fill_global_ba(NrnThread*, int, BAMechList**);
    void fill_local_ba(int*, NetCvodeThreadData&);
    void fill_local_ba_cnt(int, int*, NetCvodeThreadData&);
    BAMechList* cvbml(int, BAMech*, Cvode*);
    void maxstate_analyse();
    void maxstate_analyze_1(int, Cvode&, CvodeThreadData&);
    void fornetcon_prepare();
    int fornetcon_change_cnt_;
    double maxstate_analyse(Symbol*, double*);
    void p_construct(int);
    void ps_thread_link(PreSyn*);
    MaxStateTable* mst_;

  private:
    int maxorder_, jacobian_, stiff_;
    double maxstep_, minstep_;

    int structure_change_cnt_;
    int matrix_change_cnt_;
    bool single_;
    PreSynTable* pst_;
    int pst_cnt_;
    int playrec_change_cnt_;
    std::vector<PlayRecord*>* prl_;
    IvocVect* vec_event_store_;
    HocDataPaths create_hdp(int style);

  public:
    Cvode* gcv_;
    void set_CVRhsFn();
    bool use_partrans();
    hoc_Item* psl_;       // actually a hoc_List
    HTListList wl_list_;  // nrn_nthread of these for faster deliver_net_events when many cvode
    int pcnt_;
    NetCvodeThreadData* p;
    int enqueueing_;
    int use_long_double_;

  public:
    MUTDEC  // only for enqueueing_ so far.
    void set_enqueueing();
    double allthread_least_t(int& tid);
    int solve_when_threads(double);
    void deliver_events_when_threads(double);
    int global_microstep_when_threads();
    void allthread_handle(double, HocEvent*, NrnThread*);
    void allthread_handle();
    HocEventList* allthread_hocevents_;
};

#endif
