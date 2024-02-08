#include <../../nrnconf.h>

// define to 0 if do not wish use_min_delay_ to ever be 1
#define USE_MIN_DELAY 1

#include <stdlib.h>
#include <nrnmpi.h>
#include <errno.h>
#include <time.h>
#include <regex>
#include "classreg.h"
#include "nrnoc2iv.h"
#include "parse.hpp"
#include "cvodeobj.h"
#include "hoclist.h"
#include "pool.h"
#include "tqueue.h"
#include "ocobserv.h"
#include "nrnneosm.h"
#include "datapath.h"
#include "objcmd.h"
#include "shared/sundialsmath.h"
#include "kssingle.h"
#include "ocnotify.h"
#include "utils/enumerate.h"
#if HAVE_IV
#include "ivoc.h"
#include "glinerec.h"
#include "ocjump.h"
#endif
#include "vrecitem.h"
#include "oclist.h"
#define PROFILE 0
#include "htlist.h"
#include "ivocvect.h"
#include "netcon.h"
#include "netcvode.h"
#include "nrn_ansi.h"
#include "nrncore_write/utils/nrncore_utils.h"
#include "nrniv_mf.h"
#include "nrnste.h"
#include "profile.h"
#include "utils/profile/profiler_interface.h"

#include <array>
#include <unordered_set>
#include <utility>

typedef void (*ReceiveFunc)(Point_process*, double*, double);

#define lvardtloop(i, j)              \
    for (i = 0; i < nrn_nthread; ++i) \
        for (j = 0; j < p[i].nlcv_; ++j)

#define NVI_SUCCESS 0
#define PP2NT(pp)   ((NrnThread*) ((pp)->_vnt))
#define PP2t(pp)    (PP2NT(pp)->_t)
#define LOCK(m)     /**/
#define UNLOCK(m)   /**/
// classical and when DiscreteEvent::deliver is already in the right thread
// via a future thread instance of NrnNetItem with its own tqe.
#define POINT_RECEIVE(type, tar, w, f) (*pnt_receive[type])(tar, w, f)
// when global tqe is managed by master thread and the correct thread
// needs to be fired to execute the NET_RECEIVE block.
//#define POINT_RECEIVE(type, tar, w, f) ns->point_receive(type, tar, w, f)

#include "membfunc.h"
extern void single_event_run();
extern void setup_topology(), v_setup_vectors();
extern int nrn_errno_check(int);
extern NetCvode* net_cvode_instance;
extern cTemplate** nrn_pnt_template_;
extern double t, dt;
extern void nrn_cvfun(double t, double* y, double* ydot);
extern void nrn_cleanup_presyn(PreSyn*);
#define nt_dt nrn_threads->_dt
#define nt_t  nrn_threads->_t
extern void nrn_parent_info(Section*);
extern Object* nrn_sec2cell(Section*);
extern int nrn_sec2cell_equals(Section*, Object*);
extern ReceiveFunc* pnt_receive;
extern ReceiveFunc* pnt_receive_init;
extern short* nrn_is_artificial_;  // should be bool but not using that type in c
extern short* nrn_artcell_qindex_;
int nrn_use_selfqueue_;
void nrn_pending_selfqueue(double tt, NrnThread*);
static void all_pending_selfqueue(double tt);
static void* pending_selfqueue(NrnThread*);
extern int hoc_araypt(Symbol*, int);
extern int hoc_stacktype();
void nrn_use_daspk(int);
extern int nrn_use_daspk_;
int linmod_extra_eqn_count();
extern int nrn_modeltype();
extern Symlist* hoc_built_in_symlist;
extern Symlist* hoc_top_level_symlist;
extern TQueue* net_cvode_instance_event_queue(NrnThread*);
extern hoc_Item* net_cvode_instance_psl();
extern std::vector<PlayRecord*>* net_cvode_instance_prl();
extern void nrn_update_ps2nt();
extern void nrn_use_busywait(int);
void* nrn_interthread_enqueue(NrnThread*);
extern void (*nrnthread_v_transfer_)(NrnThread*);
Object* (*nrnpy_seg_from_sec_x)(Section*, double);
extern "C" void nrnthread_get_trajectory_requests(int tid,
                                                  int& bsize,
                                                  int& n_pr,
                                                  void**& vpr,
                                                  int& n_trajec,
                                                  int*& types,
                                                  int*& indices,
                                                  double**& pvars,
                                                  double**& varrays);
extern "C" void nrnthread_trajectory_values(int tid, int n_pr, void** vpr, double t);
extern "C" void nrnthread_trajectory_return(int tid,
                                            int n_pr,
                                            int bsize,
                                            int vecsz,
                                            void** vpr,
                                            double t);
bool nrn_trajectory_request_per_time_step_ = false;
#if NRN_MUSIC
#include "nrnmusicapi.h"
#endif

extern int hoc_return_type_code;

extern int nrn_fornetcon_cnt_;
extern int* nrn_fornetcon_index_;
extern int* nrn_fornetcon_type_;

// for use in mod files
double nrn_netcon_get_delay(NetCon* nc) {
    return nc->delay_;
}
void nrn_netcon_set_delay(NetCon* nc, double d) {
    nc->delay_ = d;
}
int nrn_netcon_weight(NetCon* nc, double** pw) {
    *pw = nc->weight_;
    return nc->cnt_;
}
double nrn_event_queue_stats(double* stats) {
#if COLLECT_TQueue_STATISTICS
    net_cvode_instance_event_queue(nrn_threads)->spike_stat(stats);
    return (stats[0] - stats[2]);
#else
    return -1.;
#endif
}
double nrn_netcon_get_thresh(NetCon* nc) {
    if (nc->src_) {
        return nc->src_->threshold_;
    } else {
        return 0;
    }
}
void nrn_netcon_set_thresh(NetCon* nc, double th) {
    if (nc->src_) {
        nc->src_->threshold_ = th;
    }
}

void nrn_netcon_event(NetCon* nc, double td) {
    nc->chktar();
    net_cvode_instance->event(td, nc, PP2NT(nc->target_));
}

Point_process* nrn_netcon_target(NetCon* nc) {
    nc->chktar();
    return nc->target_;
}

int nrn_netcon_info(NetCon* nc, double** pw, Point_process** target, double** th, double** del) {
    *target = (nc->target_) ? nc->target_ : (Point_process*) 0;
    *th = (nc->src_) ? &(nc->src_->threshold_) : (double*) 0;
    *del = &nc->delay_;
    *pw = nc->weight_;
    return nc->cnt_;
}

int nrn_presyn_count(PreSyn* ps) {
    return ps->dil_.size();
}
void* nrn_presyn_netcon(PreSyn* ps, int i) {
    return ps->dil_[i];
}

#if USENEOSIM
void neosim2nrn_advance(void*, void*, double);
void neosim2nrn_deliver(void*, void*);
void (*p_nrn2neosim_send)(void*, double);
static void* neosim_entity_;
#endif

void ncs2nrn_integrate(double tstop);
extern void (*nrn_allthread_handle)();
static void allthread_handle_callback() {
    net_cvode_instance->allthread_handle();
}

#if USENCS
// As a subroutine for NCS, NEURON simulates the cells of a subnet
// (usually a single cell) and NCS manages whatever intercellular
// NetCon connections were specified by the hoc cvode method
// Cvode.ncs_netcons(List nc_inlist, List nc_outlist) . The former list
// is normally in one to one correspondence with all the synapses. These
// NetCon objects have those synapses as targets and nullptr sources.
// The latter list is normally in one to one correspondence with
// the cells of the subnet. Those NetCon objects have sources of the form
// cell.axon.v(1) and nullptr targets.
// Note that the program that creates the hoc file knows
// how to tell NCS that a particular integer corresponds to a particular
// NetCon

// NCS tells NEURON what time to integrate to. (Using any integration method)
// Probably the step size will be the minimum delay between output spike
// and its effect on any synapse.
// void ncs2nrn_integrate(double tstop);

// NCS gives synaptic events to NEURON but tdeliver must be >= t
void ncs2nrn_inputevent(int netcon_input_index, double tdeliver);

// and NEURON notifies NCS if cell ouputs a spike during integrate
extern void nrn2ncs_outputevent(int netcon_output_index, double firetime);

// netcon_input_index specifies the NetCon object
// in the following list. The hoc file sets this up via cvode.ncs_netcons
static NetConPList* ncs2nrn_input_;

// the netcon_ouput_index is specified in a NetCon field called
// nrn2ncs_output_index_

// helper functions
void nrn2ncs_netcons();
#endif
#if NRNMPI
extern void nrn2ncs_outputevent(int netcon_output_index, double firetime);
#endif

#if NRNMPI
extern void nrn_multisend_send(PreSyn*, double t);
extern bool use_multisend_;
extern void nrn_multisend_advance();
#endif

bool nrn_use_fifo_queue_;

#if BBTQ == 5
bool nrn_use_bin_queue_;
#endif

#if NRNMPI
// for compressed info during spike exchange
extern bool nrn_use_localgid_;
extern void nrn_outputevent(unsigned char, double);
#endif

struct ForNetConsInfo {
    double** argslist;
    int size;
};

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
unsigned long ConditionEvent::send_qthresh_;
unsigned long ConditionEvent::deliver_qthresh_;
unsigned long ConditionEvent::abandon_;
unsigned long ConditionEvent::eq_abandon_;
unsigned long ConditionEvent::abandon_init_above_;
unsigned long ConditionEvent::abandon_init_below_;
unsigned long ConditionEvent::abandon_above_;
unsigned long ConditionEvent::abandon_below_;
unsigned long PreSyn::presyn_send_mindelay_;
unsigned long PreSyn::presyn_send_direct_;
unsigned long PreSyn::presyn_deliver_netcon_;
unsigned long PreSyn::presyn_deliver_direct_;
unsigned long PreSyn::presyn_deliver_ncsend_;
unsigned long PlayRecordEvent::playrecord_send_;
unsigned long PlayRecordEvent::playrecord_deliver_;
unsigned long HocEvent::hocevent_send_;
unsigned long HocEvent::hocevent_deliver_;
unsigned long KSSingle::singleevent_deliver_;
unsigned long KSSingle::singleevent_move_;

TQueue* net_cvode_instance_event_queue(NrnThread* nt) {
    return net_cvode_instance->event_queue(nt);
}

hoc_Item* net_cvode_instance_psl() {
    return net_cvode_instance->psl_;
}

std::vector<PlayRecord*>* net_cvode_instance_prl() {
    return net_cvode_instance->playrec_list();
}

void nrn_use_daspk(int b) {
    if (net_cvode_instance) {
        net_cvode_instance->use_daspk(b == 1);
    }
}

double NetCvode::eps_;

static Node* node(Object*);
Node* node(Object* ob) {
    return ob2pntproc(ob)->node;
}

PlayRecordEvent::PlayRecordEvent() {}
PlayRecordEvent::~PlayRecordEvent() {}

DiscreteEvent* PlayRecordEvent::savestate_save() {
    PlayRecordEvent* pre = new PlayRecordEvent();
    pre->plr_ = plr_;
    return pre;
}
void PlayRecordEvent::savestate_restore(double tt, NetCvode* nc) {
    nc->event(tt, plr_->event(), nrn_threads + plr_->ith_);
}
void PlayRecordEvent::savestate_write(FILE* f) {
    fprintf(f, "%d\n", PlayRecordEventType);
    fprintf(f, "%d %d\n", plr_->type(), net_cvode_instance->playrec_item(plr_));
}

DiscreteEvent* PlayRecordEvent::savestate_read(FILE* f) {
    char buf[100];
    int type, plr_index;
    nrn_assert(fgets(buf, 100, f));
    sscanf(buf, "%d %d\n", &type, &plr_index);
    PlayRecord* plr = net_cvode_instance->playrec_item(plr_index);
    assert(plr && plr->type() == type);
    return plr->event()->savestate_save();
}

PlayRecordSave* PlayRecord::savestate_save() {
    return new PlayRecordSave(this);
}

PlayRecordSave* PlayRecord::savestate_read(FILE* f) {
    PlayRecordSave* prs = nullptr;
    int type, index;
    char buf[100];
    nrn_assert(fgets(buf, 100, f));
    nrn_assert(sscanf(buf, "%d %d\n", &type, &index) == 2);
    PlayRecord* plr = net_cvode_instance->playrec_item(index);
    assert(plr->type() == type);
    switch (type) {
    case VecRecordDiscreteType:
        prs = new VecRecordDiscreteSave(plr);
        break;
    case VecRecordDtType:
        prs = new VecRecordDtSave(plr);
        break;
    case VecPlayStepType:
        prs = new VecPlayStepSave(plr);
        break;
    case VecPlayContinuousType:
        prs = new VecPlayContinuousSave(plr);
        break;
    default:
        // whenever there is no subclass specific data to save
        prs = new PlayRecordSave(plr);
        break;
    }
    prs->savestate_read(f);
    return prs;
}

PlayRecordSave::PlayRecordSave(PlayRecord* plr) {
    pr_ = plr;
    prl_index_ = net_cvode_instance->playrec_item(pr_);
    assert(prl_index_ >= 0);
}
PlayRecordSave::~PlayRecordSave() {}
void PlayRecordSave::check() {
    assert(pr_ == net_cvode_instance->playrec_item(prl_index_));
}

void NetCon::chksrc() {
    if (!src_) {
        hoc_execerror(hoc_object_name(obj_), "source is missing");
    }
}
void NetCon::chktar() {
    if (!target_) {
        hoc_execerror(hoc_object_name(obj_), "target is missing");
    }
}

void NetCon::disconnect(Observable* o) {
    Object* ob = ((ObjObservable*) o)->object();
    // printf("%s disconnect from ", hoc_object_name(obj_));
    if (target_->ob == ob) {
        // printf("target %s\n", hoc_object_name(target_->ob));
        target_ = nullptr;
        active_ = 0;
    }
}

#if 0  // way of printing dinfo
printf("NetCon from %s to ",
d->src_->osrc_ ? hoc_object_name(d->src_->osrc_) : secname(d->src_->ssrc_));
printf("%s ", hoc_object_name(d->target_->ob));
printf(" weight index=%d\n", l);
#endif

class MaxStateItem {
  public:
    Symbol* sym_;
    double max_;
    double amax_;
};

struct InterThreadEvent {
    DiscreteEvent* de_;
    double t_;
};

typedef std::vector<WatchCondition*> WatchList;
using SelfEventPool = MutexPool<SelfEvent>;
typedef std::vector<TQItem*> TQList;

// allows marshalling of all items in the event queue that need to be
// removed to avoid duplicates due to frecord_init after finitialize
static TQList* record_init_items_;

static DiscreteEvent* null_event_;

static PreSyn* unused_presyn;  // holds the NetCons with no source

static double nc_preloc(void* v) {  // user must pop section stack after call
    NetCon* d = (NetCon*) v;
    Section* s = nullptr;
    if (d->src_) {
        s = d->src_->ssrc_;
    }
    if (s) {
        nrn_pushsec(s);
        // This is a special handle, not just a pointer.
        auto const& v = d->src_->thvar_;
        nrn_parent_info(s);  // make sure parentnode exists
        // there is no efficient search for the location of
        // an arbitrary variable. Search only for v at 0 - 1.
        // Otherwise return .5 .
        if (v == s->parentnode->v_handle()) {
            return nrn_arc_position(s, s->parentnode);
        }
        for (int i = 0; i < s->nnode; ++i) {
            if (v == s->pnode[i]->v_handle()) {
                return nrn_arc_position(s, s->pnode[i]);
            }
        }
        return -2.;  // not voltage
    } else {
        return -1.;
    }
}

static Object** nc_preseg(void* v) {  // user must pop section stack after call
    NetCon* d = (NetCon*) v;
    Section* s = NULL;
    Object* obj = NULL;
    double x = -1.;
    if (d->src_) {
        s = d->src_->ssrc_;
    }
    if (s && nrnpy_seg_from_sec_x) {
        auto const& v = d->src_->thvar_;
        nrn_parent_info(s);  // make sure parentnode exists
        // there is no efficient search for the location of
        // an arbitrary variable. Search only for v at 0 -  1.
        // Otherwise return NULL.
        if (v == s->parentnode->v_handle()) {
            x = nrn_arc_position(s, s->parentnode);
        }
        for (int i = 0; i < s->nnode; ++i) {
            if (v == s->pnode[i]->v_handle()) {
                x = nrn_arc_position(s, s->pnode[i]);
                continue;
            }
        }
        // perhaps should search for v
        if (x >= 0) {
            obj = (*nrnpy_seg_from_sec_x)(s, x);
            --obj->refcount;
        }
    }
    return hoc_temp_objptr(obj);
}

static double nc_postloc(void* v) {  // user must pop section stack after call
    NetCon* d = (NetCon*) v;
    if (d->target_ && d->target_->sec) {
        nrn_pushsec(d->target_->sec);
        return nrn_arc_position(d->target_->sec, d->target_->node);
    } else {
        return -1.;
    }
}

static Object** nc_postseg(void* v) {  // user must pop section stack after call
    NetCon* d = (NetCon*) v;
    Object* obj = NULL;
    if (d->target_ && d->target_->sec && nrnpy_seg_from_sec_x) {
        double x = nrn_arc_position(d->target_->sec, d->target_->node);
        obj = (*nrnpy_seg_from_sec_x)(d->target_->sec, x);
        --obj->refcount;
    }
    return hoc_temp_objptr(obj);
}

static Object** nc_syn(void* v) {
    NetCon* d = (NetCon*) v;
    Object* ob = nullptr;
    if (d->target_) {
        ob = d->target_->ob;
    }
    return hoc_temp_objptr(ob);
}

static Object** nc_pre(void* v) {
    NetCon* d = (NetCon*) v;
    Object* ob = nullptr;
    if (d->src_) {
        ob = d->src_->osrc_;
    }
    return hoc_temp_objptr(ob);
}

static Object** newoclist(int i, OcList*& o) {
    Object** po;
    if (ifarg(i) && hoc_is_object_arg(i)) {
        po = hoc_objgetarg(i);
        check_obj_type(*po, "List");
        o = (OcList*) ((*po)->u.this_pointer);
    } else {
        o = new OcList();
        o->ref();
        Symbol* sl = hoc_lookup("List");
        po = hoc_temp_objvar(sl, o);
    }
    return po;
}

static Object** nc_prelist(void* v) {
    NetCon* d = (NetCon*) v;
    OcList* o;
    Object** po = newoclist(1, o);
    if (d->src_) {
        for (const auto& nc: d->src_->dil_) {
            if (nc->obj_) {
                o->append(nc->obj_);
            }
        }
    }
    return po;
}

static Object** nc_synlist(void* v) {
    NetCon* d = (NetCon*) v;
    OcList* o;
    Object** po = newoclist(1, o);
    hoc_Item* q;
    if (net_cvode_instance->psl_)
        ITERATE(q, net_cvode_instance->psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            for (const auto& nc: ps->dil_) {
                if (nc->obj_ && nc->target_ == d->target_) {
                    o->append(nc->obj_);
                }
            }
        }
    return po;
}

static Object** nc_postcelllist(void* v) {
    NetCon* d = (NetCon*) v;
    OcList* o;
    Object** po = newoclist(1, o);
    hoc_Item* q;
    Object* cell = nullptr;
    if (d->target_ && d->target_->sec) {
        cell = nrn_sec2cell(d->target_->sec);
    }
    if (cell && net_cvode_instance->psl_)
        ITERATE(q, net_cvode_instance->psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            for (const auto& nc: ps->dil_) {
                if (nc->obj_ && nc->target_ && nrn_sec2cell_equals(nc->target_->sec, cell)) {
                    o->append(nc->obj_);
                }
            }
        }
    return po;
}

static Object** nc_precelllist(void* v) {
    NetCon* d = (NetCon*) v;
    OcList* o;
    Object** po = newoclist(1, o);
    hoc_Item* q;
    Object* cell = nullptr;
    if (d->src_ && d->src_->ssrc_) {
        cell = nrn_sec2cell(d->src_->ssrc_);
    }
    if (cell && net_cvode_instance->psl_)
        ITERATE(q, net_cvode_instance->psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            for (const auto& nc: ps->dil_) {
                if (nc->obj_ && nc->src_ && ps->ssrc_ && nrn_sec2cell_equals(ps->ssrc_, cell)) {
                    o->append(nc->obj_);
                }
            }
        }
    return po;
}

static Object** nc_precell(void* v) {
    NetCon* d = (NetCon*) v;
    if (d->src_ && d->src_->ssrc_) {
        return hoc_temp_objptr(nrn_sec2cell(d->src_->ssrc_));
    } else {
        return hoc_temp_objptr(0);
    }
}

static Object** nc_postcell(void* v) {
    NetCon* d = (NetCon*) v;
    Object* ob = nullptr;
    if (d->target_ && d->target_->sec) {
        ob = nrn_sec2cell(d->target_->sec);
    }
    return hoc_temp_objptr(ob);
}

static double nc_setpost(void* v) {
    NetCon* d = (NetCon*) v;
    Object* otar = nullptr;
    if (ifarg(1)) {
        otar = *hoc_objgetarg(1);
    }
    if (otar && !is_point_process(otar)) {
        hoc_execerror("argument must be a point process or NULLobject", 0);
    }
    Point_process* tar = nullptr;
    if (otar) {
        tar = ob2pntproc(otar);
    }
    if (d->target_ && d->target_ != tar) {
#if DISCRETE_EVENT_OBSERVER
        ObjObservable::Detach(d->target_->ob, d);
#endif
        d->target_ = nullptr;
    }
    int cnt = 1;
    if (tar) {
        cnt = pnt_receive_size[tar->prop->_type];
        d->target_ = tar;
#if DISCRETE_EVENT_OBSERVER
        ObjObservable::Attach(otar, d);
#endif
    } else {
        d->active_ = false;
    }
    if (d->cnt_ != cnt) {
        d->cnt_ = cnt;
        delete[] std::exchange(d->weight_, new double[d->cnt_]);
    }
    return 0.;
}

static double nc_valid(void* v) {
    NetCon* d = (NetCon*) v;
    hoc_return_type_code = 2;
    if (d->src_ && d->target_) {
        return 1.;
    }
    return 0.;
}

static double nc_active(void* v) {
    NetCon* d = (NetCon*) v;
    bool a = d->active_;
    if (d->target_ && ifarg(1)) {
        d->active_ = bool(chkarg(1, 0, 1));
    }
    hoc_return_type_code = 2;
    return double(a);
}

// for threads, revised net_send to use absolute time (in the
// mod file we add the thread time when we call it).
// And we can no longer check with respect to minimum time in chkarg
static double nc_event(void* v) {
    NetCon* d = (NetCon*) v;
    double td = chkarg(1, -1e20, 1e20);
    if (d->active_ == 0) {
        return 0.0;
    }
    d->chktar();
    NrnThread* nt = PP2NT(d->target_);
    const auto nrn_thread_not_initialized_for_nc_target = nt && nt >= nrn_threads &&
                                                          nt < (nrn_threads + nrn_nthread);
    nrn_assert(nrn_thread_not_initialized_for_nc_target);
    if (ifarg(2)) {
        double flag = *getarg(2);
        Point_process* pnt = d->target_;
        int type = pnt->prop->_type;
        if (!nrn_is_artificial_[type]) {
            hoc_execerror("Can only send fake self-events to ARTIFICIAL_CELLs", 0);
        }
        auto* pq = pnt->prop->dparam + nrn_artcell_qindex_[type];
        nrn_net_send(pq, d->weight_, pnt, td, flag);
    } else {
        net_cvode_instance->event(td, d, PP2NT(d->target_));
    }
    return (double) d->active_;
}
static double nc_record(void* v) {
    NetCon* d = (NetCon*) v;
    d->chksrc();
    if (ifarg(1)) {
        if (ifarg(2)) {
            int recid = d->obj_->index;
            if (ifarg(3)) {
                recid = (int) (*getarg(3));
            }
            d->src_->record(vector_arg(1), vector_arg(2), recid);
        } else if (hoc_is_str_arg(1)) {
            d->src_->record_stmt(gargstr(1));
        } else if (is_vector_arg(1)) {
            d->src_->record(vector_arg(1));
        } else {
            d->src_->record_stmt(*hoc_objgetarg(1));
        }
    } else {
        d->src_->record(nullptr);
    }
    return 0;
}

static double nc_srcgid(void* v) {
    NetCon* d = (NetCon*) v;
    hoc_return_type_code = 1;
    if (d->src_) {
        return (double) d->src_->gid_;
    }
    return -1.;
}

static Object** nc_get_recordvec(void* v) {
    NetCon* d = (NetCon*) v;
    Object* ob = nullptr;
    if (d->src_ && d->src_->tvec_) {
        ob = d->src_->tvec_->obj_;
    }
    return hoc_temp_objptr(ob);
}

static double nc_wcnt(void* v) {
    NetCon* d = (NetCon*) v;
    return d->cnt_;
}

static Member_func members[] = {{"active", nc_active},
                                {"valid", nc_valid},
                                {"preloc", nc_preloc},
                                {"postloc", nc_postloc},
                                {"setpost", nc_setpost},
                                {"event", nc_event},
                                {"record", nc_record},
                                {"srcgid", nc_srcgid},
                                {"wcnt", nc_wcnt},
                                {"delay", 0},  // these four changed below
                                {"weight", 0},
                                {"threshold", 0},
                                {"x", 0},
                                {0, 0}};

static Member_ret_obj_func omembers[] = {{"syn", nc_syn},
                                         {"pre", nc_pre},
                                         {"precell", nc_precell},
                                         {"postcell", nc_postcell},
                                         {"preseg", nc_preseg},
                                         {"postseg", nc_postseg},
                                         {"prelist", nc_prelist},
                                         {"synlist", nc_synlist},
                                         {"precelllist", nc_precelllist},
                                         {"postcelllist", nc_postcelllist},
                                         {"get_recordvec", nc_get_recordvec},
                                         {0, 0}};

static void steer_val(void* v) {
    NetCon* d = (NetCon*) v;
    Symbol* s = hoc_spop();
    if (strcmp(s->name, "delay") == 0) {
        d->chksrc();
        hoc_pushpx(&d->delay_);
        d->src_->use_min_delay_ = 0;
    } else if (strcmp(s->name, "weight") == 0) {
        int index = 0;
        if (hoc_stack_type_is_ndim()) {
            s->arayinfo->sub[0] = d->cnt_;
            index = hoc_araypt(s, SYMBOL);
        }
        hoc_pushpx(d->weight_ + index);
    } else if (strcmp(s->name, "x") == 0) {
        static double dummy = 0.;
        d->chksrc();
        if (d->src_->thvar_) {
            hoc_push(d->src_->thvar_);
        } else {
            dummy = 0.;
            hoc_pushpx(&dummy);
        }
    } else if (strcmp(s->name, "threshold") == 0) {
        d->chksrc();
        hoc_pushpx(&d->src_->threshold_);
    }
}

static void* cons(Object* o) {
    NetCon* d;
    if (!net_cvode_instance) {
        hoc_execerror("CVode instance must exist", 0);
    }
    // source, target, threshold, delay, magnitude
    Object *osrc = nullptr, *otar;
    Section* srcsec = nullptr;
    neuron::container::data_handle<double> psrc{};
    if (hoc_is_object_arg(1)) {
        osrc = *hoc_objgetarg(1);
        if (osrc && !is_point_process(osrc)) {
            hoc_execerror("if arg 1 is an object it must be a point process or NULLObject", 0);
        }
    } else {
        psrc = hoc_hgetarg<double>(1);
        srcsec = chk_access();
    }
    otar = *hoc_objgetarg(2);
    if (otar && !is_point_process(otar)) {
        hoc_execerror("arg 2 must be a point process or NULLobject", 0);
    }
    double thresh = -1.e9;  // sentinal value. default is 10 if new PreSyn
    double delay = 1.;
    double weight = 0.;

    if (ifarg(3)) {
        thresh = *getarg(3);
        delay = chkarg(4, 0, 1e15);
        weight = *getarg(5);
    }
    d = net_cvode_instance->install_deliver(psrc, srcsec, osrc, otar, thresh, delay, weight);
    d->obj_ = o;
    return (void*) d;
}

static void destruct(void* v) {
    delete static_cast<NetCon*>(v);
}

void NetCon_reg() {
    class2oc("NetCon", cons, destruct, members, NULL, omembers, NULL);
    Symbol* nc = hoc_lookup("NetCon");
    nc->u.ctemplate->steer = steer_val;
    Symbol* s;
    s = hoc_table_lookup("delay", nc->u.ctemplate->symtable);
    s->type = VAR;
    s->arayinfo = nullptr;
    s = hoc_table_lookup("x", nc->u.ctemplate->symtable);
    s->type = VAR;
    s->arayinfo = nullptr;
    s = hoc_table_lookup("threshold", nc->u.ctemplate->symtable);
    s->type = VAR;
    s->arayinfo = nullptr;
    s = hoc_table_lookup("weight", nc->u.ctemplate->symtable);
    s->type = VAR;
    s->arayinfo = new Arrayinfo;
    s->arayinfo->refcount = 1;
    s->arayinfo->a_varn = nullptr;
    s->arayinfo->nsub = 1;
    s->arayinfo->sub[0] = 1;
}

static char* escape_bracket(const char* s) {
    static char* b;
    const char* p1;
    char* p2;
    if (!b) {
        b = new char[256];
    }
    for (p1 = s, p2 = b; *p1; ++p1, ++p2) {
        switch (*p1) {
        case '<':
            *p2 = '[';
            break;
        case '>':
            *p2 = ']';
            break;
        case '[':
        case ']':
            *p2 = '\\';
            *(++p2) = *p1;
            break;
        default:
            *p2 = *p1;
            break;
        }
    }
    *p2 = '\0';
    return b;
}

Object** NetCvode::netconlist() {
    // interface to cvode.netconlist(precell, postcell, target, [list])
    OcList* o;

    Object** po = newoclist(4, o);

    Object *opre = nullptr, *opost = nullptr, *otar = nullptr;
    std::regex spre, spost, star;

    if (hoc_is_object_arg(1)) {
        opre = *hoc_objgetarg(1);
    } else {
        std::string s(gargstr(1));
        if (s.empty()) {
            spre = std::regex(".*");
        } else {
            try {
                spre = std::regex(escape_bracket(s.data()));
            } catch (std::regex_error&) {
                hoc_execerror(gargstr(1), "not a valid regular expression");
            }
        }
    }
    if (hoc_is_object_arg(2)) {
        opost = *hoc_objgetarg(2);
    } else {
        std::string s(gargstr(2));
        if (s.empty()) {
            spost = std::regex(".*");
        } else {
            try {
                spost = std::regex(escape_bracket(s.data()));
            } catch (std::regex_error&) {
                hoc_execerror(gargstr(2), "not a valid regular expression");
            }
        }
    }
    if (hoc_is_object_arg(3)) {
        otar = *hoc_objgetarg(3);
    } else {
        std::string s(gargstr(3));
        if (s.empty()) {
            star = std::regex(".*");
        } else {
            try {
                star = std::regex(escape_bracket(s.data()));
            } catch (std::regex_error&) {
                hoc_execerror(gargstr(3), "not a valid regular expression");
            }
        }
    }

    hoc_Item* q;
    if (psl_) {
        ITERATE(q, psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            bool b = false;
            if (ps->ssrc_) {
                Object* precell = nrn_sec2cell(ps->ssrc_);
                if (opre) {
                    b = precell == opre;
                } else {
                    std::string s(hoc_object_name(precell));
                    b = std::regex_search(s, spre);
                }
            } else if (ps->osrc_) {
                Object* presyn = ps->osrc_;
                if (opre) {
                    b = presyn == opre;
                } else {
                    std::string s(hoc_object_name(presyn));
                    b = std::regex_search(s, spre);
                }
            }
            if (b) {
                for (const auto& d: ps->dil_) {
                    Object* postcell = nullptr;
                    Object* target = nullptr;
                    if (d->target_) {
                        Point_process* p = d->target_;
                        target = p->ob;
                        if (p->sec) {
                            postcell = nrn_sec2cell(p->sec);
                        }
                    }
                    if (opost) {
                        b = postcell == opost;
                    } else {
                        std::string s(hoc_object_name(postcell));
                        b = std::regex_search(s, spost);
                    }
                    if (b) {
                        if (otar) {
                            b = target == otar;
                        } else {
                            std::string s(hoc_object_name(target));
                            b = std::regex_search(s, star);
                        }
                        if (b) {
                            o->append(d->obj_);
                        }
                    }
                }
            }
        }
    }
    return po;
}

#define ITE_SIZE 10
NetCvodeThreadData::NetCvodeThreadData() {
    tpool_ = new TQItemPool(1000, 1);
    // tqe_ accessed only by thread i so no locking
    tqe_ = new TQueue(tpool_, 0);
    sepool_ = new SelfEventPool(1000, 1);
    selfqueue_ = nullptr;
    psl_thr_ = nullptr;
    tq_ = nullptr;
    lcv_ = nullptr;
    ite_size_ = ITE_SIZE;
    ite_cnt_ = 0;
    unreffed_event_cnt_ = 0;
    immediate_deliver_ = -1e100;
    inter_thread_events_ = new InterThreadEvent[ite_size_];
    nlcv_ = 0;
    MUTCONSTRUCT(1)
}

NetCvodeThreadData::~NetCvodeThreadData() {
    delete[] std::exchange(inter_thread_events_, nullptr);
    if (psl_thr_) {
        hoc_l_freelist(&psl_thr_);
    }
    delete std::exchange(tq_, nullptr);
    delete std::exchange(tqe_, nullptr);
    delete std::exchange(tpool_, nullptr);
    if (selfqueue_) {
        selfqueue_->remove_all();
        delete std::exchange(selfqueue_, nullptr);
    }
    delete std::exchange(sepool_, nullptr);
    if (lcv_) {
        for (int i = 0; i < nlcv_; ++i) {
            net_cvode_instance->delete_list(lcv_ + i);
        }
        delete[] std::exchange(lcv_, nullptr);
    }
    MUTDESTRUCT
}

void NetCvodeThreadData::interthread_send(double td, DiscreteEvent* db, NrnThread* nt) {
    // bin_event(td, db, nt);
    MUTLOCK
#if PRINT_EVENT
    if (net_cvode_instance->print_event_) {
        Printf("interthread send td=%.15g DE type=%d thread=%d target=%d %s\n",
               td,
               db->type(),
               nt->id,
               (db->type() == 2) ? PP2NT(((NetCon*) db)->target_)->id : -1,
               (db->type() == 2) ? hoc_object_name(((NetCon*) (db))->target_->ob) : "?");
    }
#endif
    if (ite_cnt_ >= ite_size_) {
        ite_size_ *= 2;
        InterThreadEvent* in = new InterThreadEvent[ite_size_];
        for (int i = 0; i < ite_cnt_; ++i) {
            in[i].de_ = inter_thread_events_[i].de_;
            in[i].t_ = inter_thread_events_[i].t_;
        }
        delete[] std::exchange(inter_thread_events_, in);
    }
    InterThreadEvent& ite = inter_thread_events_[ite_cnt_++];
    ite.de_ = db;
    ite.t_ = td;
    // race since each NetCvodeThreadData has its own lock and enqueueing_
    // is a NetCvode instance variable. enqueuing_ is not logically
    // needed but can avoid a nrn_multithread_job call in allthread_least_t
    // which does nothing if there are no interthread events.
    // int& b = net_cvode_instance->enqueueing_;
    // if (!b) { b = 1; }
    MUTUNLOCK
    // have decided to lock net_cvode_instance and set it
    net_cvode_instance->set_enqueueing();
}

void NetCvodeThreadData::enqueue(NetCvode* nc, NrnThread* nt) {
    int i;
    MUTLOCK
    for (i = 0; i < ite_cnt_; ++i) {
        InterThreadEvent& ite = inter_thread_events_[i];
#if PRINT_EVENT
        if (net_cvode_instance->print_event_) {
            Printf("interthread enqueue td=%.15g DE type=%d thread=%d target=%d %s\n",
                   ite.t_,
                   ite.de_->type(),
                   nt->id,
                   (ite.de_->type() == 2) ? PP2NT(((NetCon*) (ite.de_))->target_)->id : -1,
                   (ite.de_->type() == 2) ? hoc_object_name(((NetCon*) (ite.de_))->target_->ob)
                                          : "?");
        }
#endif
        nc->bin_event(ite.t_, ite.de_, nt);
    }
    ite_cnt_ = 0;
    MUTUNLOCK
}

NetCvode::NetCvode(bool single) {
    use_long_double_ = 0;
    empty_ = true;  // no equations (only artificial cells).
    MUTCONSTRUCT(0);
    maxorder_ = 5;
    maxstep_ = 1e9;
    minstep_ = 0.;
    rtol_ = 0.;
    atol_ = 1e-3;
    jacobian_ = 0;
    stiff_ = 2;
    mst_ = nullptr;
    condition_order_ = 1;
    null_event_ = new DiscreteEvent();
    eps_ = 100. * UNIT_ROUNDOFF;
    print_event_ = 0;
    nrn_use_fifo_queue_ = false;
    single_ = single;
    nrn_use_daspk_ = false;
    gcv_ = nullptr;
    allthread_hocevents_ = new HocEventList();
    pcnt_ = 0;
    p = nullptr;
    p_construct(1);
    // eventually these should not have to be thread safe
    pst_ = nullptr;
    pst_cnt_ = 0;
    psl_ = nullptr;
    // for parallel network simulations hardly any presyns have
    // a threshold and it can be very inefficient to check the entire
    // presyn list for thresholds during the fixed step method.
    // So keep a threshold list.
    unused_presyn = nullptr;
    structure_change_cnt_ = -1;
    fornetcon_change_cnt_ = -2;
    matrix_change_cnt_ = -1;
    playrec_change_cnt_ = 0;
    alloc_list();
    prl_ = new std::vector<PlayRecord*>();
    prl_->reserve(10);
    fixed_play_ = new std::vector<PlayRecord*>();
    fixed_play_->reserve(10);
    fixed_record_ = new std::vector<PlayRecord*>();
    fixed_record_->reserve(10);
    vec_event_store_ = nullptr;
    if (!record_init_items_) {
        record_init_items_ = new TQList();
    }
    //	re_init(t);
}

NetCvode::~NetCvode() {
    MUTDESTRUCT
    if (net_cvode_instance == (NetCvode*) this) {
        net_cvode_instance = nullptr;
    }
    delete_list();
    p_construct(0);
    // and should also iterate and delete the MaxStateItem
    delete std::exchange(mst_, nullptr);
    if (psl_) {
        hoc_Item* q;
        ITERATE(q, psl_) {
            auto* const ps = static_cast<PreSyn*>(VOIDITM(q));
            std::for_each(ps->dil_.rbegin(), ps->dil_.rend(), [](NetCon*& d) {
                d->src_ = nullptr;
                delete std::exchange(d, nullptr);
            });
            delete ps;
            VOIDITM(q) = nullptr;
        }
        hoc_l_freelist(&psl_);
    }
    delete std::exchange(pst_, nullptr);
    delete std::exchange(fixed_play_, nullptr);
    delete std::exchange(fixed_record_, nullptr);
    for (auto& item: *prl_) {
        delete item;
    }
    delete std::exchange(prl_, nullptr);
    unused_presyn = nullptr;
    wl_list_.clear();
    delete std::exchange(allthread_hocevents_, nullptr);
}

bool NetCvode::localstep() {
    return !single_;
}

bool NetCvode::is_local() {
    return (cvode_active_ && localstep());
}

void NetCvode::localstep(bool b) {
    // due to possibility of gap junctions and until the complete matrix
    // is analysed for block structure localstep and daspk are incompatible
    b = (nrn_modeltype() == 1 ? b : false);  // localstep doesn't work yet with DAE's

    if (!b != single_) {
        delete_list();
        single_ = !b;
        structure_change_cnt_ = 0;
        use_sparse13 = 0;
        nrn_use_daspk_ = false;
        re_init(nt_t);
    }
}

bool NetCvode::use_daspk() {
    return (gcv_ != 0) ? gcv_->use_daspk_ : false;
}

void NetCvode::use_daspk(bool b) {
    b = (nrn_modeltype() == 2 ? true : b);  // not optional if algebraic
    if (gcv_ && b != gcv_->use_daspk_) {
        delete_list();
        single_ = (b ? true : single_);
        structure_change_cnt_ = 0;
        nrn_use_daspk_ = b;
        // printf("NetCvode::use_daspk nrn_use_daspk=%d\n", nrn_use_daspk_);
        if (use_sparse13 != nrn_use_daspk_) {
            use_sparse13 = nrn_use_daspk_;
            diam_changed = 1;
        }
        re_init(nt_t);
    }
}

// Append new BAMechList item to arg
BAMechList::BAMechList(BAMechList** first) {  // preserve the list order
    next = nullptr;
    BAMechList* last;
    if (*first) {
        for (last = *first; last->next; last = last->next) {
        }
        last->next = this;
    } else {
        *first = this;
    }
}

void BAMechList::destruct(BAMechList** first) {
    BAMechList *b, *bn;
    for (b = *first; b; b = bn) {
        bn = b->next;
        delete b;
    }
    *first = nullptr;
}

CvodeThreadData::CvodeThreadData() {
    no_cap_count_ = 0;
    no_cap_child_count_ = 0;
    no_cap_node_ = nullptr;
    no_cap_child_ = nullptr;
    cv_memb_list_ = nullptr;
    cmlcap_ = nullptr;
    cmlext_ = nullptr;
    no_cap_memb_ = nullptr;
    before_breakpoint_ = nullptr;
    after_solve_ = nullptr;
    before_step_ = nullptr;
    rootnodecount_ = 0;
    v_node_count_ = 0;
    v_node_ = nullptr;
    v_parent_ = nullptr;
    psl_th_ = nullptr;
    watch_list_ = nullptr;
    nvoffset_ = 0;
    nvsize_ = 0;
    neq_v_ = nonvint_offset_ = 0;
    nonvint_extra_offset_ = 0;
    record_ = nullptr;
    play_ = nullptr;
}
CvodeThreadData::~CvodeThreadData() {
    if (no_cap_memb_) {
        delete_memb_list(no_cap_memb_);
    }
    if (no_cap_node_) {
        delete[] no_cap_node_;
        delete[] no_cap_child_;
    }
    if (watch_list_) {
        watch_list_->RemoveAll();
        delete watch_list_;
    }
}

void NetCvode::delete_list() {
    int i, j;
    wl_list_.clear();
    wl_list_.resize(nrn_nthread);
    if (gcv_) {
        delete_list(gcv_);
        delete std::exchange(gcv_, nullptr);
    }
    for (i = 0; i < pcnt_; ++i) {
        NetCvodeThreadData& d = p[i];
        if (d.lcv_) {
            for (j = 0; j < d.nlcv_; ++j) {
                delete_list(d.lcv_ + j);
            }
            delete[] std::exchange(d.lcv_, nullptr);
            d.nlcv_ = 0;
        }
        delete std::exchange(d.tq_, nullptr);
    }
    empty_ = true;
}

void NetCvode::delete_list(Cvode* cvode) {
    del_cv_memb_list(cvode);
    Cvode& cv = *cvode;
    cv.delete_prl();
    delete[] std::exchange(cv.ctd_, nullptr);
}

void NetCvode::del_cv_memb_list() {
    if (gcv_) {
        del_cv_memb_list(gcv_);
    }
    for (int i = 0; i < pcnt_; ++i) {
        NetCvodeThreadData& d = p[i];
        for (int j = 0; j < d.nlcv_; ++j) {
            del_cv_memb_list(d.lcv_ + j);
        }
    }
}
void NetCvode::del_cv_memb_list(Cvode* cvode) {
    if (!cvode) {
        return;
    }
    Cvode& cv = *cvode;
    for (int j = 0; j < cv.nctd_; ++j) {
        CvodeThreadData& z = cv.ctd_[j];
        if (z.psl_th_) {
            z.psl_th_->clear();
            delete std::exchange(z.psl_th_, nullptr);
        }
        if (cvode != gcv_) {
            if (z.v_node_) {
                delete[] std::exchange(z.v_node_, nullptr);
                delete[] std::exchange(z.v_parent_, nullptr);
            }
            z.delete_memb_list(std::exchange(z.cv_memb_list_, nullptr));
        } else {
            CvMembList *cml, *cmlnext;
            for (cml = std::exchange(z.cv_memb_list_, nullptr); cml; cml = cmlnext) {
                cmlnext = cml->next;
                delete cml;
            }
        }
        BAMechList::destruct(&z.before_breakpoint_);
        BAMechList::destruct(&z.after_solve_);
        BAMechList::destruct(&z.before_step_);
    }
}

void CvodeThreadData::delete_memb_list(CvMembList* cmlist) {
    CvMembList *cml, *cmlnext;
    for (cml = cmlist; cml; cml = cmlnext) {
        auto const& ml = cml->ml;
        cmlnext = cml->next;
        for (auto& ml: cml->ml) {
            delete[] std::exchange(ml.nodelist, nullptr);
            delete[] std::exchange(ml.nodeindices, nullptr);
            delete[] std::exchange(ml.prop, nullptr);
            if (!memb_func[cml->index].hoc_mech) {
                delete[] std::exchange(ml.pdata, nullptr);
            }
        }
        delete cml;
    }
}

void NetCvode::distribute_dinfo(int* cellnum, int tid) {
    int j;
    // printf("distribute_dinfo %d\n", pst_cnt_);
    if (psl_) {
        hoc_Item* q;
        ITERATE(q, psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            // printf("\tPreSyn %s\n", ps->osrc_ ? hoc_object_name(ps->osrc_):secname(ps->ssrc_));
            if (ps->thvar_) {  // artcells and presyns for gid's not on this cpu have no threshold
                               // check
                NrnThread* nt;
                Cvode* cvsrc;
                CvodeThreadData* z;
                // cvode instances poll which presyns
                if (single_) {
                    if (ps->osrc_) {
                        nt = (NrnThread*) ob2pntproc(ps->osrc_)->_vnt;
                    } else if (ps->ssrc_) {
                        nt = ps->ssrc_->pnode[0]->_nt;
                    } else {
                        nt = nrn_threads;
                    }
                    cvsrc = gcv_;
                    z = cvsrc->ctd_ + nt->id;
                    if (!z->psl_th_) {
                        z->psl_th_ = new PreSynList();
                        z->psl_th_->reserve(pst_cnt_);
                    }
                    z->psl_th_->push_back(ps);
                } else {
                    if (ps->osrc_) {
                        j = node(ps->osrc_)->v_node_index;
                        nt = (NrnThread*) ob2pntproc(ps->osrc_)->_vnt;
                    } else if (ps->ssrc_) {
                        j = ps->ssrc_->pnode[0]->v_node_index;
                        nt = ps->ssrc_->pnode[0]->_nt;
                    } else {
                        j = 0;
                        nt = nrn_threads;
                    }
                    if (tid == nt->id) {
                        cvsrc = p[tid].lcv_ + cellnum[j];
                        z = cvsrc->ctd_;
                        if (nt == cvsrc->nth_) {
                            if (!z->psl_th_) {
                                z->psl_th_ = new PreSynList();
                            }
                            z->psl_th_->push_back(ps);
                        }
                    }
                }
            }
        }
    }
}

void NetCvode::alloc_list() {
    int i;
    set_CVRhsFn();
    wl_list_.clear();
    wl_list_.resize(nrn_nthread);
    if (single_) {
        gcv_ = new Cvode();
        Cvode& cv = *gcv_;
        cv.ncv_ = this;
        cv.nctd_ = nrn_nthread;
        cv.ctd_ = new CvodeThreadData[cv.nctd_];
    } else {
        for (int id = 0; id < nrn_nthread; ++id) {
            NrnThread& nt = nrn_threads[id];
            NetCvodeThreadData& d = p[id];
            d.nlcv_ = nt.ncell;
            d.lcv_ = new Cvode[d.nlcv_];
            d.tq_ = new TQueue(d.tpool_);
            for (i = 0; i < d.nlcv_; ++i) {
                TQItem* ti = d.tq_->insert(0., d.lcv_ + i);
                d.lcv_[i].tqitem_ = ti;
                Cvode& cv = d.lcv_[i];
                cv.nth_ = &nt;
                cv.ncv_ = this;
                cv.nctd_ = 1;
                cv.ctd_ = new CvodeThreadData[cv.nctd_];
            }
        }
    }
    empty_ = false;
#if USENEOSIM
    if (p_nrn2neosim_send)
        for (i = 0; i < nlist_; ++i) {
            p.lcv_[i].neosim_self_events_ = new TQueue();
        }
#endif
}

bool NetCvode::init_global() {
    int i;
    CvMembList* cml;
    if (tree_changed) {
        setup_topology();
    }
    if (v_structure_change) {
        v_setup_vectors();
    }
    if (structure_change_cnt_ == structure_change_cnt) {
        return false;
    }
    if (diam_changed) {  // need to guarantee that the matrix is allocated
        recalc_diam();   // for the present method
    }
    structure_change_cnt_ = structure_change_cnt;
    matrix_change_cnt_ = -1;
    playrec_change_cnt_ = 0;
    NrnThread* _nt;
    // We copy Memb_list* into cml->ml below. At the moment this CVode code
    // generates its own complicated set of Memb_list* that operate in
    // list-of-handles mode instead of referring to contiguous sets of values.
    // This is a shame, as it forces that list-of-handles mode to exist.
    // Possible alternatives could include:
    // - making the sorting algorithm more sophisticated so that the values that
    //   are going to be processed together are contiguous -- this might be a
    //   bit intricate, but it shouldn't be *too* had to assert that we get the
    //   right answer -- and it's the only way of making sure the actual compute
    //   kernels are cache efficient / vectorisable.
    // - changing the type used by this code to not be Memb_list but rather some
    //   Memb_list_with_list_of_handles type and adding extra glue to the code
    //   generation so that we can call into translated MOD file code using that
    //   type
    // - Using a list of Memb_list with size 1 instead of a single Memb_list
    //   that holds a list of handles?
    auto const cache_token = nrn_ensure_model_data_are_sorted();
    if (single_) {
        if (!gcv_ || gcv_->nctd_ != nrn_nthread) {
            delete_list();
            alloc_list();
        }
        del_cv_memb_list();
        Cvode& cv = *gcv_;
        distribute_dinfo(nullptr, 0);
        FOR_THREADS(_nt) {
            CvodeThreadData& z = cv.ctd_[_nt->id];
            z.rootnodecount_ = _nt->ncell;
            z.v_node_count_ = _nt->end;
            z.v_node_ = _nt->_v_node;
            z.v_parent_ = _nt->_v_parent;

            CvMembList* last = 0;
            for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
                i = tml->index;
                const Memb_func& mf = memb_func[i];
                Memb_list* ml = tml->ml;
                if (ml->nodecount && (i == CAP || mf.current || mf.ode_count || mf.ode_matsol ||
                                      mf.ode_spec || mf.state)) {
                    // maintain same order (not reversed) for
                    // singly linked list built below
                    cml = new CvMembList{i};
                    if (!z.cv_memb_list_) {
                        z.cv_memb_list_ = cml;
                    } else {
                        last->next = cml;
                    }
                    last = cml;
                    cml->next = nullptr;
                    auto const mech_offset = cache_token.thread_cache(_nt->id).mechanism_offset.at(
                        i);
                    assert(mech_offset != neuron::container::invalid_row);
                    assert(cml->ml.size() == 1);
                    cml->ml[0].set_storage_offset(mech_offset);
                    cml->ml[0].nodecount = ml->nodecount;
                    // assumes cell info grouped contiguously
                    cml->ml[0].nodelist = ml->nodelist;
                    cml->ml[0].nodeindices = ml->nodeindices;
                    assert(ml->prop);
                    cml->ml[0].prop = ml->prop;  // used for ode_map even when hoc_mech = false
                    if (!mf.hoc_mech) {
                        cml->ml[0].pdata = ml->pdata;
                    }
                    cml->ml[0]._thread = ml->_thread;
                }
            }
            fill_global_ba(_nt, BEFORE_BREAKPOINT, &z.before_breakpoint_);
            fill_global_ba(_nt, AFTER_SOLVE, &z.after_solve_);
            fill_global_ba(_nt, BEFORE_STEP, &z.before_step_);
            // Every point process, but not artificial cells, cause at least a retreat.
            // All point processes, but not artificial cells,
            // have the global cvode as its nvi field
            for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
                i = tml->index;
                const Memb_func& mf = memb_func[i];
                if (mf.is_point && !nrn_is_artificial_[i]) {
                    Memb_list* ml = tml->ml;
                    int j;
                    for (j = 0; j < ml->nodecount; ++j) {
                        auto& datum = mf.hoc_mech ? ml->prop[j]->dparam[1] : ml->pdata[j][1];
                        auto* pp = datum.get<Point_process*>();
                        pp->nvi_ = gcv_;
                    }
                }
            }
        }
    } else {  // lvardt
        bool b = false;
        if (gcv_) {
            b = true;
        }
        if (!b)
            for (i = 0; i < pcnt_; ++i) {
                if (p[i].nlcv_ != nrn_threads[i].ncell) {
                    b = true;
                }
            }
        if (b) {
            delete_list();
            alloc_list();
        }
        del_cv_memb_list();
        // each node has a cell number
        for (int id = 0; id < nrn_nthread; ++id) {
            NrnThread* _nt = nrn_threads + id;
            NetCvodeThreadData& d = p[id];
            if (_nt->end == 0) {
                continue;
            }
            std::vector<int> cellnum(_nt->end);
            for (i = 0; i < _nt->ncell; ++i) {
                cellnum[i] = i;
            }
            for (i = _nt->ncell; i < _nt->end; ++i) {
                cellnum[i] = cellnum[_nt->_v_parent[i]->v_node_index];
            }

            for (i = 0; i < _nt->ncell; ++i) {
                d.lcv_[i].ctd_[0].v_node_count_ = 0;
            }
            for (i = 0; i < _nt->end; ++i) {
                ++d.lcv_[cellnum[i]].ctd_[0].v_node_count_;
            }
            for (i = 0; i < _nt->ncell; ++i) {
                d.lcv_[cellnum[i]].ctd_[0].v_node_ =
                    new Node*[d.lcv_[cellnum[i]].ctd_[0].v_node_count_];
                d.lcv_[cellnum[i]].ctd_[0].v_parent_ =
                    new Node*[d.lcv_[cellnum[i]].ctd_[0].v_node_count_];
            }
            for (i = 0; i < _nt->ncell; ++i) {
                d.lcv_[i].ctd_[0].v_node_count_ = 0;
                d.lcv_[i].ctd_[0].rootnodecount_ = 1;
            }
            for (i = 0; i < _nt->end; ++i) {
                d.lcv_[cellnum[i]].ctd_[0].v_node_[d.lcv_[cellnum[i]].ctd_[0].v_node_count_] =
                    _nt->_v_node[i];
                d.lcv_[cellnum[i]].ctd_[0].v_parent_[d.lcv_[cellnum[i]].ctd_[0].v_node_count_++] =
                    _nt->_v_parent[i];
            }
            // divide the memb_list info into per cell info
            // count
            std::vector<CvMembList*> last(_nt->ncell);

            // Need to determine if a Memb_list needs to be included
            // in CvMembList because there is a BEFORE/AFTER
            // statement that needs to be handled.
            std::unordered_set<int> ba_candidate;
            {
                constexpr std::array batypes{BEFORE_STEP, BEFORE_BREAKPOINT, AFTER_SOLVE};
                for (auto const bat: batypes) {
                    for (BAMech* bam = bamech_[bat]; bam; bam = bam->next) {
                        ba_candidate.insert(bam->type);
                    }
                }
            }

            for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
                i = tml->index;
                const Memb_func& mf = memb_func[i];
                Memb_list* ml = tml->ml;
                if (ml->nodecount && (mf.current || mf.ode_count || mf.ode_matsol || mf.ode_spec ||
                                      mf.state || i == CAP || ba_candidate.count(i) == 1)) {
                    // maintain same order (not reversed) for
                    // singly linked list built below
                    int j;
                    for (j = 0; j < ml->nodecount; ++j) {
                        int inode = ml->nodelist[j]->v_node_index;
                        Cvode& cv = d.lcv_[cellnum[inode]];
                        CvodeThreadData& z = cv.ctd_[0];
                        if (!z.cv_memb_list_) {
                            cml = new CvMembList{i};
                            cml->next = nullptr;
                            assert(cml->ml.size() == 1);
                            cml->ml[0].nodecount = 0;
                            z.cv_memb_list_ = cml;
                            last[cellnum[inode]] = cml;
                        }
                        if (last[cellnum[inode]]->index == i) {
                            assert(last[cellnum[inode]]->ml.size() == 1);
                            ++last[cellnum[inode]]->ml[0].nodecount;
                        } else {
                            cml = new CvMembList{i};
                            last[cellnum[inode]]->next = cml;
                            cml->next = nullptr;
                            last[cellnum[inode]] = cml;
                            assert(cml->ml.size() == 1);
                            cml->ml[0].nodecount = 1;
                        }
                    }
                }
            }
            // allocate and re-initialize count
            std::vector<CvMembList*> cvml(d.nlcv_);
            for (i = 0; i < d.nlcv_; ++i) {
                cvml[i] = d.lcv_[i].ctd_[0].cv_memb_list_;
                for (cml = cvml[i]; cml; cml = cml->next) {
                    // non-contiguous mode, so we're going to create a lot of 1-element Memb_list
                    // inside cml->ml
                    cml->ml.reserve(cml->ml[0].nodecount);
                    // remove the single entry from contiguous mode
                    cml->ml.clear();
                }
            }
            // fill pointers (and nodecount)
            // now list order is from 0 to n_memb_func
            for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
                i = tml->index;
                const Memb_func& mf = memb_func[i];
                Memb_list* ml = tml->ml;
                if (ml->nodecount && (mf.current || mf.ode_count || mf.ode_matsol || mf.ode_spec ||
                                      mf.state || i == CAP || ba_candidate.count(i) == 1)) {
                    for (int j = 0; j < ml->nodecount; ++j) {
                        int icell = cellnum[ml->nodelist[j]->v_node_index];
                        if (cvml[icell]->index != i) {
                            cvml[icell] = cvml[icell]->next;
                            assert(cvml[icell] && cvml[icell]->index);
                        }
                        cml = cvml[icell];
                        auto& newml = cml->ml.emplace_back(cml->index /* mechanism type */);
                        newml.nodecount = 1;
                        newml.nodelist = new Node*[1];
                        newml.nodelist[0] = ml->nodelist[j];
                        newml.nodeindices = new int[1]{ml->nodeindices[j]};
                        newml.prop = new Prop* [1] { ml->prop[j] };
                        if (!mf.hoc_mech) {
                            newml.set_storage_offset(ml->get_storage_offset() + j);
                            newml.pdata = new Datum* [1] { ml->pdata[j] };
                        }
                        newml._thread = ml->_thread;
                    }
                }
            }
            // do the above for the BEFORE/AFTER functions
            fill_local_ba(cellnum.data(), d);

            distribute_dinfo(cellnum.data(), id);
            // If a point process is not an artificial cell, fill its nvi_ field.
            // artifical cells have no integrator
            for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next) {
                i = tml->index;
                const Memb_func& mf = memb_func[i];
                if (mf.is_point) {
                    Memb_list* ml = tml->ml;
                    int j;
                    for (j = 0; j < ml->nodecount; ++j) {
                        auto& datum = mf.hoc_mech ? ml->prop[j]->dparam[1] : ml->pdata[j][1];
                        auto* pp = datum.get<Point_process*>();
                        if (nrn_is_artificial_[i] == 0) {
                            int inode = ml->nodelist[j]->v_node_index;
                            pp->nvi_ = d.lcv_ + cellnum[inode];
                        } else {
                            pp->nvi_ = nullptr;
                        }
                    }
                }
            }
        }
    }
    return true;
}

void NetCvode::fill_global_ba(NrnThread* nt, int bat, BAMechList** baml) {
    NrnThreadBAList* tbl;
    for (tbl = nt->tbl[bat]; tbl; tbl = tbl->next) {
        BAMechList* ba = new BAMechList(baml);
        ba->bam = tbl->bam;
        ba->ml.push_back(tbl->ml);
    }
}

void NetCvode::fill_local_ba(int* celnum, NetCvodeThreadData& d) {
    fill_local_ba_cnt(BEFORE_BREAKPOINT, celnum, d);
    fill_local_ba_cnt(AFTER_SOLVE, celnum, d);
    fill_local_ba_cnt(BEFORE_STEP, celnum, d);
}

void NetCvode::fill_local_ba_cnt(int bat, int* celnum, NetCvodeThreadData& d) {
    BAMech* bam;
    for (bam = bamech_[bat]; bam; bam = bam->next) {
        for (int icv = 0; icv < d.nlcv_; ++icv) {
            Cvode* cv = d.lcv_ + icv;
            assert(cv->nctd_ == 1);
            for (CvMembList* cml = cv->ctd_[0].cv_memb_list_; cml; cml = cml->next) {
                if (cml->index == bam->type) {
                    BAMechList* bl = cvbml(bat, bam, cv);
                    bl->bam = bam;
                    for (auto& ml: cml->ml) {
                        bl->ml.push_back(&ml);
                    }
                }
            }
        }
    }
}

BAMechList* NetCvode::cvbml(int bat, BAMech* bam, Cvode* cv) {
    BAMechList** pbml;
    BAMechList* ba;
    if (bat == BEFORE_BREAKPOINT) {
        pbml = &cv->ctd_->before_breakpoint_;
    } else if (bat == AFTER_SOLVE) {
        pbml = &cv->ctd_->after_solve_;
    } else {
        pbml = &cv->ctd_->before_step_;
    }
    if (!*pbml) {
        ba = new BAMechList(pbml);
    } else {
        for (ba = *pbml; ba; ba = ba->next) {
            if (ba->bam->type == bam->type) {
                return ba;
            }
        }
        ba = new BAMechList(pbml);
    }
    ba->bam = bam;
    return ba;
}

/*
The path through NetCvode::solve path is determined by the case dimensions of
(single step, integrate to tout) and  (no model, global dt, localdt).
For threads there is one more constraint--- do not allow t0_ to pass
by the minimum interthread netcon delay integration interval barrier
without making sure all the interthread events to be delivered before
that barrier are on the thread queue. For classical mpi spike exchange
this was ensured by a NetParEvent which provided a synchronization barrier.
We generalized this to a NetParEvent per thread and a thread barrier but
there was a consequent loss of the possibility of sequential threads since
in that context a barrier makes no sense (each job must run to completion).
Hence the desire to avoid barriers and handle the interval barrier explicity
here to make sure NetCvode::enqueue is called frequently enough.
(Actually, it is called very frequently since it is called by deliver_events,
so it is only required that the interval between the least and greatest
integrator t0_ be less than the maximum).

Without a thread barrier in NetParEvent,
this mostly affects the "integrate to tout" case. But it also affects
the single step case in that we are not allowed to handle any
events with delivery time > barrier time. Since a microstep does the
earliest next thing, we need to check that before the call.

Can we deal with multisplit conveniently without a thread barrier.
Yes if we split up the process into its three phases of triangularization,
reduced tree solve, back substitution. No if we retain a single
integration step quantum for the thread.
Without barriers, we end up with finer and finer grain nrn_multithread_job.

It is sounding more and more reasonable to sacrifice the debugging convenience
of sequential threads in favor of allowing thread barriers.
...but... a look at the nrn_fixed_step and nrn_multisplit_solve shows
that it would certainly be easy to split into the proper
nrn_multithread_job groups since it is already functionally decomposed
in that way. And we may want to limit NetParEvent to serve only as
interprocessor spike exchange signals for the main thread.
...but... a look at cvode in a multisplit context
means there must be one global cvode instance
and when f and jacob is called it would divide there and do the scatter
gather etc. Of course, with separate cvode instances with multisplit we
still have to have a proper Vector library which does the corresponding
mpi reduce functions. Conceptually much simpler with barrier and maybe
impossible without. If we went with one global cvode it would be
possible. And cvode overhead seems independent of number of equations
so confining it to the main thread may not be bad.

The principle multithread domain we design for below is the few event,
multisplit case. That is, there is one cvode instance managed by thread 0
and all events go onto one queue. However, an event is delivered according
to its proper delivery thread.  Note that the possiblity of 0 delay
events is allowed. There is nothing that can be done at the minimum
delay integration interval level since each step divides into a dozen or so
multithread jobs.

Threads and lvardt.
Consider the global cv (used to be p.lcv_[0]) as gcv_ which has ctd[nrn_nthread]
instances of separate CvodeThreadData and works with multisplit.
Now consider the lvardt method (used to be supported by p.lcv_[i]) as
NetCvodeThreadData instances of lcv_[i] and of course move tq_ into
the NetCvodeThreadData. Then each of the p.lcv_[i] would have Cvode.nctd_
equal 1 in which p.lcv_[i].ctd_[0] is the proper CvodeThreadData. Then
Cvode would never look at nrn_nthread to determine the CvodeThreadData
but only the Cvode.nctd_. And if Cvode.nctd_ > 1 which would be
possible only for gcv_, only then would nt->id be used to get the proper
CvodeThreadData.  We presume for now that if lvardt is used with nrn_nthread > 1
then the usual minimum delay interval requirements hold for interthread
events.

Inter-Thread-Events (possibility of 0 delay).

The fixed step method is that for each step, the sequence of actions
for each thread is:
1) at time tbegin: check thresholds, if source and target on same thread
   put directly onto queue. If different threads (PreSyn only), do an
   interthread send which puts the event into the target thread
   inter_thread_events list. Finally we deliver all events up to tbegin+.5dt,
   this first transfers all the threads inter-thread-events onto the queue.
2) at time tbegin + .5dt: v(tbegin) -> v(tend)
3) at time tend: integrate states one step using v(tend)
4) deliver events up to but not past tend (first transfers inter-thread-events
   onto queue).
Therefore, a zero-delay inter-thread-event from an artificial cell
generated in step 4 may or may not be delivered
in the time step it is generated. And zero-delay threshold detection interthread
events will be delivered in the time step but either in step 1 or step 4 (ie.
beginning or end of the time step).  For this reason, if there are zero-delay
NetCon, then both the source and target should be on the same thread. In practice
we should force them onto thread 0 or at least the source onto the
target thread.

What about cvode?
Global variable time step method.
As mentioned above, all events, when global cvode active, go onto the thread 0 event
queue (interthread via the thread 0 inter_thread_events list)
and from there are delivered in the context of the proper thread
for cache efficiency (by means of nrn_onethread_job.) Again, as mentioned,
the design side is for few events. With this in mind, there does not seem
any impediment to 0 delay events. But note that an event with source and
target on thread 2 (e.g. a self event) goes onto the thread 0 inter_thread_events
list (in the context of thread 2), then in the context of thread 0 is transferred
from the thread 0 inter_thread_events list to the thread 0 queue, then
in the context of thread 0, the event is taken off the queue and delivered
in the context of thread 2 (by means of nrn_onethread_job). (See why we
are thinking few events? A bothersome aspect is that after every event
delivery least_t has to enqueue onto the thread 0 queue the nthread
inter_thread_events lists.
The one queue idea for global variable time step is unsound because of the
problem that multiple threads call the queue methods and so cause excessive
cache line sharing. And if we avoided this through the use of the
inter_thread_events list then consider a 0 delay NetCon
event between artificial cells on thread 2 and thread 3.
thread 0 in deliver_events_when_threads, least event for thread 2
thread 2 (NET_RECEIVE -> net_send -> PreSyn::send -> p[3].interthead_send
thread 0 enqueue onto thread 0 queue all inter_thread_events lists
thread 0 least event for thread 3
thread 3 (NET_RECEIVE ...)
Actually, there are real problems. E.g ConditionEvent::condition is
in the context of some thread and must remove an event on the thread 0 queue.
That is a recipe for horrendous cacheline sharing and is the whole reason
we designed inter_thread_events in the first place. Also the call to
enqueue_thread0() is multiplying alarmingly (every call before least_t())
and the reason for the privileged thread 0 queue has more or less disappeared.
Instead we only need an effective reduce to find the minimum of the least_t()
in each thread. Expensive but no more expensive than the single queue and without
the cache-line shareing. And allows 0 delay events. And keeps a kind of uniformity
between fixed step, global cvode, and the future lvardt in that each thread
has its own event queue.
Far reaching Local variable time step method change.
gcv_ if it exists, refers to the global variable time step method
and there are nthread CvodeThreadData instances in ctd_. If gcv_ is 0 and
!empty_, then each NetCvodeThreadData manages an array of nlcv_
Cvode instances in the lcv_ array. Each Cvode instance has only one ctd_
Thus for global step the CvodeThreadData for a thread is gcv_->ctd_[i] whereas
for lvardt it is p[i].lcv_[jcell_in_thread_i].ctd_[0].
*/

int NetCvode::solve(double tout) {
    if (nrn_nthread > 1) {
        return solve_when_threads(tout);  // more or less a copy of below
    }
    NrnThread* nt = nrn_threads;
    int err = NVI_SUCCESS;
    if (empty_) {
        if (tout >= 0.) {
            while (p[0].tqe_->least_t() <= tout && stoprun == 0) {
                deliver_least_event(nt);
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
            }
            if (stoprun == 0) {
                nt_t = tout;
            }
        } else {
            if (p[0].tqe_->least()) {
                nt_t = p[0].tqe_->least_t();
                deliver_events(p[0].tqe_->least_t(), nt);
            } else {
                nt_t += 1e6;
            }
            if (nrn_allthread_handle) {
                (*nrn_allthread_handle)();
            }
        }
    } else if (single_) {
        if (tout >= 0.) {
            while (gcv_->t_ < tout || p[0].tqe_->least_t() < tout) {
                err = global_microstep();
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
                if (err != NVI_SUCCESS || stoprun) {
                    return err;
                }
            }
            retreat(tout, gcv_);
            gcv_->record_continuous();
        } else {
            // advance or initialized
            double tc = gcv_->t_;
            initialized_ = false;
            while (gcv_->t_ <= tc && !initialized_) {
                err = global_microstep();
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
                if (err != NVI_SUCCESS || stoprun) {
                    return err;
                }
            }
        }
    } else if (!gcv_) {  // lvardt
        auto const cache_token = nrn_ensure_model_data_are_sorted();
        if (tout >= 0.) {
            time_t rt = time(nullptr);
            //			int cnt = 0;
            TQueue* tq = p[0].tq_;
            TQueue* tqe = p[0].tqe_;
            NrnThread* nt = nrn_threads;
            while (tq->least_t() < tout || tqe->least_t() <= tout) {
                err = local_microstep(cache_token, *nt);
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
                if (err != NVI_SUCCESS || stoprun) {
                    return err;
                }
#if HAVE_IV
                IFGUI
                if (rt < time(nullptr)) {
                    //				if (++cnt > 10000) {
                    //					cnt = 0;
                    Oc oc;
                    oc.notify();
                    single_event_run();
                    rt = time(nullptr);
                }
                ENDGUI
#endif
            }
            int n = p[0].nlcv_;
            Cvode* lcv = p[0].lcv_;
            for (int i = 0; i < n; ++i) {
                local_retreat(tout, lcv + i);
                lcv[i].record_continuous();
            }
        } else {
            // an fadvance is not every microstep but
            // only when all the discontinuities at te take place or
            // tc increases.
            TQueue* tq = p[0].tq_;
            double tc = tq->least_t();
            double te = p[0].tqe_->least_t();
            while (tq->least_t() <= tc && p[0].tqe_->least_t() <= te) {
                err = local_microstep(cache_token, *nrn_threads);
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
                if (err != NVI_SUCCESS || stoprun) {
                    return err;
                }
            }
            // But make sure t is not past the least time.
            // fadvance and local step do not coexist seamlessly.
            nt_t = tq->least_t();
            if (te < nt_t) {
                nt_t = te;
            }
        }
    } else {
        nt_t += 1e9;
    }
    return err;
}

void NetCvode::deliver_least_event(NrnThread* nt) {
    TQItem* q = p[nt->id].tqe_->least();
    DiscreteEvent* de = (DiscreteEvent*) q->data_;
    double tt = q->t_;
    p[nt->id].tqe_->remove(q);
#if PRINT_EVENT
    if (print_event_) {
        de->pr("deliver", tt, this);
    }
#endif
    STATISTICS(deliver_cnt_);
    de->deliver(tt, this, nt);
}

bool NetCvode::deliver_event(double til, NrnThread* nt) {
    TQItem* q;
    if ((q = p[nt->id].tqe_->atomic_dq(til)) != 0) {
        DiscreteEvent* de = (DiscreteEvent*) q->data_;
        double tt = q->t_;
        p[nt->id].tqe_->release(q);
#if PRINT_EVENT
        if (print_event_) {
            de->pr("deliver", tt, this);
        }
#endif
        STATISTICS(deliver_cnt_);
        de->deliver(tt, this, nt);
        return true;
    } else {
        return false;
    }
}

int NetCvode::local_microstep(neuron::model_sorted_token const& sorted_token, NrnThread& ntr) {
    auto* const nt = &ntr;
    int err = NVI_SUCCESS;
    int i = nt->id;
    if (p[i].tqe_->least_t() <= p[i].tq_->least_t()) {
        deliver_least_event(nt);
    } else {
        TQItem* q = p[i].tq_->least();
        Cvode* cv = (Cvode*) q->data_;
        err = cv->handle_step(sorted_token, this, 1e100);
        p[i].tq_->move_least(cv->t_);
    }
    return err;
}

int NetCvode::global_microstep() {
    NrnThread* nt = nrn_threads;
    int err = NVI_SUCCESS;
    double tt = p[0].tqe_->least_t();
    double tdiff = tt - gcv_->t_;
    if (tdiff <= 0) {
        // since events do not internally retreat with the
        // global step, we should already be at the event time
        // if this is too strict, we could use eps(list_->t_).
        assert(tdiff == 0.0 || (gcv_->tstop_begin_ <= tt && tt <= gcv_->tstop_end_));
        deliver_events(tt, nt);
    } else {
        err = gcv_->handle_step(nrn_ensure_model_data_are_sorted(), this, tt);
    }
    if (p[0].tqe_->least_t() < gcv_->t_) {
        gcv_->interpolate(p[0].tqe_->least_t());
    }
    return err;
}

int Cvode::handle_step(neuron::model_sorted_token const& sorted_token, NetCvode* ns, double te) {
    int err = NVI_SUCCESS;
    // first order correct condition evaluation goes here
    if (ns->condition_order() == 1) {
        if (ns->gcv_) {  // global step
            for (int i = 0; i < nctd_; ++i) {
                nrn_threads[i]._t = t_;  // for global step could be assert
            }
            check_deliver();
            // done if the check puts a 0 delay event on queue
            if (nctd_ > 1) {
                int tid;
                if (ns->allthread_least_t(tid) <= t_) {
                    return err;
                }
            } else {
                if (ns->p[0].tqe_->least_t() <= t_) {
                    return err;
                }
            }
        } else {  // lvardt so in a specific thread
            // for localstep method t is for a different cvode.fun call
            nth_->_t = t_;
            check_deliver(nth_);
            if (ns->p[nth_->id].tqe_->least_t() <= t_) {
                return err;
            }
        }
    }
    if (initialize_) {
        err = init(t_);
        if (ns->gcv_) {
            ns->initialized_ = true;
        }
        // second order correct condition evaluation goes here
        if (ns->condition_order() == 2) {
            evaluate_conditions(nth_);
        }
    } else if (te <= tn_) {
        err = interpolate(te);
    } else if (t_ < tn_) {
        err = interpolate(tn_);
    } else {
        record_continuous();
        err = advance_tn(sorted_token);
        // second order correct condition evaluation goes here
        if (ns->condition_order() == 2) {
            evaluate_conditions(nth_);
        }
    }
    return err;
}

void nrn_net_move(Datum* v, Point_process* pnt, double tt) {
    auto* const q = v->get<TQItem*>();
    if (!q) {
        hoc_execerror("No event with flag=1 for net_move in ", hoc_object_name(pnt->ob));
    }
    // printf("net_move tt=%g %s *v=%p\n", tt, hoc_object_name(pnt->ob), *v);
    if (tt < PP2t(pnt)) {
        SelfEvent* se = (SelfEvent*) q->data_;
        char buf[100];
        Sprintf(buf, "net_move tt-nt_t = %g", tt - PP2t(pnt));
        se->pr(buf, tt, net_cvode_instance);
        assert(0);
        hoc_execerror("net_move tt < t", 0);
    }
    net_cvode_instance->move_event(q, tt, PP2NT(pnt));
}

void artcell_net_move(Datum* v, Point_process* pnt, double tt) {
    if (nrn_use_selfqueue_) {
        auto* const q = v->get<TQItem*>();
        if (!q) {
            hoc_execerror("No event with flag=1 for net_move in ", hoc_object_name(pnt->ob));
        }
        NrnThread* nt = PP2NT(pnt);
        NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
        // printf("artcell_net_move t=%g qt_=%g tt=%g %s *v=%p\n", nt->_t, q->t_, tt,
        // hoc_object_name(pnt->ob), *v);
        if (tt < nt->_t) {
            SelfEvent* se = (SelfEvent*) q->data_;
            char buf[100];
            Sprintf(buf, "artcell_net_move tt-nt_t = %g", tt - nt->_t);
            se->pr(buf, tt, net_cvode_instance);
            hoc_execerror("net_move tt < t", 0);
        }
        q->t_ = tt;
        if (tt < p.immediate_deliver_) {
            // printf("artcell_net_move_ %s immediate %g %g %g\n", hoc_object_name(pnt->ob),
            // PP2t(pnt), tt, p.immediate_deliver_);
            SelfEvent* se = (SelfEvent*) q->data_;
            se->deliver(tt, net_cvode_instance, nt);
        }
    } else {
        nrn_net_move(v, pnt, tt);
    }
}

void NetCvode::move_event(TQItem* q, double tnew, NrnThread* nt) {
    int tid = nt->id;
    STATISTICS(SelfEvent::selfevent_move_);
#if PRINT_EVENT
    if (print_event_) {
        SelfEvent* se = (SelfEvent*) q->data_;
        Printf("NetCvode::move_event self event target %s t=%g, old=%g new=%g\n",
               hoc_object_name(se->target_->ob),
               nt->_t,
               q->t_,
               tnew);
    }
#endif
#if USENEOSIM
    // only self events move
    if (neosim_entity_) {
        assert(0);
        // cvode_instance->neosim_self_events_->move(q, tnew);
    } else {
        p[tid].tqe_->move(q, tnew);
    }
#else
    p[tid].tqe_->move(q, tnew);
#endif
}

void NetCvode::remove_event(TQItem* q, int tid) {
    p[tid].tqe_->remove(q);
}

// for threads, revised net_send to use absolute time (in the
// mod file we add the thread time when we call it).
void nrn_net_send(Datum* v, double* weight, Point_process* pnt, double td, double flag) {
    STATISTICS(SelfEvent::selfevent_send_);
    NrnThread* nt = PP2NT(pnt);
    NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
    SelfEvent* se = p.sepool_->alloc();
    se->flag_ = flag;
    se->target_ = pnt;
    se->weight_ = weight;
    se->movable_ = v;  // needed for SaveState
    assert(net_cvode_instance);
    ++p.unreffed_event_cnt_;
    if (td < nt->_t) {
        char buf[100];
        Sprintf(buf, "net_send td-t = %g", td - nt->_t);
        se->pr(buf, td, net_cvode_instance);
        abort();
        hoc_execerror("net_send delay < 0", 0);
    }
    TQItem* q;
#if USENEOSIM
    if (neosim_entity_) {
        assert(0);
        // cvode_instance->neosim_self_events_->insert(td, se);
    } else {
        q = net_cvode_instance->event(td, se, nt);
    }
#else
    q = net_cvode_instance->event(td, se, nt);
#endif
    if (flag == 1.0) {
        *v = q;
    }
    // printf("net_send %g %s %g %p\n", td, hoc_object_name(pnt->ob), flag, *v);
}

void artcell_net_send(Datum* v, double* weight, Point_process* pnt, double td, double flag) {
    if (nrn_use_selfqueue_ && flag == 1.0) {
        STATISTICS(SelfEvent::selfevent_send_);
        NrnThread* nt = PP2NT(pnt);
        NetCvodeThreadData& p = net_cvode_instance->p[nt->id];
        SelfEvent* se = p.sepool_->alloc();
        se->flag_ = flag;
        se->target_ = pnt;
        se->weight_ = weight;
        se->movable_ = v;  // needed for SaveState
        assert(net_cvode_instance);
        ++p.unreffed_event_cnt_;
        if (td < nt->_t) {
            char buf[100];
            Sprintf(buf, "net_send td-t = %g", td - nt->_t);
            se->pr(buf, td, net_cvode_instance);
            hoc_execerror("net_send delay < 0", 0);
        }
        TQItem* q = p.selfqueue_->insert(se);
        q->t_ = td;
        *v = q;
        // printf("artcell_net_send %g %s %g %p\n", td, hoc_object_name(pnt->ob), flag, v);
        if (q->t_ < p.immediate_deliver_) {
            // printf("artcell_net_send_  %s immediate %g %g %g\n", hoc_object_name(pnt->ob),
            // nt->_t, q->t_, p.immediate_deliver_);
            SelfEvent* se = (SelfEvent*) q->data_;
            p.selfqueue_->remove(q);
            se->deliver(td, net_cvode_instance, nt);
        }
    } else {
        nrn_net_send(v, weight, pnt, td, flag);
    }
}

// Deprecated overloads for backwards compatibility
void artcell_net_send(void* v, double* weight, Point_process* pnt, double td, double flag) {
    artcell_net_send(static_cast<Datum*>(v), weight, pnt, td, flag);
}

void nrn_net_send(void* v, double* weight, Point_process* pnt, double td, double flag) {
    nrn_net_send(static_cast<Datum*>(v), weight, pnt, td, flag);
}

void net_event(Point_process* pnt, double time) {
    STATISTICS(net_event_cnt_);
    PreSyn* ps = (PreSyn*) pnt->presyn_;
    if (ps) {
        if (time < PP2t(pnt)) {
            char buf[100];
            Sprintf(buf, "net_event time-t = %g", time - PP2t(pnt));
            ps->pr(buf, time, net_cvode_instance);
            hoc_execerror("net_event time < t", 0);
        }
#if USENEOSIM
        if (neosim_entity_) {
            (*p_nrn2neosim_send)(neosim_entity_, nt_t);
        } else {
#endif
            ps->send(time, net_cvode_instance, ps->nt_);
#if USENEOSIM
        }
#endif
    }
}

void _nrn_watch_activate(Datum* d,
                         double (*c)(Point_process*),
                         int i,
                         Point_process* pnt,
                         int r,
                         double flag) {
    auto* wl = d->get<WatchList*>();
    auto* wc = d[i].get<WatchCondition*>();
    if (!wl || !wc) {
        // When c is NULL, i.e. called from CoreNEURON,
        // we never get here because we made sure
        // _nrn_watch_allocated for this has been called earlier from
        // within the translated mod file.
        _nrn_watch_allocate(d, c, i, pnt, flag);
        // d[0] and d[i] have now been updated
        wl = d->get<WatchList*>();
        wc = d[i].get<WatchCondition*>();
    }
    if (r == 0) {
        for (auto wc1: *wl) {
            wc1->Remove();
            if (wc1->qthresh_) {  // is it on the queue?
                net_cvode_instance->remove_event(wc1->qthresh_, PP2NT(pnt)->id);
                wc1->qthresh_ = nullptr;
            }
        }
        wl->clear();
    }
    wl->push_back(wc);
    wc->activate(flag);  // nr_flag_ (NetReceive flag) not flag_ for above threshold.
}

/*
An example of a call to _nrn_watch_activate from within the NET_RECEIVE
block of a translated mod file is
_nrn_watch_activate(_watch_array, _watch1_cond, 1, _pnt, _watch_rm++, 2.0);

_watch_array begins at _ppvar+first_index_of_watch_information and the number
  of indices is 1 more than the number of watch statements. Each of those
  (starting at index 1) is a pointer to a WatchCondition.
  The 0th is a pointer to a HTList (HeadTailList) of the active WatchConditions.
  Third arg is index of the WatchCondition to be activated.
  The _watch_rm when 0 means, empty the HTList before adding the
  WatchCondition to the HTList, when > 0, just add (It is possible for
  several WatchCondition to be active at the same time. That is useful when
  the conceptual condition is for a variable to be within a specific range
  or when multiple variables need to be watched at the same time.
  The last arg is the flag value used by the NET_RECEIVE block to activate
  a new set of WATCH statements. Note that that particular flag does not
  have to result in the activation of a new set. In that case, the old set
  stays active.

Note. At time of writing this note,
  _watch_array[1:] are only created the first time that 'i' WatchCondition
  is activated. And at that time the 2nd callback arg is stored in the
  WatchCondition. So, at the moment, return from CoreNEURON cannot count on
  all WatchConditions being in existence. Given that the 2nd callback arg
  is only known to the translated mod file, the most straightforward solution
  is for nocmodl to generate
  an "initialization" function that calls all possible _nrn_watch_activate.
  That could be done at the Point_process* pnt creation time (or after
  transfer of WATCH info to CoreNEURON (using structure_change_cnt_).)
  It could even be done as necessary when CoreNEURON sends back activated
  WatchCondition info when the WatchCondition* slot on the NEURON side
  is NULL. Then the "initialization" function could call a stripped down
  version of _nrn_watch_activate for only its NULL slots.

Here are some more notes about WatchCondition, HTList, HTListList, and
  WatchList.
  The (WatchList*)d->_pvoid is used only in _nrn_watch_activate to
  iterate over its previously activated list in order to remove all those from
  its HTList.
  WatchCondition is a subclass of ConditionEvent and HTList
  Because of the latter, we can say
  WatchCondition.Remove() and HTList.append(WatchCondition)
  The former can be called any number of times. When it is called it becomes
  a singleton WatchCondition (only in itself as an HTList).
  The latter can be called any number of times. Whereever the
  WatchCondition is located before calling HTList.append ---
  singleton, in another list, or already in this list --- it will end up
  as the htlist.Last()

*/

/** Introduced so corenrn->nrn can request the mod file to make sure
 *  all WatchCondition are allocated. When that is the case then
 *  corenrn can call _nrn_watch_activate with all args filled out
 *  because the allocated WatchCondition has double (*c_)(Point_process)
 *  and flag_ filled in.
 **/
void _nrn_watch_allocate(Datum* d,
                         double (*c)(Point_process*),
                         int i,
                         Point_process* pnt,
                         double flag) {
    if (!d->get<WatchList*>()) {
        *d = new WatchList;
    }
    if (!d[i].get<WatchCondition*>()) {
        WatchCondition* wc = new WatchCondition(pnt, c);
        wc->c_ = c;
        wc->nrflag_ = flag;
        d[i] = wc;
        // Simplify transfer to CoreNEURON
        // To avoid searching for the beginning of _watch_array after
        // transfer to CoreNEURON, compute the offset with respect to
        // dparam.  That, of course, assumes NEURON and CoreNEURON
        // have same pdata arrangement.
        wc->watch_index_ = i + (d - pnt->prop->dparam);
    }
}

/** Watch info corenrn->nrn transfer requires all activated
 *  WatchCondition be deactivated prior to mirroring the activation
 *  that exists on the corenrn side.
 **/
void nrn_watch_clear() {
    assert(net_cvode_instance->wl_list_.size() == (size_t) nrn_nthread);
    for (auto& htlists_of_thread: net_cvode_instance->wl_list_) {
        for (HTList* wl: htlists_of_thread) {
            wl->RemoveAll();
        }
    }
    // not necessary to empty the WatchList in the Point_process dparam array
    // as that will happen when _nrn_watch_activate is called with an r
    // arg of 0.
}

/** Called by Point_process destructor in translated mod file **/
void _nrn_free_watch(Datum* d, int offset, int n) {
    int i;
    int nn = offset + n;
    if (auto* d_offset = d[offset].get<WatchList*>(); d_offset) {
        delete d_offset;
        d[offset] = nullptr;
    }
    for (i = offset + 1; i < nn; ++i) {
        if (auto* wc = d[i].get<WatchCondition*>(); wc) {
            wc->Remove();
            delete wc;
            d[i] = nullptr;
        }
    }
}

void NetCvode::vec_event_store() {
    // not destroyed when vector destroyed.
    // should resize to 0 or remove before storing, just keeps incrementing
    if (vec_event_store_) {
        vec_event_store_ = nullptr;
    }
    if (ifarg(1)) {
        vec_event_store_ = vector_arg(1);
    }
}

#if BBTQ == 4
TQItem* NetCvode::fifo_event(double td, DiscreteEvent* db) {
    if (nrn_use_fifo_queue_) {
#if PRINT_EVENT
        if (print_event_) {
            db->pr("send", td, this);
        }
        if (vec_event_store_) {
            Vect* x = vec_event_store_;
            int n = x->size();
            x->resize_chunk(n + 2);
            x->elem(n) = t;
            x->elem(n + 1) = td;
        }
#endif
        return p[0].tqe_->insert_fifo(td, db);
    } else {
        return p[0].tqe_->insert(td, db);
    }
}
#else
#define fifo_event event
#endif

#if BBTQ == 5
TQItem* NetCvode::bin_event(double td, DiscreteEvent* db, NrnThread* nt) {
    if (nrn_use_bin_queue_) {
#if PRINT_EVENT
        if (print_event_) {
            db->pr("binq send", td, this);
        }
        if (vec_event_store_) {
            assert(0);
            Vect* x = vec_event_store_;
            x->push_back(nt_t);
            x->push_back(td);
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
#else
#define bin_event event
#endif

TQItem* NetCvode::event(double td, DiscreteEvent* db, NrnThread* nt) {
#if PRINT_EVENT
    if (print_event_) {
        db->pr("send", td, this);
    }
    if (vec_event_store_) {
        Vect* x = vec_event_store_;
        x->push_back(nt_t);
        x->push_back(td);
    }
#endif
    return p[nt->id].tqe_->insert(td, db);
}

void NetCvode::null_event(double tt) {
    assert(0);
    NrnThread* nt = nrn_threads;
    if (tt - nt->_t < 0) {
        return;
    }
#if USENEOSIM
    if (neosim_entity_) {
        // ignore for neosim. There is no appropriate cvode_instance
        // cvode_instance->neosim_self_events_->insert(nt_t + delay, null_event_);
    } else {
        event(tt, null_event_, nt);
    }
#else
    event(tt, null_event_, nt);
#endif
}

void NetCvode::hoc_event(double tt, const char* stmt, Object* ppobj, int reinit, Object* pyact) {
    if (!ppobj && tt - nt_t < 0) {
        return;
    }
#if USENEOSIM
    if (neosim_entity_) {
        // ignore for neosim. There is no appropriate cvode_instance
        // cvode_instance->neosim_self_events_->insert(nt_t + delay, null_event_);
    } else
#endif
    {
        NrnThread* nt = nrn_threads;
        if (nrn_nthread > 1 && (!cvode_active_ || localstep())) {
            if (ppobj) {
                int i = PP2NT(ob2pntproc(ppobj))->id;
                p[i].interthread_send(tt, HocEvent::alloc(stmt, ppobj, reinit, pyact), nt + i);
                nrn_interthread_enqueue(nt + i);
            } else {
                HocEvent* he = HocEvent::alloc(stmt, nullptr, 0, pyact);
                // put on each queue. The first thread to execute the deliver
                // for he will set the nrn_allthread_handle
                // callback which will cause all threads to rejoin at the
                // end of the current fixed step or, for var step methods,
                // after all events at this time are delivered. It is up
                // to the callers of the multithread_job functions
                // to do the right thing.
                for (int i = 0; i < nrn_nthread; ++i) {
                    p[i].interthread_send(tt, he, nt + i);
                }
                nrn_multithread_job(nrn_interthread_enqueue);
            }
        } else {
            event(tt, HocEvent::alloc(stmt, ppobj, reinit, pyact), nt);
        }
    }
}

void NetCvode::allthread_handle() {
    nrn_allthread_handle = nullptr;
    t = nt_t;
    while (!allthread_hocevents_->empty()) {
        HocEvent* he = (*allthread_hocevents_)[0];
        allthread_hocevents_->erase(allthread_hocevents_->begin());
        he->allthread_handle();
    }
}

void NetCvode::allthread_handle(double tt, HocEvent* he, NrnThread* nt) {
    // printf("allthread_handle tt=%g nt=%d nt_t=%g\n", tt, nt->id, nt->_t);
    nt->_stop_stepping = 1;
    if (is_local()) {
        int i, n = p[nt->id].nlcv_;
        Cvode* lcv = p[nt->id].lcv_;
        if (n)
            for (i = 0; i < n; ++i) {
                local_retreat(tt, lcv + i);
                if (!he->stmt()) {
                    lcv[i].record_continuous();
                }
            }
        else {
            nt->_t = tt;
        }
    } else if (!he->stmt() && cvode_active_ && gcv_) {
        assert(MyMath::eq2(tt, gcv_->t_, NetCvode::eps(tt)));
        gcv_->record_continuous();
    }
    if (nt->id == 0) {
        nrn_allthread_handle = allthread_handle_callback;
        allthread_hocevents_->push_back(he);
        nt->_t = tt;
    }
    if (cvode_active_ && gcv_ && nrnmpi_numprocs > 1) {
        assert(nrn_nthread == 1);
        return;
    }
    // deliver any other events at this time (in particular, a possible NetParEvent)
    // to guarantee consistency of the NetParEvent for all threads
    // Otherwise, if some threads do a NetParEvent and others not, then
    // the interthread enqueue can put an earlier event onto the thread queue
    // than the last delivered event
    deliver_events(tt, nt);
}

#if 0
struct PPArgs {
	int type;
	Point_process* pp;
	double* w;
	double f;
};

static PPArgs* ppargs;

static void point_receive_job(NrnThread* nt) {
	PPArgs* p = ppargs + nt->id;
	(*pnt_receive[p->_type])(p->pp, p->w, p->f);
}

void NetCvode::point_receive(int type, Point_process* pp, double* w, double f) {
	// this is the master thread. need to execute the thread associated
	// with the pp.
	int id = PP2NT(pp)->id;
	if (id == 0) { // execute on this, the master thread
		(*pnt_receive[type])(pp, w, f);
	}else{
		// marshall the args
		PPArgs* p = ppargs + id;
		p->_type = type;
		p->pp = pp;
		p->w = w;
		p->f = f;
		nrn_onethread_job(id, point_receive_job);
	}
	// global queue with different threads putting things in
	// means no guarantee that something goes into the queue
	// to be delivered earlier than something last extracted.
	// so return to calling main thread only after the
	// worker thread is done. Too bad...
	// this needs to be worked on. Only thread id is executing.
	nrn_wait_for_threads();
}
#endif

void NetCvode::clear_events() {
    int i;
    deliver_cnt_ = net_event_cnt_ = 0;
    NetCon::netcon_send_active_ = 0;
    NetCon::netcon_send_inactive_ = 0;
    NetCon::netcon_deliver_ = 0;
    ConditionEvent::init_above_ = 0;
    ConditionEvent::send_qthresh_ = 0;
    ConditionEvent::deliver_qthresh_ = 0;
    ConditionEvent::abandon_ = 0;
    ConditionEvent::eq_abandon_ = 0;
    ConditionEvent::abandon_init_above_ = 0;
    ConditionEvent::abandon_init_below_ = 0;
    ConditionEvent::abandon_above_ = 0;
    ConditionEvent::abandon_below_ = 0;
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
    PlayRecordEvent::playrecord_deliver_ = 0;
    PlayRecordEvent::playrecord_send_ = 0;
    HocEvent::hocevent_send_ = 0;
    HocEvent::hocevent_deliver_ = 0;
    DiscreteEvent::discretevent_send_ = 0;
    DiscreteEvent::discretevent_deliver_ = 0;
    KSSingle::singleevent_deliver_ = 0;
    KSSingle::singleevent_move_ = 0;

    // SelfEvents need to be "freed". Other kinds of DiscreteEvents may
    // already have gone out of existence so the tqe_ may contain many
    // invalid item data pointers
    HocEvent::reclaim();
    allthread_hocevents_->clear();
    nrn_allthread_handle = nullptr;
#if USENEOSIM
    if (p_nrn2neosim_send)
        for (i = 0; i < nlist_; ++i) {
            TQueue* tq = p.lcv_[i].neosim_self_events_;
            while (tq->least()) {
                tq->remove(tq->least());
            }
            // and have already been reclaimed by SelfEvent::reclaim()
        }
#endif
    if (!MUTCONSTRUCTED) {
        MUTCONSTRUCT(1);
    }
    enqueueing_ = 0;
    for (i = 0; i < nrn_nthread; ++i) {
        NetCvodeThreadData& d = p[i];
        delete std::exchange(d.tqe_, new TQueue(p[i].tpool_));
        d.unreffed_event_cnt_ = 0;
        if (d.sepool_) {
            d.sepool_->free_all();
        }
        d.immediate_deliver_ = -1e100;
        d.ite_cnt_ = 0;
        if (nrn_use_selfqueue_) {
            if (!d.selfqueue_) {
                d.selfqueue_ = new SelfQueue(d.tpool_, 0);
            } else {
                d.selfqueue_->remove_all();
            }
        }
#if BBTQ == 5
        d.tqe_->nshift_ = -1;
        d.tqe_->shift_bin(nt_t - 0.5 * nt_dt);
#endif
    }
    // I don't believe this is needed anymore since cvode not needed
    // til delivery.
    if (cvode_active_) {  // in case there is a net_send from INITIAL cvode
        // then this needs to be done before INITIAL blocks are called
        init_global();
    }
}

// Frees the allocated memory for the SelfEvent pool and TQItemPool after cleaning them
void NetCvode::free_event_pools() {
    clear_events();
    for (int i = 0; i < nrn_nthread; ++i) {
        NetCvodeThreadData& d = p[i];
        delete std::exchange(d.sepool_, nullptr);
        delete std::exchange(d.selfqueue_, nullptr);
        delete std::exchange(d.tqe_, nullptr);
        if (d.tpool_) {
            d.tpool_->free_all();
            delete std::exchange(d.tpool_, nullptr);
        }
    }
}

void NetCvode::init_events() {
    hoc_Item* q;
    int i, j;
#if BBTQ == 5
    for (i = 0; i < nrn_nthread; ++i) {
        p[i].tqe_->nshift_ = -1;
        // first bin starts 1/2 time step early because per time step
        // binq delivery during simulation from deliver_net_events,
        // after delivering all events in the current bin, shifts to
        // nt->_t + 0.5*nt->_dt where nt->_t is a multiple of dt.
        p[i].tqe_->shift_bin(nt_t - 0.5 * nt_dt);
    }
#endif
    if (psl_) {
        ITERATE(q, psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            ps->init();
            ps->flag_ = false;
            NetConPList& dil = ps->dil_;
            ps->use_min_delay_ = 0;
#if USE_MIN_DELAY
            // also decide what to do about use_min_delay_
            // the rule for now is to use it if all delays are
            // the same and there are more than 2
#if BBTQ == 4
            // but
            // if we desire nrn_use_fifo_queue_ then use it
            // even if just one
            double fifodelay;
            if (nrn_use_fifo_queue_) {
                if (!dil.empty()) {
                    ps->use_min_delay_ = 1;
                    ps->delay_ = dil[0]->delay_;
                    fifodelay = ps->delay_;
                }
            } else
#endif  // BBTQ
            {
                if (dil.size() > 2) {
                    ps->use_min_delay_ = 1;
                    ps->delay_ = dil[0]->delay_;
                }
            }
#endif  // USE_MIN_DELAY

            for (const auto& d: dil) {
                if (ps->use_min_delay_ && ps->delay_ != d->delay_) {
                    ps->use_min_delay_ = false;
                }
#if BBTQ == 4
                if (nrn_use_fifo_queue_ && d->delay_ != fifodelay) {
                    hoc_warning(
                        "Use of the event fifo queue is turned off due to more than one value for "
                        "NetCon.delay",
                        0);
                    nrn_use_fifo_queue_ = false;
                }
#endif
            }
        }
    }
    // iterate over all NetCon in creation order to call
    // NETRECEIVE INITIAL blocks.
    static hoc_List* nclist = NULL;
    if (!nclist) {
        Symbol* sym = hoc_lookup("NetCon");
        nclist = sym->u.ctemplate->olist;
    }
    ITERATE(q, nclist) {
        Object* obj = OBJ(q);
        auto* d = static_cast<NetCon*>(obj->u.this_pointer);
        if (d->target_) {
            int type = d->target_->prop->_type;  // somehow prop is non-deterministically-null here
            if (pnt_receive_init[type]) {
                (*pnt_receive_init[type])(d->target_, d->weight_, 0);
            } else {
                // not the first
                for (j = d->cnt_ - 1; j > 0; --j) {
                    d->weight_[j] = 0.;
                }
            }
        }
    }
    if (gcv_) {
        for (int j = 0; j < nrn_nthread; ++j) {
            if (gcv_->ctd_[j].watch_list_) {
                gcv_->ctd_[j].watch_list_->RemoveAll();
            }
        }
    } else {
        for (int j = 0; j < nrn_nthread; ++j) {
            NetCvodeThreadData& d = p[j];
            for (i = 0; i < d.nlcv_; ++i) {
                if (d.lcv_[i].ctd_[0].watch_list_) {
                    d.lcv_[i].ctd_[0].watch_list_->RemoveAll();
                }
            }
        }
    }
}

double PreSyn::mindelay() {
    double md = 1e9;
    for (const auto& d: dil_) {
        if (md > d->delay_) {
            md = d->delay_;
        }
    }
    return md;
}

void NetCvode::deliver_events(double til, NrnThread* nt) {
    // printf("deliver_events til %20.15g\n", til);
    p[nt->id].enqueue(this, nt);
    while (deliver_event(til, nt)) {
        ;
    }
}

static IvocVect* peqvec;  // if not nullptr then the sorted times on the event queue.
static void peq(const TQItem*, int);
static void peq(const TQItem* q, int) {
    if (peqvec) {
        peqvec->push_back(q->t_);
    } else {
        DiscreteEvent* d = (DiscreteEvent*) q->data_;
        d->pr("", q->t_, net_cvode_instance);
    }
}

void NetCvode::print_event_queue() {
    // dangerous since many events can go out of existence after
    // a simulation and before NetCvode::clear at the next initialization
    if (ifarg(1)) {
        peqvec = vector_arg(1);
        peqvec->resize(0);
    }
    p[0].tqe_->forall_callback(peq);
    peqvec = nullptr;
}

static int event_info_type_;
static IvocVect* event_info_tvec_;
static IvocVect* event_info_flagvec_;
static OcList* event_info_list_;  // netcon or point_process

static void event_info_callback(const TQItem*, int);
static void event_info_callback(const TQItem* q, int) {
    DiscreteEvent* d = (DiscreteEvent*) q->data_;
    switch (d->type()) {
    case NetConType:
        if (event_info_type_ == NetConType) {
            auto* nc = static_cast<NetCon*>(d);
            event_info_tvec_->push_back(q->t_);
            event_info_list_->append(nc->obj_);
        }
        break;
    case SelfEventType:
        if (event_info_type_ == SelfEventType) {
            auto* se = static_cast<SelfEvent*>(d);
            event_info_tvec_->push_back(q->t_);
            event_info_flagvec_->push_back(se->flag_);
            event_info_list_->append(se->target_->ob);
        }
        break;
    case PreSynType:
        if (event_info_type_ == NetConType) {
            auto* ps = static_cast<PreSyn*>(d);
            for (const auto& nc: reverse(ps->dil_)) {
                double td = nc->delay_ - ps->delay_;
                event_info_tvec_->push_back(q->t_ + td);
                event_info_list_->append(nc->obj_);
            }
        }
        break;
    }
}

void NetCvode::event_queue_info() {
    // dangerous since many events can go out of existence after
    // a simulation and before NetCvode::clear at the next initialization
    int i = 1;
    event_info_type_ = (int) chkarg(i++, 2, 3);
    event_info_tvec_ = vector_arg(i++);
    event_info_tvec_->resize(0);
    if (event_info_type_ == SelfEventType) {
        event_info_flagvec_ = vector_arg(i++);
        event_info_flagvec_->resize(0);
    }
    Object* o = *hoc_objgetarg(i++);
    check_obj_type(o, "List");
    event_info_list_ = (OcList*) o->u.this_pointer;
    event_info_list_->remove_all();
    p[0].tqe_->forall_callback(event_info_callback);
}

void DiscreteEvent::send(double tt, NetCvode* ns, NrnThread* nt) {
    STATISTICS(discretevent_send_);
    ns->event(tt, this, nt);
}

void DiscreteEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    STATISTICS(discretevent_deliver_);
}

NrnThread* DiscreteEvent::thread() {
    return nrn_threads;
}

void DiscreteEvent::pgvts_deliver(double tt, NetCvode* ns) {
    STATISTICS(discretevent_deliver_);
}

void DiscreteEvent::pr(const char* s, double tt, NetCvode* ns) {
    Printf("%s DiscreteEvent %.15g\n", s, tt);
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
    } else {
        STATISTICS(netcon_send_inactive_);
    }
}

void NetCon::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    assert(target_);
    int type = target_->prop->_type;
    std::string ss("net-receive-");
    ss += memb_func[type].sym->name;
    nrn::Instrumentor::phase p_get_pnt_receive(ss.c_str());
    if (PP2NT(target_) != nt) {
        Printf("NetCon::deliver nt=%d target=%d\n", nt->id, PP2NT(target_)->id);
    }
    assert(PP2NT(target_) == nt);
    Cvode* cv = (Cvode*) target_->nvi_;
    if (nrn_use_selfqueue_ && nrn_is_artificial_[type]) {
        auto& datum = target_->prop->dparam[nrn_artcell_qindex_[type]];
        TQItem* q;
        while ((q = datum.get<TQItem*>()) && q->t_ < tt) {
            double t1 = q->t_;
            auto* const se = static_cast<SelfEvent*>(ns->p[nt->id].selfqueue_->remove(q));
            // printf("%d NetCon::deliver %g , earlier selfevent at %g\n", nrnmpi_myid, tt, q->t_);
            se->deliver(t1, ns, nt);
        }
    }
    if (cvode_active_ && cv) {
        ns->local_retreat(tt, cv);
        cv->set_init_flag();
    } else {
        // no interpolation necessary for local step method and ARTIFICIAL_CELL
        nt->_t = tt;
    }

    // printf("NetCon::deliver t=%g tt=%g %s\n", t, tt, hoc_object_name(target_->ob));
    STATISTICS(netcon_deliver_);
    POINT_RECEIVE(type, target_, weight_, 0);
    if (errno) {
        if (nrn_errno_check(type)) {
            hoc_warning("errno set during NetCon deliver to NET_RECEIVE", (char*) 0);
        }
    }
}

NrnThread* NetCon::thread() {
    return PP2NT(target_);
}

void NetCon::pgvts_deliver(double tt, NetCvode* ns) {
    assert(target_);
    int type = target_->prop->_type;
    STATISTICS(netcon_deliver_);
    POINT_RECEIVE(type, target_, weight_, 0);
    if (errno) {
        if (nrn_errno_check(type)) {
            hoc_warning("errno set during NetCon deliver to NET_RECEIVE", (char*) 0);
        }
    }
}

void NetCon::pr(const char* s, double tt, NetCvode* ns) {
    Printf("%s %s", s, hoc_object_name(obj_));
    if (src_) {
        Printf(" src=%s", src_->osrc_ ? hoc_object_name(src_->osrc_) : secname(src_->ssrc_));
    } else {
        Printf(" src=nullptr");
    }
    Printf(" target=%s %.15g\n", (target_ ? hoc_object_name(target_->ob) : "nullptr"), tt);
}

void PreSyn::send(double tt, NetCvode* ns, NrnThread* nt) {
    int i;
    record(tt);
#ifndef USENCS
    if (use_min_delay_) {
        STATISTICS(presyn_send_mindelay_);
#if BBTQ == 4
        ns->fifo_event(tt + delay_, this);
#else
#if BBTQ == 5
        for (i = 0; i < nrn_nthread; ++i) {
            if (nt->id == i) {
                ns->bin_event(tt + delay_, this, nt);
            } else {
                ns->p[i].interthread_send(tt + delay_, this, nrn_threads + i);
            }
        }
#else
        ns->event(tt + delay_, this);
#endif
#endif
    } else {
        STATISTICS(presyn_send_direct_);
        for (const auto& d: dil_) {
            if (d->active_ && d->target_) {
                NrnThread* n = PP2NT(d->target_);
#if BBTQ == 5
                if (nt == n) {
                    ns->bin_event(tt + d->delay_, d, n);
                } else {
                    ns->p[n->id].interthread_send(tt + d->delay_, d, n);
                }
#else
                ns->event(tt + d->delay_, d, PP2NT(d->target_));
#endif
            }
        }
    }
#endif  // ndef USENCS
#if USENCS || NRNMPI
    if (output_index_ >= 0) {
#if NRNMPI
        if (use_multisend_) {
            nrn_multisend_send(this, tt);
        } else {
            if (nrn_use_localgid_) {
                nrn_outputevent(localgid_, tt);
            } else
#endif  // NRNMPI
                nrn2ncs_outputevent(output_index_, tt);
#if NRNMPI
        }
#endif  // NRNMPI
#if NRN_MUSIC
        if (music_port_) {
            nrnmusic_injectlist(music_port_, tt);
        }
#endif  // NRN_MUSIC
    }
#endif  // USENCS || NRNMPI
}

void PreSyn::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    if (qthresh_) {
        // the thread is the one that owns the PreSyn
        assert(nt == nt_);
        qthresh_ = nullptr;
        // printf("PreSyn::deliver %s condition event tt=%20.15g\n", ssrc_?secname(ssrc_):"", tt);
        STATISTICS(deliver_qthresh_);
        // If local variable time step and send is recorded,
        // tt will be recorded correctly. But if the recording results
        // in a callback, the user might inquire about other variables
        // as well, so perhaps we need to interpolate first.
        if (!ns->gcv_ && stmt_) {
            int i = nt->id;
            TQItem* q = ns->p[i].tq_->least();
            Cvode* cv = (Cvode*) q->data_;
            if (tt < cv->t_) {
                int err = NVI_SUCCESS;
                err = cv->handle_step(nrn_ensure_model_data_are_sorted(), ns, tt);
                ns->p[i].tq_->move_least(cv->t_);
            }
        }
        send(tt, ns, nt);
        return;
    }
    // the thread is the one that owns the targets
    STATISTICS(presyn_deliver_netcon_);
    for (const auto& d: dil_) {
        if (d->active_ && d->target_ && PP2NT(d->target_) == nt) {
            double dtt = d->delay_ - delay_;
            if (dtt == 0.) {
                STATISTICS(presyn_deliver_direct_);
                STATISTICS(deliver_cnt_);
                d->deliver(tt, ns, nt);
            } else if (dtt < 0.) {
                hoc_execerror("internal error: Source delay is > NetCon delay", 0);
            } else {
                STATISTICS(presyn_deliver_ncsend_);
                ns->event(tt + dtt, d, nt);
            }
        }
    }
}

// used by bbsavestate since during restore, some NetCon spikes may
// have already been delivered while others need to be delivered in
// the future. Not implemented fof qthresh_ case. No statistics.
void PreSyn::fanout(double td, NetCvode* ns, NrnThread* nt) {
    for (const auto& d: dil_) {
        if (d->active_ && d->target_ && PP2NT(d->target_) == nt) {
            double dtt = d->delay_ - delay_;
            ns->bin_event(td + dtt, d, nt);
        }
    }
}

NrnThread* PreSyn::thread() {
    return nt_;
}

void PreSyn::pgvts_deliver(double tt, NetCvode* ns) {
    NrnThread* nt = 0;
    assert(0);
    if (qthresh_) {
        qthresh_ = nullptr;
        // printf("PreSyn::deliver %s condition event tt=%20.15g\n", ssrc_?secname(ssrc_):"", tt);
        STATISTICS(deliver_qthresh_);
        send(tt, ns, nt);
        return;
    }
    STATISTICS(presyn_deliver_netcon_);
    for (const auto& d: dil_) {
        if (d->active_ && d->target_) {
            double dtt = d->delay_ - delay_;
            if (dtt < 0.) {
                hoc_execerror("internal error: Source delay is > NetCon delay", 0);
            } else {
                STATISTICS(presyn_deliver_ncsend_);
                ns->event(tt + dtt, d, nt);
            }
        }
    }
}

void PreSyn::pr(const char* s, double tt, NetCvode* ns) {
    Printf("%s", s);
    Printf(" PreSyn src=%s", osrc_ ? hoc_object_name(osrc_) : secname(ssrc_));
    Printf(" %.15g\n", tt);
}

SelfEvent::SelfEvent() {}
SelfEvent::~SelfEvent() {}

DiscreteEvent* SelfEvent::savestate_save() {
    //	pr("savestate_save", 0, net_cvode_instance);
    SelfEvent* se = new SelfEvent();
    se->flag_ = flag_;
    se->target_ = target_;
    se->weight_ = weight_;
    se->movable_ = movable_;
    return se;
}

void SelfEvent::savestate_restore(double tt, NetCvode* nc) {
    //	pr("savestate_restore", tt, nc);
    nrn_net_send(movable_, weight_, target_, tt, flag_);
}

DiscreteEvent* SelfEvent::savestate_read(FILE* f) {
    SelfEvent* se = new SelfEvent();
    char buf[300];
    char ppname[200];
    int ppindex, ncindex, moff, pptype;
    double flag;
    nrn_assert(fgets(buf, 300, f));
    nrn_assert(
        sscanf(buf, "%s %d %d %d %d %lf\n", ppname, &ppindex, &pptype, &ncindex, &moff, &flag) ==
        6);
    se->target_ = SelfEvent::index2pp(pptype, ppindex);
    se->weight_ = nullptr;
    if (ncindex >= 0) {
        NetCon* nc = NetConSave::index2netcon(ncindex);
        se->weight_ = nc->weight_;
    }
    se->flag_ = flag;
    se->movable_ = (moff >= 0) ? (se->target_->prop->dparam + moff) : nullptr;
    return se;
}

std::unique_ptr<SelfEventPPTable> SelfEvent::sepp_;

Point_process* SelfEvent::index2pp(int type, int oindex) {
    // code the type and object index together
    if (!sepp_) {
        int i;
        sepp_.reset(new SelfEventPPTable());
        sepp_->reserve(211);
        // should only be the ones that call net_send
        for (i = 0; i < n_memb_func; ++i)
            if (pnt_receive[i]) {
                hoc_List* hl = nrn_pnt_template_[i]->olist;
                hoc_Item* q;
                ITERATE(q, hl) {
                    Object* o = OBJ(q);
                    (*sepp_)[i + n_memb_func * o->index] = ob2pntproc(o);
                }
            }
    }
    const auto& iter = sepp_->find(type + n_memb_func * oindex);
    nrn_assert(iter != sepp_->end());
    return iter->second;
}

void SelfEvent::savestate_free() {
    sepp_.reset();
}

void SelfEvent::savestate_write(FILE* f) {
    fprintf(f, "%d\n", SelfEventType);
    int const moff = movable_ ? (movable_ - target_->prop->dparam) : -1;
    int ncindex = -1;
    // find the NetCon index for weight_
    if (weight_) {
        NetCon* nc = NetConSave::weight2netcon(weight_);
        assert(nc);
        ncindex = nc->obj_->index;
    }

    fprintf(f,
            "%s %d %d %d %d %g\n",
            target_->ob->ctemplate->sym->name,
            target_->ob->index,
            target_->prop->_type,
            ncindex,
            moff,
            flag_);
}

void SelfEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    Cvode* cv = (Cvode*) target_->nvi_;
    int type = target_->prop->_type;
    assert(nt == PP2NT(target_));
    if (nrn_use_selfqueue_ && nrn_is_artificial_[type]) {  // handle possible earlier flag=1 self
                                                           // event
        if (flag_ == 1.0) {
            *movable_ = nullptr;
        }
        TQItem* q;
        while ((q = movable_->get<TQItem*>()) != 0 && q->t_ <= tt) {
            // printf("handle earlier %g selfqueue event from within %g SelfEvent::deliver\n",
            // q->t_, tt);
            double t1 = q->t_;
            SelfEvent* se = (SelfEvent*) ns->p[nt->id].selfqueue_->remove(q);
            PP2t(target_) = t1;
            se->call_net_receive(ns);
        }
    }
    if (cvode_active_ && cv) {
        ns->local_retreat(tt, cv);
        cv->set_init_flag();
    } else {
        PP2t(target_) = tt;
    }
    // printf("SelfEvent::deliver t=%g tt=%g %s\n", PP2t(target), tt, hoc_object_name(target_->ob));
    call_net_receive(ns);
}

NrnThread* SelfEvent::thread() {
    return PP2NT(target_);
}

void SelfEvent::pgvts_deliver(double tt, NetCvode* ns) {
    call_net_receive(ns);
}
void SelfEvent::call_net_receive(NetCvode* ns) {
    STATISTICS(selfevent_deliver_);
    POINT_RECEIVE(target_->prop->_type, target_, weight_, flag_);
    if (errno) {
        if (nrn_errno_check(target_->prop->_type)) {
            hoc_warning("errno set during SelfEvent deliver to NET_RECEIVE", (char*) 0);
        }
    }
    NetCvodeThreadData& nctd = ns->p[PP2NT(target_)->id];
    --nctd.unreffed_event_cnt_;
    nctd.sepool_->hpfree(this);
}

void SelfEvent::pr(const char* s, double tt, NetCvode* ns) {
    Printf("%s", s);
    Printf(" SelfEvent target=%s %.15g flag=%g\n", hoc_object_name(target_->ob), tt, flag_);
}

void PlayRecordEvent::frecord_init(TQItem* q) {
    plr_->frecord_init(q);
}

void PlayRecordEvent::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    if (plr_->cvode_ && plr_->cvode_->nth_) {
        assert(nt == plr_->cvode_->nth_);
        ns->local_retreat(tt, plr_->cvode_);
    }
    STATISTICS(playrecord_deliver_);
    plr_->deliver(tt, ns);
}

NrnThread* PlayRecordEvent::thread() {
    return nrn_threads + plr_->ith_;
}

void PlayRecordEvent::pr(const char* s, double tt, NetCvode* ns) {
    Printf("%s PlayRecordEvent %.15g ", s, tt);
    plr_->pr();
}

#include <hocevent.cpp>

void NetCvode::local_retreat(double t, Cvode* cv) {
    if (!cvode_active_) {
        return;
    }
    TQueue* tq = p[cv->nth_ ? cv->nth_->id : 0].tq_;
    if (tq) {
#if PRINT_EVENT
        if (print_event_) {
            Printf("microstep local retreat from %g (cvode_%p is at %g) for event onset=%g\n",
                   cv->tqitem_->t_,
                   cv,
                   cv->t_,
                   t);
        }
#endif
        cv->interpolate(t);
        tq->move(cv->tqitem_, t);
#if PRINT_EVENT
        if (print_event_ > 1) {
            Printf("after target solve time for %p is %g , dt=%g\n", cv, cv->time(), nt_dt);
        }
#endif
    } else {
        assert(t == cv->t_ || (cv->tstop_begin_ <= t && t <= cv->tstop_end_));
    }
}

void NetCvode::retreat(double t, Cvode* cv) {
    if (!cvode_active_) {
        return;
    }
    TQueue* tq = p[cv->nth_ ? cv->nth_->id : 0].tq_;
#if PRINT_EVENT
    if (print_event_) {
        Printf("microstep retreat from %g (cvode_%p is at %g) for event onset=%g\n",
               tq ? cv->tqitem_->t_ : cv->t_,
               cv,
               cv->t_,
               t);
    }
#endif
    cv->interpolate(t);
    if (tq) {
        tq->move(cv->tqitem_, t);
    }
#if PRINT_EVENT
    if (print_event_ > 1) {
        Printf("after target solve time for %p is %g , dt=%g\n", cv, cv->time(), dt);
    }
#endif
}

#if USENEOSIM

bool neosim_deliver_self_events(TQueue* tqe, double til);
bool neosim_deliver_self_events(TQueue* tqe, double til) {
    bool b;
    TQItem* q;
    DiscreteEvent* d;
    b = false;
    while (tqe->least_t() <= til + .5e-8) {
        b = true;
        q = tqe->least();
        t = q->t_;
        d = (DiscreteEvent*) q->data_;
        assert(d->type() == SelfEventType);
        tqe->remove(q);
        d->deliver(t, net_cvode_instance);
    }
}

void neosim2nrn_advance(void* e, void* v, double tout) {
    neosim_entity_ = e;
    NetCon* d = (NetCon*) v;
    TQueue* tqe;

    // now can integrate to tout since it is guaranteed there will
    // be no further real events to this cell before tout.
    // but we must handle self events. The implementation is
    // analogous to the NetCvode::solve with single and tout
    Cvode* cv = (Cvode*) d->target_->nvi_;  // so self event from INITIAL block
    tqe = cv->neosim_self_events_;
    // not a bug even if there is no BREAKPOINT block. I.e.
    // artificial cells will work.
    t = cv->time();
    while (tout > t) {
        do {
            cv->check_deliver();
        } while (neosim_deliver_self_events(tqe, t));
        cv->solve();
    }
    cv->interpolate(tout);
    cv->check_deliver();
}

void neosim2nrn_deliver(void* e, void* v) {
    neosim_entity_ = e;
    NetCon* d = (NetCon*) v;
    Cvode* cv = (Cvode*) d->target_->nvi_;
    d->deliver(cv->t_, net_cvode_instance);
}

#endif

// parallel global variable time-step
int NetCvode::pgvts(double tstop) {
    int err = NVI_SUCCESS;
    double tt = nt_t;
    while (tt < tstop && !stoprun && err == NVI_SUCCESS) {
        err = pgvts_event(tt);
    }
    return err;
}

// parallel global variable time-step event handling
// return is what cvode call to make and the value of tt to make it at
// in response to the next global event. We try to do only one
// allreduce for a given event. Since all processes have to stay together
// with respect to cvode, we have to factor out those calls from the
// classical DiscreteEvent::deliver methods. I.e. deliver can only
// deliver an event, it cannot interpolate, etc.
// Assume events are sparse and handle them one at a time.
int NetCvode::pgvts_event(double& tt) {
    int op, err, init;
    DiscreteEvent* de;
    assert(gcv_);
    de = pgvts_least(tt, op, init);
    err = pgvts_cvode(tt, op);
    if (init) {
        gcv_->set_init_flag();
    }
    if (de) {  // handle the event and others just like it
        de->pgvts_deliver(tt, this);
        while (p[0].tqe_->least_t() == tt) {
            TQItem* q = p[0].tqe_->least();
            de = (DiscreteEvent*) q->data_;
            int i1;
            if (de->pgvts_op(i1) == op && i1 == init) {
                p[0].tqe_->remove(q);
                de->pgvts_deliver(tt, this);
            } else {
                break;
            }
        }
    }
    if (nrn_allthread_handle) {
        (*nrn_allthread_handle)();
    }
    return err;
}

DiscreteEvent* NetCvode::pgvts_least(double& tt, int& op, int& init) {
    DiscreteEvent* de = nullptr;
#if NRNMPI
    TQItem* q = nullptr;
    if (gcv_->initialize_ && p[0].tqe_->least_t() > gcv_->t_) {
        tt = gcv_->t_;
        op = 3;
        init = 0;
    } else if (gcv_->tn_ < p[0].tqe_->least_t()) {
        tt = gcv_->tn_;
        op = 1;
        init = 0;
    } else {
        // If there are several events at the same time we need the
        // highest priority (in particular, NetParEvent last).
        // This is due to the fact that NetParEvent.deliver
        // handles all the events at that time so there better not
        // be any after it on the queue.
        q = p[0].tqe_->least();
        if (q) {
            de = (DiscreteEvent*) q->data_;
            tt = q->t_;
            op = de->pgvts_op(init);
            if (op == 4) {  // is there another event at the same time?
                TQItem* q2 = p[0].tqe_->second_least(tt);
                if (q2) {
                    q = q2;
                    de = (DiscreteEvent*) q2->data_;
                    op = de->pgvts_op(init);
                    assert(op != 4);
                    // printf("%d Type %d event after NetParEvent with deliver %g and t=%g\n",
                    // nrnmpi_myid, de->type(), tt, gcv_->t_);
                }
            }
        } else {
            tt = 1e20;
            op = 1;
            init = 0;
        }
    }
    double ts = tt;
    int ops = op;
    if (nrnmpi_pgvts_least(&tt, &op, &init)) {
        if (q) {
            p[0].tqe_->remove(q);
        }
    } else if (op == 4) {  // NetParEvent need to be done all together
        p[0].tqe_->remove(q);
    } else if (ts == tt && q && ops == op) {  // safe to do this event as well
        p[0].tqe_->remove(q);
    } else {
        de = nullptr;
    }
#endif
    return de;
}

int NetCvode::pgvts_cvode(double tt, int op) {
    int err = NVI_SUCCESS;
    // this is the only place where we can enter cvode
    switch (op) {
    case 1:  // advance
        if (condition_order() == 1) {
            gcv_->check_deliver();
        }
        gcv_->record_continuous();
        err = gcv_->advance_tn(nrn_ensure_model_data_are_sorted());
        if (condition_order() == 2) {
            gcv_->evaluate_conditions();
        }
        break;
    case 2:  // interpolate
        err = gcv_->interpolate(tt);
        break;
    case 3:  // initialize
        err = gcv_->init(tt);
        initialized_ = true;
        if (condition_order() == 2) {
            gcv_->evaluate_conditions();
        }
        break;
    }
    return err;
}

bool NetCvode::use_partrans() {
#if NRNMPI
    if (gcv_) {
        return gcv_->use_partrans_;
    } else {
        return 0;
    }
#endif
    return 0;
}

void ncs2nrn_integrate(double tstop) {
    double ts;
    nrn_use_busywait(1);  // just a possibility
    auto const cache_token = nrn_ensure_model_data_are_sorted();
    if (cvode_active_) {
#if NRNMPI
        if (net_cvode_instance->use_partrans()) {
            net_cvode_instance->pgvts(tstop);
            t = nt_t;
            dt = nt_dt;
        } else
#endif
        {
            net_cvode_instance->solve(tstop);
            t = nt_t;
            dt = nt_dt;
        }
    } else {
#if 1
        int n = (int) ((tstop - nt_t) / dt + 1e-9);
        if (n > 3 && !nrnthread_v_transfer_) {
            nrn_fixed_step_group(cache_token, n);
        } else
#endif
        {
#if NRNMPI && !defined(USENCS)
            ts = tstop - dt;
            assert(nt_t <= tstop);
            // It may very well be the case that we do not advance at all
            while (nt_t <= ts) {
#else
            ts = tstop - .5 * dt;
            while (nt_t < ts) {
#endif
                nrn_fixed_step(cache_token);
                if (stoprun) {
                    break;
                }
            }
        }
    }
    // handle all the pending flag=1 self events
    for (int i = 0; i < nrn_nthread; ++i) {
        assert(nrn_threads[i]._t == nt_t);
    }
    all_pending_selfqueue(nt_t);
    nrn_use_busywait(0);  // certainly not
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
    TQItem *q1, *q2;
    nctd.immediate_deliver_ = tt;
    for (q1 = sq->first(); q1; q1 = q2) {
        if (q1->t_ <= tt) {
            SelfEvent* se = (SelfEvent*) q1->data_;
            // printf("ncs2nrn_integrate %g SelfEvent for %s at %g\n", tstop,
            // hoc_object_name(se->target_->ob), q1->t_);
            se->deliver(q1->t_, net_cvode_instance, nt);
            // could it add another self-event?, check before removal
            q2 = sq->next(q1);
            sq->remove(q1);
        } else {
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
        // for (int i=0; i < nrn_nthread; ++i) { assert(nrn_threads[i]._t == nt_t);}
        pending_selfqueue_deliver_ = tt;
        nrn_multithread_job(pending_selfqueue);
    }
}

#if USENCS

void ncs2nrn_inputevent(int i, double tdeliver) {
    NrnThread* nt = nrn_threads;
    net_cvode_instance->event(tdeliver, ncs2nrn_input_->item(i), nt);
}

// hoc tells us which are the input NetCons and which are the
// output NetCons

void nrn2ncs_netcons() {
    int i;
    Object* o;
    NetCon* nc;
    o = *hoc_objgetarg(1);
    check_obj_type(o, "List");
    OcList* list = (OcList*) (o->u.this_pointer);
    if (ncs2nrn_input_) {
        for (i = 0; i < ncs2nrn_input_->count(); ++i) {
            hoc_obj_unref(ncs2nrn_input_->item(i)->obj_);
        }
        ncs2nrn_input_->remove_all();
    } else {
        ncs2nrn_input_ = new NetConPList(list->count());
    }
    for (i = 0; i < list->count(); ++i) {
        hoc_obj_ref(list->object(i));
        nc = (NetCon*) (list->object(i)->u.this_pointer);
        ncs2nrn_input_->append(nc);
    }

    o = *hoc_objgetarg(2);
    check_obj_type(o, "List");
    list = (OcList*) (o->u.this_pointer);
    for (i = 0; i < list->count(); ++i) {
        nc = (NetCon*) (list->object(i)->u.this_pointer);
        assert(nc->src_);
        nc->src_->output_index_ = i;
    }
}

#endif  // USENCS

void NetCvode::statistics(int i) {
    int id, j, ii = 0;
    if (gcv_) {
        gcv_->statistics();
    } else {
        lvardtloop(id, j) {
            if (i < 0 || ii++ == i) {
                p[id].lcv_[j].statistics();
            }
        }
    }
    Printf("NetCon active=%lu (not sent)=%lu delivered=%lu\n",
           NetCon::netcon_send_active_,
           NetCon::netcon_send_inactive_,
           NetCon::netcon_deliver_);
    Printf(
        "Condition O2 thresh detect=%lu via init=%lu effective=%lu abandoned=%lu (unnecesarily=%lu "
        "init+=%lu init-=%lu above=%lu below=%lu)\n",
        ConditionEvent::send_qthresh_,
        ConditionEvent::init_above_,
        ConditionEvent::deliver_qthresh_,
        ConditionEvent::abandon_,
        ConditionEvent::eq_abandon_,
        ConditionEvent::abandon_init_above_,
        ConditionEvent::abandon_init_below_,
        ConditionEvent::abandon_above_,
        ConditionEvent::abandon_below_);
    Printf("PreSyn send: mindelay=%lu direct=%lu\n",
           PreSyn::presyn_send_mindelay_,
           PreSyn::presyn_send_direct_);
    Printf("PreSyn deliver: O2 thresh=%lu  NetCon=%lu (send=%lu  deliver=%lu)\n",
           ConditionEvent::deliver_qthresh_,
           PreSyn::presyn_deliver_netcon_,
           PreSyn::presyn_deliver_ncsend_,
           PreSyn::presyn_deliver_direct_);
    Printf("SelfEvent send=%lu move=%lu deliver=%lu\n",
           SelfEvent::selfevent_send_,
           SelfEvent::selfevent_move_,
           SelfEvent::selfevent_deliver_);
    Printf("Watch send=%lu deliver=%lu\n",
           WatchCondition::watch_send_,
           WatchCondition::watch_deliver_);
    Printf("PlayRecord send=%lu deliver=%lu\n",
           PlayRecordEvent::playrecord_send_,
           PlayRecordEvent::playrecord_deliver_);
    Printf("HocEvent send=%lu deliver=%lu\n",
           HocEvent::hocevent_send_,
           HocEvent::hocevent_deliver_);
    Printf("SingleEvent deliver=%lu move=%lu\n",
           KSSingle::singleevent_deliver_,
           KSSingle::singleevent_move_);
    Printf("DiscreteEvent send=%lu deliver=%lu\n",
           DiscreteEvent::discretevent_send_,
           DiscreteEvent::discretevent_deliver_);
    Printf("%lu total events delivered  net_event=%lu\n", deliver_cnt_, net_event_cnt_);
    Printf("Discrete event TQueue\n");
    p[0].tqe_->statistics();
    if (p[0].tq_) {
        Printf("Variable step integrator TQueue\n");
        p[0].tq_->statistics();
    }
}

void NetCvode::spike_stat() {
    Vect* v = vector_arg(1);
    v->resize(11);
    double* d = vector_vec(v);
    int i, j, n = 0;
    if (gcv_) {
        n += gcv_->neq_;
    } else {
        lvardtloop(i, j) {
            n += p[i].lcv_[j].neq_;
        }
    }
    d[0] = n;
    Symbol* nc = hoc_lookup("NetCon");
    d[1] = nc->u.ctemplate->count;
    d[2] = deliver_cnt_;
    d[3] = NetCon::netcon_deliver_;
    d[4] = PreSyn::presyn_send_mindelay_ + PreSyn::presyn_send_direct_;
    d[5] = SelfEvent::selfevent_deliver_;
    d[6] = SelfEvent::selfevent_send_;
    d[7] = SelfEvent::selfevent_move_;
    // should do all threads
    p[0].tqe_->spike_stat(d + 8);
}

void NetCvode::solver_prepare() {
    int i, j;
    fornetcon_prepare();
    if (nrn_modeltype() == 0) {
        delete_list();
    } else {
        init_global();
        if (cvode_active_) {
            if (matrix_change_cnt_ != nrn_matrix_cnt_) {
                structure_change();
                matrix_change_cnt_ = nrn_matrix_cnt_;
            }
            if (gcv_) {
                gcv_->use_daspk_ = nrn_use_daspk_;
                gcv_->init_prepare();
                // since there may be Vector.play events and INITIAL send events
                // at time 0 before actual initialization of integrators.
                gcv_->can_retreat_ = false;
            } else {
                lvardtloop(i, j) {
                    Cvode& cv = p[i].lcv_[j];
                    cv.use_daspk_ = nrn_use_daspk_;
                    cv.init_prepare();
                    cv.can_retreat_ = false;
                }
            }
        }
    }
    if (playrec_change_cnt_ != structure_change_cnt_) {
        playrec_setup();
    }
}

void NetCvode::re_init(double t) {
    int i, j;
    if (nrn_modeltype() == 0) {
        if (gcv_) {
            gcv_->t_ = t;
            gcv_->tn_ = t;
        } else {
            lvardtloop(i, j) {
                Cvode& cv = p[i].lcv_[j];
                cv.t_ = t;
                cv.tn_ = t;
            }
        }
        return;
    }
    double dtsav = nt_dt;
    solver_prepare();
    if (gcv_) {
        gcv_->stat_init();
        gcv_->init(t);
        if (condition_order() == 2) {
            gcv_->evaluate_conditions();
        }
    } else {
        lvardtloop(i, j) {
            Cvode& cv = p[i].lcv_[j];
            cv.stat_init();
            cv.init(t);
            cv.tqitem_->t_ = t;
            if (condition_order() == 2) {
                cv.evaluate_conditions();
            }
        }
    }
    nt_dt = dtsav;
}

void NetCvode::fornetcon_prepare() {
    NrnThread* nt;
    NrnThreadMembList* tml;
    if (fornetcon_change_cnt_ == structure_change_cnt) {
        return;
    }
    fornetcon_change_cnt_ = structure_change_cnt;
    if (nrn_fornetcon_cnt_ == 0) {
        return;
    }
    int i, j;
    // initialize a map from type to dparam index, -1 means no FOR_NETCONS statement
    std::vector<int> t2i(n_memb_func, -1);
    // create ForNetConsInfo in all the relevant point processes
    // and fill in the t2i map.
    for (i = 0; i < nrn_fornetcon_cnt_; ++i) {
        int index = nrn_fornetcon_index_[i];
        int type = nrn_fornetcon_type_[i];
        t2i[type] = index;
        if (nrn_is_artificial_[type]) {
            auto* const m = &memb_list[type];
            for (j = 0; j < m->nodecount; ++j) {
                // Save ForNetConsInfo* as void* to avoid needing to expose the
                // definition of ForNetConsInfo to translated MOD file code
                void** v = &(m->pdata[j][index].literal_value<void*>());
                _nrn_free_fornetcon(v);
                ForNetConsInfo* fnc = new ForNetConsInfo;
                *v = fnc;
                fnc->argslist = 0;
                fnc->size = 0;
            }
        } else {
            FOR_THREADS(nt) for (tml = nt->tml; tml; tml = tml->next) if (tml->index == type) {
                Memb_list* m = tml->ml;
                for (j = 0; j < m->nodecount; ++j) {
                    void** v = &(m->pdata[j][index].literal_value<void*>());
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
    if (psl_)
        ITERATE(q, psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            const NetConPList& dil = ps->dil_;
            for (const auto& d1: dil) {
                Point_process* pnt = d1->target_;
                if (pnt && t2i[pnt->prop->_type] > -1) {
                    auto* fnc = static_cast<ForNetConsInfo*>(
                        pnt->prop->dparam[t2i[pnt->prop->_type]].get<void*>());
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
            auto* const m = &memb_list[type];
            for (j = 0; j < m->nodecount; ++j) {
                auto* fnc = static_cast<ForNetConsInfo*>(m->pdata[j][index].get<void*>());
                if (fnc->size > 0) {
                    fnc->argslist = new double*[fnc->size];
                    fnc->size = 0;
                }
            }
        } else {
            FOR_THREADS(nt)
            for (tml = nt->tml; tml; tml = tml->next)
                if (tml->index == nrn_fornetcon_type_[i]) {
                    Memb_list* m = tml->ml;
                    for (j = 0; j < m->nodecount; ++j) {
                        auto* fnc = static_cast<ForNetConsInfo*>(m->pdata[j][index].get<void*>());
                        if (fnc->size > 0) {
                            fnc->argslist = new double*[fnc->size];
                            fnc->size = 0;
                        }
                    }
                }
        }
    }
    // fill in argslist and count again
    if (psl_) {
        ITERATE(q, psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            const NetConPList& dil = ps->dil_;
            for (const auto& d1: dil) {
                Point_process* pnt = d1->target_;
                if (pnt && t2i[pnt->prop->_type] > -1) {
                    auto* fnc = static_cast<ForNetConsInfo*>(
                        pnt->prop->dparam[t2i[pnt->prop->_type]].get<void*>());
                    fnc->argslist[fnc->size] = d1->weight_;
                    fnc->size += 1;
                }
            }
        }
    }
}

int _nrn_netcon_args(void* v, double*** argslist) {
    auto* fnc = static_cast<ForNetConsInfo*>(v);
    assert(fnc);
    *argslist = fnc->argslist;
    return fnc->size;
}

void _nrn_free_fornetcon(void** v) {
    if (auto* fnc = static_cast<ForNetConsInfo*>(*v); fnc) {
        delete[] std::exchange(fnc->argslist, nullptr);
        delete fnc;
        *v = nullptr;
    }
}

void record_init_clear(const TQItem* q, int) {
    DiscreteEvent* d = (DiscreteEvent*) q->data_;
    d->frecord_init((TQItem*) q);
}

void NetCvode::record_init() {
    if (!prl_->empty()) {
        // there may be some events on the queue descended from
        // finitialize that need to be removed
        record_init_items_->clear();
        p[0].tqe_->forall_callback(record_init_clear);
        for (auto tq: *record_init_items_) {
            p[0].tqe_->remove(tq);
        }
        record_init_items_->clear();
    }
    for (auto& item: *prl_) {
        item->record_init();
    }
}

void NetCvode::play_init() {
    for (auto& item: *prl_) {
        item->play_init();
    }
}

int NetCvode::cellindex() {
    Section* sec = chk_access();
    int i, j, ii;
    if (single_) {
        return 0;
    } else {
        ii = 0;
        lvardtloop(i, j) {
            if (sec == p[i].lcv_[j].ctd_[0].v_node_[p[i].lcv_[j].ctd_[0].rootnodecount_]->sec) {
                return ii;
            }
            ii++;
        }
    }
    hoc_execerror(secname(sec), " is not the root section for any local step cvode instance");
    return 0;
}

void NetCvode::states() {
    Vect* v = vector_arg(1);
    if (!cvode_active_) {
        v->resize(0);
        return;
    }
    int n{};
    if (gcv_) {
        n += gcv_->neq_;
    } else {
        int i, j;
        lvardtloop(i, j) {
            n += p[i].lcv_[j].neq_;
        }
    }
    v->resize(n);
    double* vp{vector_vec(v)};
    if (gcv_) {
        gcv_->states(vp);
    } else {
        int i{}, j{}, k{};
        lvardtloop(i, j) {
            p[i].lcv_[j].states(vp + k);
            k += p[i].lcv_[j].neq_;
        }
    }
}

void NetCvode::dstates() {
    int i, j, k, n;
    Vect* v = vector_arg(1);
    if (!cvode_active_) {
        v->resize(0);
        return;
    }
    double* vp;
    n = 0;
    if (gcv_) {
        n += gcv_->neq_;
    } else {
        lvardtloop(i, j) {
            n += p[i].lcv_[j].neq_;
        }
    }
    v->resize(n);
    vp = vector_vec(v);
    k = 0;
    if (gcv_) {
        gcv_->dstates(vp);
    } else {
        lvardtloop(i, j) {
            p[i].lcv_[j].dstates(vp + k);
            k += p[i].lcv_[j].neq_;
        }
    }
}

void nrn_cvfun(double t, double* y, double* ydot) {
    NetCvode* d = net_cvode_instance;
    d->gcv_->fun_thread(nrn_ensure_model_data_are_sorted(), t, y, ydot, nrn_threads);
}

double nrn_hoc2fixed_step(void*) {
    nrn_fixed_step(nrn_ensure_model_data_are_sorted());
    return 0.;
}

double nrn_hoc2fun(void* v) {
    NetCvode* d = (NetCvode*) v;
    double tt = *getarg(1);
    Vect* s = vector_arg(2);
    Vect* ds = vector_arg(3);
    if (!d->gcv_) {
        hoc_execerror("not global variable time step", 0);
    }
    if (s->size() != size_t(d->gcv_->neq_)) {
        hoc_execerror("size of state vector != number of state equations", 0);
    }
    if (nrn_nthread > 1) {
        hoc_execerror("only one thread allowed", 0);
    }
    ds->resize(s->size());
    nrn_cvfun(tt, vector_vec(s), vector_vec(ds));
    return 0.;
}

double nrn_hoc2scatter_y(void* v) {
    NetCvode* d = (NetCvode*) v;
    Vect* s = vector_arg(1);
    if (!d->gcv_) {
        hoc_execerror("not global variable time step", 0);
    }
    if (s->size() != size_t(d->gcv_->neq_)) {
        hoc_execerror("size of state vector != number of state equations", 0);
    }
    if (nrn_nthread > 1) {
        hoc_execerror("only one thread allowed", 0);
    }
    d->gcv_->scatter_y(nrn_ensure_model_data_are_sorted(), vector_vec(s), 0);
    return 0.;
}

double nrn_hoc2gather_y(void* v) {
    NetCvode* d = (NetCvode*) v;
    Vect* s = vector_arg(1);
    if (!d->gcv_) {
        hoc_execerror("not global variable time step", 0);
    }
    if (nrn_nthread > 1) {
        hoc_execerror("only one thread allowed", 0);
    }
    s->resize(d->gcv_->neq_);
    d->gcv_->gather_y(vector_vec(s), 0);
    return s->size();
}

void NetCvode::error_weights() {
    int i, j, k, n;
    Vect* v = vector_arg(1);
    if (!cvode_active_) {
        v->resize(0);
        return;
    }
    double* vp;
    n = 0;
    if (gcv_) {
        n += gcv_->neq_;
    } else {
        lvardtloop(i, j) {
            n += p[i].lcv_[j].neq_;
        }
    }
    v->resize(n);
    vp = vector_vec(v);
    k = 0;
    if (gcv_) {
        gcv_->error_weights(vp);
    } else {
        lvardtloop(i, j) {
            p[i].lcv_[j].error_weights(vp + k);
            k += p[i].lcv_[j].neq_;
        }
    }
}

void NetCvode::acor() {
    int i, j, k, n;
    Vect* v = vector_arg(1);
    if (!cvode_active_) {
        v->resize(0);
        return;
    }
    double* vp;
    n = 0;
    if (gcv_) {
        n += gcv_->neq_;
    } else {
        lvardtloop(i, j) {
            n += p[i].lcv_[j].neq_;
        }
    }
    v->resize(n);
    vp = vector_vec(v);
    k = 0;
    if (gcv_) {
        gcv_->acor(vp);
    } else {
        lvardtloop(i, j) {
            p[i].lcv_[j].acor(vp + k);
            k += p[i].lcv_[j].neq_;
        }
    }
}

/** @brief Create a lookup table for variable names.
 *
 *  This is only created on-demand because it involves building a lookup table
 *  of pointers, some of which are obtained from data_handles (and are therefore
 *  unstable). Eventually the operator<< of data_handle might provide the
 *  necessary functionality and this could be dropped completely.
 */
HocDataPaths NetCvode::create_hdp(int style) {
    int n{};
    if (gcv_) {
        n += gcv_->neq_;
    } else {
        int i, j;
        lvardtloop(i, j) {
            n += p[i].lcv_[j].neq_;
        }
    }
    HocDataPaths hdp{2 * n, style};
    if (gcv_) {
        for (int it = 0; it < nrn_nthread; ++it) {
            CvodeThreadData& z = gcv_->ctd_[it];
            for (int j = 0; j < z.nonvint_extra_offset_; ++j) {
                hdp.append(static_cast<double*>(z.pv_[j]));
            }
        }
    } else {
        int i, it;
        lvardtloop(it, i) {
            auto const neq = p[it].lcv_[i].ctd_[0].nvsize_;
            auto& pv = p[it].lcv_[i].ctd_[0].pv_;
            for (int j = 0; j < neq; ++j) {
                hdp.append(static_cast<double*>(pv[j]));
            }
        }
    }
    hdp.search();
    return hdp;
}

std::string NetCvode::statename(int is, int style) {
    if (!cvode_active_) {
        hoc_execerror("Cvode is not active", 0);
    }
    auto const n = [&]() {
        int n{};
        if (gcv_) {
            n += gcv_->neq_;
        } else {
            int i, j;
            lvardtloop(i, j) {
                n += p[i].lcv_[j].neq_;
            }
        }
        return n;
    }();
    if (is >= n) {
        hoc_execerror("Cvode::statename argument out of range", nullptr);
    }
    auto const impl = [hdp = create_hdp(style), style, this](auto& handle) -> std::string {
        auto* const raw_ptr = static_cast<double*>(handle);
        if (style == 2) {
            auto* sym = hdp.retrieve_sym(raw_ptr);
            assert(sym);
            return sym2name(sym);
        } else {
            std::string s = hdp.retrieve(raw_ptr);
            return !s.empty() ? s.c_str() : "unknown";
        }
    };
    int j{};
    if (gcv_) {
        for (int it = 0; it < nrn_nthread; ++it) {
            CvodeThreadData& z = gcv_->ctd_[it];
            if (j + z.nvoffset_ + z.nvsize_ > is) {
                return impl(z.pv_[is - j]);
            }
            j += z.nvsize_;
        }
    } else {
        int it, i;
        lvardtloop(it, i) {
            if (j + p[it].lcv_[i].neq_ > is) {
                CvodeThreadData& z = p[it].lcv_[i].ctd_[0];
                return impl(z.pv_[is - j]);
            }
            j += p[it].lcv_[i].neq_;
        }
    }
    return "unknown";
}

const char* NetCvode::sym2name(Symbol* sym) {
    if (sym->type == RANGEVAR && sym->u.rng.type > 1 && memb_func[sym->u.rng.type].is_point) {
        static char buf[200];
        Sprintf(buf, "%s.%s", memb_func[sym->u.rng.type].sym->name, sym->name);
        return buf;
    } else {
        return sym->name;
    }
}

Symbol* NetCvode::name2sym(const char* name) {
    std::vector<char> buf(strlen(name) + 1);
    strcpy(buf.data(), name);
    char* cp;
    for (cp = buf.data(); *cp; ++cp) {
        if (*cp == '.') {
            *cp = '\0';
            ++cp;
            break;
        }
    }
    Symbol* sym = hoc_table_lookup(buf.data(), hoc_built_in_symlist);
    if (!sym) {
        sym = hoc_table_lookup(buf.data(), hoc_top_level_symlist);
    }
    if (sym && *cp == '\0' && (sym->type == RANGEVAR || strcmp(sym->name, "Vector") == 0)) {
        return sym;
    } else if (sym && sym->type == TEMPLATE && *cp != '\0') {
        sym = hoc_table_lookup(cp, sym->u.ctemplate->symtable);
        if (sym) {
            return sym;
        }
    }
    hoc_execerror(name, "must be in form rangevar or Template.var");
    return nullptr;
}

void NetCvode::rtol(double x) {
    rtol_ = x;
}
void NetCvode::atol(double x) {
    atol_ = x;
}
void NetCvode::stiff(int x) {
    if ((stiff_ == 0) != (x == 0)) {  // need to free if change between 0 and nonzero
        if (gcv_) {
            gcv_->free_cvodemem();
        } else {
            int i, j;
            lvardtloop(i, j) {
                p[i].lcv_[j].free_cvodemem();
            }
        }
    }
    stiff_ = x;
}
void NetCvode::maxorder(int x) {
    maxorder_ = x;
    if (gcv_) {
        gcv_->free_cvodemem();
        gcv_->maxorder(maxorder_);
    } else {
        int i, j;
        lvardtloop(i, j) {
            p[i].lcv_[j].free_cvodemem();
            p[i].lcv_[j].maxorder(maxorder_);
        }
    }
}
int NetCvode::order(int ii) {
    int o = 0;
    if (gcv_) {
        o = gcv_->order();
    } else {
        int i, j, i2 = 0;
        lvardtloop(i, j) {
            if (ii == i2++) {
                o = p[i].lcv_[j].order();
            }
        }
    }
    return o;
}
void NetCvode::minstep(double x) {
    minstep_ = x;
    if (gcv_) {
        gcv_->minstep(minstep_);
    } else {
        int i, j;
        lvardtloop(i, j) {
            p[i].lcv_[j].minstep(minstep_);
        }
    }
}
void NetCvode::maxstep(double x) {
    maxstep_ = x;
    if (gcv_) {
        gcv_->maxstep(maxstep_);
    } else {
        int i, j;
        lvardtloop(i, j) {
            p[i].lcv_[j].maxstep(maxstep_);
        }
    }
}
void NetCvode::jacobian(int x) {
    jacobian_ = x;
}
void NetCvode::structure_change() {
    if (gcv_) {
        gcv_->structure_change_ = true;
    } else {
        int i, j;
        lvardtloop(i, j) {
            p[i].lcv_[j].structure_change_ = true;
        }
    }
}

NetCon* NetCvode::install_deliver(neuron::container::data_handle<double> dsrc,
                                  Section* ssrc,
                                  Object* osrc,
                                  Object* target,
                                  double threshold,
                                  double delay,
                                  double magnitude) {
    PreSyn* ps = nullptr;
    neuron::container::data_handle<double> psrc{};
    if (ssrc) {
        consist_sec_pd("NetCon", ssrc, dsrc);
    }
    if (!pst_) {
        pst_ = new PreSynTable(1000);
        pst_cnt_ = 0;
    }
    if (!psl_) {
        psl_ = hoc_l_newlist();
    }
    if (osrc) {
        assert(!dsrc);
        char buf[256];
        if (hoc_table_lookup("x", osrc->ctemplate->symtable)) {
            Point_process* pp = ob2pntproc(osrc);
            assert(pp && pp->prop);
            if (!pnt_receive[pp->prop->_type]) {  // only if no NET_RECEIVE block
                Sprintf(buf, "%s.x", hoc_object_name(osrc));
                psrc = hoc_val_handle(buf);
            }
        }
    } else {
        psrc = dsrc;
    }
    if (psrc) {
        auto psti = pst_->find(psrc);
        if (psti == pst_->end()) {
            ps = new PreSyn(psrc, osrc, ssrc);
            ps->hi_ = hoc_l_insertvoid(psl_, ps);
            (*pst_)[psrc] = ps;
            ++pst_cnt_;
        } else {
            ps = psti->second;
        }
        if (threshold != -1e9) {
            ps->threshold_ = threshold;
        }
    } else if (osrc) {
        Point_process* pnt = ob2pntproc(osrc);
        if (pnt->presyn_) {
            ps = (PreSyn*) pnt->presyn_;
        } else {
            ps = new PreSyn(psrc, osrc, ssrc);
            if (threshold != -1e9) {
                ps->threshold_ = threshold;
            }
            ps->hi_ = hoc_l_insertvoid(psl_, ps);
            pnt->presyn_ = ps;
        }
    } else if (target) {  // no source so use the special presyn
        if (!unused_presyn) {
            unused_presyn = new PreSyn({}, nullptr, nullptr);
            unused_presyn->hi_ = hoc_l_insertvoid(psl_, unused_presyn);
        }
        ps = unused_presyn;
    }
    ps_thread_link(ps);
    NetCon* d = new NetCon(ps, target);
    d->delay_ = delay;
    d->weight_[0] = magnitude;
    structure_change_cnt_ = 0;
    return d;
}

void NetCvode::psl_append(PreSyn* ps) {
    if (!psl_) {
        psl_ = hoc_l_newlist();
    }
    ps->hi_ = hoc_l_insertvoid(psl_, ps);
}

void NetCvode::presyn_disconnect(PreSyn* ps) {
    if (ps == unused_presyn) {
        unused_presyn = nullptr;
    }
    if (ps->hi_) {
        hoc_l_delete(ps->hi_);
        ps->hi_ = nullptr;
    }
    if (ps->hi_th_) {
        hoc_l_delete(ps->hi_th_);
        ps->hi_th_ = nullptr;
    }
    if (ps->thvar_) {
        --pst_cnt_;
        pst_->erase(ps->thvar_);
        ps->thvar_ = {};
    }
    if (gcv_) {
        for (int it = 0; it < gcv_->nctd_; ++it) {
            PreSynList* psl = gcv_->ctd_[it].psl_th_;
            if (psl)
                for (size_t j = 0; j < psl->size(); ++j) {
                    if ((*psl)[j] == ps) {
                        psl->erase(psl->begin() + j);
                        return;
                    }
                }
        }
    } else {
        int i, j;
        lvardtloop(i, j) {
            PreSynList* psl = p[i].lcv_[j].ctd_[0].psl_th_;
            if (psl)
                for (size_t j = 0; j < psl->size(); ++j) {
                    if ((*psl)[j] == ps) {
                        psl->erase(psl->begin() + j);
                        return;
                    }
                }
        }
    }
}

DiscreteEvent::DiscreteEvent() {}
DiscreteEvent::~DiscreteEvent() {}

DiscreteEvent* DiscreteEvent::savestate_save() {
    //	pr("savestate_save", 0, net_cvode_instance);
    if (this != null_event_) {
        pr("savestate_save", 0, net_cvode_instance);
        hoc_execerror("DiscreteEvent::savestate_save:", " is not the null_event_");
    }
    return new DiscreteEvent();
}

void DiscreteEvent::savestate_restore(double tt, NetCvode* nc) {
    //	pr("savestate_restore", tt, nc);
    Printf("null_event_ onto queue\n");
    nc->null_event(tt);
}

DiscreteEvent* DiscreteEvent::savestate_read(FILE* f) {
    return new DiscreteEvent();
}

void DiscreteEvent::savestate_write(FILE* f) {
    fprintf(f, "%d\n", DiscreteEventType);
}

NetCon::NetCon(PreSyn* src, Object* target) {
    NetConSave::invalid();
    obj_ = nullptr;
    src_ = src;
    delay_ = 1.0;
    if (src_) {
        src_->dil_.push_back(this);
        src_->use_min_delay_ = 0;
    }
    if (target == nullptr) {
        target_ = nullptr;
        active_ = false;
        cnt_ = 1;
        weight_ = new double[cnt_];
        weight_[0] = 0.0;
        return;
    }
    target_ = ob2pntproc(target);
    active_ = true;
#if DISCRETE_EVENT_OBSERVER
    ObjObservable::Attach(target, this);
#endif
    if (!pnt_receive[target_->prop->_type]) {
        hoc_execerror("No NET_RECEIVE in target PointProcess:", hoc_object_name(target));
    }
    cnt_ = pnt_receive_size[target_->prop->_type];
    weight_ = nullptr;
    if (cnt_) {
        weight_ = new double[cnt_];
        for (int i = 0; i < cnt_; ++i) {
            weight_[i] = 0.0;
        }
    }
}

NetCon::~NetCon() {
    // printf("~NetCon\n");
    NetConSave::invalid();
    rmsrc();
    if (cnt_) {
        delete[] weight_;
    }
#if DISCRETE_EVENT_OBSERVER
    if (target_) {
        ObjObservable::Detach(target_->ob, this);
    }
#endif
}

void NetCon::rmsrc() {
    if (src_) {
        for (size_t i = 0; i < src_->dil_.size(); ++i) {
            if (src_->dil_[i] == this) {
                src_->dil_.erase(src_->dil_.begin() + i);
                if (src_->dil_.size() == 0 && src_->tvec_ == NULL && src_->idvec_ == NULL) {
                    if (src_->output_index_ == -1) {
                        delete std::exchange(src_, nullptr);
                    }
                }
                break;
            }
        }
    }
    src_ = nullptr;
}

void NetCon::replace_src(PreSyn* p) {
    rmsrc();
    src_ = p;
    if (src_) {
        src_->dil_.push_back(this);
        src_->use_min_delay_ = 0;
    }
}

DiscreteEvent* NetCon::savestate_save() {
    //	pr("savestate_save", 0, net_cvode_instance);
    return new NetConSave(this);
}

NetConSave::NetConSave(NetCon* netcon) {
    netcon_ = netcon;
}
NetConSave::~NetConSave() {}

void NetConSave::savestate_restore(double tt, NetCvode* nc) {
    //	netcon_->pr("savestate_restore", tt, nc);
    NrnThread* nt;
    if (netcon_ && netcon_->target_) {
        nt = PP2NT(netcon_->target_);
        // printf("  on thread %d\n", nt->id);
    } else {
        nt = nrn_threads;
    }
    nc->event(tt, netcon_, nt);
}

DiscreteEvent* NetCon::savestate_read(FILE* f) {
    int index;
    char buf[200];
    //	fscanf(f, "%d\n", &index);
    nrn_assert(fgets(buf, 200, f));
    sscanf(buf, "%d\n", &index);
    NetCon* nc = NetConSave::index2netcon(index);
    assert(nc);
    return new NetConSave(nc);
}

void NetConSave::savestate_write(FILE* f) {
    fprintf(f, "%d\n", NetConType);
    fprintf(f, "%d\n", netcon_->obj_->index);
}

NetConSaveWeightTable* NetConSave::wtable_;
NetConSaveIndexTable* NetConSave::idxtable_;

void NetConSave::invalid() {
    delete std::exchange(wtable_, nullptr);
    delete std::exchange(idxtable_, nullptr);
}

NetCon* NetConSave::weight2netcon(double* pd) {
    NetCon* nc;
    if (!wtable_) {
        hoc_Item* q;
        Symbol* sym = hoc_lookup("NetCon");
        wtable_ = new NetConSaveWeightTable(2 * sym->u.ctemplate->count);
        ITERATE(q, sym->u.ctemplate->olist) {
            Object* obj = OBJ(q);
            nc = (NetCon*) obj->u.this_pointer;
            if (nc->weight_) {
                (*wtable_)[nc->weight_] = nc;
            }
        }
    }
    auto wti = wtable_->find(pd);
    if (wti != wtable_->end()) {
        nc = wti->second;
        assert(nc->weight_ == pd);
        return nc;
    } else {
        return nullptr;
    }
}

NetCon* NetConSave::index2netcon(long id) {
    NetCon* nc;
    if (!idxtable_) {
        hoc_Item* q;
        Symbol* sym = hoc_lookup("NetCon");
        idxtable_ = new NetConSaveIndexTable(2 * sym->u.ctemplate->count);
        ITERATE(q, sym->u.ctemplate->olist) {
            Object* obj = OBJ(q);
            nc = (NetCon*) obj->u.this_pointer;
            if (nc->weight_) {
                (*idxtable_)[obj->index] = nc;
            }
        }
    }
    auto idxti = idxtable_->find(id);
    if (idxti != idxtable_->end()) {
        nc = idxti->second;
        assert(nc->obj_->index == id);
        return nc;
    } else {
        return nullptr;
    }
}

void nrn_update_ps2nt() {
    net_cvode_instance->update_ps2nt();
}

void NetCvode::ps_thread_link(PreSyn* ps) {
    if (!ps) {
        return;
    }
    ps->nt_ = nullptr;
    if (!v_structure_change) {  // PP2NT etc are correct
        if (ps->osrc_) {
            ps->nt_ = PP2NT(ob2pntproc(ps->osrc_));
        } else if (ps->ssrc_) {
            ps->nt_ = ps->ssrc_->prop->dparam[9].get<NrnThread*>();
        }
    }
    if (!ps->nt_) {  // premature, reorder_secorder() not called yet
        return;
    }
    if (ps->thvar_) {
        int i = ps->nt_->id;
        if (!p[i].psl_thr_) {
            p[i].psl_thr_ = hoc_l_newlist();
        }
        ps->hi_th_ = hoc_l_insertvoid(p[i].psl_thr_, ps);
    }
}

void NetCvode::update_ps2nt() {
    int i;
    // first, opportunistically create p[]
    p_construct(nrn_nthread);
    // iterate over all threshold PreSyn and fill the NrnThread field
    hoc_Item* q;
    for (i = 0; i < nrn_nthread; ++i) {
        if (p[i].psl_thr_) {
            hoc_l_freelist(&p[i].psl_thr_);
        }
    }
    if (psl_)
        ITERATE(q, psl_) {
            PreSyn* ps = (PreSyn*) VOIDITM(q);
            ps_thread_link(ps);
        }
}

void NetCvode::p_construct(int n) {
    int i;
    if (pcnt_ != n) {
        delete[] std::exchange(p, nullptr);
        if (n > 0) {
            p = new NetCvodeThreadData[n];
        }
        pcnt_ = n;
    }
    for (i = 0; i < n; ++i) {
        p[i].unreffed_event_cnt_ = 0;
    }
}

PreSyn::PreSyn(neuron::container::data_handle<double> src, Object* osrc, Section* ssrc)
    : thvar_{std::move(src)} {
    //	printf("Presyn %x %s\n", (long)this, osrc?hoc_object_name(osrc):"nullptr");
    PreSynSave::invalid();
    hi_index_ = -1;
    hi_th_ = nullptr;
    flag_ = false;
    valthresh_ = 0;
    osrc_ = osrc;
    ssrc_ = ssrc;
    threshold_ = 10.;
    use_min_delay_ = 0;
    tvec_ = nullptr;
    idvec_ = nullptr;
    stmt_ = nullptr;
    gid_ = -1;
    nt_ = nullptr;
    if (thvar_) {
        if (osrc) {
            nt_ = PP2NT(ob2pntproc(osrc));
        } else if (ssrc) {
            nt_ = ssrc->prop->dparam[9].get<NrnThread*>();
        }
    }
    if (osrc_ && !thvar_) {
        nt_ = PP2NT(ob2pntproc(osrc));
    }
#if 1 || USENCS || NRNMPI
    output_index_ = -1;
#endif
#if NRNMPI
    bgp.multisend_send_ = 0;
#endif
#if NRN_MUSIC
    music_port_ = 0;
#endif
#if DISCRETE_EVENT_OBSERVER
    if (thvar_) {
        neuron::container::notify_when_handle_dies(thvar_, this);
    } else if (osrc_) {
        nrn_notify_when_void_freed(osrc_, this);
    }
#endif
}

PreSyn::~PreSyn() {
    PreSynSave::invalid();
    //	printf("~PreSyn %p\n", this);
    nrn_cleanup_presyn(this);
    delete std::exchange(stmt_, nullptr);
#if DISCRETE_EVENT_OBSERVER
    if (tvec_) {
        ObjObservable::Detach(tvec_->obj_, this);
        tvec_ = nullptr;
    }
    if (idvec_) {
        ObjObservable::Detach(idvec_->obj_, this);
        idvec_ = nullptr;
    }
#endif
    if (thvar_ || osrc_) {
#if DISCRETE_EVENT_OBSERVER
        nrn_notify_pointer_disconnect(this);
#endif
        if (!thvar_) {
            // even if the point process section was deleted earlier
            Point_process* pnt = ob2pntproc_0(osrc_);
            if (pnt) {
                pnt->presyn_ = nullptr;
            }
        }
    }
    for (const auto& d: dil_) {
        d->src_ = nullptr;
    }
    net_cvode_instance->presyn_disconnect(this);
}

DiscreteEvent* PreSyn::savestate_save() {
    //	pr("savestate_save", 0, net_cvode_instance);
    return new PreSynSave(this);
}

PreSynSave::PreSynSave(PreSyn* presyn) {
    presyn_ = presyn;
}
PreSynSave::~PreSynSave() {}

void PreSynSave::savestate_restore(double tt, NetCvode* nc) {
    //	presyn_->pr("savestate_restore", tt, nc);
    nc->event(tt, presyn_, presyn_->nt_);
}

DiscreteEvent* PreSyn::savestate_read(FILE* f) {
    PreSyn* ps = nullptr;
    char buf[200];
    int index, tid;
    nrn_assert(fgets(buf, 200, f));
    nrn_assert(sscanf(buf, "%d %d\n", &index, &tid) == 2);
    ps = PreSynSave::hindx2presyn(index);
    assert(ps);
    ps->nt_ = nrn_threads + tid;
    return new PreSynSave(ps);
}

void PreSynSave::savestate_write(FILE* f) {
    fprintf(f, "%d\n", PreSynType);
    fprintf(f, "%ld %d\n", presyn_->hi_index_, presyn_->nt_ ? presyn_->nt_->id : 0);
}

PreSynSaveIndexTable* PreSynSave::idxtable_;

void PreSynSave::invalid() {
    delete std::exchange(idxtable_, nullptr);
}

PreSyn* PreSynSave::hindx2presyn(long id) {
    PreSyn* ps;
    if (!idxtable_) {
        hoc_Item* q;
        int cnt = 0;
        ITERATE(q, net_cvode_instance->psl_) {
            ++cnt;
        }
        // printf("%d PreSyn instances\n", cnt);
        idxtable_ = new PreSynSaveIndexTable(2 * cnt);
        cnt = 0;
        ITERATE(q, net_cvode_instance->psl_) {
            ps = (PreSyn*) VOIDITM(q);
            assert(ps->hi_index_ == cnt);
            (*idxtable_)[ps->hi_index_] = ps;
            ++cnt;
        }
    }
    auto idxti = idxtable_->find(id);
    if (idxti != idxtable_->end()) {
        ps = idxti->second;
        assert(ps->hi_index_ == id);
        return ps;
    } else {
        return nullptr;
    }
}

void PreSyn::init() {
    qthresh_ = nullptr;
    if (tvec_) {
        tvec_->resize(0);
    }
    if (idvec_) {
        idvec_->resize(0);
    }
}

void PreSyn::record_stmt(const char* stmt) {
    delete std::exchange(stmt_, nullptr);
    if (strlen(stmt) > 0) {
        stmt_ = new HocCommand(stmt);
    }
}

void PreSyn::record_stmt(Object* pyact) {
    delete std::exchange(stmt_, nullptr);
    if (pyact) {
        stmt_ = new HocCommand(pyact);
    }
}

void PreSyn::record(IvocVect* vec, IvocVect* idvec, int rec_id) {
#if DISCRETE_EVENT_OBSERVER
    if (tvec_) {
        ObjObservable::Detach(tvec_->obj_, this);
    }
    if (idvec_) {
        ObjObservable::Detach(idvec_->obj_, this);
    }
#endif
    tvec_ = vec;
    idvec_ = idvec;
    rec_id_ = rec_id;
#if DISCRETE_EVENT_OBSERVER
    if (tvec_) {
        ObjObservable::Attach(tvec_->obj_, this);
    }
    if (idvec_) {
        ObjObservable::Attach(idvec_->obj_, this);
        tvec_->mutconstruct(1);
    }
#endif
}

void PreSyn::record(double tt) {
    if (tvec_) {
        // need to lock the vector if shared by other PreSyn
        // since we get here in the thread that manages the
        // threshold detection (or net_event from NET_RECEIVE).
        if (idvec_) {
            tvec_->lock();
        }
        tvec_->push_back(tt);
        if (idvec_) {
            idvec_->push_back(rec_id_);
            tvec_->unlock();
        }
    }
    if (stmt_) {
        if (nrn_nthread > 1) {
            nrn_hoc_lock();
        }
        t = tt;
        stmt_->execute(false);
        if (nrn_nthread > 1) {
            nrn_hoc_unlock();
        }
    }
}

void PreSyn::disconnect(Observable* o) {
    // printf("PreSyn::disconnect %s\n", hoc_object_name(((ObjObservable*)o)->object()));
    if (tvec_ && tvec_->obj_ == ((ObjObservable*) o)->object()) {
        tvec_ = nullptr;
    }
    if (idvec_ && idvec_->obj_ == ((ObjObservable*) o)->object()) {
        idvec_ = nullptr;
    }
    if (dil_.size() == 0 && tvec_ == nullptr && idvec_ == nullptr && output_index_ == -1) {
        delete this;
    }
}

void PreSyn::update(Observable* o) {  // should be disconnect
                                      // printf("PreSyn::update\n");
    for (const auto& d: dil_) {
#if 0  // osrc_ below is invalid
if (d->obj_) {
	printf("%s disconnect from ", hoc_object_name(d->obj_));
	printf("source %s\n", osrc_ ? hoc_object_name(osrc_) : secname(ssrc_));
}
#endif
        d->src_ = nullptr;
    }
    if (tvec_) {
#if DISCRETE_EVENT_OBSERVER
        ObjObservable::Detach(tvec_->obj_, this);
#endif
        tvec_ = nullptr;
    }
    if (idvec_) {
#if DISCRETE_EVENT_OBSERVER
        ObjObservable::Detach(idvec_->obj_, this);
#endif
        idvec_ = nullptr;
    }
    net_cvode_instance->presyn_disconnect(this);
    thvar_ = {};
    osrc_ = nullptr;
    delete this;
}

void ConditionEvent::check(NrnThread* nt, double tt, double teps) {
    if (value() > 0.0) {
        if (flag_ == false) {
            flag_ = true;
            valthresh_ = 0.;
#if USENEOSIM
            if (neosim_entity_) {
                (*p_nrn2neosim_send)(neosim_entity_, tt);
            } else {
#endif
                send(tt + teps, net_cvode_instance, nt);
#if USENEOSIM
            }
#endif
        }
    } else {
        flag_ = false;
    }
}

ConditionEvent::ConditionEvent() {
    qthresh_ = NULL;
    valold_ = 0.0;
}
ConditionEvent::~ConditionEvent() {}

void ConditionEvent::condition(Cvode* cv) {  // logic for high order threshold detection
    // printf("ConditionEvent::condition f=%d t=%20.15g v=%20.15g\n", flag_, t, value());
    NrnThread* nt = thread();
    if (qthresh_) {  // the threshold event has not
        // been handled. i.e. the cell must have retreated to
        // a time not later  than the threshold time.
        assert(nt->_t <= qthresh_->t_);
        abandon_statistics(cv);
        // abandon the event
        STATISTICS(abandon_);
        net_cvode_instance->remove_event(qthresh_, nt->id);
        qthresh_ = nullptr;
        valthresh_ = 0.;
        flag_ = false;
    }

    double val = value();
    if (flag_ == false && val >= 0.0) {  // above threshold
        flag_ = true;
        valthresh_ = 0.;
        if (cv->t0_ == cv->tn_) {  // inited
            // means immediate threshold event now.
            // no need for qthresh since there is
            // no question of abandoning it so instead
            // of a qthresh it is a send event.
            STATISTICS(init_above_);
            send(nt->_t, net_cvode_instance, nt);
        } else {  // crossed somewhere in the told to t interval
            STATISTICS(send_qthresh_);
            // reset the flag_ when the value goes lower than
            // valold since value() when qthresh_ handled
            // may in fact be below 0.0
            valthresh_ = valold_;
            double th = -valold_ / (val - valold_);
            th = th * nt->_t + (1. - th) * told_;
            assert(th >= cv->t0_ && th <= cv->tn_);
            qthresh_ = net_cvode_instance->event(th, this, nt);
        }
    } else if (flag_ == true && valold_ < valthresh_ && val < valthresh_) {
        // below threshold
        // previous step crossed in negative direction
        // and there was not any retreat or initialization
        // to give spurious crossing.
        flag_ = false;
    }
    valold_ = val;
    told_ = nt->_t;
}

double STECondition::value() {
    double val = stet_->value();
    return val;
}

void ConditionEvent::abandon_statistics(Cvode* cv) {
#if 1
    // printf("ConditionEvent::condition %s t=%20.15g abandon event at %20.15g\n",
    // ssrc_?secname(ssrc_):"", t, qthresh_->t_);
    if (nt_t == qthresh_->t_) {  // it is not clear whether
        // this could happen and if it does it may
        // take fastidiousness to
        // an extreme
        STATISTICS(eq_abandon_);
        Printf("abandon when t == qthresh_->t_ = %20.15g\n", nt_t);
    }
    if (cv->t0_ == cv->tn_) {  // inited
        if (value() > 0.0) {   // above threshold
            STATISTICS(abandon_init_above_);
        } else {
            STATISTICS(abandon_init_below_);
        }
    } else {
        if (value() > 0.0) {  // above threshold
            STATISTICS(abandon_above_);
        } else {
            STATISTICS(abandon_below_);
        }
    }
#endif
}

WatchCondition::WatchCondition(Point_process* pnt, double (*c)(Point_process*))
    : HTList(nullptr) {
    pnt_ = pnt;
    c_ = c;
    watch_index_ = 0;  // For transfer, will be a small positive integer.
}

WatchCondition::~WatchCondition() {
    // printf("~WatchCondition\n");
    Remove();
}

// A WatchCondition but with different deliver
STECondition::STECondition(Point_process* pnt, double (*c)(Point_process*))
    : WatchCondition(pnt, c) {
    // printf("STECondition\n");
}

STECondition::~STECondition() {
    // printf("~STECondition\n");
}

void WatchCondition::activate(double flag) {
    Cvode* cv = NULL;
    int id = 0;
    qthresh_ = nullptr;
    flag_ = (value() >= -hoc_epsilon) ? true : false;
    valthresh_ = 0.;
    nrflag_ = flag;
    if (!pnt_) {  // possible for StateTransitionEvent
        // but only if 1 thread and no lvardt
        assert(nrn_nthread == 1);
        assert(net_cvode_instance->localstep() == false);
        cv = net_cvode_instance->gcv_;
    } else {
        cv = (Cvode*) pnt_->nvi_;
    }
    assert(cv);
    id = (cv->nctd_ > 1) ? thread()->id : 0;
    HTList*& wl = cv->ctd_[id].watch_list_;
    if (!wl) {
        wl = new HTList(nullptr);
        net_cvode_instance->wl_list_[id].push_back(wl);
    }
    Remove();
    wl->Append(this);
}

void WatchCondition::asf_err() {
    fprintf(stderr, "WATCH condition with flag=%g for %s\n", nrflag_, hoc_object_name(pnt_->ob));
}

void PreSyn::asf_err() {
    fprintf(stderr, "PreSyn threshold for %s\n", osrc_ ? hoc_object_name(osrc_) : secname(ssrc_));
}

void WatchCondition::send(double tt, NetCvode* nc, NrnThread* nt) {
    qthresh_ = nc->event(tt, this, nt);
    STATISTICS(watch_send_);
}

void WatchCondition::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    if (qthresh_) {
        qthresh_ = nullptr;
        STATISTICS(deliver_qthresh_);
    }
    Cvode* cv = (Cvode*) pnt_->nvi_;
    int type = pnt_->prop->_type;
    if (cvode_active_ && cv) {
        ns->local_retreat(tt, cv);
        cv->set_init_flag();
    } else {
        PP2t(pnt_) = tt;
    }
    STATISTICS(watch_deliver_);
    POINT_RECEIVE(type, pnt_, nullptr, nrflag_);
    if (errno) {
        if (nrn_errno_check(type)) {
            hoc_warning("errno set during WatchCondition deliver to NET_RECEIVE", (char*) 0);
        }
    }
}

void StateTransitionEvent::transition(int src,
                                      int dest,
                                      neuron::container::data_handle<double> var1,
                                      neuron::container::data_handle<double> var2,
                                      std::unique_ptr<HocCommand> hc) {
    STETransition& st = states_[src].add_transition(pnt_);
    st.dest_ = dest;
    st.var1_ = std::move(var1);
    st.var2_ = std::move(var2);
    st.hc_ = std::move(hc);
    st.ste_ = this;
    st.var1_is_time_ = (static_cast<double*>(st.var1_) == &t);
}

void STETransition::activate() {
    if (var1_is_time_) {
        var1_ = neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                       &stec_->thread()->_t};
    }
    if (stec_->qthresh_) {  // is it on the queue
        net_cvode_instance->remove_event(stec_->qthresh_, stec_->thread()->id);
        stec_->qthresh_ = NULL;
    }
    stec_->activate(0);
}

void STETransition::deactivate() {
    if (stec_->qthresh_) {  // is it on the queue
        net_cvode_instance->remove_event(stec_->qthresh_, stec_->thread()->id);
        stec_->qthresh_ = nullptr;
    }
    stec_->Remove();
}

void STECondition::deliver(double tt, NetCvode* ns, NrnThread* nt) {
    if (qthresh_) {
        qthresh_ = nullptr;
        STATISTICS(deliver_qthresh_);
    }
    if (!pnt_) {
        assert(nrn_nthread == 1 && ns->localstep() == false);
        if (cvode_active_) {
            Cvode* cv = ns->gcv_;
            ns->local_retreat(tt, cv);
            cv->set_init_flag();
        } else {
            nt->_t = tt;
        }
    } else {
        Cvode* cv = (Cvode*) pnt_->nvi_;
        if (cvode_active_ && cv) {
            ns->local_retreat(tt, cv);
            cv->set_init_flag();
        } else {
            PP2t(pnt_) = tt;
        }
    }
    STATISTICS(watch_deliver_);
    t = tt;
    stet_->event();
}

NrnThread* WatchCondition::thread() {
    return PP2NT(pnt_);
}

NrnThread* STECondition::thread() {
    if (pnt_) {
        return PP2NT(pnt_);
    } else {
        assert(nrn_nthread == 1);
        return nrn_threads;
    }
}

void WatchCondition::pgvts_deliver(double tt, NetCvode* ns) {
    assert(0);
    if (qthresh_) {
        qthresh_ = nullptr;
        STATISTICS(deliver_qthresh_);
    }
    int type = pnt_->prop->_type;
    STATISTICS(watch_deliver_);
    POINT_RECEIVE(type, pnt_, nullptr, nrflag_);
    if (errno) {
        if (nrn_errno_check(type)) {
            hoc_warning("errno set during WatchCondition deliver to NET_RECEIVE", (char*) 0);
        }
    }
}

void STECondition::pgvts_deliver(double tt, NetCvode* ns) {
    assert(0);
    if (qthresh_) {
        qthresh_ = nullptr;
        STATISTICS(deliver_qthresh_);
    }
    int type = pnt_->prop->_type;
    STATISTICS(watch_deliver_);
    stet_->event();
    if (errno) {
        if (nrn_errno_check(type)) {
            hoc_warning("errno set during STECondition pgvts_deliver to NET_RECEIVE", (char*) 0);
        }
    }
}

void WatchCondition::pr(const char* s, double tt, NetCvode* ns) {
    Printf("%s", s);
    Printf(" WatchCondition %s %.15g flag=%g\n", hoc_object_name(pnt_->ob), tt, nrflag_);
}

static Cvode* eval_cv;
static void* eval_cond(NrnThread* nt) {
    eval_cv->evaluate_conditions(nt);
    return 0;
}
void Cvode::evaluate_conditions(NrnThread* nt) {
    if (!nt) {
        if (nrn_nthread > 1 && nctd_ > 1) {
            eval_cv = this;
            nrn_multithread_job(eval_cond);
            return;
        }
        nt = nrn_threads;
    }
    CvodeThreadData& z = CTD(nt->id);
    if (z.psl_th_) {
        // originally was a reverse iteration, but I thing that was
        // just to avoid continually calling count() on the List.
        for (auto ps: *(z.psl_th_)) {
            ps->condition(this);
        }
    }
    if (z.watch_list_) {
        for (HTList* item = z.watch_list_->First(); item != z.watch_list_->End();
             item = item->Next()) {
            ((WatchCondition*) item)->condition(this);
        }
    }
}

static void* chk_deliv(NrnThread* nt) {
    eval_cv->check_deliver(nt);
    return 0;
}
void Cvode::check_deliver(NrnThread* nt) {
    if (!nt) {
        if (nrn_nthread > 1) {
            eval_cv = this;
            nrn_multithread_job(chk_deliv);
            return;
        }
        nt = nrn_threads;
    }
    CvodeThreadData& z = CTD(nt->id);
    if (z.psl_th_) {
        // originally was a reverse iteration, but I thing that was
        // just to avoid continually calling count() on the List.
        for (auto ps: *(z.psl_th_)) {
            ps->check(nt, nt->_t);
        }
    }
    if (z.watch_list_) {
        for (HTList* item = z.watch_list_->First(); item != z.watch_list_->End();
             item = item->Next()) {
            ((WatchCondition*) item)->check(nt, nt->_t);
        }
    }
}

void NetCvode::fixed_record_continuous(neuron::model_sorted_token const& cache_token,
                                       NrnThread& nt) {
    nrn_ba(cache_token, nt, BEFORE_STEP);
    for (auto& pr: *fixed_record_) {
        if (pr->ith_ == nt.id) {
            pr->continuous(nt._t);
        }
    }
}

void NetCvode::fixed_play_continuous(NrnThread* nt) {
    for (auto& pr: *fixed_play_) {
        if (pr->ith_ == nt->id) {
            pr->continuous(nt->_t);
        }
    }
}

// nrnthread_get_trajectory_requests helper for buffered trajectories
// also for per time step return (no Vector and varrays is NULL)
// if bsize > 0 then CoreNEURON will write that number of values to the vectors.
// If CoreNEURON has a start time > 0, (current value of t),
// the amount to augment the vector depends
// on the vector's current size in the current NEURON context and the
// varray[i_trajec] double* will be determined by that current size.
// However determination of whether to do per time step return or buffering
// can be specified on the NEURON side.
static int trajec_buffered(NrnThread& nt,
                           int bsize,
                           IvocVect* v,
                           neuron::container::data_handle<double> pd,
                           int i_pr,
                           PlayRecord* pr,
                           void** vpr,
                           int i_trajec,
                           int* types,
                           int* indices,
                           double** pvars,
                           double** varrays) {
    int err = 0;  // success
    if (bsize > 0) {
        int cur_size = v->size();
        if (v->buffer_size() < bsize + cur_size) {
            v->buffer_size(bsize + cur_size);
        }
        // nrnthread_trajectory_values will resize to correct size.
        v->resize(bsize + cur_size);
        varrays[i_trajec] = vector_vec(v) + cur_size;  // begin filling here
    } else {
        // Danger, think this through better
        pvars[i_trajec] = static_cast<double*>(pd);
    }
    vpr[i_pr] = pr;
    if (static_cast<double const*>(pd) == &nt._t) {
        types[i_trajec] = 0;
        indices[i_trajec] = 0;
    } else {
        err = nrn_dblpntr2nrncore(pd, nt, types[i_trajec], indices[i_trajec]);
        if (err) {
            Fprintf(stderr,
                    "Pointer %p of PlayRecord type %d ignored because not a Range Variable",
                    static_cast<double*>(pd),
                    pr->type());
        }
    }
    return err;
}

// Transfer of trajectories from CoreNEURON to NEURON.
// bsize == 0: Most flexible interactivity, transfer on a per time step basis.
// bsize > 0: Greatest performance, fill the entire trajectory arrays on
// the CoreNEURON side and transferring at the end of a CoreNEURON run.
// Here, we ensure the arrays have at least bsize size (i.e. at least tstop/dt)
// beyond their current size and CoreNEURON will start filling from the
// current fill time, h.t, location of the arrays. I.e. starting at CoreNEURON's
// start time. (Multiple calls to psolve append to these arrays.)
// n_pr refers the the number of PlayRecord instances in the vpr array.
// n_trajec refers to the number of trajectories to be recorded on the
// CoreNEURON side and is the size of the types, indices, and varrays.
// n_pr is different from n_trajec when one of the GLineRecord instances has
// a gl_->name that is an expression that contains several range variables.
// Per time step transfer now uses pvars so CoreNEURON scatters values
// to pvars instead of collecting in double array.
void nrnthread_get_trajectory_requests(int tid,
                                       int& bsize,
                                       int& n_pr,
                                       void**& vpr,
                                       int& n_trajec,
                                       int*& types,
                                       int*& indices,
                                       double**& pvars,
                                       double**& varrays) {
    if (bsize > 0) {  // but would NEURON rather use per time step mode
        if (nrn_trajectory_request_per_time_step_) {
            bsize = 0;
        }
    }
    n_pr = 0;
    n_trajec = 0;
    types = NULL;
    indices = NULL;
    vpr = NULL;
    varrays = NULL;
    pvars = NULL;
    if (tid < nrn_nthread) {
        NrnThread& nt = nrn_threads[tid];
        auto* fr = net_cvode_instance->fixed_record_;
        // allocate
        for (auto& pr: *fr) {
            if (pr->ith_ == tid) {
                if (pr->type() == TvecRecordType || pr->type() == YvecRecordType) {
                    n_pr++;
                    n_trajec++;
#if HAVE_IV
                } else if (pr->type() == GLineRecordType) {
                    n_pr++;
                    if (!pr->pd_) {
                        GLineRecord* glr = (GLineRecord*) pr;
                        assert(glr->gl_->expr_);
                        glr->fill_pd();
                        if (pr->pd_) {
                            n_trajec++;
                        } else {
                            n_trajec += glr->pd_and_vec_.size();
                        }
                    } else {
                        n_trajec++;
                    }
                } else if (pr->type() == GVectorRecordType) {
                    GVectorRecord* gvr = (GVectorRecord*) pr;
                    if (gvr->count()) {
                        bsize = 0;
                        n_pr++;
                        for (int j = 0; j < gvr->count(); ++j) {
                            if (gvr->pdata(j)) {
                                n_trajec++;
                            }
                        }
                    }
#endif  // HAVE_IV
                }
            }
        }
        if (n_pr == 0) {
            return;
        }
        vpr = new void*[n_pr];
        types = new int[n_trajec];
        indices = new int[n_trajec];
        if (bsize > 0) {
            varrays = new double*[n_trajec];
        } else {
            pvars = new double*[n_trajec];
        }  // if both varrays and pvars are NULL then CoreNEURON will return values every time step
        // everything allocated, start over and fill
        n_pr = 0;
        n_trajec = 0;
        for (auto& pr: *fr) {
            int err = 0;
            if (pr->ith_ == tid) {
                if (1) {  // buffered or per time step value return
                    if (pr->type() == TvecRecordType) {
                        IvocVect* v = ((TvecRecord*) pr)->t_;
                        err = trajec_buffered(nt,
                                              bsize,
                                              v,
                                              neuron::container::data_handle<double>{
                                                  neuron::container::do_not_search, &nt._t},
                                              n_pr++,
                                              pr,
                                              vpr,
                                              n_trajec++,
                                              types,
                                              indices,
                                              pvars,
                                              varrays);
                        if (err) {
                            n_pr--;
                            n_trajec--;
                        }
                    } else if (pr->type() == YvecRecordType) {
                        IvocVect* v = ((YvecRecord*) pr)->y_;
                        err = trajec_buffered(nt,
                                              bsize,
                                              v,
                                              pr->pd_,
                                              n_pr++,
                                              pr,
                                              vpr,
                                              n_trajec++,
                                              types,
                                              indices,
                                              pvars,
                                              varrays);
                        if (err) {
                            n_pr--;
                            n_trajec--;
                        }
#if HAVE_IV
                    } else if (pr->type() == GLineRecordType) {
                        GLineRecord* glr = (GLineRecord*) pr;
                        if (pr->pd_) {  // glr->gl_->name is an expression resolved to a double*
                            if (bsize && !glr->v_) {
                                glr->v_ = new IvocVect(bsize);
                            }
                            IvocVect* v = glr->v_;
                            err = trajec_buffered(nt,
                                                  bsize,
                                                  v,
                                                  pr->pd_,
                                                  n_pr++,
                                                  pr,
                                                  vpr,
                                                  n_trajec++,
                                                  types,
                                                  indices,
                                                  pvars,
                                                  varrays);
                            if (err) {
                                n_pr--;
                                n_trajec--;
                            }
                        } else {  // glr->gl_->name expression involves several range variables
                            int n = n_trajec;
                            for (auto&& [pd, v]: glr->pd_and_vec_) {
                                assert(pd);
                                if (bsize && v == nullptr) {
                                    v = new IvocVect(bsize);
                                }
                                // TODO avoid the conversion?
                                err = trajec_buffered(nt,
                                                      bsize,
                                                      v,
                                                      neuron::container::data_handle<double>{pd},
                                                      n_pr,
                                                      pr,
                                                      vpr,
                                                      n_trajec++,
                                                      types,
                                                      indices,
                                                      pvars,
                                                      varrays);
                                if (err) {
                                    break;
                                }
                            }
                            n_pr++;
                            if (err) {  // single error removes entire GLineRecord
                                n_pr--;
                                n_trajec = n;
                            }
                        }
                    } else if (pr->type() == GVectorRecordType) {
                        GVectorRecord* gvr = (GVectorRecord*) pr;
                        if (gvr->count()) {
                            int n = n_trajec;
                            for (int j = 0; j < gvr->count(); ++j) {
                                if (gvr->pdata(j)) {
                                    err = trajec_buffered(nt,
                                                          bsize,
                                                          NULL,
                                                          gvr->pdata(j),
                                                          n_pr,
                                                          pr,
                                                          vpr,
                                                          n_trajec++,
                                                          types,
                                                          indices,
                                                          pvars,
                                                          varrays);
                                    if (err) {
                                        break;
                                    }
                                }
                            }
                            n_pr++;
                            if (err) {  // single error removes entire GVectorRecord
                                n_pr--;
                                n_trajec = n;
                            }
                        }
#endif  // HAVE_IV
                    }
                }
            }
        }
        if (n_trajec == 0) {  // if errors reduced to 0, clean up
            assert(n_pr == 0);
            delete[] std::exchange(types, nullptr);
            delete[] std::exchange(indices, nullptr);
            delete[] std::exchange(vpr, nullptr);
            delete[] std::exchange(varrays, nullptr);
            delete[] std::exchange(pvars, nullptr);
        }
#if 0
    printf("nrnthread_get_trajectory_requests tid=%d bsize=%d n_pr=%d n_trajec=%d\n", tid, bsize, n_pr, n_trajec);
    int i_trajec = 0;
    double* pd;
    for (int i=0; i < n_pr; ++i) {
      PlayRecord* pr = (PlayRecord*)vpr[i];
      pd = pr->pd_;
      if (pd) {
        printf("    %d %d prtype=%d %p type=%d  index=%d\n", i, i_trajec, pr->type(), pd, types[i_trajec], indices[i_trajec]);
        i_trajec++;
      }else{
        assert(pr->type() == GLineRecordType);
        GLineRecord* glr = (GLineRecord*)pr;
        GLineRecordEData& ed = glr->pd_and_vec_;
        i_trajec += ed.size();
      }
    }
#endif
    }
}

void nrnthread_trajectory_values(int tid, int n_pr, void** vpr, double tt) {  //, int n_trajec,
                                                                              // double* values) {
    if (tid < 0) {
        return;
    }
    if (tid < nrn_nthread) {
#if HAVE_IV
        ObjectContext obc(NULL);  // in case GLineRecord with expression.
#endif                            // HAVE_IV
        nrn_threads[tid]._t = tt;
        if (tid == 0) {
            t = tt;
        }
        int flush = 0;
        for (int i = 0; i < n_pr; ++i) {
            PlayRecord* pr = (PlayRecord*) vpr[i];
            pr->continuous(tt);
#if HAVE_IV
            if (pr->type() == GVectorRecordType) {  // see the movie
                flush = 1;
            }
        }
        if (flush) {
            Oc oc;
            oc.run("screen_update()\n");
        }
#else
        }
#endif  // HAVE_IV
    }
}

// CoreNEURON received pointers to the data of each Vector (resized so that
// the data is long enough to hold the trajectory (tstop/dt). On calling
// this, each Vector needs only be resized to the actual vecsz.
// Note that we have not bothered to fill the variables on the hoc side with
// the last values, though we did fill t.
// In the case where glr->pd_ = NULL, the GraphLine is an expression that
// cannot be interpreted as a pointer and the expression involves one or
// more range variables that individually were resolved into pointers. In
// this case in glr->plot, the relevant Vector elements are copied into those
// pointers and the expression is then evaluated and plotted.
void nrnthread_trajectory_return(int tid, int n_pr, int bsize, int vecsz, void** vpr, double tt) {
    if (tid < 0) {
        return;
    }
    if (tid < nrn_nthread) {
        nrn_threads[tid]._t = tt;
        if (tid == 0) {
            t = tt;
        }
        for (int i = 0; i < n_pr; ++i) {
            PlayRecord* pr = (PlayRecord*) vpr[i];
            IvocVect* v = NULL;
            if (pr->type() == TvecRecordType) {
                v = ((TvecRecord*) pr)->t_;
                // reserved bsize but only used vecsz. (need non-zeroing resize).
                v->resize(v->size() - (bsize - vecsz));  // do not zero
            } else if (pr->type() == YvecRecordType) {
                v = ((YvecRecord*) pr)->y_;
                v->resize(v->size() - (bsize - vecsz));  // do not zero
#if HAVE_IV
            } else if (pr->type() == GLineRecordType) {
                GLineRecord* glr = (GLineRecord*) pr;
                glr->plot(vecsz, tt);
#endif  // HAVE_IV
            } else {
                assert(0);
            }
        }
    }
}

// factored this out from deliver_net_events so we can
// stay in the cache
void NetCvode::check_thresh(NrnThread* nt) {  // for default method
    nrn::Instrumentor::phase p_check_threshold("check-threshold");
    hoc_Item* pth = p[nt->id].psl_thr_;

    if (pth) { /* only look at ones with a threshold */
        hoc_Item* q1;
        ITERATE(q1, pth) {
            PreSyn* ps = (PreSyn*) VOIDITM(q1);
            // only the ones for this thread
            if (ps->nt_ == nt) {
                if (ps->thvar_) {
                    ps->check(nt, nt->_t, 1e-10);
                }
            }
        }
    }

    for (HTList* wl: wl_list_[nt->id]) {
        for (HTList* item = wl->First(); item != wl->End(); item = item->Next()) {
            WatchCondition* wc = (WatchCondition*) item;
            wc->check(nt, nt->_t);
        }
    }
}

/** In nrncore_callbacks.cpp **/
extern void nrn2core_transfer_WatchCondition(WatchCondition* wc,
                                             void (*cb)(int, int, int, int, int));
extern "C" {
void nrn2core_transfer_WATCH(void (*cb)(int, int, int, int, int));
}
void nrn2core_transfer_WATCH(void (*cb)(int, int, int, int, int)) {
    // should be revisited for possible simplification since wl_list now
    // segregated by threads.
    for (auto& htlists_of_thread: net_cvode_instance->wl_list_) {
        for (HTList* wl: htlists_of_thread) {
            for (HTList* item = wl->First(); item != wl->End(); item = item->Next()) {
                WatchCondition* wc = (WatchCondition*) item;
                nrn2core_transfer_WatchCondition(wc, cb);
            }
        }
    }
}

void NetCvode::deliver_net_events(NrnThread* nt) {  // for default method
    TQItem* q;
    double tm, tsav;
#if NRNMPI
    if (use_multisend_) {
        nrn_multisend_advance();
    }
#endif
    int tid = nt->id;
    tsav = nt->_t;
    tm = nt->_t + 0.5 * nt->_dt;
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
        // it was noticed on binq + compressed spike exchange +
        // threads that a transferred event may be languishing in
        // the interthread event buffer. Perhaps this is better done
        // as a multithread job at the end of nrn_spike_exchange
        // instead of every time step --- but here we are
        // already in a multithread job, so what is the overhead of
        // starting such a small one in nrn_spike_exchange.
#if NRNMPI
        extern bool nrn_use_compress_;
        if (nrn_use_compress_ && nrn_nthread > 1) {
            p[tid].enqueue(this, nt);
        }
#endif
        while ((q = p[tid].tqe_->dequeue_bin()) != 0) {
            DiscreteEvent* db = (DiscreteEvent*) q->data_;
#if PRINT_EVENT
            if (print_event_) {
                db->pr("binq deliver", nt_t, this);
            }
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
        if (p[tid].tqe_->top()) {
            goto tryagain;
        }
        p[tid].tqe_->shift_bin(tm);
    }
#endif
    nt->_t = tsav;
}

void NetCvode::playrec_add(PlayRecord* pr) {  // called by PlayRecord constructor
                                              // printf("NetCvode::playrec_add %p\n", pr);
    playrec_change_cnt_ = 0;
    prl_->push_back(pr);
}

void NetCvode::playrec_remove(PlayRecord* pr) {  // called by PlayRecord destructor
                                                 // printf("NetCvode::playrec_remove %p\n", pr);
    playrec_change_cnt_ = 0;
    erase_first(*prl_, pr);
    erase_first(*fixed_play_, pr);
    erase_first(*fixed_record_, pr);
}

int NetCvode::playrec_item(PlayRecord* pr) {
    for (const auto&& [i, e]: enumerate(*prl_)) {
        if (e == pr) {
            return i;
        }
    }
    return -1;
}

PlayRecord* NetCvode::playrec_item(int i) {
    return prl_->at(i);
}

PlayRecord* NetCvode::playrec_uses(void* v) {
    for (auto& item: *prl_) {
        if (item->uses(v)) {
            return item;
        }
    }
    return nullptr;
}

PlayRecord::PlayRecord(neuron::container::data_handle<double> pd, Object* ppobj)
    : pd_{std::move(pd)} {
    // printf("PlayRecord::PlayRecord %p\n", this);
    cvode_ = nullptr;
    ith_ = 0;
    if (pd_) {
        neuron::container::notify_when_handle_dies(pd_, this);
    }
    ppobj_ = ppobj;
    if (ppobj_) {
        ObjObservable::Attach(ppobj_, this);
    }
    net_cvode_instance->playrec_add(this);
}

PlayRecord::~PlayRecord() {
    // printf("PlayRecord::~PlayRecord %p\n", this);
    nrn_notify_pointer_disconnect(this);
    if (ppobj_) {
        ObjObservable::Detach(ppobj_, this);
    }
    net_cvode_instance->playrec_remove(this);
}

void PlayRecord::disconnect(Observable*) {
    // printf("PlayRecord::disconnect %ls\n", (long)this);
    delete this;
}

void PlayRecord::record_add(Cvode* cv) {
    cvode_ = cv;
    if (cv) {
        cv->record_add(this);
    }
    net_cvode_instance->fixed_record_->push_back(this);
}

void PlayRecord::play_add(Cvode* cv) {
    cvode_ = cv;
    if (cv) {
        cv->play_add(this);
    }
    net_cvode_instance->fixed_play_->push_back(this);
}

void PlayRecord::pr() {
    Printf("PlayRecord\n");
}

TvecRecord::TvecRecord(Section* sec, IvocVect* t, Object* ppobj)
    : PlayRecord(sec->pnode[0]->v_handle(), ppobj) {
    // printf("TvecRecord\n");
    t_ = t;
    ObjObservable::Attach(t_->obj_, this);
}

TvecRecord::~TvecRecord() {
    // printf("~TvecRecord\n");
    ObjObservable::Detach(t_->obj_, this);
}

void TvecRecord::disconnect(Observable*) {
    //	printf("%s TvecRecord disconnect\n", hoc_object_name(t_->obj_));
    delete this;
}

void TvecRecord::install(Cvode* cv) {
    record_add(cv);
}

void TvecRecord::record_init() {
    t_->resize(0);
}

void TvecRecord::continuous(double tt) {
    t_->push_back(tt);
}

YvecRecord::YvecRecord(neuron::container::data_handle<double> dh, IvocVect* y, Object* ppobj)
    : PlayRecord(std::move(dh), ppobj) {
    // printf("YvecRecord\n");
    y_ = y;
    ObjObservable::Attach(y_->obj_, this);
}

YvecRecord::~YvecRecord() {
    // printf("~YvecRecord\n");
    ObjObservable::Detach(y_->obj_, this);
}

void YvecRecord::disconnect(Observable*) {
    //	printf("%s YvecRecord disconnect\n", hoc_object_name(y_->obj_));
    delete this;
}

void YvecRecord::install(Cvode* cv) {
    record_add(cv);
}

void YvecRecord::record_init() {
    if (!pd_) {
        hoc_execerr_ext("%s recording from invalid data reference.", hoc_object_name(y_->obj_));
    }
    y_->resize(0);
}

void YvecRecord::continuous(double tt) {
    y_->push_back(*pd_);
}

VecRecordDiscrete::VecRecordDiscrete(neuron::container::data_handle<double> dh,
                                     IvocVect* y,
                                     IvocVect* t,
                                     Object* ppobj)
    : PlayRecord(std::move(dh), ppobj) {
    // printf("VecRecordDiscrete\n");
    y_ = y;
    t_ = t;
    ObjObservable::Attach(y_->obj_, this);
    ObjObservable::Attach(t_->obj_, this);
    e_ = new PlayRecordEvent();
    e_->plr_ = this;
}

VecRecordDiscrete::~VecRecordDiscrete() {
    // printf("~VecRecordDiscrete\n");
    ObjObservable::Detach(y_->obj_, this);
    ObjObservable::Detach(t_->obj_, this);
    delete e_;
}

PlayRecordSave* VecRecordDiscrete::savestate_save() {
    return new VecRecordDiscreteSave(this);
}

VecRecordDiscreteSave::VecRecordDiscreteSave(PlayRecord* prl)
    : PlayRecordSave(prl) {
    cursize_ = ((VecRecordDiscrete*) pr_)->y_->size();
}
VecRecordDiscreteSave::~VecRecordDiscreteSave() {}
void VecRecordDiscreteSave::savestate_restore() {
    check();
    VecRecordDiscrete* vrd = (VecRecordDiscrete*) pr_;
    vrd->y_->resize(cursize_);
    assert(size_t(cursize_) <= vrd->t_->size());
}
void VecRecordDiscreteSave::savestate_write(FILE* f) {
    fprintf(f, "%d\n", cursize_);
}
void VecRecordDiscreteSave::savestate_read(FILE* f) {
    char buf[100];
    nrn_assert(fgets(buf, 100, f));
    nrn_assert(sscanf(buf, "%d\n", &cursize_) == 1);
}

void VecRecordDiscrete::disconnect(Observable*) {
    //	printf("%s VecRecordDiscrete disconnect\n", hoc_object_name(y_->obj_));
    delete this;
}

void VecRecordDiscrete::install(Cvode* cv) {
    record_add(cv);
}

void VecRecordDiscrete::record_init() {
    y_->resize(0);
    if (t_->size() > 0) {
        e_->send(t_->elem(0), net_cvode_instance, nrn_threads);
    }
}

void VecRecordDiscrete::frecord_init(TQItem* q) {
    record_init_items_->push_back(q);
}

void VecRecordDiscrete::deliver(double tt, NetCvode* nc) {
    y_->push_back(*pd_);
    assert(MyMath::eq(t_->elem(y_->size() - 1), tt, 1e-8));
    if (y_->size() < t_->size()) {
        e_->send(t_->elem(y_->size()), nc, nrn_threads);
    }
}

VecRecordDt::VecRecordDt(neuron::container::data_handle<double> pd,
                         IvocVect* y,
                         double dt,
                         Object* ppobj)
    : PlayRecord(std::move(pd), ppobj) {
    // printf("VecRecordDt\n");
    y_ = y;
    dt_ = dt;
    ObjObservable::Attach(y_->obj_, this);
    e_ = new PlayRecordEvent();
    e_->plr_ = this;
}

VecRecordDt::~VecRecordDt() {
    // printf("~VecRecordDt\n");
    ObjObservable::Detach(y_->obj_, this);
    delete e_;
}

PlayRecordSave* VecRecordDt::savestate_save() {
    return new VecRecordDtSave(this);
}

VecRecordDtSave::VecRecordDtSave(PlayRecord* prl)
    : PlayRecordSave(prl) {}
VecRecordDtSave::~VecRecordDtSave() {}
void VecRecordDtSave::savestate_restore() {
    check();
}

void VecRecordDt::disconnect(Observable*) {
    //	printf("%s VecRecordDt disconnect\n", hoc_object_name(y_->obj_));
    delete this;
}

void VecRecordDt::install(Cvode* cv) {
    record_add(cv);
}

void VecRecordDt::record_init() {
    y_->resize(0);
    e_->send(nrn_threads->_t, net_cvode_instance, nrn_threads);
}

void VecRecordDt::frecord_init(TQItem* q) {
    record_init_items_->push_back(q);
}

void VecRecordDt::deliver(double tt, NetCvode* nc) {
    auto* const ptr = static_cast<double*>(pd_);
    if (ptr == &t) {
        y_->push_back(tt);
    } else {
        y_->push_back(*ptr);
    }
    e_->send(tt + dt_, nc, nrn_threads);
}

void NetCvode::vecrecord_add() {
    auto const pd = hoc_hgetarg<double>(1);
    consist_sec_pd("Cvode.record", chk_access(), pd);
    IvocVect* y = vector_arg(2);
    IvocVect* t = vector_arg(3);
    delete playrec_uses(y);
    bool discrete = ((ifarg(4) && (int) chkarg(4, 0, 1) == 1) ? true : false);
    if (discrete) {
        new VecRecordDiscrete(pd, y, t);
    } else {
        delete playrec_uses(t);
        new TvecRecord(chk_access(), t);
        new YvecRecord(pd, y);
    }
}

void NetCvode::vec_remove() {
    while (auto* const pr = playrec_uses(vector_arg(1))) {
        delete pr;
    }
}

void NetCvode::playrec_setup() {
    long i, j;
    fixed_record_->clear();
    fixed_play_->clear();
    if (gcv_) {
        gcv_->delete_prl();
    } else {
        lvardtloop(i, j) {
            p[i].lcv_[j].delete_prl();
        }
    }
    std::vector<PlayRecord*> to_delete{};
    for (auto& pr: *prl_) {
        if (!pr->pd_) {
            // Presumably the recorded value was invalidated elsewhere, e.g. it
            // was a voltage of a deleted node, or a range variable of a deleted
            // mechanism instance
            to_delete.push_back(pr);
            continue;
        }
        bool b = false;
        if (single_) {
            pr->install(gcv_);
            b = true;
        } else {
            if (pr->ppobj_ && ob2pntproc(pr->ppobj_)->nvi_) {
                pr->install((Cvode*) ob2pntproc(pr->ppobj_)->nvi_);
                b = true;
            } else {
                lvardtloop(i, j) {
                    Cvode& cv = p[i].lcv_[j];
                    if (cv.is_owner(pr->pd_)) {
                        pr->install(&cv);
                        b = true;
                        break;
                    }
                }
            }
        }
        if (b == false) {
            hoc_execerror("We were unable to associate a PlayRecord item with a RANGE variable",
                          nullptr);
        }
        // and need to know the thread owners
        if (pr->ppobj_) {
            i = PP2NT(ob2pntproc(pr->ppobj_))->id;
        } else {
            i = owned_by_thread(pr->pd_);
        }
        if (i < 0) {
            hoc_execerror("We were unable to associate a PlayRecord item with a thread", nullptr);
        }
        pr->ith_ = i;
    }
    for (auto* pr: to_delete) {
        // Destructor should de-register things
        delete pr;
    }
    playrec_change_cnt_ = structure_change_cnt_;
}

// is a pointer to range variable in this cell
bool Cvode::is_owner(neuron::container::data_handle<double> const& handle) {
    int in, it;
    for (it = 0; it < nrn_nthread; ++it) {
        CvodeThreadData& z = CTD(it);
        for (in = 0; in < z.v_node_count_; ++in) {
            Node* nd = z.v_node_[in];
            if (handle == nd->v_handle()) {
                return true;
            }
            auto* pd = static_cast<double const*>(handle);
            Prop* p;
            for (p = nd->prop; p; p = p->next) {
                if (p->owns(handle)) {
                    return true;
                }
            }
            if (nd->extnode) {
                if (pd >= nd->extnode->v && pd < (nd->extnode->v + nlayer)) {
                    return true;
                }
            }
            // will need to check the linear mechanisms when there is a cvode
            // specific list of them and IDA is allowed for local step method.
        }
        if (nth_) {
            break;
        }  // lvardt
    }
    return false;
}

int NetCvode::owned_by_thread(neuron::container::data_handle<double> const& handle) {
    if (nrn_nthread == 1) {
        return 0;
    }
    int in, it;
    for (it = 0; it < nrn_nthread; ++it) {
        NrnThread& nt = nrn_threads[it];
        int i1 = 0;
        int i3 = nt.end;
        for (in = i1; in < i3; ++in) {
            Node* nd = nt._v_node[in];
            if (handle == nd->v_handle()) {
                return it;
            }
            for (Prop* p = nd->prop; p; p = p->next) {
                if (p->owns(handle)) {
                    return it;
                }
            }
            if (nd->extnode) {
                auto* pd = static_cast<double const*>(handle);
                if (pd >= nd->extnode->v && pd < (nd->extnode->v + nlayer)) {
                    return it;
                }
            }
            // will need to check the line mechanisms when there is a cvode
            // specific list of them and IDA is allowed for local step method.
        }
    }
    return -1;
}

void NetCvode::consist_sec_pd(const char* msg,
                              Section* sec,
                              neuron::container::data_handle<double> const& handle) {
    int in;
    Node* nd;
    for (in = -1; in < sec->nnode; ++in) {
        if (in == -1) {
            nd = sec->parentnode;  // in case &v(0)
            if (!nd) {
                continue;
            }
        } else {
            nd = sec->pnode[in];
        }
        if (nd->v_handle() == handle) {
            return;
        }
        Prop* p;
        auto* const pd = static_cast<double const*>(handle);
        for (p = nd->prop; p; p = p->next) {
            if (p->owns(handle)) {
                return;
            }
        }
        if (nd->extnode) {
            if (pd >= nd->extnode->v && pd < (nd->extnode->v + nlayer)) {
                return;
            }
        }
        // will need to check the linear mechanisms when there is a cvode
        // specific list of them and IDA is allowed for local step method.
    }
    hoc_execerror(msg,
                  " pointer not associated with currently accessed section\n\
Use section ... (&var(x)...) intead of ...(&section.var(x)...)\n");
}

double NetCvode::state_magnitudes() {
    if (hoc_is_double_arg(1)) {
        int on = (int) chkarg(1, 0, 2);
        int i, j;
        if (on == 2) {
            maxstate_analyse();
        } else {
            if (gcv_) {
                gcv_->activate_maxstate(on ? true : false);
            } else {
                lvardtloop(i, j) {
                    p[i].lcv_[j].activate_maxstate(on ? true : false);
                }
            }
        }
        return 0.;
    } else if (hoc_is_str_arg(1)) {
        Symbol* sym = name2sym(gargstr(1));
        double dummy;
        double* pamax = &dummy;
        if (ifarg(2)) {
            pamax = hoc_pgetarg(2);
        }
        return maxstate_analyse(sym, pamax);
    } else {
        int i, j, n;
        Vect* v = vector_arg(1);
        if (!cvode_active_) {
            v->resize(0);
            return 0.;
        }
        double* vp;
        n = 0;
        if (gcv_) {
            n += gcv_->neq_;
        } else {
            lvardtloop(i, j) {
                n += p[i].lcv_[j].neq_;
            }
        }
        v->resize(n);
        vp = vector_vec(v);
        int getacor = 0;
        if (ifarg(2)) {
            getacor = (int) chkarg(2, 0, 1);
        }
        j = 0;
        if (gcv_) {
            if (gcv_->maxstate_) {
                if (getacor) {
                    gcv_->maxacor(vp);
                } else {
                    gcv_->maxstate(vp);
                }
            }
        } else {
            lvardtloop(i, j) {
                Cvode& cv = p[i].lcv_[j];
                if (cv.maxstate_) {
                    if (getacor) {
                        cv.maxacor(vp + j);
                    } else {
                        cv.maxstate(vp + j);
                    }
                }
                j += cv.neq_;
            }
        }
        return 0.;
    }
}
void NetCvode::maxstate_analyze_1(int it, Cvode& cv, CvodeThreadData& z) {
    int j, n;
    Symbol* sym;
    double* ms;
    double* ma;
    n = z.nvsize_;
    ms = cv.n_vector_data(cv.maxstate_, it);
    ma = cv.n_vector_data(cv.maxacor_, it);
    auto const hdp = create_hdp(2);
    for (j = 0; j < n; ++j) {
        sym = hdp.retrieve_sym(static_cast<double*>(z.pv_[j]));
        auto msti = mst_->find((void*) sym);
        MaxStateItem* msi;
        if (msti == mst_->end()) {
            msi = new MaxStateItem();
            msi->sym_ = sym;
            msi->max_ = -1e9;
            msi->amax_ = -1e9;
            (*mst_)[(void*) sym] = msi;
        } else {
            msi = msti->second;
        }
        if (msi->max_ < ms[j]) {
            msi->max_ = ms[j];
        }
        if (msi->amax_ < ma[j]) {
            msi->amax_ = ma[j];
        }
    }
}

void NetCvode::maxstate_analyse() {
    int i, it, j;
    Symbol* sym;
    if (!mst_) {
        int n = 0;
        for (sym = hoc_built_in_symlist->first; sym; sym = sym->next) {
            ++n;
        }
        mst_ = new MaxStateTable(3 * n);
    }
    for (auto ti: *mst_) {
        MaxStateItem* msi = ti.second;
        msi->max_ = -1e9;
        msi->amax_ = -1e9;
    }
    if (empty_) {
        return;
    }
    statename(0, 2);
    if (gcv_) {
        for (it = 0; it < nrn_nthread; ++it) {
            maxstate_analyze_1(it, *gcv_, gcv_->ctd_[it]);
        }
    } else {
        lvardtloop(i, j) {
            Cvode& cv = p[i].lcv_[j];
            maxstate_analyze_1(i, cv, cv.ctd_[0]);
        }
    }
}

double NetCvode::maxstate_analyse(Symbol* sym, double* pamax) {
    if (mst_) {
        auto msti = mst_->find((void*) sym);
        if (msti != mst_->end()) {
            *pamax = msti->second->amax_;
            return msti->second->max_;
        }
    }
    *pamax = -1e9;
    return -1e9;
}

static double lvardt_tout_;

static void lvardt_integrate(neuron::model_sorted_token const& token, NrnThread& ntr) {
    auto* const nt = &ntr;
    size_t err = NVI_SUCCESS;
    int id = nt->id;
    NetCvode* nc = net_cvode_instance;
    NetCvodeThreadData& p = nc->p[id];
    TQueue* tq = p.tq_;
    TQueue* tqe = p.tqe_;
    double tout = lvardt_tout_;
    nt->_stop_stepping = 0;
    while (tq->least_t() < tout || tqe->least_t() <= tout) {
        err = nc->local_microstep(token, ntr);
        if (nt->_stop_stepping) {
            nt->_stop_stepping = 0;
            return;
        }
        if (err != NVI_SUCCESS || stoprun) {
            return;
        }
    }
    int n = p.nlcv_;
    Cvode* lcv = p.lcv_;
    if (n)
        for (int i = 0; i < n; ++i) {
            nc->retreat(tout, lcv + i);
            lcv[i].record_continuous();
        }
    else {
        nt->_t = tout;
    }
}

int NetCvode::solve_when_threads(double tout) {
    int err = NVI_SUCCESS;
    int tid;
    double til;
    nrn_use_busywait(1);  // just a possibility
    auto const cache_token = nrn_ensure_model_data_are_sorted();
    if (empty_) {
        if (tout >= 0.) {
            while (nt_t < tout && !stoprun) {
                deliver_events_when_threads(tout);
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
            }
            if (stoprun == 0) {
                nt_t = tout;
            }
        } else {
            if ((til = allthread_least_t(tid)) < 1e10) {
                deliver_events_when_threads(til);
            } else {
                nt_t += 1e6;
            }
            if (nrn_allthread_handle) {
                (*nrn_allthread_handle)();
            }
        }
    } else if (gcv_) {
        if (tout >= 0.) {
            while (gcv_->t_ < tout || allthread_least_t(tid) < tout) {
                err = global_microstep_when_threads();
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
                if (err != NVI_SUCCESS || stoprun) {
                    return err;
                }
            }
            retreat(tout, gcv_);
            gcv_->record_continuous();
        } else {
            // advance or initialized
            double tc = gcv_->t_;
            initialized_ = false;
            while (gcv_->t_ <= tc && !initialized_) {
                err = global_microstep_when_threads();
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
                if (err != NVI_SUCCESS || stoprun) {
                    return err;
                }
            }
        }
    } else {  // lvardt
        if (tout >= 0.) {
            // Each thread could integrate independently to tout
            // as long as no thread got more than
            // a minimum delay interval ahead of any other.
            // For now just integrate by min delay intervals.
            lvardt_tout_ = tout;
            while (nt_t < tout) {
                nrn_multithread_job(cache_token, lvardt_integrate);
                if (nrn_allthread_handle) {
                    (*nrn_allthread_handle)();
                }
                if (err != NVI_SUCCESS || stoprun) {
                    return err;
                }
                int tid;
                allthread_least_t(tid);
            }
        } else {
            // nthread>1 is more or less purposeless if we mean
            // that only the least cvode of all threads advances.
            // (which is required if minimum delay = 0)
            if (nrn_nthread > 1) {
                hoc_execerror("Lvardt method from fadvance()",
                              "presently limited to single thread.");
            }
        }
    }
    nrn_use_busywait(0);
    t = nt_t;
    dt = nt_dt;
    return err;
}

static void* deliver_for_thread(NrnThread* nt) {
    NetCvode* nc = net_cvode_instance;
    NetCvodeThreadData& d = nc->p[nt->id];
    TQItem* q = d.tqe_->least();
    DiscreteEvent* de = (DiscreteEvent*) q->data_;
    double tt = q->t_;
    d.tqe_->remove(q);
#if PRINT_EVENT
    if (nc->print_event_) {
        de->pr("deliver", tt, nc);
    }
#endif
    de->deliver(tt, nc, nt);
    return 0;
}

void NetCvode::deliver_events_when_threads(double til) {
    // printf("deliver_events til %20.15g\n", til);
    int tid;
    while (allthread_least_t(tid) <= til) {
        STATISTICS(deliver_cnt_);
        nrn_onethread_job(tid, deliver_for_thread);
        if (stoprun || nrn_allthread_handle) {
            return;
        }
    }
}

int NetCvode::global_microstep_when_threads() {
    int err = NVI_SUCCESS;
    int tid;
    double tt = allthread_least_t(tid);
    double tdiff = tt - gcv_->t_;
    if (tdiff <= 0) {
        // since events do not internally retreat with the
        // global step, we should already be at the event time
        // if this is too strict, we could use eps(list_->t_).
        // if (tdiff != 0.0) { printf("tdiff=%g\n", tdiff); }
        assert(tdiff == 0.0 || (gcv_->tstop_begin_ <= tt && tt <= gcv_->tstop_end_));
        deliver_events_when_threads(tt);
    } else {
        err = gcv_->handle_step(nrn_ensure_model_data_are_sorted(), this, tt);
    }
    if ((tt = allthread_least_t(tid)) < gcv_->t_) {
        gcv_->interpolate(tt);
    }
    return err;
}

void* nrn_interthread_enqueue(NrnThread* nt) {
    net_cvode_instance->p[nt->id].enqueue(net_cvode_instance, nt);
    return 0;
}

void NetCvode::set_enqueueing() {
    MUTLOCK
    enqueueing_ = 1;
    MUTUNLOCK
}

double NetCvode::allthread_least_t(int& tid) {
    // reduce (take minimum) of p[i].tqe_->least_t()
    double tt, min = 1e50;
    // setting enqueueing_ in interthread_send was a race. Logically it is not
    // needed. It is not clear if higher performance would result in having
    // a MUTEX for the NetCvode instance but that is the current implementation
    // instead of commenting out the enqueuing related lines.
    if (enqueueing_) {
        nrn_multithread_job(nrn_interthread_enqueue);
        enqueueing_ = 0;
    }
    for (int id = 0; id < pcnt_; ++id) {
        tt = p[id].tqe_->least_t();
        if (tt < min) {
            tid = id;
            min = tt;
        }
    }
    return min;
}
