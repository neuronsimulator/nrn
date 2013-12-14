#include <float.h>
#include "simcore/nrnconf.h"
#include "simcore/nrnoc/multicore.h"
#include "simcore/nrnoc/nrnoc_decl.h"
#include "simcore/nrniv/netcon.h"
#include "simcore/nrniv/netcvode.h"
#include "simcore/nrniv/ivlist.h"
#include "simcore/nrniv/ivtable.h"
#include "simcore/nrniv/ivocvect.h"
#include "simcore/nrniv/nrniv_decl.h"
#include "simcore/nrniv/output_spikes.h"

#define UNIT_ROUNDOFF DBL_EPSILON
#define PP2NT(pp) ((NrnThread*)((pp)->_vnt))
#define PP2t(pp) (PP2NT(pp)->_t)
#define POINT_RECEIVE(type, tar, w, f) (*pnt_receive[type])(tar, w, f)
#define nt_t nrn_threads->_t

typedef void (*ReceiveFunc)(Point_process*, double*, double);

declarePtrList(PreSynList, PreSyn)
implementPtrList(PreSynList, PreSyn)
declarePtrList(HTListList, HTList)
implementPtrList(HTListList, HTList)
declarePtrList(WatchList, WatchCondition)
implementPtrList(WatchList, WatchCondition)
declareTable(PreSynTable, double*, PreSyn*)
implementTable(PreSynTable, double*, PreSyn*)
declarePool(SelfEventPool, SelfEvent)
implementPool(SelfEventPool, SelfEvent)
declarePtrList(TQList, TQItem)
implementPtrList(TQList, TQItem)

implementPtrList(NetConPList, NetCon)
static NetConPList* ncs2nrn_input_;

double NetCvode::eps_;
NetCvode* net_cvode_instance;
int nrn_use_selfqueue_;
#if BBTQ == 5
bool nrn_use_bin_queue_;
#endif

void mk_netcvode() {
	if (!net_cvode_instance) {
		net_cvode_instance = new NetCvode();
	}
}

static DiscreteEvent* null_event_;
static DiscreteEvent* tstop_event_;

extern "C" {
extern int nrn_modeltype();
extern ReceiveFunc* pnt_receive;
extern ReceiveFunc* pnt_receive_init;
extern short* pnt_receive_size;
extern short* nrn_is_artificial_; // should be bool but not using that type in c
extern short* nrn_artcell_qindex_;
extern bool nrn_use_localgid_;
extern void nrn_outputevent(unsigned char, double);
extern void nrn2ncs_outputevent(int netcon_output_index, double firetime);
void net_send(void**, double*, Point_process*, double, double);
void net_event(Point_process* pnt, double time);
void net_move(void**, Point_process*, double);
void artcell_net_send(void**, double*, Point_process*, double, double);
void artcell_net_move(void**, Point_process*, double);
extern void nrn_fixed_step();
extern void nrn_fixed_step_group(int);

//temporary 
static int nrn_errno_check(int type) { assert(0); return 1;}

}

static void all_pending_selfqueue(double tt);
static void* pending_selfqueue(NrnThread*);

void net_send(void** v, double* weight, Point_process* pnt, double td, double flag) {
	STATISTICS(SelfEvent::selfevent_send_);
	NrnThread* nt = PP2NT(pnt);
	NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
	SelfEvent* se = p.sepool_->alloc();
	se->flag_ = flag;
	se->target_ = pnt;
	se->weight_ = weight;
	se->movable_ = v; // needed for SaveState
	assert(net_cvode_instance);
	++p.unreffed_event_cnt_;
	if (td < nt->_t) {
		char buf[100];
		sprintf(buf, "net_send td-t = %g", td - nt->_t);
		se->pr(buf, td, net_cvode_instance);
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
    if (nrn_use_selfqueue_ && flag == 1.0) {
	STATISTICS(SelfEvent::selfevent_send_);
	NrnThread* nt = PP2NT(pnt);
	NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
	SelfEvent* se = p.sepool_->alloc();
	se->flag_ = flag;
	se->target_ = pnt;
	se->weight_ = weight;
	se->movable_ = v; // needed for SaveState
	assert(net_cvode_instance);
	++p.unreffed_event_cnt_;
	if (td < nt->_t) {
		char buf[100];
		sprintf(buf, "net_send td-t = %g", td - nt->_t);
		se->pr(buf, td, net_cvode_instance);
		hoc_execerror("net_send delay < 0", 0);
	}
	TQItem* q;
	q = p.selfqueue_->insert(se);
	q->t_ = td;
	*v = (void*)q;
//printf("artcell_net_send %g %s %g %p\n", td, pnt_name(pnt), flag, v);
	if (q->t_ < p.immediate_deliver_) {
//printf("artcell_net_send_  %s immediate %g %g %g\n", pnt_name(pnt), nt->_t, q->t_, p.immediate_deliver_);
		SelfEvent* se = (SelfEvent*)q->data_;
		p.selfqueue_->remove(q);
		se->deliver(td, net_cvode_instance, nt);
	}
    }else{
	net_send(v, weight, pnt, td, flag);
    }
}

void net_event(Point_process* pnt, double time) {
	STATISTICS(net_event_cnt_);
	PreSyn* ps = (PreSyn*)pnt->presyn_;
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

void _nrn_watch_activate(Datum* d, double (*c)(Point_process*), int i, Point_process* pnt, int r, double flag) {
// Alex: may or may not need to figure this out. I don't thing WATCH is used
// in any mod files
#if 0
//	printf("_nrn_cond_activate %s flag=%g first return = %g\n", pnt_name(pnt), flag, c(pnt));
	if (!d->_pvoid) {
		d->_pvoid = (void*)new WatchList();
	}
	if (!d[i]._pvoid) {
		d[i]._pvoid = (void*)new WatchCondition(pnt, c);
	}
	WatchList* wl = (WatchList*)d->_pvoid;
	if (r == 0) {
		int j;
		for (j=0; j < wl->count(); ++j) {
			WatchCondition* wc1 = wl->item(j);
			wc1->Remove();
			if (wc1->qthresh_) { // is it on the queue?
				net_cvode_instance->remove_event(wc1->qthresh_, PP2NT(pnt)->id);
				wc1->qthresh_ = nil;
			}
		}
		wl->remove_all();
	}
	WatchCondition* wc = (WatchCondition*)d[i]._pvoid;
	wl->append(wc);
	wc->activate(flag);
#endif
}

void _nrn_free_watch(Datum* d, int offset, int n) {
#if 0
	int i;
	int nn = offset + n;
	if (d[offset]._pvoid) {
		WatchList* wl = (WatchList*)d[offset]._pvoid;
		delete wl;
	}
	for (i=offset+1; i < nn; ++i) {
		if (d[i]._pvoid) {
			WatchCondition* wc = (WatchCondition*)d[i]._pvoid;
			wc->Remove();
			delete wc;
		}
	}
#endif
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
	psl_thr_ = nil;
	tq_ = nil;
	ite_size_ = ITE_SIZE;
	ite_cnt_ = 0;
	unreffed_event_cnt_ = 0;
	immediate_deliver_ = -1e100;
	inter_thread_events_ = new InterThreadEvent[ite_size_];
	nlcv_ = 0;
	MUTCONSTRUCT(1)
}

NetCvodeThreadData::~NetCvodeThreadData() {
	delete [] inter_thread_events_;
	if (psl_thr_) { delete psl_thr_; }
	if (tq_) { delete tq_; }
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
	int& b = net_cvode_instance->enqueueing_;
	if (!b) { b = 1; }
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

static unsigned long deliver_cnt_, net_event_cnt_;
unsigned long DiscreteEvent::discretevent_send_;
unsigned long DiscreteEvent::discretevent_deliver_;
unsigned long NetCon::netcon_send_active_;
unsigned long NetCon::netcon_send_inactive_;
unsigned long NetCon::netcon_deliver_;
unsigned long SelfEvent::selfevent_send_;
unsigned long SelfEvent::selfevent_move_;
unsigned long SelfEvent::selfevent_deliver_;
unsigned long WatchCondition::watch_send_;
unsigned long WatchCondition::watch_deliver_;
unsigned long ConditionEvent::init_above_;
unsigned long PreSyn::presyn_send_mindelay_;
unsigned long PreSyn::presyn_send_direct_;
unsigned long PreSyn::presyn_deliver_netcon_;
unsigned long PreSyn::presyn_deliver_direct_;
unsigned long PreSyn::presyn_deliver_ncsend_;

NetCvode::NetCvode(bool single) {
	use_long_double_ = 0;
	empty_ = true; // no equations (only artificial cells).
	null_event_ = new DiscreteEvent();
	tstop_event_ = new TstopEvent();
	eps_ = 100.*UNIT_ROUNDOFF;
	print_event_ = 0;
	wl_list_ = new HTListList();
	pcnt_ = 0;
	p = nil;
	p_construct(1);
	// eventually these should not have to be thread safe
	pst_ = nil;
	pst_cnt_ = 0;
	psl_ = nil;
	// for parallel network simulations hardly any presyns have
	// a threshold and it can be very inefficient to check the entire
	// presyn list for thresholds during the fixed step method.
	// So keep a threshold list.
	structure_change_cnt_ = -1;
	fornetcon_change_cnt_ = -2;
	matrix_change_cnt_ = -1;
	vec_event_store_ = nil;
//	re_init(t);
}

NetCvode::~NetCvode() {
	if (net_cvode_instance == (NetCvode*)this) {
		net_cvode_instance = nil;
	}	
	p_construct(0);
	if (psl_) {
		HTList* q;
		for (q = psl_->First(); q != psl_->End(); q = q->Next()) {
			PreSyn* ps = (PreSyn*)q->vptr();
			for (int i = ps->nc_cnt_ - 1; i >= 0; --i) {
				NetCon* d = ps->ncl_[i];
				d->src_ = nil;
				delete d;
			}
			if (ps->ncl_) { delete [] ps->ncl_; }
			delete ps;
		}
		delete psl_;
	}
	if (pst_) {
		delete pst_;
	}
	delete wl_list_;		
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

void NetCvode::psl_append(PreSyn* ps) {
	if (!psl_) {
		psl_ = new HTList(NULL);
	}
	ps->hi_ = new HTList(ps);
	psl_->Append(ps->hi_);
}

void NetCvode::solver_prepare() {
	int i, j;
	fornetcon_prepare();
  if (nrn_modeltype() == 0) {
		delete_list();
  }else{
	assert(0); // need to understand whether a cvode instance is necessary
	//init_global();
  }
}

void NetCvode::delete_list() {
	int i, j;
	wl_list_->remove_all();
	for (i = 0; i < pcnt_; ++i) {
		NetCvodeThreadData& d = p[i];
		if (d.tq_) {
			delete d.tq_;
			d.tq_ = nil;
		}
	}
	empty_ = true;
}

struct ForNetConsInfo {
	double** argslist;
	int size;
};

void NetCvode::fornetcon_prepare() {
// need to figure out how to deal with
// &(m->pdata[j][index]._pvoid);
assert(0);
// InputPresyn are not in the psl_ list so need to check gid2in_.
#if 0
	NrnThread* nt;
	NrnThreadMembList* tml;
	if (fornetcon_change_cnt_ == structure_change_cnt) { return; }
	fornetcon_change_cnt_ = structure_change_cnt;
	if (nrn_fornetcon_cnt_ == 0) { return; }
	int i, j;
	// initialize a map from type to dparam index, -1 means no FOR_NETCONS statement
	int* t2i = new int[n_memb_func];
	for (i=0; i < n_memb_func; ++i) { t2i[i] = -1; }
	// create ForNetConsInfo in all the relevant point processes
	// and fill in the t2i map.
	for (i = 0; i < nrn_fornetcon_cnt_; ++i) {
		int index = nrn_fornetcon_index_[i];
		int type = nrn_fornetcon_type_[i];
		t2i[type] = index;
	  if (nrn_is_artificial_[type]) {
		Memb_list* m = memb_list + type;
		for (j = 0; j < m->nodecount; ++j) {
			void** v = &(m->pdata[j][index]._pvoid);
			_nrn_free_fornetcon(v);
			ForNetConsInfo* fnc = new ForNetConsInfo;
			*v = fnc;
			fnc->argslist = 0;
			fnc->size = 0;
		}
	  }else{
	    FOR_THREADS(nt) for(tml = nt->tml; tml; tml = tml->next) if (tml->index == type) {
		Memb_list* m = tml->ml;
		for (j = 0; j < m->nodecount; ++j) {
			void** v = &(m->pdata[j][index]._pvoid);
			_nrn_free_fornetcon(v);
			ForNetConsInfo* fnc = new ForNetConsInfo;
			*v = fnc;
			fnc->argslist = 0;
			fnc->size = 0;
		}
	    }
	  }
	}
	// two loops over all netcons. one to count, one to fill in argslist
	// count
	hoc_Item* q;
	if (psl_) ITERATE(q, psl_) {
		PreSyn* ps = (PreSyn*)VOIDITM(q);
		NetConPList& dil = ps->dil_;
		for (int i=0; i < dil.count(); ++i) {
			NetCon* d1 = dil.item(i);
			Point_process* pnt = d1->target_;
			if (pnt && t2i[pnt->prop->type] > -1) {
ForNetConsInfo* fnc = (ForNetConsInfo*)pnt->prop->dparam[t2i[pnt->prop->type]]._pvoid;
				assert(fnc);
				fnc->size += 1;
			}
		}
	}

	// allocate argslist space and initialize for another count
	for (i = 0; i < nrn_fornetcon_cnt_; ++i) {
		int index = nrn_fornetcon_index_[i];
		int type = nrn_fornetcon_type_[i];
	  if (nrn_is_artificial_[type]) {
		Memb_list* m = memb_list + type;
		for (j = 0; j < m->nodecount; ++j) {
			ForNetConsInfo* fnc = (ForNetConsInfo*)m->pdata[j][index]._pvoid;
			if (fnc->size > 0) {
				fnc->argslist = new double*[fnc->size];
				fnc->size = 0;
			}
		}
	  }else{
	    FOR_THREADS(nt) for(tml = nt->tml; tml; tml = tml->next) if (tml->index == nrn_fornetcon_type_[i]) {
		Memb_list* m = tml->ml;
		for (j = 0; j < m->nodecount; ++j) {
			ForNetConsInfo* fnc = (ForNetConsInfo*)m->pdata[j][index]._pvoid;
			if (fnc->size > 0) {
				fnc->argslist = new double*[fnc->size];
				fnc->size = 0;
			}
		}
	    }
	  }
	}
	// fill in argslist and count again
	if (psl_) ITERATE(q, psl_) {
		PreSyn* ps = (PreSyn*)VOIDITM(q);
		NetConPList& dil = ps->dil_;
		for (int i=0; i < dil.count(); ++i) {
			NetCon* d1 = dil.item(i);
			Point_process* pnt = d1->target_;
			if (pnt && t2i[pnt->prop->type] > -1) {
ForNetConsInfo* fnc = (ForNetConsInfo*)pnt->prop->dparam[t2i[pnt->prop->type]]._pvoid;
				fnc->argslist[fnc->size] = d1->weight_;
				fnc->size += 1;
			}
		}
	}
	delete [] t2i;
#endif
}

int _nrn_netcon_args(void* v, double*** argslist) {
	ForNetConsInfo* fnc = (ForNetConsInfo*)v;
	assert(fnc);
	*argslist = fnc->argslist;
	return fnc->size;
}

void _nrn_free_fornetcon(void** v) {
	ForNetConsInfo* fnc = (ForNetConsInfo*)(*v);
	if (fnc) {
		if (fnc->argslist) {
			delete [] fnc->argslist;
		}
		delete fnc;
		*v = nil;
	}
}	

#if BBTQ == 5
TQItem* NetCvode::bin_event(double td, DiscreteEvent* db, NrnThread* nt) {
    if (nrn_use_bin_queue_) {
#if PRINT_EVENT
	if (print_event_) {db->pr("binq send", td, this);}
	if (vec_event_store_) {
		assert(0);
		IvocVect* x = vec_event_store_;
		int n = x->capacity();
		x->resize_chunk(n+2);
		x->elem(n) = nt_t;
		x->elem(n+1) = td;
	}
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
	if (vec_event_store_) {
		IvocVect* x = vec_event_store_;
		int n = x->capacity();
		x->resize_chunk(n+2);
		x->elem(n) = nt_t;
		x->elem(n+1) = td;
	}
#endif
	return p[nt->id].tqe_->insert(td, db);
}

void NetCvode::null_event(double tt) {
	assert(0);
	NrnThread* nt = nrn_threads;
	if (tt - nt->_t < 0) { return; }
	event(tt, null_event_, nt);
}

void NetCvode::tstop_event(double tt) {
	if (tt - nt_t < 0) { return; }
	NrnThread* nt;
	FOR_THREADS(nt) {
		event(tt, tstop_event_, nt);
	}
}
void NetCvode::clear_events() {
	int i;
	deliver_cnt_ = net_event_cnt_ = 0;
	NetCon::netcon_send_active_ = 0;
	NetCon::netcon_send_inactive_ = 0;
	NetCon::netcon_deliver_ = 0;
	ConditionEvent::init_above_ = 0;
	PreSyn::presyn_send_mindelay_ = 0;
	PreSyn::presyn_send_direct_ = 0;
	PreSyn::presyn_deliver_netcon_ = 0;
	PreSyn::presyn_deliver_direct_ = 0;
	PreSyn::presyn_deliver_ncsend_ = 0;
	SelfEvent::selfevent_send_ = 0;
	SelfEvent::selfevent_move_ = 0;
	SelfEvent::selfevent_deliver_ = 0;
	WatchCondition::watch_send_ = 0;
	WatchCondition::watch_deliver_ = 0;
	DiscreteEvent::discretevent_send_ = 0;
	DiscreteEvent::discretevent_deliver_ = 0;

	// SelfEvents need to be "freed". Other kinds of DiscreteEvents may
	// already have gone out of existence so the tqe_ may contain many
	// invalid item data pointers
	enqueueing_ = 0;
	for (i=0; i < nrn_nthread; ++i) {
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
	HTList* q;
	int i, j;
#if BBTQ == 5
	for (i=0; i < nrn_nthread; ++i) {
		p[i].tqe_->nshift_ = -1;
		p[i].tqe_->shift_bin(nt_t);
	}
#endif
	if (psl_) {
		for (q = psl_->First(); q != psl_->End(); q = q->Next()) {
			PreSyn* ps = (PreSyn*)q->vptr();
			ps->init();
			ps->flag_ = false;
			NetCon** ncl = ps->ncl_;
			ps->use_min_delay_ = 0;
#if USE_MIN_DELAY
			// also decide what to do about use_min_delay_
			// the rule for now is to use it if all delays are
			// the same and there are more than 2
			{
				if (ps->nc_cnt_ > 2) {
					ps->use_min_delay_ = 1;
					ps->delay_ = ncl[0]->delay_;
				}
			}
#endif // USE_MIN_DELAY

			for (i=ps->nc_cnt_-1; i >= 0; --i) {
				NetCon* d = ncl[i];
				if (d->target_) {
					int type = d->target_->type;
					if (pnt_receive_init[type]) {
(*pnt_receive_init[type])(d->target_, d->weight_, 0);
					}else{
						//not the first
						for (j = d->cnt_-1; j > 0; --j) {
							d->weight_[j] = 0.;
						}
					}
				}
				if (ps->use_min_delay_ && ps->delay_ != d->delay_) {
					ps->use_min_delay_ = false;
				}
			}
		}
	}
}

double PreSyn::mindelay() {
	double md = 1e9;
	int i;
	for (i=nc_cnt_-1; i >= 0; --i) {
		NetCon* d = ncl_[i];
		if (md > d->delay_) {
			md = d->delay_;
		}
	}
	return md;
}

double InputPreSyn::mindelay() {
	double md = 1e9;
	int i;
	for (i=nc_cnt_-1; i >= 0; --i) {
		NetCon* d = ncl_[i];
		if (md > d->delay_) {
			md = d->delay_;
		}
	}
	return md;
}

void NetCvode::handle_tstop_event(double tt, NrnThread* nt) {
	assert(0);
}

void NetCvode::deliver_least_event(NrnThread* nt) {
	TQItem* q = p[nt->id].tqe_->least();
	DiscreteEvent* de = (DiscreteEvent*)q->data_;
	double tt = q->t_;
	p[nt->id].tqe_->remove(q);
#if PRINT_EVENT
	if (print_event_) { de->pr("deliver", tt, this); }
#endif
	STATISTICS(deliver_cnt_);
	de->deliver(tt, this, nt);
}

#if BGPDMA > 1
#define RP_COUNT 50
static int rp_count;
#endif

bool NetCvode::deliver_event(double til, NrnThread* nt) {
	TQItem* q;
	if ((q = p[nt->id].tqe_->atomic_dq(til)) != 0) {
		DiscreteEvent* de = (DiscreteEvent*)q->data_;
		double tt = q->t_;
		p[nt->id].tqe_->release(q);
#if PRINT_EVENT
		if (print_event_) { de->pr("deliver", tt, this); }
#endif
		STATISTICS(deliver_cnt_);
		de->deliver(tt, this, nt);
#if BGPDMA > 1
		if (use_dcmf_record_replay) if (--rp_count < 0) {
			nrnbgp_messager_advance();
			rp_count = RP_COUNT;
		}
#endif
		return true;
	}else{
		return false;
	}
}

void net_move(void** v, Point_process* pnt, double tt) {
	if (!(*v)) {
		hoc_execerror( "No event with flag=1 for net_move in ", memb_func[pnt->type].sym);
	}
	TQItem* q = (TQItem*)(*v);
//printf("net_move tt=%g %s *v=%p\n", tt, memb_func[pnt->type].sym, *v);
	if (tt < PP2t(pnt)) {
		SelfEvent* se = (SelfEvent*)q->data_;
		char buf[100];
		sprintf(buf, "net_move tt-nt_t = %g", tt-PP2t(pnt));
		se->pr(buf, tt, net_cvode_instance);
		assert(0);
		hoc_execerror("net_move tt < t", 0);
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
		se->pr(buf, tt, net_cvode_instance);
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
	STATISTICS(SelfEvent::selfevent_move_);
#if PRINT_EVENT
if (print_event_) {
	SelfEvent* se = (SelfEvent*)q->data_;
printf("NetCvode::move_event self event target %s t=%g, old=%g new=%g\n", memb_func[se->target_->type].sym, nt->_t, q->t_, tnew);
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
	cnt_ = 0; active_ = false; weight_ = nil;
}

void NetCon::init(DiscreteEvent* src, Point_process* target) {
	src_ = src;
	delay_ = 1.0;
	if (src_) {
		if (src_->type() == PreSynType) {
			PreSyn* ps = (PreSyn*)src;
			//ps->dil_.append((NetCon*)this);
			// ncl_ was preallocated in nrn_setup.cpp
			ps->ncl_[ps->nc_cnt_++] = (NetCon*)this;
			ps->use_min_delay_ = 0;
		}else{
			assert(src_->type() == InputPreSynType);
			InputPreSyn* ps = (InputPreSyn*)src;
			ps->ncl_[ps->nc_cnt_++] = (NetCon*)this;
			ps->use_min_delay_ = 0;
		}
	}
	if (target == nil) {
		target_ = nil;
		active_ = false;
		cnt_ = 1;
		weight_ = new double[cnt_];
		return;
	}
	target_ = target;
	active_ = true;
	if (!pnt_receive[target_->type]) {
hoc_execerror("No NET_RECEIVE in target PointProcess:", pnt_name(target));
	}
	cnt_ = pnt_receive_size[target_->type];
	weight_ = nil;
	if (cnt_) {
		weight_ = new double[cnt_];
	}
}

NetCon::~NetCon() {
//printf("~NetCon\n");
	rmsrc();
	if (cnt_) {
		delete [] weight_;
	}
}

void NetCon::rmsrc() {
assert(0);
#if 0 //replace ps.dil if needed
	if (src_) {
	    if (src_->type() == PreSynType) {
		PreSyn* ps = (PreSyn*)src_;
		for (int i=0; i < ps->dil_.count(); ++i) {
			if (ps->dil_.item(i) == this) {
				ps->dil_.remove(i);
				if (ps->dil_.count() == 0 && ps->tvec_ == nil
				    && ps->idvec_ == nil) {
#if 1 || NRNMPI
	if (ps->output_index_ == -1)
#endif
					delete src_;
				}
				break;
			}
		}
	    }else{
		assert(src_->type() == InputPreSynType);
		InputPreSyn* ps = (InputPreSyn*)src_;
		for (int i=0; i < ps->dil_.count(); ++i) {
			if (ps->dil_.item(i) == this) {
				ps->dil_.remove(i);
				if (ps->dil_.count() == 0) {
					delete src_;
				}
				break;
			}
		}
	    }
	}
	src_ = nil;
#endif
}

PreSyn::PreSyn(double* src, Point_process* psrc, NrnThread* nt) {
	ncl_ = NULL;
	nc_cnt_ = 0;
	hi_th_ = nil;
	flag_ = false;
	valthresh_ = 0;
	thvar_ = src;
	pntsrc_ = psrc;;
	threshold_ = 10.;
	use_min_delay_ = 0;
	tvec_ = nil;
	idvec_ = nil;
	gid_ = -1;
	nt_ = nil;
	if (src) {
		if (psrc) {
			nt_ = PP2NT(psrc);
		}else{
			assert(nt);
			nt_ = nt;
		}
	}
	if (psrc && !src) {
		nt_ = PP2NT(psrc);
	}
	output_index_ = -1;
#if BGPDMA
	bgp.dma_send_ = 0;
#endif
	if (nt_ && thvar_) {
		NetCvodeThreadData& p = net_cvode_instance->p[nt_->id];
		if (!p.psl_thr_) {
			p.psl_thr_ = new HTList(NULL);
		}
		p.psl_thr_->Append(new HTList(this));
	}
}

InputPreSyn::InputPreSyn() {
	ncl_ = NULL;
	nc_cnt_ = 0;
	use_min_delay_ = 0;
	gid_ = -1;
#if BGPDMA
	bgp.dma_send_ = 0;
#endif
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
	for (int i=0; i < nc_cnt_; ++i) {
		ncl_[i]->src_ = nil;
	}
	if (ncl_) { delete [] ncl_; }
}

InputPreSyn::~InputPreSyn() {
//	printf("~InputPreSyn %p\n", this);
	nrn_cleanup_presyn(this);
	for (int i=0; i < nc_cnt_; ++i) {
		ncl_[i]->src_ = nil;
	}
	if (ncl_) { delete [] ncl_; }
}

void PreSyn::init() {
	if (tvec_) {
		tvec_->resize(0);
	}
	if (idvec_) {
		idvec_->resize(0);
	}
}

void PreSyn::record(IvocVect* vec, IvocVect* idvec, int rec_id) {
	tvec_ = vec;
	idvec_ = idvec;
	rec_id_ = rec_id;
	if (idvec_) {
		tvec_->mutconstruct(1);
	}
}

void PreSyn::record(double tt) {
	int i;
#if 0
	if (tvec_) {
		// need to lock the vector if shared by other PreSyn
		// since we get here in the thread that manages the
		// threshold detection (or net_event from NET_RECEIVE).
		if (idvec_) {tvec_->lock();}
		i = tvec_->capacity();
		tvec_->resize_chunk(i+1);
		tvec_->elem(i) = tt;
		if (idvec_) {
			i = idvec_->capacity();
			idvec_->resize_chunk(i+1);
			idvec_->elem(i) = rec_id_;
			tvec_->unlock();
		}
	}
#else
	assert(spikevec_size < spikevec_buffer_size);
	spikevec_gid[spikevec_size] = gid_;
	spikevec_time[spikevec_size] = tt;
	++spikevec_size;
#endif
}

void ConditionEvent::check(NrnThread* nt, double tt, double teps) {
	if (value() > 0.0) {
		if (flag_ == false) {
			flag_ = true;
			valthresh_ = 0.;
			send(tt + teps, net_cvode_instance, nt);
		}
	}else{
		flag_ = false;
	}
}

ConditionEvent::ConditionEvent() {}
ConditionEvent::~ConditionEvent() {}

WatchCondition::WatchCondition(Point_process* pnt, double(*c)(Point_process*))
 : HTList(nil)
{
	pnt_ = pnt;
	c_ = c;
}

WatchCondition::~WatchCondition() {
//printf("~WatchCondition\n");
	Remove();
}

void WatchCondition::activate(double flag) {
	assert(0);
	// need to figure this out
#if 0
	flag_ = (value() > 0.) ? true: false;
	valthresh_ = 0.;
	nrflag_ = flag;
	Cvode* cv = (Cvode*)pnt_->nvi_;
	assert(cv);
	int id = (cv->nctd_ > 1) ? thread()->id : 0;
	HTList*& wl = cv->ctd_[id].watch_list_;
	if (!wl) {
		wl = new HTList(nil);
		net_cvode_instance->wl_list_->append(wl);
	}
	Remove();
	wl->Append(this);
#endif
}

void WatchCondition::asf_err() {
fprintf(stderr, "WATCH condition with flag=%g for %s\n",
	nrflag_, memb_func[pnt_->type].sym);
}

void PreSyn::asf_err() {
assert(0);
//fprintf(stderr, "PreSyn threshold for %s\n", osrc_ ? hoc_object_name(osrc_):secname(ssrc_));
}

void WatchCondition::send(double tt, NetCvode* nc, NrnThread* nt) {
	assert(0);
#if 0
	qthresh_ = nc->event(tt, this, nt);
	STATISTICS(watch_send_);
#endif
}

void WatchCondition::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	int type = pnt_->type;
	PP2t(pnt_) = tt;
	STATISTICS(watch_deliver_);
	POINT_RECEIVE(type, pnt_, nil, nrflag_);
	if (errno) {
		if (nrn_errno_check(type)) {
hoc_warning("errno set during WatchCondition deliver to NET_RECEIVE", (char*)0);
		}
	}
}

NrnThread* WatchCondition::thread() { return PP2NT(pnt_); }

void WatchCondition::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s", s);
	printf(" WatchCondition %s %.15g flag=%g\n", pnt_name(pnt_), tt, nrflag_);
}


void DiscreteEvent::send(double tt, NetCvode* ns, NrnThread* nt) {
	STATISTICS(discretevent_send_);
	ns->event(tt, this, nt);
}

void DiscreteEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	STATISTICS(discretevent_deliver_);
}

NrnThread* DiscreteEvent::thread() { return nrn_threads; }

void DiscreteEvent::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s DiscreteEvent %.15g\n", s, tt);
}

void NetCon::send(double tt, NetCvode* ns, NrnThread* nt) {
	if (active_ && target_) {
		assert(PP2NT(target_) == nt);
		STATISTICS(netcon_send_active_);
#if BBTQ == 5
		ns->bin_event(tt, this, PP2NT(target_));
#else
		ns->event(tt, this, PP2NT(target_));
#endif
	}else{
		STATISTICS(netcon_send_inactive_);
	}
}
	
void NetCon::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	assert(target_);
if (PP2NT(target_) != nt) {
printf("NetCon::deliver nt=%d target=%d\n", nt->id, PP2NT(target_)->id);
}
	assert(PP2NT(target_) == nt);
	int type = target_->type;
	if (nrn_use_selfqueue_ && nrn_is_artificial_[type]) {
		assert(0);
		// need to figure out what to do
#if 0
		TQItem** pq = (TQItem**)(&target_->prop->dparam[nrn_artcell_qindex_[type]]._pvoid);
		TQItem* q;
		while ((q = *(pq)) != nil && q->t_ < tt) {
			double t1 = q->t_;
			SelfEvent* se = (SelfEvent*)ns->p[nt->id].selfqueue_->remove(q);
//printf("%d NetCon::deliver %g , earlier selfevent at %g\n", nrnmpi_myid, tt, q->t_);
			se->deliver(t1, ns, nt);
		}	
#endif
	}
	nt->_t = tt;

//printf("NetCon::deliver t=%g tt=%g %s\n", t, tt, pnt_name(target_));
	STATISTICS(netcon_deliver_);
	POINT_RECEIVE(type, target_, weight_, 0);
	if (errno) {
		if (nrn_errno_check(type)) {
hoc_warning("errno set during NetCon deliver to NET_RECEIVE", (char*)0);
		}
	}
}

NrnThread* NetCon::thread() { return PP2NT(target_); }

void NetCon::pr(const char* s, double tt, NetCvode* ns) {
	assert(0);
#if 0
	printf("%s %s", s, hoc_object_name(obj_));
	if (src_) {
		printf(" src=%s",  src_->osrc_ ? hoc_object_name(src_->osrc_):secname(src_->ssrc_));
	}else{
		printf(" src=nil");
	}
	printf(" target=%s %.15g\n", (target_?hoc_object_name(target_->ob):"nil"), tt);
#endif
}

void PreSyn::send(double tt, NetCvode* ns, NrnThread* nt) {
	int i;
	record(tt);
#ifndef USENCS
	if (use_min_delay_) {
		STATISTICS(presyn_send_mindelay_);
#if BBTQ == 5
		for (i=0; i < nrn_nthread; ++i) {
			if (nt->id == i) {
				ns->bin_event(tt+delay_, this, nt);
			}else{
				ns->p[i].interthread_send(tt+delay_, this, nrn_threads + i);
			}
		}
#else
		ns->event(tt+delay_, this);
#endif
	}else{
		STATISTICS(presyn_send_direct_);
		for (int i = nc_cnt_-1; i >= 0; --i) {
			NetCon* d = ncl_[i];
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
#endif //ndef USENCS
#if USENCS || NRNMPI
	if (output_index_ >= 0) {
#if BGPDMA
	    if (use_bgpdma_) {
		bgp_dma_send(this, tt);
	    }else{
#endif //BGPDMA

#if NRNMPI
		if (nrn_use_localgid_) {
			nrn_outputevent(localgid_, tt);
		}else
#endif //NRNMPI
		nrn2ncs_outputevent(output_index_, tt);
#if BGPDMA
	    }
#endif //BGPDMA
	}
#endif //USENCS || NRNMPI
}
	
void InputPreSyn::send(double tt, NetCvode* ns, NrnThread* nt) {
	int i;
#ifndef USENCS
	if (use_min_delay_) {
		//STATISTICS(presyn_send_mindelay_);
#if BBTQ == 5
		for (i=0; i < nrn_nthread; ++i) {
			if (nt->id == i) {
				ns->bin_event(tt+delay_, this, nt);
			}else{
				ns->p[i].interthread_send(tt+delay_, this, nrn_threads + i);
			}
		}
#else
		ns->event(tt+delay_, this);
#endif
	}else{
		//STATISTICS(presyn_send_direct_);
		for (int i = nc_cnt_-1; i >= 0; --i) {
			NetCon* d = ncl_[i];
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
#endif //ndef USENCS
}
	
void PreSyn::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	// the thread is the one that owns the targets
	int i, n = nc_cnt_;
	STATISTICS(presyn_deliver_netcon_);
	for (i=0; i < n; ++i) {
		NetCon* d = ncl_[i];
		if (d->active_ && d->target_ && PP2NT(d->target_) == nt) {
			double dtt = d->delay_ - delay_;
			if (dtt == 0.) {
				STATISTICS(presyn_deliver_direct_);
				STATISTICS(deliver_cnt_);
				d->deliver(tt, ns, nt);
			}else if (dtt < 0.) {
hoc_execerror("internal error: Source delay is > NetCon delay", 0);
			}else{
				STATISTICS(presyn_deliver_ncsend_);
				ns->event(tt + dtt, d, nt);
			}
		}
	}
}

void InputPreSyn::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	// the thread is the one that owns the targets
	int i, n = nc_cnt_;
	//STATISTICS(presyn_deliver_netcon_);
	for (i=0; i < n; ++i) {
		NetCon* d = ncl_[i];
		if (d->active_ && d->target_ && PP2NT(d->target_) == nt) {
			double dtt = d->delay_ - delay_;
			if (dtt == 0.) {
				//STATISTICS(presyn_deliver_direct_);
				//STATISTICS(deliver_cnt_);
				d->deliver(tt, ns, nt);
			}else if (dtt < 0.) {
hoc_execerror("internal error: Source delay is > NetCon delay", 0);
			}else{
				//STATISTICS(presyn_deliver_ncsend_);
				ns->event(tt + dtt, d, nt);
			}
		}
	}
}

NrnThread* PreSyn::thread() { return nt_; }

void PreSyn::pr(const char* s, double tt, NetCvode* ns) {
	assert(0);
#if 0
	printf("%s", s);
	printf(" PreSyn src=%s",  osrc_ ? hoc_object_name(osrc_):secname(ssrc_));
	printf(" %.15g\n", tt);
#endif
}

void InputPreSyn::pr(const char* s, double tt, NetCvode* ns) {
	assert(0);
#if 0
	printf("%s", s);
	printf(" PreSyn src=%s",  osrc_ ? hoc_object_name(osrc_):secname(ssrc_));
	printf(" %.15g\n", tt);
#endif
}

SelfEvent::SelfEvent() {}
SelfEvent::~SelfEvent() {}

void SelfEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	int type = target_->type;
	assert(nt == PP2NT(target_));
	if (nrn_use_selfqueue_ && nrn_is_artificial_[type]) { // handle possible earlier flag=1 self event
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
	STATISTICS(selfevent_deliver_);
	POINT_RECEIVE(target_->type, target_, weight_, flag_);
	if (errno) {
		if (nrn_errno_check(target_->type)) {
hoc_warning("errno set during SelfEvent deliver to NET_RECEIVE", (char*)0);
		}
	}
	NetCvodeThreadData& nctd = ns->p[PP2NT(target_)->id];
	--nctd.unreffed_event_cnt_;
	nctd.sepool_->hpfree(this);
}
	
void SelfEvent::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s", s);
	printf(" SelfEvent target=%s %.15g flag=%g\n", pnt_name(target_), tt, flag_);
}

// For localstep makes sure all cvode instances are at this time and 
// makes sure the continuous record records values at this time.
TstopEvent::TstopEvent() {}
TstopEvent::~TstopEvent() {}

void TstopEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
	STATISTICS(discretevent_deliver_);
	ns->handle_tstop_event(tt, nt);
}

void TstopEvent::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s TstopEvent %.15g\n", s, tt);
}

void ncs2nrn_integrate(double tstop) {
	double ts;
	    int n = (int)((tstop - nt_t)/dt + 1e-9);
	    if (n > 3) {
		nrn_fixed_step_group(n);
	    }else{
#if NRNMPI && !defined(USENCS)
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

	HTList* pth = p[nt->id].psl_thr_;

	if (pth) { /* only look at ones with a threshold */
		HTList* q1;
		for (q1 = pth->First(); q1 != pth->End(); q1 = q1->Next()) {
			PreSyn* ps = (PreSyn*)q1->vptr();
		    // only the ones for this thread
		    if (ps->nt_ == nt) {
			if (ps->thvar_) {
				ps->check(nt, nt->_t, 1e-10);
			}
		    }
		}
	}
	for (i=0; i < wl_list_->count(); ++i) {
		HTList* wl = wl_list_->item(i);
		for (HTList* item = wl->First(); item != wl->End(); item = item->Next()) {
		    WatchCondition* wc = (WatchCondition*)item;
		    if (PP2NT(wc->pnt_) == nt) {
			wc->check(nt, nt->_t);
		    }
		}
	}
}

void NetCvode::deliver_net_events(NrnThread* nt) { // for default method
	TQItem* q;
	double tm, tt, tsav;
#if BGPDMA
	if (use_bgpdma_) { nrnbgp_messager_advance(); }
#endif
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
}

