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

#define PP2NT(pp) (nrn_threads + (pp)->_tid)
#define PP2t(pp) (PP2NT(pp)->_t)
#define POINT_RECEIVE(type, tar, w, f) (*pnt_receive[type])(tar, w, f)

typedef void (*ReceiveFunc)(Point_process*, double*, double);

double NetCvode::eps_;
NetCvode* net_cvode_instance;
int cvode_active_;

void mk_netcvode() {
	if (!net_cvode_instance) {
		net_cvode_instance = new NetCvode();
	}
}

extern "C" {
extern pnt_receive_t* pnt_receive;
extern pnt_receive_t* pnt_receive_init;
extern short* nrn_artcell_qindex_;
extern bool nrn_use_localgid_;
extern void nrn_outputevent(unsigned char, double);
extern void nrn2ncs_outputevent(int netcon_output_index, double firetime);
void net_send(void**, double*, Point_process*, double, double);
void net_event(Point_process* pnt, double time);
void net_move(void**, Point_process*, double);
void artcell_net_send(void**, double*, Point_process*, double, double);
void artcell_net_move(void**, Point_process*, double);
extern void nrn_fixed_step_minimal();
extern void nrn_fixed_step_group_minimal(int);

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

void net_send(void** v, double* weight, Point_process* pnt, double td, double flag) {
	NrnThread* nt = PP2NT(pnt);
	NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
    SelfEvent* se = new SelfEvent;
	se->flag_ = flag;
	se->target_ = pnt;
	se->weight_ = weight;
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

void artcell_net_send(void** v, double* weight, Point_process* pnt, double td, double flag) {
  net_send(v, weight, pnt, td, flag);
}

void net_event(Point_process* pnt, double time) {
	PreSyn* ps = (PreSyn*)pnt->_presyn;
	if (ps) {
		if (time < PP2t(pnt)) {
			char buf[100];
			sprintf(buf, "net_event time-t = %g", time-PP2t(pnt));
			ps->pr(buf, time, net_cvode_instance);
			hoc_execerror("net_event time < t", 0);
		}
		ps->send(time, net_cvode_instance, ps->nt_);
	}
}

struct InterThreadEvent {
	DiscreteEvent* de_;	
	double t_;
};

#define ITE_SIZE 10
NetCvodeThreadData::NetCvodeThreadData() {
	tqe_ = new TQueue();
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
	MUTDESTRUCT
}

/// If the PreSyn is on a different thread than the target
/// We have to lock the buffer
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
    eps_ = 100.*DBL_EPSILON;
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


TQItem* NetCvode::bin_event(double td, DiscreteEvent* db, NrnThread* nt) {
#if PRINT_EVENT
	if (print_event_) {db->pr("send", td, this);}
#endif
	return p[nt->id].tqe_->insert(td, db);
}

TQItem* NetCvode::event(double td, DiscreteEvent* db, NrnThread* nt) {
#if PRINT_EVENT
	if (print_event_) { db->pr("send", td, this); }
#endif
	return p[nt->id].tqe_->insert(td, db);
}


void NetCvode::clear_events() {
	// SelfEvents need to be "freed". Other kinds of DiscreteEvents may
	// already have gone out of existence so the tqe_ may contain many
	// invalid item data pointers
	enqueueing_ = 0;
    for (int i=0; i < nrn_nthread; ++i) {
		NetCvodeThreadData& d = p[i];
		delete d.tqe_;
		d.tqe_ = new TQueue();
		d.unreffed_event_cnt_ = 0;
		d.immediate_deliver_ = -1e100;
		d.ite_cnt_ = 0;
		d.tqe_->nshift_ = -1;
        d.tqe_->shift_bin(nrn_threads->_t);
	}
}

void NetCvode::init_events() {
	for (int i=0; i < nrn_nthread; ++i) {
		p[i].tqe_->nshift_ = -1;
        p[i].tqe_->shift_bin(nrn_threads->_t);
	}
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
(*pnt_receive_init[type])(d->target_, d->u.weight_, 0);
				}else{
					int cnt = pnt_receive_size[type]; 
					double* wt = d->u.weight_;
					//not the first
					for (int j = 1; j < cnt; ++j) {
						wt[j] = 0.;
					}
				}
			}
		}
	}
}


bool NetCvode::deliver_event(double til, NrnThread* nt) {
	TQItem* q;
	if ((q = p[nt->id].tqe_->atomic_dq(til)) != 0) {
		DiscreteEvent* de = (DiscreteEvent*)q->data_;
		double tt = q->t_;
		delete q;
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
  net_move(v, pnt, tt);
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

void NetCvode::deliver_events(double til, NrnThread* nt) {
//printf("deliver_events til %20.15g\n", til);
  /// Enqueue any outstanding events in the interthread event buffer
  p[nt->id].enqueue(this, nt);
  while(deliver_event(til, nt)) {
    ;
  }
}

DiscreteEvent::DiscreteEvent() {}
DiscreteEvent::~DiscreteEvent() {}

NetCon::NetCon() {
	active_ = false; u.weight_ = NULL;
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


void DiscreteEvent::send(double tt, NetCvode* ns, NrnThread* nt) {
	ns->event(tt, this, nt);
}

void DiscreteEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	(void)tt; (void)ns; (void)nt;
}

void DiscreteEvent::pr(const char* s, double tt, NetCvode* ns) {
	(void)ns;
	printf("%s DiscreteEvent %.15g\n", s, tt);
}

void NetCon::send(double tt, NetCvode* ns, NrnThread* nt) {
	if (active_ && target_) {
        nrn_assert(PP2NT(target_) == nt);
	ns->bin_event(tt, this, PP2NT(target_));
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
	nt->_t = tt;

//printf("NetCon::deliver t=%g tt=%g %s\n", t, tt, pnt_name(target_));
	POINT_RECEIVE(typ, target_, u.weight_, 0);
#ifdef DEBUG
	if (errno) {
		if (nrn_errno_check(typ)) {
hoc_warning("errno set during NetCon deliver to NET_RECEIVE", (char*)0);
		}
	}
#endif
}


void PreSyn::send(double tt, NetCvode* ns, NrnThread* nt) {
	record(tt);
    {
		for (int i = nc_cnt_-1; i >= 0; --i) {
			NetCon* d = netcon_in_presyn_order_[nc_index_ + i];
			if (d->active_ && d->target_) {
				NrnThread* n = PP2NT(d->target_);
				if (nt == n) {
					ns->bin_event(tt + d->delay_, d, n);
				}else{
					ns->p[n->id].interthread_send(tt + d->delay_, d, n);
				}
			}
		}
	}
#if NRNMPI
	if (output_index_ >= 0) {
		if (nrn_use_localgid_) {
			nrn_outputevent(localgid_, tt);
		}else
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
				if (nt == n) {
					ns->bin_event(tt + d->delay_, d, n);
				}else{
					ns->p[n->id].interthread_send(tt + d->delay_, d, n);
				}
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


SelfEvent::SelfEvent() {}
SelfEvent::~SelfEvent() {}

void SelfEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	assert(nt == PP2NT(target_));
	PP2t(target_) = tt;
//printf("SelfEvent::deliver t=%g tt=%g %s\n", PP2t(target), tt, pnt_name(target_));
	call_net_receive(ns);
}


void SelfEvent::call_net_receive(NetCvode* ns) {
	POINT_RECEIVE(target_->_type, target_, weight_, flag_);
#ifdef DEBUG
	if (errno) {
		if (nrn_errno_check(target_->_type)) {
hoc_warning("errno set during SelfEvent deliver to NET_RECEIVE", (char*)0);
		}
	}
#endif
	NetCvodeThreadData& nctd = ns->p[PP2NT(target_)->id];
	--nctd.unreffed_event_cnt_;
}

void SelfEvent::pr(const char* s, double tt) {
	printf("%s", s);
	printf(" SelfEvent target=%s %.15g flag=%g\n", pnt_name(target_), tt, flag_);
}

void ncs2nrn_integrate(double tstop) {
	double ts;
        int n = (int)((tstop - nrn_threads->_t)/dt + 1e-9);
	    if (n > 3 || !nrnthread_v_transfer_) {
        nrn_fixed_step_group_minimal(n);
	    }else{
#if NRNMPI
		ts = tstop - dt;
        assert(nrn_threads->_t <= tstop);
		// It may very well be the case that we do not advance at all
        while (nrn_threads->_t <= ts) {
#else
		ts = tstop - .5*dt;
        while (nrn_threads->_t < ts) {
#endif
            nrn_fixed_step_minimal();
			if (stoprun) {break;}
		}
	    }
	// handle all the pending flag=1 self events
for (int i=0; i < nrn_nthread; ++i) { assert(nrn_threads[i]._t == nrn_threads->_t);}
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
    double tm, tsav;
    tsav = nt->_t;
    tm = nt->_t + 0.5*nt->_dt;

    deliver_events(tm, nt);

    nt->_t = tsav;
}

