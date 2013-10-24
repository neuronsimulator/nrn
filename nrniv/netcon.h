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
#define STATISTICS /**/
#endif

class PreSyn;
class PlayRecord;
class TQueue;
class TQItem;
struct NrnThread;
class NetCvode;
class SelfEventPPTable;
class IvocVect;
class BGP_DMASend;
class BGP_DMASend_Phase2;

#define DiscreteEventType 0
#define TstopEventType 1
#define NetConType 2
#define SelfEventType 3
#define PreSynType 4
#define PlayRecordEventType 6
// the above will in turn steer to proper PlayRecord type
#define NetParEventType 7

class DiscreteEvent {
public:
	DiscreteEvent();
	virtual ~DiscreteEvent();
	virtual void send(double deliverytime, NetCvode*, NrnThread*);
	virtual void deliver(double t, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual void disconnect(Observable*) {};
	virtual NrnThread* thread();

	virtual int type() { return DiscreteEventType; }

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
	virtual NrnThread* thread();

	virtual int type() { return NetConType; }

	double delay_;
	PreSyn* src_;
	Point_process* target_;
	double* weight_;
	int cnt_;
	bool active_;

	static unsigned long netcon_send_active_;
	static unsigned long netcon_send_inactive_;
	static unsigned long netcon_deliver_;
};

class SelfEvent : public DiscreteEvent {
public:
	SelfEvent();
	virtual ~SelfEvent();
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	void clear(){} // called by sepool_->free_all

	virtual int type() { return SelfEventType; }
	virtual NrnThread* thread();

	double flag_;
	Point_process* target_;
	double* weight_;
	void** movable_; // actually a TQItem**

	static unsigned long selfevent_send_;
	static unsigned long selfevent_move_;
	static unsigned long selfevent_deliver_;
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
	virtual void asf_err() = 0;

	double valold_, told_;
	double valthresh_; // go below this to reset threshold detector.
	bool flag_; // true when below, false when above.

	static unsigned long init_above_;
	static unsigned long send_qthresh_;
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
	virtual NrnThread* thread();
	
	double nrflag_;
	Point_process* pnt_;
	double(*c_)(Point_process*);

	static unsigned long watch_send_;
	static unsigned long watch_deliver_;
};

class PreSyn : public ConditionEvent {
public:
	PreSyn(double* src, Object* osrc, Section* ssrc = nil);
	virtual ~PreSyn();
	virtual void send(double sendtime, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual void asf_err();
	virtual NrnThread* thread();

	virtual int type() { return PreSynType; }
	virtual double value() { return *thvar_ - threshold_; }

	void record(IvocVect*, IvocVect* idvec = nil, int rec_id = 0);
	void record(double t);
	void init();
	double mindelay();

	NetConPList dil_;
	double threshold_;
	double delay_;
	double* thvar_;
	IvocVect* tvec_;
	IvocVect* idvec_;
	NrnThread* nt_;
	hoc_Item* hi_; // in the netcvode psl_
	hoc_Item* hi_th_; // in the netcvode psl_th_
	int use_min_delay_;
	int rec_id_;
	int output_index_;
	int gid_;
#if NRNMPI
	unsigned char localgid_; // compressed gid for spike transfer
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

class TstopEvent : public DiscreteEvent {
public:
	TstopEvent();
	virtual ~TstopEvent();
	virtual void deliver(double t, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
};

class NetParEvent : public DiscreteEvent {
public:
	NetParEvent();
	virtual ~NetParEvent();
	virtual void send(double, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);

	virtual int type() { return NetParEventType; }
public:
	double wx_, ws_; // exchange time and "spikes to Presyn" time
	int ithread_; // for pr()
};

#endif
