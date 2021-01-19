/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <float.h>
#include <map>
#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/network/netcon.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/network/netpar.hpp"
#include "coreneuron/utils/ivocvect.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/io/output_spikes.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/gpu/nrn_acc_manager.hpp"
#include "coreneuron/network/multisend.hpp"
#include "coreneuron/mechanism/membfunc.hpp"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

#ifdef _OPENACC
#include <openacc.h>
#endif
namespace coreneuron {
#define PP2NT(pp) (nrn_threads + (pp)->_tid)
#define PP2t(pp) (PP2NT(pp)->_t)
//#define POINT_RECEIVE(type, tar, w, f) (*pnt_receive[type])(tar, w, f)

using ReceiveFunc = void (*)(Point_process*, double*, double);

double NetCvode::eps_;
NetCvode* net_cvode_instance;
bool cvode_active_;

/// Flag to use the bin queue
bool nrn_use_bin_queue_ = 0;

void mk_netcvode() {
    if (!net_cvode_instance) {
        net_cvode_instance = new NetCvode();
    }
}

#ifdef DEBUG
// temporary
static int nrn_errno_check(int type) {
    printf("nrn_errno_check() was called on pid %d: errno=%d type=%d\n", nrnmpi_myid, errno, type);
    //  assert(0);
    type = 0;
    return 1;
}
#endif

// for _OPENACC and/or NET_RECEIVE_BUFFERING
// sem 0:3 send event move
void net_sem_from_gpu(int sendtype,
                      int i_vdata,
                      int weight_index_,
                      int ith,
                      int ipnt,
                      double td,
                      double flag) {
    NrnThread& nt = nrn_threads[ith];
    Point_process* pnt = (Point_process*)nt._vdata[ipnt];
    if (sendtype == 0) {
        net_send(nt._vdata + i_vdata, weight_index_, pnt, td, flag);
    } else if (sendtype == 2) {
        net_move(nt._vdata + i_vdata, pnt, td);
    } else {
        net_event(pnt, td);
    }
}

void net_send(void** v, int weight_index_, Point_process* pnt, double td, double flag) {
    NrnThread* nt = PP2NT(pnt);
    NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
    SelfEvent* se = new SelfEvent;
    se->flag_ = flag;
    se->target_ = pnt;
    se->weight_index_ = weight_index_;
    se->movable_ = v;  // needed for SaveState
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
    // printf("net_send %g %s %g %p\n", td, pnt_name(pnt), flag, *v);
}

void artcell_net_send(void** v, int weight_index_, Point_process* pnt, double td, double flag) {
    net_send(v, weight_index_, pnt, td, flag);
}

void net_event(Point_process* pnt, double time) {
    NrnThread* nt = PP2NT(pnt);
    PreSyn* ps = nt->presyns + nt->pnt2presyn_ix[corenrn.get_pnttype2presyn()[pnt->_type]][pnt->_i_instance];
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

NetCvodeThreadData::NetCvodeThreadData() {
    tqe_ = new TQueue<QTYPE>();
    unreffed_event_cnt_ = 0;
    inter_thread_events_.reserve(1000);
    MUTCONSTRUCT(1)
}

NetCvodeThreadData::~NetCvodeThreadData() {
    inter_thread_events_.clear();
    delete tqe_;
    MUTDESTRUCT
}

/// If the PreSyn is on a different thread than the target,
/// we have to lock the buffer
void NetCvodeThreadData::interthread_send(double td, DiscreteEvent* db, NrnThread* nt) {
    (void)nt;  // avoid unused warning
    MUTLOCK

    InterThreadEvent ite;
    ite.de_ = db;
    ite.t_ = td;
    inter_thread_events_.push_back(ite);

    MUTUNLOCK
}

void NetCvodeThreadData::enqueue(NetCvode* nc, NrnThread* nt) {
    MUTLOCK
    for (size_t i = 0; i < inter_thread_events_.size(); ++i) {
        InterThreadEvent ite = inter_thread_events_[i];
        nc->bin_event(ite.t_, ite.de_, nt);
    }
    inter_thread_events_.clear();
    MUTUNLOCK
}

NetCvode::NetCvode(void) {
    eps_ = 100. * DBL_EPSILON;
    print_event_ = 0;
    pcnt_ = 0;
    p = nullptr;
    p_construct(1);
    // eventually these should not have to be thread safe
    // for parallel network simulations hardly any presyns have
    // a threshold and it can be very inefficient to check the entire
    // presyn list for thresholds during the fixed step method.
    // So keep a threshold list.
}

NetCvode::~NetCvode() {
    if (net_cvode_instance == (NetCvode*)this)
        net_cvode_instance = nullptr;

    p_construct(0);
}

void nrn_p_construct() {
    net_cvode_instance->p_construct(nrn_nthread);
}

void NetCvode::p_construct(int n) {
    int i;

    if (pcnt_ != n) {
        if (p) {
            delete[] p;
            p = nullptr;
        }

        if (n > 0)
            p = new NetCvodeThreadData[n];
        else
            p = nullptr;

        pcnt_ = n;
    }

    for (i = 0; i < n; ++i)
        p[i].unreffed_event_cnt_ = 0;
}

TQItem* NetCvode::bin_event(double td, DiscreteEvent* db, NrnThread* nt) {
    if (nrn_use_bin_queue_) {
#if PRINT_EVENT
        if (print_event_) {
            db->pr("binq send", td, this);
        }
#endif
        return p[nt->id].tqe_->enqueue_bin(td, db);
    } else {
#if PRINT_EVENT
        if (print_event_) {
            db->pr("send", td, this);
        }
#endif
        return p[nt->id].tqe_->insert(td, db);
    }
}

TQItem* NetCvode::event(double td, DiscreteEvent* db, NrnThread* nt) {
#if PRINT_EVENT
    if (print_event_) {
        db->pr("send", td, this);
    }
#endif
    return p[nt->id].tqe_->insert(td, db);
}

void NetCvode::clear_events() {
    // DiscreteEvents may already have gone out of existence so the tqe_
    // may contain many invalid item data pointers
    enqueueing_ = 0;
    for (int i = 0; i < nrn_nthread; ++i) {
        NetCvodeThreadData& d = p[i];
        delete d.tqe_;
        d.tqe_ = new TQueue<QTYPE>();
        d.unreffed_event_cnt_ = 0;
        d.inter_thread_events_.clear();
        d.tqe_->nshift_ = -1;
        d.tqe_->shift_bin(nrn_threads->_t);
    }
}

void NetCvode::init_events() {
    for (int i = 0; i < nrn_nthread; ++i) {
        p[i].tqe_->nshift_ = -1;
        p[i].tqe_->shift_bin(nrn_threads->_t);
    }

    for (int tid = 0; tid < nrn_nthread; ++tid) {  // can be done in parallel
        NrnThread* nt = nrn_threads + tid;

        for (int ipre = 0; ipre < nt->n_presyn; ++ipre) {
            PreSyn* ps = nt->presyns + ipre;
            ps->flag_ = false;
        }

        for (int inetc = 0; inetc < nt->n_netcon; ++inetc) {
            NetCon* d = nt->netcons + inetc;
            if (d->target_) {
                int type = d->target_->_type;
                if (corenrn.get_pnt_receive_init()[type]) {
                    (*corenrn.get_pnt_receive_init()[type])(d->target_, d->u.weight_index_, 0);
                } else {
                    int cnt = corenrn.get_pnt_receive_size()[type];
                    double* wt = nt->weights + d->u.weight_index_;
                    // not the first
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
        if (print_event_) {
            de->pr("deliver", tt, this);
        }
#endif
        de->deliver(tt, this, nt);

        /// In case of a self event we need to delete the self event
        if (de->type() == SelfEventType)
            delete (SelfEvent*)de;

        return true;
    } else
        return false;
}

void net_move(void** v, Point_process* pnt, double tt) {
    if (!(*v))
        hoc_execerror("No event with flag=1 for net_move in ",
                      corenrn.get_memb_func(pnt->_type).sym);

    TQItem* q = (TQItem*)(*v);
    // printf("net_move tt=%g %s *v=%p\n", tt, memb_func[pnt->_type].sym, *v);
    if (tt < PP2t(pnt))
        nrn_assert(0);

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
        printf("NetCvode::move_event self event target %s t=%g, old=%g new=%g\n",
               memb_func[se->target_->_type].sym, nt->_t, q->t_, tnew);
    }
#endif

    p[tid].tqe_->move(q, tnew);
}

void NetCvode::deliver_events(double til, NrnThread* nt) {
    // printf("deliver_events til %20.15g\n", til);
    /// Enqueue any outstanding events in the interthread event buffer
    p[nt->id].enqueue(this, nt);

    /// Deliver events. When the map is used, the loop is explicit
    while (deliver_event(til, nt))
        ;
}

DiscreteEvent::DiscreteEvent() {
}
DiscreteEvent::~DiscreteEvent() {
}

NetCon::NetCon() {
    active_ = false;
    u.weight_index_ = 0;
    target_ = nullptr;
    delay_ = 1.0;
}

NetCon::~NetCon() {
}

PreSyn::PreSyn() {
    nc_index_ = 0;
    nc_cnt_ = 0;
    flag_ = false;
    thvar_index_ = -1;
    pntsrc_ = nullptr;
    threshold_ = 10.;
    gid_ = -1;
#if NRNMPI
    localgid_ = 0;
#endif
#if NRN_MULTISEND
    multisend_index_ = -1;
#endif
    output_index_ = 0;
}

InputPreSyn::InputPreSyn() {
    nc_index_ = -1;
    nc_cnt_ = 0;
#if NRN_MULTISEND
    multisend_phase2_index_ = -1;
#endif
}

PreSyn::~PreSyn() {
    //	printf("~PreSyn %p\n", this);
    if (pntsrc_) {
        pntsrc_ = nullptr;
    }
}

InputPreSyn::~InputPreSyn() {
}

void PreSyn::record(double tt) {
    spikevec_lock();
    if (gid_ > -1) {
        spikevec_gid.push_back(gid_);
        spikevec_time.push_back(tt);
    }
    spikevec_unlock();
}

bool ConditionEvent::check(NrnThread* nt) {
    if (value(nt) > 0.0) {
        if (flag_ == false) {
            flag_ = true;
            return true;
        }
    } else {
        flag_ = false;
    }
    return false;
}

ConditionEvent::ConditionEvent() {
}
ConditionEvent::~ConditionEvent() {
}

void DiscreteEvent::send(double tt, NetCvode* ns, NrnThread* nt) {
    ns->event(tt, this, nt);
}

void DiscreteEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    (void)tt;
    (void)ns;
    (void)nt;
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
    nrn_assert(target_);

    if (PP2NT(target_) != nt)
        printf("NetCon::deliver nt=%d target=%d\n", nt->id, PP2NT(target_)->id);

    nrn_assert(PP2NT(target_) == nt);
    int typ = target_->_type;
    nt->_t = tt;

    // printf("NetCon::deliver t=%g tt=%g %s\n", t, tt, pnt_name(target_));
    (*corenrn.get_pnt_receive()[typ])(target_, u.weight_index_, 0);
#ifdef DEBUG
    if (errno && nrn_errno_check(typ))
        hoc_warning("errno set during NetCon deliver to NET_RECEIVE", (char*)0);
#endif
}

void NetCon::pr(const char* s, double tt, NetCvode* ns) {
    (void)ns;
    Point_process* pp = target_;
    printf("%s NetCon target=%s[%d] %.15g\n", s, corenrn.get_memb_func(pp->_type).sym, pp->_i_instance, tt);
}

void PreSyn::send(double tt, NetCvode* ns, NrnThread* nt) {
    record(tt);
    for (int i = nc_cnt_ - 1; i >= 0; --i) {
        NetCon* d = netcon_in_presyn_order_[nc_index_ + i];
        if (d->active_ && d->target_) {
            NrnThread* n = PP2NT(d->target_);

            if (nt == n)
                ns->bin_event(tt + d->delay_, d, n);
            else
                ns->p[n->id].interthread_send(tt + d->delay_, d, n);
        }
    }

#if NRNMPI
    if (output_index_ >= 0) {
#if NRN_MULTISEND
        if (use_multisend_) {
            nrn_multisend_send(this, tt, nt);
        } else {
#else
        {
#endif
            if (nrn_use_localgid_) {
                nrn_outputevent(localgid_, tt);
            } else {
                nrn2ncs_outputevent(output_index_, tt);
            }
        }
    }
#endif  // NRNMPI
}

void InputPreSyn::send(double tt, NetCvode* ns, NrnThread* nt) {
    for (int i = nc_cnt_ - 1; i >= 0; --i) {
        NetCon* d = netcon_in_presyn_order_[nc_index_ + i];
        if (d->active_ && d->target_) {
            NrnThread* n = PP2NT(d->target_);

            if (nt == n)
                ns->bin_event(tt + d->delay_, d, n);
            else
                ns->p[n->id].interthread_send(tt + d->delay_, d, n);
        }
    }
}

void PreSyn::deliver(double, NetCvode*, NrnThread*) {
    assert(0);  // no PreSyn delay.
}

void InputPreSyn::deliver(double, NetCvode*, NrnThread*) {
    assert(0);  // no InputPreSyn delay.
}

SelfEvent::SelfEvent() {
}
SelfEvent::~SelfEvent() {
}

void SelfEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    nrn_assert(nt == PP2NT(target_));
    PP2t(target_) = tt;
    // printf("SelfEvent::deliver t=%g tt=%g %s\n", PP2t(target), tt, pnt_name(target_));
    call_net_receive(ns);
}

void SelfEvent::call_net_receive(NetCvode* ns) {
    (*corenrn.get_pnt_receive()[target_->_type])(target_, weight_index_, flag_);

#ifdef DEBUG
    if (errno && nrn_errno_check(target_->_type))
        hoc_warning("errno set during SelfEvent deliver to NET_RECEIVE", (char*)0);
#endif

    NetCvodeThreadData& nctd = ns->p[PP2NT(target_)->id];
    --nctd.unreffed_event_cnt_;
}

void SelfEvent::pr(const char* s, double tt, NetCvode*) {
    printf("%s", s);
    printf(" SelfEvent target=%s %.15g flag=%g\n", pnt_name(target_), tt, flag_);
}

void ncs2nrn_integrate(double tstop) {
    int total_sim_steps = static_cast<int>((tstop - nrn_threads->_t) / dt + 1e-9);

    if (total_sim_steps > 3 && !nrn_have_gaps) {
        nrn_fixed_step_group_minimal(total_sim_steps);
    } else {
        nrn_fixed_single_steps_minimal(total_sim_steps, tstop);
    }

    // handle all the pending flag=1 self events
    for (int i = 0; i < nrn_nthread; ++i)
        nrn_assert(nrn_threads[i]._t == nrn_threads->_t);
}

// factored this out from deliver_net_events so we can
// stay in the cache
// net_send_buffer added so checking can be done on gpu
// while event queueing is on cpu.
// Remember: passsing reference variable causes cray
// compiler bug

static bool pscheck(double var, double thresh, int* flag) {
    if (var > thresh) {
        if (*flag == false) {
            *flag = true;
            return true;
        }
    } else {
        *flag = false;
    }
    return false;
}

double PreSyn::value(NrnThread* nt) {
    return nt->_actual_v[thvar_index_] - threshold_;
}

void NetCvode::check_thresh(NrnThread* nt) {  // for default method
    int i;
    double teps = 1e-10;

    nt->_net_send_buffer_cnt = 0;
    int net_send_buf_count = 0;
    PreSyn* presyns = nt->presyns;
    PreSynHelper* presyns_helper = nt->presyns_helper;
    double* actual_v = nt->_actual_v;

#if defined(_OPENACC)
    int stream_id = nt->stream_id;
#endif

    if (nt->ncell == 0)
        return;

//_net_send_buffer_cnt is no longer used in openacc kernel, remove this?
//#ifdef _OPENACC
//    if(nt->compute_gpu)
//        acc_update_device(&(nt->_net_send_buffer_cnt), sizeof(int));
//#endif

// on GPU...
// clang-format off
    #pragma acc parallel loop present(                  \
        nt[0:1], presyns_helper[0:nt->n_presyn],        \
        presyns[0:nt->n_presyn], actual_v[0:nt->end])   \
        copy(net_send_buf_count) if (nt->compute_gpu)   \
        async(stream_id)
    // clang-format on
    for (i = 0; i < nt->ncell; ++i) {
        PreSyn* ps = presyns + i;
        PreSynHelper* psh = presyns_helper + i;
        int idx = 0;
        int thidx = ps->thvar_index_;
        double v = actual_v[thidx];
        double threshold = ps->threshold_;
        int* flag = &(psh->flag_);

        if (pscheck(v, threshold, flag)) {
#ifndef _OPENACC
            nt->_net_send_buffer_cnt = net_send_buf_count;
            if (nt->_net_send_buffer_cnt >= nt->_net_send_buffer_size) {
                nt->_net_send_buffer_size *= 2;
                nt->_net_send_buffer =
                    (int*)erealloc(nt->_net_send_buffer, nt->_net_send_buffer_size * sizeof(int));
            }
#endif

// clang-format off
            #pragma acc atomic capture
            // clang-format on
            idx = net_send_buf_count++;

            nt->_net_send_buffer[idx] = i;
        }
    }

// clang-format off
    #pragma acc wait(stream_id)
    // clang-format on
    nt->_net_send_buffer_cnt = net_send_buf_count;

    if (nt->_net_send_buffer_cnt) {
#ifdef _OPENACC
        int* nsbuffer = nt->_net_send_buffer;
#endif
// clang-format off
        #pragma acc update host(nsbuffer[0:nt->_net_send_buffer_cnt]) if (nt->compute_gpu) async(stream_id)
        #pragma acc wait(stream_id)
        // clang-format on
    }

    // on CPU...
    for (i = 0; i < nt->_net_send_buffer_cnt; ++i) {
        PreSyn* ps = nt->presyns + nt->_net_send_buffer[i];
        ps->send(nt->_t + teps, net_cvode_instance, nt);
    }

    // Types that have WATCH statements. If exist, then last element is 0.
    if (nt->_watch_types) {
        for (int i = 0; nt->_watch_types[i] != 0; ++i) {
            int type = nt->_watch_types[i];
            (*corenrn.get_watch_check()[type])(nt, nt->_ml_list[type]);
            // may generate net_send events (with 0 (teps) delay)
        }
    }
}

// WATCH statements are rare. Conceptually they are very similar to
// PreSyn thresholds as above but an optimal peformance implementation for GPU is
// not obvious. Each WATCH statement threshold test could make use of
// pscheck.  Note that it is possible that there are several active WATCH
// statements for a given POINT_PROCESS instance as well as none active.
// Also WATCH statements switch between active and inactive state.
//
// In NEURON,
// both PreSyn and WatchCondition were subclasses of ConditionEvent. When
// a WatchCondition fired in the fixed step method, it was placed on the queue
// with a delivery time of t+teps. WatchCondition::deliver called the NET_RECEIVE
// block with proper flag ( but nullptr weight vector). WatchConditions
// were created,added/removed,destroyed from a list as necessary.
// Perhaps the most commonly used WATCH statement is in the context of a
// ThresholdDetect Point_process which watches voltage and compares to
// an instance specific threshold parameter. A firing ThresholdDetect instance
// would call net_event(tdeliver) which then feeds into the standard
// artcell PreSyn sequence (using pntsrc_ instead of thvar_index_).
//
// So... the PreSyns have the same order as they are checked (although PreSyn
// data is AoS instead of SoA and nested 'if' means a failure of SIMD.)
// But if multiple WATCH, there is (from one kind of implementation viewpoint),
// yet another 'if' with regard to whether a WATCH is active. And if there
// are multiple WATCH, the size of the list is dynamic.
//
// An experimental implementation is to check all WATCH of all instances
// of a type with the proviso that there is an active flag for each WATCH.
// ie. active, below, var1, var2 are all SoA (except one of the var may
// be voltage). Can use 'if (active && pscheck(var1, var2, &below)'
// The mod file net_send_buffering fragments can be used which
// ultimately call net_send using a transient SelfEvent. ie. all
// checking computation takes place in the context of the mod file without
// using explicit WatchCondition instances.

// events including binqueue events up to t+dt/2
void NetCvode::deliver_net_events(NrnThread* nt) {  // for default method
    TQItem* q;
    double tm, tsav;
#if NRN_MULTISEND
    if (use_multisend_ && nt->id == 0) {
        nrn_multisend_advance();
    }
#endif
    int tid = nt->id;
    tsav = nt->_t;
    tm = nt->_t + 0.5 * nt->_dt;
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
            if (print_event_) {
                db->pr("binq deliver", nrn_threads->_t, this);
            }
#endif

            delete q;
            db->deliver(nt->_t, this, nt);
        }
        // assert(int(tm/nt->_dt)%1000 == p[tid].tqe_->nshift_);
    }

    deliver_events(tm, nt);

    if (nrn_use_bin_queue_) {
        if (p[tid].tqe_->top()) {
            goto tryagain;
        }
        p[tid].tqe_->shift_bin(tm);
    }

    nt->_t = tsav;

    /*before executing on gpu, we have to update the NetReceiveBuffer_t on GPU */
    update_net_receive_buffer(nt);

    for (auto& net_buf_receive : corenrn.get_net_buf_receive()) {
        (*net_buf_receive.first)(nt);
    }
}
}  // namespace coreneuron
