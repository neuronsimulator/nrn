#ifndef netcon_h
#define netcon_h

#undef check
#if MAC
#define NetCon nrniv_Dinfo
#endif

#include <InterViews/observe.h>
#include <OS/list.h>
#include "htlist.h"
#include "nrnneosm.h"
#include "nrnmpi.h"

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
class HocEventPool;
class HocCommand;
class SelfEventPPTable;
class NetConSaveWeightTable;
class NetConSaveIndexTable;
class PreSynSaveIndexTable;
class STETransition;
class IvocVect;
class BGP_DMASend;
class BGP_DMASend_Phase2;

#define DiscreteEventType 0
#define TstopEventType 1
#define NetConType 2
#define SelfEventType 3
#define PreSynType 4
#define HocEventType 5
#define PlayRecordEventType 6
// the above will in turn steer to proper PlayRecord type
#define NetParEventType 7

#if DISCRETE_EVENT_OBSERVER
class DiscreteEvent : public Observer {
#else
class DiscreteEvent {
#endif
public:
	DiscreteEvent();
	virtual ~DiscreteEvent();
	virtual void send(double deliverytime, NetCvode*, NrnThread*);
	virtual void deliver(double t, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual void disconnect(Observable*) {};
	virtual int pgvts_op(int& i) { i = 0; return 2; }
	virtual void pgvts_deliver(double t, NetCvode*);
	virtual NrnThread* thread();

	virtual int type() { return DiscreteEventType; }
	virtual DiscreteEvent* savestate_save();
	virtual void savestate_restore(double deliverytime, NetCvode*);
	virtual void savestate_write(FILE*);
	static DiscreteEvent* savestate_read(FILE*);

	// actions performed over each item in the event queue.
	virtual void frecord_init(TQItem*) {};

	static unsigned long discretevent_send_;
	static unsigned long discretevent_deliver_;
};

class NetCon : public DiscreteEvent {
public:
	NetCon();
	NetCon(PreSyn* src, Object* target);
	virtual ~NetCon();
	virtual void send(double sendtime, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual int pgvts_op(int& i) { i = 1; return 2; }
	virtual void pgvts_deliver(double t, NetCvode*);
	virtual NrnThread* thread();

	virtual int type() { return NetConType; }
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
class NetConSave : public DiscreteEvent {
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

class SelfEvent : public DiscreteEvent {
public:
	SelfEvent();
	virtual ~SelfEvent();
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	void clear(){} // called by sepool_->free_all
	virtual int pgvts_op(int& i) { i = 1; return 2; }
	virtual void pgvts_deliver(double t, NetCvode*);

	virtual int type() { return SelfEventType; }
	virtual DiscreteEvent* savestate_save();
	virtual void savestate_restore(double deliverytime, NetCvode*);
	virtual void savestate_write(FILE*);
	static DiscreteEvent* savestate_read(FILE*);
	virtual NrnThread* thread();

	double flag_;
	Point_process* target_;
	double* weight_;
	void** movable_; // actually a TQItem**

	static unsigned long selfevent_send_;
	static unsigned long selfevent_move_;
	static unsigned long selfevent_deliver_;
	static void savestate_free();
private:
	void call_net_receive(NetCvode*);
	static Point_process* index2pp(int type, int oindex);
	static SelfEventPPTable* sepp_;
};

declarePtrList(NetConPList, NetCon)

class ConditionEvent : public DiscreteEvent {
public:
	// condition detection factored out of PreSyn for re-use
	ConditionEvent();
	virtual ~ConditionEvent();
	virtual void check(NrnThread*, double sendtime, double teps = 0.0);
	virtual double value() { return -1.; }
	void condition(Cvode*);
	void abandon_statistics(Cvode*);
	virtual void asf_err() = 0;

	double valold_, told_;
	double valthresh_; // go below this to reset threshold detector.
	TQItem* qthresh_;
	bool flag_; // true when below, false when above.

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

class WatchCondition : public ConditionEvent, public HTList {
public:
	WatchCondition(Point_process*, double(*)(Point_process*));
	virtual ~WatchCondition();
	virtual double value() { return (*c_)(pnt_); }
	virtual void send(double, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	void activate(double flag);
	virtual void asf_err();
	virtual int pgvts_op(int& i) { i = 1; return 2; }
	virtual void pgvts_deliver(double t, NetCvode*);
	virtual NrnThread* thread();
	
	double nrflag_;
	Point_process* pnt_;
	double(*c_)(Point_process*);

	static unsigned long watch_send_;
	static unsigned long watch_deliver_;
};

class STECondition : public WatchCondition {
public:
	STECondition(Point_process*, double(*)(Point_process*) = NULL);
	virtual ~STECondition();
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pgvts_deliver(double t, NetCvode*);
	virtual double value();
	virtual NrnThread* thread();

	STETransition* stet_;
};

class PreSyn : public ConditionEvent {
public:
	PreSyn(double* src, Object* osrc, Section* ssrc = nil);
	virtual ~PreSyn();
	virtual void send(double sendtime, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual void asf_err();
	virtual int pgvts_op(int& i) { i = 0; return 0; }
	virtual void pgvts_deliver(double t, NetCvode*);
	virtual NrnThread* thread();

	virtual int type() { return PreSynType; }
	virtual DiscreteEvent* savestate_save();
	static DiscreteEvent* savestate_read(FILE*);

	virtual double value() { return *thvar_ - threshold_; }

	void update(Observable*);
	void disconnect(Observable*);
	void update_ptr(double*);
	void record_stmt(const char*);
	void record_stmt(Object*);
	void record(IvocVect*, IvocVect* idvec = nil, int rec_id = 0);
	void record(double t);
	void init();
	double mindelay();

	NetConPList dil_;
	double threshold_;
	double delay_;
	double* thvar_;
	Object* osrc_;
	Section* ssrc_;
	IvocVect* tvec_;
	IvocVect* idvec_;
	HocCommand* stmt_;
	NrnThread* nt_;
	hoc_Item* hi_; // in the netcvode psl_
	hoc_Item* hi_th_; // in the netcvode psl_th_
	long hi_index_; // for SaveState read and write
	int use_min_delay_;
	int rec_id_;
	int output_index_;
	int gid_;
#if NRNMPI
	unsigned char localgid_; // compressed gid for spike transfer
#endif
#if NRN_MUSIC
	void* music_port_;
#endif
#if BGPDMA
	union { // A PreSyn cannot be both a source spike generator
		// and a receiver of off-host spikes.
		BGP_DMASend* dma_send_;
		BGP_DMASend_Phase2* dma_send_phase2_;
		int srchost_;
	} bgp;
#endif

	static unsigned long presyn_send_mindelay_;
	static unsigned long presyn_send_direct_;
	static unsigned long presyn_deliver_netcon_;
	static unsigned long presyn_deliver_direct_;
	static unsigned long presyn_deliver_ncsend_;
};
class PreSynSave : public DiscreteEvent {
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


class HocEvent : public DiscreteEvent {
public:
	HocEvent();
	virtual ~HocEvent();
	virtual void pr(const char*, double t, NetCvode*);
	static HocEvent* alloc(const char* stmt, Object*, int, Object* pyact=nil);
	void hefree();
	void clear(); // called by hepool_->free_all
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void allthread_handle();
	static void reclaim();
	virtual int pgvts_op(int& i) { i = 0; return 2; }
	virtual void pgvts_deliver(double t, NetCvode*);
	HocCommand* stmt() { return stmt_; }

	virtual int type() { return HocEventType; }
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

class TstopEvent : public DiscreteEvent {
public:
	TstopEvent();
	virtual ~TstopEvent();
	virtual void deliver(double t, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual int pgvts_op(int& i) { i = 0; return 2; }
	virtual void pgvts_deliver(double t, NetCvode*);

	virtual int type() { return TstopEventType; }
	virtual DiscreteEvent* savestate_save();
	virtual void savestate_restore(double deliverytime, NetCvode*);
	virtual void savestate_write(FILE*);
	static DiscreteEvent* savestate_read(FILE*);
};

class NetParEvent : public DiscreteEvent {
public:
	NetParEvent();
	virtual ~NetParEvent();
	virtual void send(double, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual int pgvts_op(int& i) { i = 0; return 4; }
	virtual void pgvts_deliver(double t, NetCvode*);

	virtual int type() { return NetParEventType; }
	virtual DiscreteEvent* savestate_save();
	virtual void savestate_restore(double deliverytime, NetCvode*);
	virtual void savestate_write(FILE*);
	static DiscreteEvent* savestate_read(FILE*);
public:
	double wx_, ws_; // exchange time and "spikes to Presyn" time
	int ithread_; // for pr()
};

extern "C" {
extern PreSyn* nrn_gid2outputpresyn(int gid);
}

#endif
