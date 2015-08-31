/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <float.h>
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrniv/netcon.h"
#include "coreneuron/nrniv/netcvode.h"
#include "coreneuron/nrniv/ivocvect.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/output_spikes.h"
#include "coreneuron/nrniv/nrn_assert.h"

#define UNIT_ROUNDOFF DBL_EPSILON
#define PP2NT(pp) (nrn_threads + (pp)->_tid)
#define PP2t(pp) (PP2NT(pp)->_t)
#define POINT_RECEIVE(type, tar, w, f) (*pnt_receive[type])(tar, w, f)
#define nt_t nrn_threads->_t

typedef void (*ReceiveFunc)(Point_process*, double*, double);

declarePool(SelfEventPool, SelfEvent)
implementPool(SelfEventPool, SelfEvent)

double NetCvode::eps_;
NetCvode* net_cvode_instance;
int cvode_active_;
int nrn_use_selfqueue_;
#if BBTQ == 5
bool nrn_use_bin_queue_;
#endif

void mk_netcvode() {
	if (!net_cvode_instance) {
		net_cvode_instance = new NetCvode();
	}
}

static DiscreteEvent* tstop_event_;

extern "C" {
extern int nrn_modeltype();
extern pnt_receive_t* pnt_receive;
extern pnt_receive_t* pnt_receive_init;
extern short* nrn_artcell_qindex_;
extern bool nrn_use_localgid_;
extern void nrn_outputevent(unsigned char, double);
extern void nrn2ncs_outputevent(int netcon_output_index, double firetime);
void net_send(void**, int, Point_process*, double, double);
void net_event(Point_process* pnt, double time);
void net_move(void**, Point_process*, double);
void artcell_net_send(void**, int, Point_process*, double, double);
void artcell_net_move(void**, Point_process*, double);
extern void nrn_fixed_step();
extern void nrn_fixed_step_group(int);

#ifdef DEBUG
//temporary 
static int nrn_errno_check(int type) 
{ 
  printf("nrn_errno_check() was called on pid %d: errno=%d type=%d\n", nrnmpi_myid, errno, type);
//  assert(0);
  type = 0; 
  return 1;
}
#endif

}

static void all_pending_selfqueue(double tt);
static void* pending_selfqueue(NrnThread*);

void net_send(void** v, int weight_index_, Point_process* pnt, double td, double flag) {
	NrnThread* nt = PP2NT(pnt);
	NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
	SelfEvent* se = p.sepool_->alloc();
	se->flag_ = flag;
	se->target_ = pnt;
	se->weight_index_ = weight_index_;
	se->movable_ = v; // needed for SaveState
	assert(net_cvode_instance);
	++p.unreffed_event_cnt_;
	if (td < nt->_t) {
		char buf[100];
		sprintf(buf, "net_send td-t = %g", td - nt->_t);
		se->pr(buf, td);
		abort();
		hoc_execerror("net_send delay < 0", 0);
	}
	TQItem* q;
	q = net_cvode_instance->event(td, se, nt);
	if (flag == 1.0) {
		*v = (void*)q;
	}
//printf("net_send %g %s %g %p\n", td, pnt_name(pnt), flag, *v);
}

void artcell_net_send(void** v, int weight_index_, Point_process* pnt, double td, double flag) {
    if (nrn_use_selfqueue_ && flag == 1.0) {
    NrnThread* nt = PP2NT(pnt);
	NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
	SelfEvent* se = p.sepool_->alloc();
	se->flag_ = flag;
	se->target_ = pnt;
	se->weight_index_ = weight_index_;
	se->movable_ = v; // needed for SaveState
	assert(net_cvode_instance);
	++p.unreffed_event_cnt_;
	if (td < nt->_t) {
		char buf[100];
		sprintf(buf, "net_send td-t = %g", td - nt->_t);
		se->pr(buf, td);
		hoc_execerror("net_send delay < 0", 0);
	}
	TQItem* q;
	q = p.selfqueue_->insert(se);
	q->t_ = td;
	*v = (void*)q;
//printf("artcell_net_send %g %s %g %p\n", td, pnt_name(pnt), flag, v);
	if (q->t_ < p.immediate_deliver_) {
//printf("artcell_net_send_  %s immediate %g %g %g\n", pnt_name(pnt), nt->_t, q->t_, p.immediate_deliver_);
		SelfEvent* se2 = (SelfEvent*)q->data_;
		p.selfqueue_->remove(q);
		se2->deliver(td, net_cvode_instance, nt);
	}
    }else{
	net_send(v, weight_index_, pnt, td, flag);
    }
}

void net_event(Point_process* pnt, double time) {
	NrnThread* nt = PP2NT(pnt);
	PreSyn* ps = nt->presyns + nt->pnt2presyn_ix[pnttype2presyn[pnt->_type]][pnt->_i_instance];
	if (ps) {
		if (time < nt->_t) {
			char buf[100];
			sprintf(buf, "net_event time-t = %g", time - nt->_t);
			ps->pr(buf, time, net_cvode_instance);
			hoc_execerror("net_event time < t", 0);
		}
		ps->send(time, net_cvode_instance, nt);
	}
}

struct InterThreadEvent {
	DiscreteEvent* de_;	
	double t_;
};

#define ITE_SIZE 10
NetCvodeThreadData::NetCvodeThreadData() {
	tpool_ = new TQItemPool(1000, 1);
	// tqe_ accessed only by thread i so no locking
	tqe_ = new TQueue(tpool_, 0);
	sepool_ = new SelfEventPool(1000,1);
	selfqueue_ = nil;
	ite_size_ = ITE_SIZE;
	ite_cnt_ = 0;
	unreffed_event_cnt_ = 0;
	immediate_deliver_ = -1e100;
	inter_thread_events_ = new InterThreadEvent[ite_size_];
	MUTCONSTRUCT(1)
}

NetCvodeThreadData::~NetCvodeThreadData() {
	delete [] inter_thread_events_;
	delete tqe_;
	delete tpool_;
	if (selfqueue_) {
		selfqueue_->remove_all();
		delete selfqueue_;
	}
	delete sepool_;
	MUTDESTRUCT
}

void NetCvodeThreadData::interthread_send(double td, DiscreteEvent* db, NrnThread* nt) {
	//bin_event(td, db, nt);
	(void)nt; // avoid unused warning
	
	MUTLOCK
	if(ite_cnt_ >= ite_size_) {
		ite_size_ *= 2;
		InterThreadEvent* in = new InterThreadEvent[ite_size_];
		for (int i=0; i < ite_cnt_; ++i) {
			in[i].de_ = inter_thread_events_[i].de_;
			in[i].t_ = inter_thread_events_[i].t_;
		}
		delete [] inter_thread_events_;
		inter_thread_events_ = in;
	}
	InterThreadEvent& ite = inter_thread_events_[ite_cnt_++];
	ite.de_ = db;
	ite.t_ = td;

	/* this is race condition for pthread implementation.
	 * we are not using cvode in coreneuron and hence 
	 * it's safe to comment out following lines. Remember 
	 * some locks are per thread and hence not safe to 
	 * lock global variables 
	 */
	//int& b = net_cvode_instance->enqueueing_;
	//if (!b) { b = 1; }
	MUTUNLOCK
}

void NetCvodeThreadData::enqueue(NetCvode* nc, NrnThread* nt) {
	int i;
	MUTLOCK
	for (i = 0; i < ite_cnt_; ++i) {
		InterThreadEvent& ite = inter_thread_events_[i];
		nc->bin_event(ite.t_, ite.de_, nt);
	}
	ite_cnt_ = 0;
	MUTUNLOCK
}

NetCvode::NetCvode(void) {
	tstop_event_ = new TstopEvent();
	eps_ = 100.*UNIT_ROUNDOFF;
	print_event_ = 0;
	pcnt_ = 0;
	p = nil;
	p_construct(1);
	// eventually these should not have to be thread safe
	// for parallel network simulations hardly any presyns have
	// a threshold and it can be very inefficient to check the entire
	// presyn list for thresholds during the fixed step method.
	// So keep a threshold list.
}

NetCvode::~NetCvode() {
	if (net_cvode_instance == (NetCvode*)this) {
		net_cvode_instance = nil;
	}	
	p_construct(0);
}


void nrn_p_construct() {
	net_cvode_instance->p_construct(nrn_nthread);
}


void NetCvode::p_construct(int n) {
	int i;
	if (pcnt_ != n) {
		if (p) {
			delete [] p;
			p = nil;
		}
		if (n > 0) {
			p = new NetCvodeThreadData[n];
		}else{
			p = nil;
		}
		pcnt_ = n;
	}
	for (i=0; i < n; ++i) {
		p[i].unreffed_event_cnt_ = 0;
	}
}


#if BBTQ == 5
TQItem* NetCvode::bin_event(double td, DiscreteEvent* db, NrnThread* nt) {
    if (nrn_use_bin_queue_) {
#if PRINT_EVENT
	if (print_event_) {db->pr("binq send", td, this);}
#endif
	return p[nt->id].tqe_->enqueue_bin(td, db);
    }else{
#if PRINT_EVENT
	if (print_event_) {db->pr("send", td, this);}
#endif
	return p[nt->id].tqe_->insert(td, db);
    }
}
#else
#define bin_event event
#endif

TQItem* NetCvode::event(double td, DiscreteEvent* db, NrnThread* nt) {
#if PRINT_EVENT
	if (print_event_) { db->pr("send", td, this); }
#endif
	return p[nt->id].tqe_->insert(td, db);
}


void NetCvode::tstop_event(double tt) {
	if (tt - nt_t < 0) { return; }
	NrnThread* nt;
	FOR_THREADS(nt) {
		event(tt, tstop_event_, nt);
	}
}
void NetCvode::clear_events() {
	// SelfEvents need to be "freed". Other kinds of DiscreteEvents may
	// already have gone out of existence so the tqe_ may contain many
	// invalid item data pointers
	enqueueing_ = 0;
    for (int i=0; i < nrn_nthread; ++i) {
		NetCvodeThreadData& d = p[i];
		delete d.tqe_;
		d.tqe_ = new TQueue(p[i].tpool_);
		d.unreffed_event_cnt_ = 0;
		d.sepool_->free_all();
		d.immediate_deliver_ = -1e100;
		d.ite_cnt_ = 0;
		if (nrn_use_selfqueue_) {
			if (!d.selfqueue_) {
				d.selfqueue_ = new SelfQueue(d.tpool_, 0);
			}else{
				d.selfqueue_->remove_all();
			}
		}
#if BBTQ == 5
		d.tqe_->nshift_ = -1;
		d.tqe_->shift_bin(nt_t);
#endif
	}
}

void NetCvode::init_events() {
#if BBTQ == 5
	for (int i=0; i < nrn_nthread; ++i) {
		p[i].tqe_->nshift_ = -1;
		p[i].tqe_->shift_bin(nt_t);
	}
#endif
	for (int tid=0; tid < nrn_nthread; ++tid) {// can be done in parallel
		NrnThread* nt = nrn_threads + tid;

		for (int ipre = 0; ipre < nt->n_presyn; ++ ipre) {
			PreSyn* ps = nt->presyns + ipre;
			ps->flag_ = false;
		}

		for (int inetc = 0; inetc < nt->n_netcon; ++inetc) {
			NetCon* d = nt->netcons + inetc;
			if (d->target_) {
				int type = d->target_->_type;
				if (pnt_receive_init[type]) {
(*pnt_receive_init[type])(d->target_, d->u.weight_index_, 0);
				}else{
					int cnt = pnt_receive_size[type]; 
					double* wt = nt->weights + d->u.weight_index_;
					//not the first
					for (int j = 1; j < cnt; ++j) {
						wt[j] = 0.;
					}
				}
			}
		}
	}
}

void NetCvode::deliver_least_event(NrnThread* nt) {
	TQItem* q = p[nt->id].tqe_->least();
	DiscreteEvent* de = (DiscreteEvent*)q->data_;
	double tt = q->t_;
	p[nt->id].tqe_->remove(q);
#if PRINT_EVENT
	if (print_event_) { de->pr("deliver", tt, this); }
#endif
	de->deliver(tt, this, nt);
}

bool NetCvode::deliver_event(double til, NrnThread* nt) {
	TQItem* q;
	if ((q = p[nt->id].tqe_->atomic_dq(til)) != 0) {
		DiscreteEvent* de = (DiscreteEvent*)q->data_;
		double tt = q->t_;
		p[nt->id].tqe_->release(q);
#if PRINT_EVENT
		if (print_event_) { de->pr("deliver", tt, this); }
#endif
		de->deliver(tt, this, nt);
		return true;
	}else{
		return false;
	}
}

void net_move(void** v, Point_process* pnt, double tt) {
	if (!(*v)) {
		hoc_execerror( "No event with flag=1 for net_move in ", memb_func[pnt->_type].sym);
	}
	TQItem* q = (TQItem*)(*v);
//printf("net_move tt=%g %s *v=%p\n", tt, memb_func[pnt->_type].sym, *v);
	if (tt < PP2t(pnt)) {
		assert(0);
	}
	net_cvode_instance->move_event(q, tt, PP2NT(pnt));
}

void artcell_net_move(void** v, Point_process* pnt, double tt) {
    if (nrn_use_selfqueue_) {
	if (!(*v)) {
		hoc_execerror( "No event with flag=1 for net_move in ", pnt_name(pnt));
	}
	NrnThread* nt = PP2NT(pnt);
	NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
	TQItem* q = (TQItem*)(*v);
//printf("artcell_net_move t=%g qt_=%g tt=%g %s *v=%p\n", nt->_t, q->t_, tt, pnt_name(pnt), *v);
	if (tt < nt->_t) {
		SelfEvent* se = (SelfEvent*)q->data_;
		char buf[100];
		sprintf(buf, "artcell_net_move tt-nt_t = %g", tt - nt->_t);
		se->pr(buf, tt);
		hoc_execerror("net_move tt < t", 0);
	}
	q->t_ = tt;
	if (tt < p.immediate_deliver_) {
//printf("artcell_net_move_ %s immediate %g %g %g\n", pnt_name(pnt), PP2t(pnt), tt, p.immediate_deliver_);
		SelfEvent* se = (SelfEvent*)q->data_;
		se->deliver(tt, net_cvode_instance, nt);
	}
    }else{
	net_move(v, pnt, tt);
    }
}

void NetCvode::move_event(TQItem* q, double tnew, NrnThread* nt) {
	int tid = nt->id;
#if PRINT_EVENT
if (print_event_) {
	SelfEvent* se = (SelfEvent*)q->data_;
printf("NetCvode::move_event self event target %s t=%g, old=%g new=%g\n", memb_func[se->target_->_type].sym, nt->_t, q->t_, tnew);
}
#endif
	p[tid].tqe_->move(q, tnew);
}

void NetCvode::remove_event(TQItem* q, int tid) {
	p[tid].tqe_->remove(q);
}

void NetCvode::deliver_events(double til, NrnThread* nt) {
//printf("deliver_events til %20.15g\n", til);
	p[nt->id].enqueue(this, nt);
	while(deliver_event(til, nt)) {
		;
	}
}

DiscreteEvent::DiscreteEvent() {}
DiscreteEvent::~DiscreteEvent() {}

NetCon::NetCon() {
	active_ = false; u.weight_index_ = 0;
	src_  = NULL; target_ = NULL;
	delay_ = 1.0;
}

NetCon::~NetCon() {
}


PreSyn::PreSyn() {
    nc_index_ = 0;
	nc_cnt_ = 0;
	flag_ = false;
	thvar_ = NULL;
	pntsrc_ = NULL;
	threshold_ = 10.;
	gid_ = -1;
	nt_ = NULL;
	localgid_ = 0;
	output_index_ = 0;
}

InputPreSyn::InputPreSyn() {
	nc_index_ = -1;
	nc_cnt_ = 0;
	gid_ = -1;
}

PreSyn::~PreSyn() {
//	printf("~PreSyn %p\n", this);
	nrn_cleanup_presyn(this);
	if (thvar_ || pntsrc_) {
		if (!thvar_) {
			if (pntsrc_) {
				pntsrc_ = nil;
			}
		}
	}
}

InputPreSyn::~InputPreSyn() {
//	printf("~InputPreSyn %p\n", this);
	nrn_cleanup_presyn(this);
}


void PreSyn::record(double tt) {
	spikevec_lock();
	assert(spikevec_size < spikevec_buffer_size);
	spikevec_gid[spikevec_size] = gid_;
	spikevec_time[spikevec_size] = tt;
	++spikevec_size;
	spikevec_unlock();
}

void ConditionEvent::check(NrnThread* nt, double tt, double teps) {
	if (value() > 0.0) {
		if (flag_ == false) {
			flag_ = true;
			send(tt + teps, net_cvode_instance, nt);
		}
	}else{
		flag_ = false;
	}
}

ConditionEvent::ConditionEvent() {}
ConditionEvent::~ConditionEvent() {}

WatchCondition::WatchCondition(Point_process* pnt, double(*c)(Point_process*))
{
	pnt_ = pnt;
	c_ = c;
}

WatchCondition::~WatchCondition() {
}

void WatchCondition::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	(void)ns; (void)nt; // avoid unused arg warning
	int typ = pnt_->_type;
	PP2t(pnt_) = tt;
	POINT_RECEIVE(typ, pnt_, 0, nrflag_);
#ifdef DEBUG
	if (errno) {
		if (nrn_errno_check(typ)) {
hoc_warning("errno set during WatchCondition deliver to NET_RECEIVE", (char*)0);
		}
	}
#endif
}

NrnThread* WatchCondition::thread() { return PP2NT(pnt_); }

void WatchCondition::pr(const char* s, double tt) {
	printf("%s", s);
	printf(" WatchCondition %s %.15g flag=%g\n", pnt_name(pnt_), tt, nrflag_);
}


void DiscreteEvent::send(double tt, NetCvode* ns, NrnThread* nt) {
	ns->event(tt, this, nt);
}

void DiscreteEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	(void)tt; (void)ns; (void)nt;
}

NrnThread* DiscreteEvent::thread() { return nrn_threads; }

void DiscreteEvent::pr(const char* s, double tt, NetCvode* ns) {
	(void)ns;
	printf("%s DiscreteEvent %.15g\n", s, tt);
}

void NetCon::send(double tt, NetCvode* ns, NrnThread* nt) {
	if (active_ && target_) {
        nrn_assert(PP2NT(target_) == nt);
#if BBTQ == 5
		ns->bin_event(tt, this, PP2NT(target_));
#else
		ns->event(tt, this, PP2NT(target_));
#endif
    }
}
	
void NetCon::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    (void)ns;
    assert(target_);
    if (PP2NT(target_) != nt) {
        printf("NetCon::deliver nt=%d target=%d\n", nt->id, PP2NT(target_)->id);
    }
	assert(PP2NT(target_) == nt);
	int typ = target_->_type;
	if (nrn_use_selfqueue_ && nrn_is_artificial_[typ]) {
		assert(0);
		// need to figure out what to do
	}
	nt->_t = tt;

//printf("NetCon::deliver t=%g tt=%g %s\n", t, tt, pnt_name(target_));
	POINT_RECEIVE(typ, target_, u.weight_index_, 0);
#ifdef DEBUG
	if (errno) {
		if (nrn_errno_check(typ)) {
hoc_warning("errno set during NetCon deliver to NET_RECEIVE", (char*)0);
		}
	}
#endif
}

NrnThread* NetCon::thread() { return PP2NT(target_); }

void PreSyn::send(double tt, NetCvode* ns, NrnThread* nt) {
	record(tt);
    {
		for (int i = nc_cnt_-1; i >= 0; --i) {
			NetCon* d = netcon_in_presyn_order_[nc_index_ + i];
			if (d->active_ && d->target_) {
				NrnThread* n = PP2NT(d->target_);
#if BBTQ == 5
				if (nt == n) {
					ns->bin_event(tt + d->delay_, d, n);
				}else{
					ns->p[n->id].interthread_send(tt + d->delay_, d, n);
				}
#else
				ns->event(tt + d->delay_, d, PP2NT(d->target_));
#endif
			}
		}
	}
#if NRNMPI
	if (output_index_ >= 0) {

#if NRNMPI
		if (nrn_use_localgid_) {
			nrn_outputevent(localgid_, tt);
		}else
#endif //NRNMPI
		nrn2ncs_outputevent(output_index_, tt);
	}
#endif //NRNMPI
}
	
void InputPreSyn::send(double tt, NetCvode* ns, NrnThread* nt) {
    {
		for (int i = nc_cnt_-1; i >= 0; --i) {
			NetCon* d = netcon_in_presyn_order_[nc_index_ + i];
			if (d->active_ && d->target_) {
				NrnThread* n = PP2NT(d->target_);
#if BBTQ == 5
				if (nt == n) {
					ns->bin_event(tt + d->delay_, d, n);
				}else{
					ns->p[n->id].interthread_send(tt + d->delay_, d, n);
				}
#else
				ns->event(tt + d->delay_, d, PP2NT(d->target_));
#endif
			}
		}
	}
}
	
void PreSyn::deliver(double, NetCvode*, NrnThread*) {
	assert(0); // no PreSyn delay.
}

void InputPreSyn::deliver(double, NetCvode*, NrnThread*) {
	assert(0); // no InputPreSyn delay.
}

NrnThread* PreSyn::thread() { return nt_; }

SelfEvent::SelfEvent() {}
SelfEvent::~SelfEvent() {}

void SelfEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	int typ = target_->_type;
	assert(nt == PP2NT(target_));
	if (nrn_use_selfqueue_ && nrn_is_artificial_[typ]) { // handle possible earlier flag=1 self event
		if (flag_ == 1.0) { *movable_ = 0; }
		TQItem* q;
		while ((q = (TQItem*)(*movable_)) != 0 && q->t_ <= tt) {
//printf("handle earlier %g selfqueue event from within %g SelfEvent::deliver\n", q->t_, tt);
			double t1 = q->t_;
			SelfEvent* se = (SelfEvent*)ns->p[nt->id].selfqueue_->remove(q);
			PP2t(target_) = t1;
			se->call_net_receive(ns);
		}
	}
	PP2t(target_) = tt;
//printf("SelfEvent::deliver t=%g tt=%g %s\n", PP2t(target), tt, pnt_name(target_));
	call_net_receive(ns);
}

NrnThread* SelfEvent::thread() { return PP2NT(target_); }

void SelfEvent::call_net_receive(NetCvode* ns) {
	POINT_RECEIVE(target_->_type, target_, weight_index_, flag_);
#ifdef DEBUG
	if (errno) {
		if (nrn_errno_check(target_->_type)) {
hoc_warning("errno set during SelfEvent deliver to NET_RECEIVE", (char*)0);
		}
	}
#endif
	NetCvodeThreadData& nctd = ns->p[PP2NT(target_)->id];
	--nctd.unreffed_event_cnt_;
	nctd.sepool_->hpfree(this);
}
	
void SelfEvent::pr(const char* s, double tt) {
	printf("%s", s);
	printf(" SelfEvent target=%s %.15g flag=%g\n", pnt_name(target_), tt, flag_);
}

// For localstep makes sure all cvode instances are at this time and 
// makes sure the continuous record records values at this time.
TstopEvent::TstopEvent() {}
TstopEvent::~TstopEvent() {}

void TstopEvent::deliver() {
}

void TstopEvent::pr(const char* s, double tt) {
	printf("%s TstopEvent %.15g\n", s, tt);
}

void ncs2nrn_integrate(double tstop) {
	double ts;
	    int n = (int)((tstop - nt_t)/dt + 1e-9);
	    if (n > 3) {
		nrn_fixed_step_group(n);
	    }else{
#if NRNMPI
		ts = tstop - dt;
		assert(nt_t <= tstop);
		// It may very well be the case that we do not advance at all
		while (nt_t <= ts) {
#else
		ts = tstop - .5*dt;
		while (nt_t < ts) {
#endif
			nrn_fixed_step();
			if (stoprun) {break;}
		}
	    }
	// handle all the pending flag=1 self events
for (int i=0; i < nrn_nthread; ++i) { assert(nrn_threads[i]._t == nt_t);}
	all_pending_selfqueue(nt_t);
}

TQueue* NetCvode::event_queue(NrnThread* nt) {
	return p[nt->id].tqe_;
}

static double pending_selfqueue_deliver_;
static void* pending_selfqueue(NrnThread* nt) {
	nrn_pending_selfqueue(pending_selfqueue_deliver_, nt);
	return 0;
}

void nrn_pending_selfqueue(double tt, NrnThread* nt) {
	NetCvodeThreadData& nctd = net_cvode_instance->p[nt->id];
	double ts = nt->_t;
//	net_cvode_instance->deliver_events(nctd.immediate_deliver_, nt);
	SelfQueue* sq = nctd.selfqueue_;
	TQItem* q1, *q2;
	nctd.immediate_deliver_ = tt;
	for (q1 = sq->first(); q1; q1 = q2) {
		if (q1->t_ <= tt) {
			SelfEvent* se = (SelfEvent*)q1->data_;
//printf("ncs2nrn_integrate %g SelfEvent for %s at %g\n", tstop, pnt_name(se->target_), q1->t_);
			se->deliver(q1->t_, net_cvode_instance, nt);
			// could it add another self-event?, check before removal
			q2 = sq->next(q1);
			sq->remove(q1);
		}else{
			q2 = sq->next(q1);
		}
	}
	assert(nctd.tqe_->least_t() >= tt);
	nt->_t = ts;
	nctd.immediate_deliver_ = -1e100;
}

// only the main thread can calls this
static void all_pending_selfqueue(double tt) {
	if (nrn_use_selfqueue_) {
		nrn_wait_for_threads();
//for (int i=0; i < nrn_nthread; ++i) { assert(nrn_threads[i]._t == nt_t);}
		pending_selfqueue_deliver_ = tt;
		nrn_multithread_job(pending_selfqueue);
	}
}

// factored this out from deliver_net_events so we can
// stay in the cache
void NetCvode::check_thresh(NrnThread* nt) { // for default method
	int i;

	for (i=0; i < nt->ncell; ++i) {
		PreSyn* ps = nt->presyns + i;
		assert(ps->thvar_);
		ps->check(nt, nt->_t, 1e-10);
	}
}

void NetCvode::deliver_net_events(NrnThread* nt) { // for default method
	TQItem* q;
	double tm, tsav;
	int tid = nt->id;
	tsav = nt->_t;
	tm = nt->_t + 0.5*nt->_dt;
#if BBTQ == 5
    tryagain:
    // one of the events on the main queue may be a NetParEvent
    // which due to dt round off error can result in an event
    // placed on the bin queue to be delivered now, which
    // can put 0 delay events on to the main queue. So loop til
    // no events. The alternative would be to deliver an idt=0 event
    // immediately but that would very much change the sequence
    // with respect to what is being done here and it is unclear
    // how to fix the value of t there. This can be a do while loop
    // but I do not want to affect the case of not using a bin queue.

	if (nrn_use_bin_queue_) {
		while ((q = p[tid].tqe_->dequeue_bin()) != 0) {
			DiscreteEvent* db = (DiscreteEvent*)q->data_;
#if PRINT_EVENT
if (print_event_) {db->pr("binq deliver", nt_t, this);}
#endif
			p[tid].tqe_->release(q);
			db->deliver(nt->_t, this, nt);
		}
//		assert(int(tm/nt->_dt)%1000 == p[tid].tqe_->nshift_);
	}
#endif

	deliver_events(tm, nt);

#if BBTQ == 5
	if (nrn_use_bin_queue_) {
		if (p[tid].tqe_->top()) { goto tryagain; }
		p[tid].tqe_->shift_bin(tm);
	}
#endif
	nt->_t = tsav;
	for (int i=0; i < net_buf_receive_cnt_; ++i) {
		(*net_buf_receive_[i])(nt);
	}
}

