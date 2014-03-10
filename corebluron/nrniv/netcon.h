#ifndef netcon_h
#define netcon_h

#include "corebluron/nrniv/ivlist.h"
#include "corebluron/nrniv/htlist.h"
#include "corebluron/nrnmpi/nrnmpi.h"

#undef check
#if MAC
#define NetCon nrniv_Dinfo
#endif

#if 0
#define STATISTICS(arg) ++arg
#else
#define STATISTICS(arg) /**/
#endif

class PreSyn;
class InputPreSyn;
class TQueue;
class TQItem;
struct NrnThread;
struct Point_process;
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
#define NetParEventType 7
#define InputPreSynType 20

class DiscreteEvent {
public:
	DiscreteEvent();
	virtual ~DiscreteEvent();
	virtual void send(double deliverytime, NetCvode*, NrnThread*);
	virtual void deliver(double t, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
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
	void init(DiscreteEvent* src, Point_process* target);
	virtual ~NetCon();
	virtual void send(double sendtime, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);
	virtual NrnThread* thread();

	virtual int type() { return NetConType; }
	void rmsrc();

	double delay_;
	DiscreteEvent* src_; // either a PreSyn or an InputPreSyn or NULL
	Point_process* target_;
	union {
		double* weight_;
		int srcgid_; // only to help InputPreSyn during setup
		// before weights are read and stored. Saves on transient
		// memory requirements by avoiding storage of all group file
		// netcon_srcgid lists. ie. that info is copied into here.
	} u;
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
	PreSyn();
	PreSyn(double* src, Point_process* psrc, NrnThread*);
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
	void construct_init();
	void vecinit();
	double mindelay();

	//NetConPList dil_;
	int nc_index_; //replaces dil_, index into global NetCon** netcon_in_presyn_order_
	double threshold_;
	double delay_;
	double* thvar_;
	Point_process* pntsrc_;
	IvocVect* tvec_;
	IvocVect* idvec_;
	NrnThread* nt_;
	HTList* hi_; // in the netcvode psl_
	HTList* hi_th_; // in the netcvode psl_th_
	int nc_cnt_; // how many netcon starting at nc_index_
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

class InputPreSyn : public DiscreteEvent {
public:
	InputPreSyn();
	virtual ~InputPreSyn();
	virtual void send(double sendtime, NetCvode*, NrnThread*);
	virtual void deliver(double, NetCvode*, NrnThread*);
	virtual void pr(const char*, double t, NetCvode*);

	virtual int type() { return InputPreSynType; }

	double mindelay();

	//NetConPList dil_;
	int nc_index_; //replaces dil_, index into global NetCon** netcon_in_presyn_order_
	double delay_; // can be eliminated since only a few targets on a process
#if BGPDMA
	union { // A PreSyn cannot be both a source spike generator
		// and a receiver of off-host spikes.
		BGP_DMASend* dma_send_;
		BGP_DMASend_Phase2* dma_send_phase2_;
		int srchost_;
	} bgp;
#endif
	int nc_cnt_; // how many netcon starting at nc_index_
	int use_min_delay_;
	int gid_;

#if 0
	static unsigned long presyn_send_mindelay_;
	static unsigned long presyn_send_direct_;
	static unsigned long presyn_deliver_netcon_;
	static unsigned long presyn_deliver_direct_;
	static unsigned long presyn_deliver_ncsend_;
#endif
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
