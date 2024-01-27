#ifndef netcon_h
#define netcon_h

#undef check

#include "htlist.h"
#include "neuron/container/data_handle.hpp"
#include "nrnmpi.h"
#include "nrnneosm.h"
#include "pool.h"

#include <InterViews/observe.h>

#include <memory>
#include <unordered_map>
#include <vector>

#if 0
#define STATISTICS(arg) ++arg
#else
#define STATISTICS(arg) /**/
#endif

class PreSyn;
class PlayRecord;
class Cvode;
class TQueue;
class TQItem;
struct NrnThread;
class NetCvode;
class HocEvent;
using HocEventPool = MutexPool<HocEvent>;
class HocCommand;
struct STETransition;
class IvocVect;
class Multisend_Send;
class Multisend_Send_Phase2;
struct hoc_Item;
struct Object;
struct Point_process;
struct Section;
using SelfEventPPTable = std::unordered_map<long, Point_process*>;

#define DiscreteEventType   0
#define TstopEventType      1  // no longer used
#define NetConType          2
#define SelfEventType       3
#define PreSynType          4
#define HocEventType        5
#define PlayRecordEventType 6
// the above will in turn steer to proper PlayRecord type
#define NetParEventType 7

#if DISCRETE_EVENT_OBSERVER
class DiscreteEvent: public Observer {
#else
class DiscreteEvent {
#endif
  public:
    DiscreteEvent();
    virtual ~DiscreteEvent();
    virtual void send(double deliverytime, NetCvode*, NrnThread*);
    virtual void deliver(double t, NetCvode*, NrnThread*);
    virtual void pr(const char*, double t, NetCvode*);
    virtual void disconnect(Observable*){};
    virtual int pgvts_op(int& i) {
        i = 0;
        return 2;
    }
    virtual void pgvts_deliver(double t, NetCvode*);
    virtual NrnThread* thread();

    virtual int type() {
        return DiscreteEventType;
    }
    virtual DiscreteEvent* savestate_save();
    virtual void savestate_restore(double deliverytime, NetCvode*);
    virtual void savestate_write(FILE*);
    static DiscreteEvent* savestate_read(FILE*);

    // actions performed over each item in the event queue.
    virtual void frecord_init(TQItem*){};

    static unsigned long discretevent_send_;
    static unsigned long discretevent_deliver_;
};

class NetCon: public DiscreteEvent {
  public:
    NetCon(PreSyn* src, Object* target);
    virtual ~NetCon();
    virtual void send(double sendtime, NetCvode*, NrnThread*);
    virtual void deliver(double, NetCvode*, NrnThread*);
    virtual void pr(const char*, double t, NetCvode*);
    virtual int pgvts_op(int& i) {
        i = 1;
        return 2;
    }
    virtual void pgvts_deliver(double t, NetCvode*);
    virtual NrnThread* thread();

    virtual int type() {
        return NetConType;
    }
    virtual DiscreteEvent* savestate_save();
    static DiscreteEvent* savestate_read(FILE*);

    void chksrc();
    void chktar();
    void rmsrc();
    void replace_src(PreSyn*);
    virtual void disconnect(Observable*);

    double delay_;
    PreSyn* src_;
    Point_process* target_;
    double* weight_;
    Object* obj_;
    int cnt_;
    bool active_;

    static unsigned long netcon_send_active_;
    static unsigned long netcon_send_inactive_;
    static unsigned long netcon_deliver_;
};

typedef std::unordered_map<void*, NetCon*> NetConSaveWeightTable;
typedef std::unordered_map<long, NetCon*> NetConSaveIndexTable;

class NetConSave: public DiscreteEvent {
  public:
    NetConSave(NetCon*);
    virtual ~NetConSave();
    virtual void savestate_restore(double deliverytime, NetCvode*);
    virtual void savestate_write(FILE*);
    NetCon* netcon_;

    static void invalid();
    static NetCon* weight2netcon(double*);
    static NetCon* index2netcon(long);

  private:
    static NetConSaveWeightTable* wtable_;
    static NetConSaveIndexTable* idxtable_;
};

class SelfEvent: public DiscreteEvent {
  public:
    SelfEvent();
    virtual ~SelfEvent();
    virtual void deliver(double, NetCvode*, NrnThread*);
    virtual void pr(const char*, double t, NetCvode*);
    void clear() {}  // called by sepool_->free_all
    virtual int pgvts_op(int& i) {
        i = 1;
        return 2;
    }
    virtual void pgvts_deliver(double t, NetCvode*);

    virtual int type() {
        return SelfEventType;
    }
    virtual DiscreteEvent* savestate_save();
    virtual void savestate_restore(double deliverytime, NetCvode*);
    virtual void savestate_write(FILE*);
    static DiscreteEvent* savestate_read(FILE*);
    virtual NrnThread* thread();

    double flag_;
    Point_process* target_;
    double* weight_;
    Datum* movable_;  // pointed-to Datum holds TQItem*

    static unsigned long selfevent_send_;
    static unsigned long selfevent_move_;
    static unsigned long selfevent_deliver_;
    static void savestate_free();

  private:
    void call_net_receive(NetCvode*);
    static Point_process* index2pp(int type, int oindex);
    static std::unique_ptr<SelfEventPPTable> sepp_;
};

using NetConPList = std::vector<NetCon*>;

class ConditionEvent: public DiscreteEvent {
  public:
    // condition detection factored out of PreSyn for re-use
    ConditionEvent();
    virtual ~ConditionEvent();
    virtual void check(NrnThread*, double sendtime, double teps = 0.0);
    virtual double value() {
        return -1.;
    }
    void condition(Cvode*);
    void abandon_statistics(Cvode*);
    virtual void asf_err() = 0;

    double valold_, told_;
    double valthresh_;  // go below this to reset threshold detector.
    TQItem* qthresh_;
    bool flag_;  // true when below, false when above.

    static unsigned long init_above_;
    static unsigned long send_qthresh_;
    static unsigned long abandon_;
    static unsigned long eq_abandon_;
    static unsigned long abandon_init_above_;
    static unsigned long abandon_init_below_;
    static unsigned long abandon_above_;
    static unsigned long abandon_below_;
    static unsigned long deliver_qthresh_;
};

class WatchCondition: public ConditionEvent, public HTList {
  public:
    WatchCondition(Point_process*, double (*)(Point_process*));
    virtual ~WatchCondition();
    virtual double value() {
        return (*c_)(pnt_);
    }
    virtual void send(double, NetCvode*, NrnThread*);
    virtual void deliver(double, NetCvode*, NrnThread*);
    virtual void pr(const char*, double t, NetCvode*);
    void activate(double flag);
    virtual void asf_err();
    virtual int pgvts_op(int& i) {
        i = 1;
        return 2;
    }
    virtual void pgvts_deliver(double t, NetCvode*);
    virtual NrnThread* thread();

    double nrflag_;
    Point_process* pnt_;
    double (*c_)(Point_process*);
    // For WatchCondition transfer to CoreNEURON.
    // Could be figured out from watch semantics and
    // the index where this == _watch_array[index]
    // At least this avoids a search over the _watch_array.
    int watch_index_;

    static unsigned long watch_send_;
    static unsigned long watch_deliver_;
};

class STECondition: public WatchCondition {
  public:
    STECondition(Point_process*, double (*)(Point_process*) = NULL);
    virtual ~STECondition();
    virtual void deliver(double, NetCvode*, NrnThread*);
    virtual void pgvts_deliver(double t, NetCvode*);
    virtual double value();
    virtual NrnThread* thread();

    STETransition* stet_;
};

class PreSyn: public ConditionEvent {
  public:
    PreSyn(neuron::container::data_handle<double> src, Object* osrc, Section* ssrc = nullptr);
    virtual ~PreSyn();
    virtual void send(double sendtime, NetCvode*, NrnThread*);
    virtual void deliver(double, NetCvode*, NrnThread*);
    virtual void pr(const char*, double t, NetCvode*);
    virtual void asf_err();
    virtual int pgvts_op(int& i) {
        i = 0;
        return 0;
    }
    virtual void pgvts_deliver(double t, NetCvode*);
    virtual NrnThread* thread();

    virtual int type() {
        return PreSynType;
    }
    virtual DiscreteEvent* savestate_save();
    static DiscreteEvent* savestate_read(FILE*);

    virtual double value() {
        assert(thvar_);
        return *thvar_ - threshold_;
    }

    void update(Observable*);
    void disconnect(Observable*);
    void record_stmt(const char*);
    void record_stmt(Object*);
    void record(IvocVect*, IvocVect* idvec = nullptr, int rec_id = 0);
    void record(double t);
    void init();
    double mindelay();
    void fanout(double, NetCvode*, NrnThread*);  // used by bbsavestate

    NetConPList dil_;
    double threshold_;
    double delay_;
    neuron::container::data_handle<double> thvar_{};
    Object* osrc_;
    Section* ssrc_;
    IvocVect* tvec_;
    IvocVect* idvec_;
    HocCommand* stmt_;
    NrnThread* nt_;
    hoc_Item* hi_;     // in the netcvode psl_
    hoc_Item* hi_th_;  // in the netcvode psl_th_
    long hi_index_;    // for SaveState read and write
    int use_min_delay_;
    int rec_id_;
    int output_index_;
    int gid_;
#if NRNMPI
    unsigned char localgid_;  // compressed gid for spike transfer
#endif
#if NRN_MUSIC
    void* music_port_;
#endif
#if NRNMPI
    union {  // A PreSyn cannot be both a source spike generator
        // and a receiver of off-host spikes.
        Multisend_Send* multisend_send_;
        Multisend_Send_Phase2* multisend_send_phase2_;
        int srchost_;
    } bgp;
#endif

    static unsigned long presyn_send_mindelay_;
    static unsigned long presyn_send_direct_;
    static unsigned long presyn_deliver_netcon_;
    static unsigned long presyn_deliver_direct_;
    static unsigned long presyn_deliver_ncsend_;
};

typedef std::unordered_map<long, PreSyn*> PreSynSaveIndexTable;

class PreSynSave: public DiscreteEvent {
  public:
    PreSynSave(PreSyn*);
    virtual ~PreSynSave();
    virtual void savestate_restore(double deliverytime, NetCvode*);
    virtual void savestate_write(FILE*);
    PreSyn* presyn_;
    bool have_qthresh_;

    static void invalid();
    static PreSyn* hindx2presyn(long);

  private:
    static PreSynSaveIndexTable* idxtable_;
};


class HocEvent: public DiscreteEvent {
  public:
    HocEvent();
    virtual ~HocEvent();
    virtual void pr(const char*, double t, NetCvode*);
    static HocEvent* alloc(const char* stmt, Object*, int, Object* pyact = nullptr);
    void hefree();
    void clear();  // called by hepool_->free_all
    virtual void deliver(double, NetCvode*, NrnThread*);
    virtual void allthread_handle();
    static void reclaim();
    virtual int pgvts_op(int& i) {
        i = 0;
        return 2;
    }
    virtual void pgvts_deliver(double t, NetCvode*);
    HocCommand* stmt() {
        return stmt_;
    }

    virtual int type() {
        return HocEventType;
    }
    virtual DiscreteEvent* savestate_save();
    virtual void savestate_restore(double deliverytime, NetCvode*);
    virtual void savestate_write(FILE*);
    static DiscreteEvent* savestate_read(FILE*);

    static unsigned long hocevent_send_;
    static unsigned long hocevent_deliver_;

  private:
    HocCommand* stmt_;
    Object* ppobj_;
    int reinit_;
    static HocEvent* next_del_;
    static HocEventPool* hepool_;
};

class NetParEvent: public DiscreteEvent {
  public:
    NetParEvent();
    virtual ~NetParEvent();
    virtual void send(double, NetCvode*, NrnThread*);
    virtual void deliver(double, NetCvode*, NrnThread*);
    virtual void pr(const char*, double t, NetCvode*);
    virtual int pgvts_op(int& i) {
        i = 0;
        return 4;
    }
    virtual void pgvts_deliver(double t, NetCvode*);

    virtual int type() {
        return NetParEventType;
    }
    virtual DiscreteEvent* savestate_save();
    virtual void savestate_restore(double deliverytime, NetCvode*);
    virtual void savestate_write(FILE*);
    static DiscreteEvent* savestate_read(FILE*);

  public:
    double wx_, ws_;  // exchange time and "spikes to Presyn" time
    int ithread_;     // for pr()
};

extern PreSyn* nrn_gid2outputpresyn(int gid);

#endif
