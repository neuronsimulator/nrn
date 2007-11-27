#include <../../nrnconf.h>

//define to 0 if do not wish use_min_delay_ to ever be 1
#define USE_MIN_DELAY 1

#include <nrnmpi.h>
#include <errno.h>
#include <time.h>
#include <InterViews/resource.h>
#include <OS/list.h>
#include <OS/math.h>
#include <OS/table.h>
#include <InterViews/regexp.h>
#include "classreg.h"
#include "nrnoc2iv.h"
#include "parse.h"
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
#if HAVE_IV
#include "ivoc.h"
#endif
#include "oclist.h"
#define PROFILE 0
#include "profile.h"
#include "ivocvect.h"
#include "vrecitem.h"
#include "nrnste.h"
#include "netcon.h"
#include "netcvode.h"
#include "htlist.h"

typedef void (*ReceiveFunc)(Point_process*, double*, double);

#define NVI_SUCCESS 0

extern "C" {
#include "membfunc.h"
extern void single_event_run();
extern void setup_topology(), v_setup_vectors();
extern int structure_change_cnt, v_structure_change, tree_changed, nrn_matrix_cnt_;
extern int diam_changed;
extern int nrn_errno_check(int);
extern int stoprun;
extern void nrn_ba(int);
extern Cvode* cvode_instance;
extern int cvode_active_;
extern NetCvode* net_cvode_instance;
extern double t, dt;
extern void nrn_parent_info(Section*);
extern ReceiveFunc* pnt_receive;
extern ReceiveFunc* pnt_receive_init;
extern short* pnt_receive_size;
extern short* nrn_is_artificial_; // should be boolean but not using that type in c
extern short* nrn_artcell_qindex_;
void net_send(void**, double*, Point_process*, double, double);
void net_move(void**, Point_process*, double);
void artcell_net_send(void**, double*, Point_process*, double, double);
void artcell_net_move(void**, Point_process*, double);
int nrn_use_selfqueue_;
void nrn_pending_selfqueue(double tt);
void net_event(Point_process*, double);
void _nrn_watch_activate(Datum*, double (*)(Point_process*), int, Point_process*, int, double);
void _nrn_free_watch(Datum*, int, int);
extern int hoc_araypt(Symbol*, int);
extern int hoc_stacktype();
extern Point_process* ob2pntproc(Object*);
void nrn_use_daspk(int);
extern int nrn_use_daspk_;
int linmod_extra_eqn_count();
extern int nrn_modeltype();
extern Symlist* hoc_built_in_symlist;
extern Symlist* hoc_top_level_symlist;
extern TQueue* net_cvode_instance_event_queue();
extern hoc_Item* net_cvode_instance_psl();
extern PlayRecList* net_cvode_instance_prl();

extern int nrn_fornetcon_cnt_;
extern int* nrn_fornetcon_index_;
extern int* nrn_fornetcon_type_;
void _nrn_free_fornetcon(void**);
int _nrn_netcon_args(void*, double***);

// for use in mod files
double nrn_netcon_get_delay(NetCon* nc) { return nc->delay_; }
void nrn_netcon_set_delay(NetCon* nc, double d) { nc->delay_ = d; }
int nrn_netcon_weight(NetCon* nc, double** pw) {
	*pw = nc->weight_;
	return nc->cnt_;
}
double nrn_event_queue_stats(double* stats) {
#if COLLECT_TQueue_STATISTICS
	net_cvode_instance_event_queue()->spike_stat(stats);  
	return (stats[0]-stats[2]);
#else
	return -1.;
#endif
}
double nrn_netcon_get_thresh(NetCon* nc) {
	if (nc->src_) {
		return nc->src_->threshold_;
	}else{
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
	net_cvode_instance->event(td, nc);
}

Point_process* nrn_netcon_target(NetCon* nc) {
	nc->chktar();
	return nc->target_;
}

int nrn_netcon_info(NetCon* nc, double **pw, Point_process **target,
     double **th, double **del) {
	*target=(nc->target_)? nc->target_:(Point_process*)0;
	*th=(nc->src_)?&(nc->src_->threshold_):(double*)0;
	*del=&nc->delay_;
	*pw = nc->weight_;
	return nc->cnt_;
}

int nrn_presyn_count(PreSyn* ps) {
	return ps->dil_.count();
}
void* nrn_presyn_netcon(PreSyn* ps, int i) {
	return ps->dil_.item(i);
}

#if USENEOSIM
void neosim2nrn_advance(void*, void*, double);
void neosim2nrn_deliver(void*, void*);
void (*p_nrn2neosim_send)(void*, double);
static void* neosim_entity_;
#endif

void ncs2nrn_integrate(double tstop);
void nrn_fixed_step();

#if USENCS
// As a subroutine for NCS, NEURON simulates the cells of a subnet
// (usually a single cell) and NCS manages whatever intercellular
// NetCon connections were specified by the hoc cvode method
// Cvode.ncs_netcons(List nc_inlist, List nc_outlist) . The former list
// is normally in one to one correspondence with all the synapses. These
// NetCon objects have those synapses as targets and nil sources.
// The latter list is normally in one to one correspondence with
// the cells of the subnet. Those NetCon objects have sources of the form
// cell.axon.v(1) and nil targets.
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
#if PARANEURON
extern int nrnmpi_pgvts_least(double* tt, int* op, int* init);
#endif
}; //extern "C"

#if BGPDMA
extern void bgp_dma_send(PreSyn*, double t);
extern void bgpdma_cleanup_presyn(PreSyn*);
extern int use_bgpdma_;
#endif

boolean nrn_use_fifo_queue_;

#if BBTQ == 5
boolean nrn_use_bin_queue_;
#endif

#if NRNMPI
// for compressed info during spike exchange
extern boolean nrn_use_localgid_;
extern void nrn_outputevent(unsigned char, double);
#endif

static SelfQueue* selfqueue_;
static double immediate_deliver_;

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

TQueue* net_cvode_instance_event_queue() {
	return net_cvode_instance->event_queue();
}

hoc_Item* net_cvode_instance_psl() {
	return net_cvode_instance->psl_;
}

PlayRecList* net_cvode_instance_prl() {
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
	nc->event(tt, plr_->event());
}
void PlayRecordEvent::savestate_write(FILE* f) {
	fprintf(f, "%d\n", PlayRecordEventType);
	fprintf(f, "%d %d\n", plr_->type(), net_cvode_instance->playrec_item(plr_));
}

DiscreteEvent* PlayRecordEvent::savestate_read(FILE* f) {
	DiscreteEvent* de = nil;
	char buf[100];
	int type, plr_index;
	fgets(buf, 100, f);
	sscanf(buf, "%d %d\n", &type, &plr_index);
	PlayRecord* plr = net_cvode_instance->playrec_item(plr_index);
	assert(plr && plr->type() == type);
	return plr->event()->savestate_save();
}

PlayRecordSave* PlayRecord::savestate_save() {
	return new PlayRecordSave(this);
}

PlayRecordSave* PlayRecord::savestate_read(FILE* f) {
	PlayRecordSave* prs = nil;
	int type, index;
	char buf[100];
	fgets(buf, 100, f);
	assert(sscanf(buf, "%d %d\n", &type, &index) == 2);
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
PlayRecordSave::~PlayRecordSave() {
}
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
	Object* ob = ((ObjObservable*)o)->object();
//printf("%s disconnect from ", hoc_object_name(obj_));
	if (target_->ob == ob) {
//printf("target %s\n", hoc_object_name(target_->ob));
		target_ = nil;
		active_ = 0;
	}
}

#if 0	// way of printing dinfo
printf("NetCon from %s to ",
d->src_->osrc_ ? hoc_object_name(d->src_->osrc_) : secname(d->src_->ssrc_));
printf("%s ", hoc_object_name(d->target_->ob));
printf(" weight index=%d\n", l);
#endif

implementPtrList(NetConPList, NetCon)

class MaxStateItem {
public:
	Symbol* sym_;
	double max_;
	double amax_;
};

declareTable(MaxStateTable, void*, MaxStateItem*)
implementTable(MaxStateTable, void*, MaxStateItem*)
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
// allows marshalling of all items in the event queue that need to be
// removed to avoid duplicates due to frecord_init after finitialize
static TQList* record_init_items_;

SelfEventPool* SelfEvent::sepool_;
static DiscreteEvent* null_event_;
static DiscreteEvent* tstop_event_;

static PreSyn* unused_presyn; // holds the NetCons with no source

static double nc_preloc(void* v) { // user must pop section stack after call
	NetCon* d = (NetCon*)v;
	Section* s = nil;
	if (d->src_) {
		s = d->src_->ssrc_;
	}
	if (s) {
		nrn_pushsec(s);
		double* v = d->src_->thvar_;
		nrn_parent_info(s); // make sure parentnode exists
		// there is no efficient search for the location of
		// an arbitrary variable. Search only for v at 0, 1.
		// Otherwise return .5 .
		if (v == &NODEV(s->parentnode)) { return 0.; }
		if (v == &NODEV(s->pnode[s->nnode-1])) { return 1.; }
		return .5;	// perhaps should search for v
	}else{
		return -1.;
	}
}

static double nc_postloc(void* v) { // user must pop section stack after call
	NetCon* d = (NetCon*)v;
	if (d->target_ && d->target_->sec) {
		nrn_pushsec(d->target_->sec);
		return nrn_arc_position(d->target_->sec, d->target_->node);
	}else{
		return -1.;
	}
}

static Object** nc_syn(void* v) {
	NetCon* d = (NetCon*)v;
	Object* ob = nil;
	if (d->target_) {
		ob = d->target_->ob;
	}
	return hoc_temp_objptr(ob);
}

static Object** nc_pre(void* v) {
	NetCon* d = (NetCon*)v;
	Object* ob = nil;
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
		o = (OcList*)((*po)->u.this_pointer);
	}else{
		o = new OcList();
		o->ref();
		Symbol* sl = hoc_lookup("List");
		po = hoc_temp_objvar(sl, o);
	}
	return po;
}

static Object** nc_prelist(void* v) {
	NetCon* d = (NetCon*)v;
	OcList* o;
	Object** po = newoclist(1, o);
	if (d->src_) {
		NetConPList& dil = d->src_->dil_;
		for (int i=0; i < dil.count(); ++i) {
			if (dil.item(i)->obj_) {
				o->append(dil.item(i)->obj_);
			}
		}
	}
	return po;
}

static Object** nc_synlist(void* v) {
	NetCon* d = (NetCon*)v;
	OcList* o;
	Object** po = newoclist(1, o);
	hoc_Item* q;
	if (net_cvode_instance->psl_) ITERATE(q, net_cvode_instance->psl_) {
		PreSyn* ps = (PreSyn*)VOIDITM(q);
		NetConPList& dil = ps->dil_;
		for (int i=0; i < dil.count(); ++i) {
			NetCon* d1 = dil.item(i);
			if (d1->obj_ && d1->target_ == d->target_) {
				o->append(d1->obj_);
			}
		}
	}
	return po;
}

static Object** nc_postcelllist(void* v) {
	NetCon* d = (NetCon*)v;
	OcList* o;
	Object** po = newoclist(1, o);
	hoc_Item* q;
	Object* cell = nil;
	if (d->target_ && d->target_->sec) {
		cell = d->target_->sec->prop->dparam[6].obj;
	}
	if (cell && net_cvode_instance->psl_) ITERATE(q, net_cvode_instance->psl_) {
		PreSyn* ps = (PreSyn*)VOIDITM(q);
		NetConPList& dil = ps->dil_;
		for (int i=0; i < dil.count(); ++i) {
			NetCon* d1 = dil.item(i);
			if (d1->obj_ && d1->target_
				&& d1->target_->sec->prop->dparam[6].obj == cell) {
				o->append(d1->obj_);
			}
		}
	}
	return po;
}

static Object** nc_precelllist(void* v) {
	NetCon* d = (NetCon*)v;
	OcList* o;
	Object** po = newoclist(1, o);
	hoc_Item* q;
	Object* cell = nil;
	if (d->src_ && d->src_->ssrc_) { cell = d->src_->ssrc_->prop->dparam[6].obj;}
	if (cell && net_cvode_instance->psl_) ITERATE(q, net_cvode_instance->psl_) {
		PreSyn* ps = (PreSyn*)VOIDITM(q);
		NetConPList& dil = ps->dil_;
		for (int i=0; i < dil.count(); ++i) {
			NetCon* d1 = dil.item(i);
			if (d1->obj_ && d1->src_ && ps->ssrc_
				&& ps->ssrc_->prop->dparam[6].obj == cell) {
				o->append(d1->obj_);
			}
		}
	}
	return po;
}

static Object** nc_precell(void* v) {
	NetCon* d = (NetCon*)v;
	if (d->src_ && d->src_->ssrc_) {
		return hoc_temp_objptr(d->src_->ssrc_->prop->dparam[6].obj);
	}else{
		return hoc_temp_objptr(0);
	}
}

static Object** nc_postcell(void* v) {
	NetCon* d = (NetCon*)v;
	Object* ob = nil;
	if (d->target_ && d->target_->sec) {
		ob = d->target_->sec->prop->dparam[6].obj;
	}
	return hoc_temp_objptr(ob);
}

static double nc_setpost(void* v) {
	NetCon* d = (NetCon*)v;
	Object* otar = nil;
	if (ifarg(1)) {
		otar = *hoc_objgetarg(1);
	}
	if (otar && !is_point_process(otar)) {
		hoc_execerror("argument must be a point process or NULLobject",0);
	}
	Point_process* tar = nil;
	if (otar) {
		tar = ob2pntproc(otar);
	}
	if (d->target_ && d->target_ != tar) {
#if DISCRETE_EVENT_OBSERVER
		ObjObservable::Detach(d->target_->ob, d);
#endif
		d->target_ = nil;
	}
	int cnt = 1;
	if (tar) {
		cnt = pnt_receive_size[tar->prop->type];
		d->target_ = tar;
#if DISCRETE_EVENT_OBSERVER
		ObjObservable::Attach(otar, d);
#endif
	}else{
		d->active_ = false;
	}
	if (d->cnt_ != cnt) {
		d->cnt_ = cnt;
		delete [] d->weight_;
		d->weight_ = new double[d->cnt_];
	}
	return 0.;
}

static double nc_valid(void* v) {
	NetCon* d = (NetCon*)v;
	if (d->src_ && d->target_) {
		return 1.;
	}
	return 0.;
}

static double nc_active(void* v) {
	NetCon* d = (NetCon*)v;
	boolean a = d->active_;
	if (d->target_ && ifarg(1)) {
		d->active_ = boolean(chkarg(1, 0, 1));
	}
	return double(a);
}

static double nc_event(void* v) {
	NetCon* d = (NetCon*)v;
	double td = chkarg(1,t,1e20);
	d->chktar();
	if (ifarg(2)) {
		double flag = *getarg(2);
		Point_process* pnt = d->target_;
		int type = pnt->prop->type;
		if (!nrn_is_artificial_[type]) {
			hoc_execerror("Can only send fake self-events to ARTIFICIAL_CELLs",0);
		}
		void** pq = (void**)(&pnt->prop->dparam[nrn_artcell_qindex_[type]]._pvoid);
		net_send(pq, d->weight_, pnt, td - t, flag);
	}else{
		net_cvode_instance->event(td, d);
	}
	return (double)d->active_;
}
static double nc_record(void* v) {
	NetCon* d = (NetCon*)v;
	d->chksrc();
	if (ifarg(1)) {
		if (ifarg(2)) {
			int recid = d->obj_->index;
			if (ifarg(3)) {
				recid = (int)(*getarg(3));
			}
			d->src_->record(vector_arg(1), vector_arg(2), recid);
		}else if (hoc_is_str_arg(1)) {
			d->src_->record_stmt(gargstr(1));
		}else{
			d->src_->record(vector_arg(1));
		}
	}else{
		d->src_->record((IvocVect*)nil);
	}
	return 0;
}

static double nc_srcgid(void* v) {
	NetCon* d = (NetCon*)v;
	if (d->src_) {
		return (double)d->src_->gid_;
	}
	return -1.;
}

static Object** nc_get_recordvec(void* v) {
	NetCon* d = (NetCon*)v;
	Object* ob = nil;
	if (d->src_ && d->src_->tvec_) {
		ob = d->src_->tvec_->obj_;
	}
	return hoc_temp_objptr(ob);
}

static double nc_wcnt(void* v) {
	NetCon* d = (NetCon*)v;
	return d->cnt_;
}

static Member_func members[] = {
	"active", nc_active,
	"valid", nc_valid,
	"preloc", nc_preloc,
	"postloc", nc_postloc,
	"setpost", nc_setpost,
	"event", nc_event,
	"record", nc_record,
	"srcgid", nc_srcgid,
	"wcnt", nc_wcnt,
	"delay", 0,	// these four changed below
	"weight", 0,
	"threshold", 0,
	"x", 0,
	0, 0
};

static Member_ret_obj_func omembers[] = {
	"syn", nc_syn,
	"pre", nc_pre,
	"precell", nc_precell,
	"postcell", nc_postcell,
	"prelist", nc_prelist,
	"synlist", nc_synlist,
	"precelllist", nc_precelllist,
	"postcelllist", nc_postcelllist,
	"get_recordvec", nc_get_recordvec,
	0, 0
};

static void steer_val(void* v) {
	NetCon* d = (NetCon*)v;
	Symbol* s = hoc_spop();
	if (strcmp(s->name, "delay") == 0) {
		d->chksrc();
		hoc_pushpx(&d->delay_);
		d->src_->use_min_delay_ = 0;
	}else if (strcmp(s->name, "weight") == 0) {
		int index = 0;
		if (hoc_stacktype() == NUMBER) {
			s->arayinfo->sub[0] = d->cnt_;
			index = hoc_araypt(s, SYMBOL);
		}
		hoc_pushpx(d->weight_ + index);
	}else if (strcmp(s->name, "x") == 0) {
		static double dummy = 0.;
		d->chksrc();
		if (d->src_->thvar_) {
			hoc_pushpx(d->src_->thvar_);
		}else{
			dummy = 0.;
			hoc_pushpx(&dummy);
		}
	}else if (strcmp(s->name, "threshold") == 0) {
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
	Object* osrc = nil, *otar;
	Section* srcsec = nil;
	double* psrc = nil;
	if (hoc_is_object_arg(1)) {
		osrc = *hoc_objgetarg(1); 
		if (osrc && !is_point_process(osrc)) {
			hoc_execerror("if arg 1 is an object it must be a point process or NULLObject", 0);
		}
	}else{
		psrc = hoc_pgetarg(1);
		srcsec = chk_access();
	}
	otar = *hoc_objgetarg(2);
	if (otar && !is_point_process(otar)) {
		hoc_execerror("arg 2 must be a point process or NULLobject", 0);
	}
	double thresh = -1.e9; // sentinal value. default is 10 if new PreSyn
	double delay = 1.;
	double weight = 0.;
	
	if (ifarg(3)) {
		thresh = *getarg(3);
		delay = chkarg(4, 0, 1e15);
		weight = *getarg(5);
	}
	d = net_cvode_instance->install_deliver(psrc, srcsec, osrc, otar,
		thresh, delay, weight);
	d->obj_ = o;
	return (void*)d;
}

static void destruct(void* v) {
	NetCon* d = (NetCon*)v;
	delete d;
}

void NetCon_reg() {
	class2oc("NetCon", cons, destruct, members, nil, omembers);
	Symbol* nc = hoc_lookup("NetCon");
	nc->u.ctemplate->steer = steer_val;
	Symbol* s;
	s = hoc_table_lookup("delay", nc->u.ctemplate->symtable);
	s->type = VAR;
	s->arayinfo = nil;
	s = hoc_table_lookup("x", nc->u.ctemplate->symtable);
	s->type = VAR;
	s->arayinfo = nil;
	s = hoc_table_lookup("threshold", nc->u.ctemplate->symtable);
	s->type = VAR;
	s->arayinfo = nil;
	s = hoc_table_lookup("weight", nc->u.ctemplate->symtable);
	s->type = VAR;
	s->arayinfo = new Arrayinfo;
	s->arayinfo->refcount = 1;
	s->arayinfo->a_varn = nil;
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

	Object *opre = nil, *opost = nil, *otar = nil;
	Regexp* spre = nil, *spost = nil, *star = nil;
	char* s;
	int n;

	if (hoc_is_object_arg(1)) {
		opre = *hoc_objgetarg(1);
	}else{
		s = gargstr(1);
		if (s[0] == '\0') {
			spre = new Regexp(".*");
		}else{
			spre = new Regexp(escape_bracket(s));
		}
		if(!spre->pattern()) {
			hoc_execerror(gargstr(1), "not a valid regular expression");
		}
	}
	if (hoc_is_object_arg(2)) {
		opost = *hoc_objgetarg(2);
	}else{
		s = gargstr(2);
		if (s[0] == '\0') {
			spost = new Regexp(".*");
		}else{
			spost = new Regexp(escape_bracket(s));
		}
		if(!spost->pattern()) {
			hoc_execerror(gargstr(2), "not a valid regular expression");
		}
	}
	if (hoc_is_object_arg(3)) {
		otar = *hoc_objgetarg(3);
	}else{
		s = gargstr(3);
		if (s[0] == '\0') {
			star = new Regexp(".*");
		}else{
			star = new Regexp(escape_bracket(s));
		}
		if(!star->pattern()) {
			hoc_execerror(gargstr(3), "not a valid regular expression");
		}
	}
	
	boolean b;
	hoc_Item* q;
	if (psl_) ITERATE(q, psl_) {
		PreSyn* ps = (PreSyn*)VOIDITM(q);
		b = false;
		if (ps->ssrc_) {
			Object* precell = ps->ssrc_->prop->dparam[6].obj;
			if (opre) {
				if (precell == opre) {
					b = true;
				}else{
					b = false;
				}
			}else{
				s = hoc_object_name(precell);
				n = strlen(s);
				if (spre->Match(s, n, 0) > 0) {
					b = true;
				}else{
					b = false;
				}
			}
		}else if (ps->osrc_) {
			Object* presyn = ps->osrc_;
			if (opre) {
				if (presyn == opre) {
					b = true;
				}else{
					b = false;
				}
			}else{
				s = hoc_object_name(presyn);
				n = strlen(s);
				if (spre->Match(s, n, 0) > 0) {
					b = true;
				}else{
					b = false;
				}
			}
		}
		if (b == true) {
			NetConPList& dil = ps->dil_;
			for (int i=0; i < dil.count(); ++i) {
				NetCon* d = dil.item(i);
				Object* postcell = nil;
				Object* target = nil;
				if (d->target_) {
					Point_process* p = d->target_;
					target = p->ob;
					if (p->sec) {
						postcell =  p->sec->prop->dparam[6].obj;
					}
				}
				if (opost) {
					if (postcell == opost) {
						b = true;
					}else{
						b = false;
					}
				}else{
					s = hoc_object_name(postcell);
					n = strlen(s);
					if (spost->Match(s, n, 0) > 0) {
						b = true;
					}else{
						b = false;
					}
				}
				if (b == true) {
					if (otar) {
						if (target == otar) {
							b = true;
						}else{
							b = false;
						}
					}else{
						s = hoc_object_name(target);
						n = strlen(s);
						if (star->Match(s, n, 0) > 0) {
							b = true;
						}else{
							b = false;
						}
					}
					if (b == true) {
						o->append(d->obj_);
					}
				}
			}
		}
	}
	if (spre) delete spre;
	if (spost) delete spost;
	if (star) delete star;				
	return po;
}

NetCvode::NetCvode(boolean single) {
	immediate_deliver_ = -1e100; // when using the selfqueue_
	maxorder_ = 5;
	maxstep_ = 1e9;
	minstep_ = 0.;
	rtol_ = 0.;
	atol_ = 1e-3;
	jacobian_ = 0;
	stiff_ = 2;
	mst_ = nil;
	condition_order_ = 1;
	null_event_ = new DiscreteEvent();
	tstop_event_ = new TstopEvent();
	eps_ = 100.*UNIT_ROUNDOFF;
	hdp_ = nil;
	print_event_ = 0;
	unreffed_event_cnt_ = 0;
	nrn_use_fifo_queue_ = false;
	single_ = single;
	nrn_use_daspk_ = false;
	nlist_ = 0;
	list_ = nil;
	wl_list_ = new HTListList();
	tq_ = nil;
	tqe_ = new TQueue();
	pst_ = nil;
	pst_cnt_ = 0;
	psl_ = nil;
	// for parallel network simulations hardly any presyns have
	// a threshold and it can be very inefficient to check the entire
	// presyn list for thresholds during the fixed step method.
	// So keep a threshold list.
	psl_th_ = nil;
	unused_presyn = nil;
	structure_change_cnt_ = -1;
	fornetcon_change_cnt_ = -2;
	matrix_change_cnt_ = -1;
	playrec_change_cnt_ = 0;
	alloc_list();
	prl_ = new PlayRecList(10);
	fixed_play_ = new PlayRecList(10);
	fixed_record_ = new PlayRecList(10);
	vec_event_store_ = nil;
	if (!record_init_items_) {
		record_init_items_ = new TQList();
	}
//	re_init(t);
}

NetCvode::~NetCvode() {
	if (net_cvode_instance == (NetCvode*)this) {
		net_cvode_instance = nil;
	}	
	if (hdp_) {
		hdp_ = nil;
	}
	delete_list();
	delete tqe_;
	if (mst_) {
		// and should also iterate and delete the MaxStateItem
		delete mst_;
	}
	if (psl_) {
		hoc_Item* q;
		ITERATE(q, psl_) {
			PreSyn* ps = (PreSyn*)VOIDITM(q);
			for (int i = ps->dil_.count() - 1; i >= 0; --i) {
				NetCon* d = ps->dil_.item(i);
				d->src_ = nil;
				delete d;
			}
			delete ps;
		}
		hoc_l_freelist(&psl_);
	}
	if (psl_th_) {hoc_l_freelist(&psl_th_);}
	if (pst_) {
		delete pst_;
	}
	delete fixed_play_;
	delete fixed_record_;
	while(prl_->count()) {
		delete prl_->item(prl_->count()-1);
	}
	delete prl_;
	unused_presyn = nil;
	delete wl_list_;		
}

boolean NetCvode::localstep(){
	return !single_;
}

void NetCvode::localstep(boolean b) {
	// due to possibility of gap junctions and until the complete matrix
	// is analysed for block structure localstep and daspk are incompatible
	b = (nrn_modeltype() == 1 ? b : false); // localstep doesn't work yet with DAE's

	if (!b != single_) {
		delete_list();
		single_ = !b;
		structure_change_cnt_ = 0;
		use_sparse13 = 0;
		nrn_use_daspk_ = false;
		re_init(t);
	}
}

boolean NetCvode::use_daspk(){
	return (list_ != 0) ? list_[0].use_daspk_ : false;
}

void NetCvode::use_daspk(boolean b) {
	b = (nrn_modeltype() == 2 ? true : b); // not optional if algebraic
	if (list_ && b != list_[0].use_daspk_) {
		delete_list();
		single_ = (b ? true : single_);
		structure_change_cnt_ = 0;
		nrn_use_daspk_ = b;
//printf("NetCvode::use_daspk nrn_use_daspk=%d\n", nrn_use_daspk_);
		if (use_sparse13 != nrn_use_daspk_) {
			use_sparse13 = nrn_use_daspk_;
			diam_changed = 1;
		}
		re_init(t);
	}
}

BAMechList::BAMechList(BAMechList** first) { // preserve the list order
	next = nil;
	indices = nil;
	cnt = 0;
	BAMechList* last;
	if (*first) {
		for (last = *first; last->next; last = last->next){}
		last->next = this;
	}else{
		*first = this;
	}
}

void BAMechList::destruct(BAMechList** first) {
	BAMechList* b, *bn;
	for (b = *first; b; b = bn) {
		bn = b->next;
		if (b->indices) {
			delete [] b->indices;
			delete b;
		}
	}
	*first = nil;
}

void NetCvode::delete_list() {
	int i;
	wl_list_->remove_all();
	if (list_) {
		del_cv_memb_list();
		for (i=0; i < nlist_; ++i) {
			if (list_[i].psl_th_) {
				delete list_[i].psl_th_;
			}
			if (list_[i].ste_list_ && list_[i].ste_list_ != StateTransitionEvent::stelist_) {
				delete list_[i].ste_list_;
			}
		}
		delete [] list_;
		list_ = nil;
		nlist_ = 0;
	}
	if (tq_) {
		delete tq_;
		tq_ = nil;
	}
}

void NetCvode::del_cv_memb_list() {
	int i;
	for (i=0; i < nlist_; ++i) {
		if (list_[i].psl_th_) {
			list_[i].psl_th_->remove_all();
		}
		if (list_[i].ste_list_ && list_[i].ste_list_ != StateTransitionEvent::stelist_) {
			list_[i].ste_list_->remove_all();
		}
		if (!single_) {
			if (list_[i].v_node_) {
				delete [] list_[i].v_node_;
				delete [] list_[i].v_parent_;
				list_[i].v_node_ = nil;
				list_[i].v_parent_ = nil;
			}
			list_[i].delete_memb_list(list_[i].cv_memb_list_);
		}else{
			CvMembList* cml, *cmlnext;
			for (cml = list_[i].cv_memb_list_; cml; cml = cmlnext) {
				cmlnext = cml->next;
				delete cml;
			}
		}
		list_[i].cv_memb_list_ = nil;
		BAMechList::destruct(&list_[i].before_breakpoint_);
		BAMechList::destruct(&list_[i].after_solve_);
		BAMechList::destruct(&list_[i].before_step_);
	}
}

CvMembList::CvMembList() {
	ml = new Memb_list;
}
CvMembList::~CvMembList() {
	delete ml;
}

void Cvode::delete_memb_list(CvMembList* cmlist) {
	CvMembList* cml, *cmlnext;
	for (cml = cmlist; cml; cml = cmlnext) {
		Memb_list* ml = cml->ml;
		cmlnext = cml->next;
		delete [] ml->nodelist;
#if CACHEVEC
		if (ml->nodeindices) {
			delete [] ml->nodeindices;
		}
#endif
		if (memb_func[cml->index].hoc_mech) {
			delete [] ml->prop;
		}else{
			delete [] ml->data;
			delete [] ml->pdata;
		}
		delete cml;
	}
}

void NetCvode::distribute_dinfo(int* cellnum) {
	int i, j;
//printf("distribute_dinfo %d\n", pst_cnt_);
    if (psl_) {
	hoc_Item* q;
	ITERATE(q, psl_) { 
		PreSyn* ps = (PreSyn*)VOIDITM(q);
//printf("\tPreSyn %s\n", ps->osrc_ ? hoc_object_name(ps->osrc_):secname(ps->ssrc_));
		Cvode* cvsrc;
	    if (ps->thvar_) { // artcells and presyns for gid's not on this cpu have no threshold check
		// cvode instances poll which presyns
		if (single_) {
			cvsrc = list_;
			if (!cvsrc->psl_th_) {
				cvsrc->psl_th_ = new PreSynList(pst_cnt_);
			}
			j=0;
		}else{
			if (ps->osrc_) {
				j = node(ps->osrc_)->v_node_index;
			}else if (ps->ssrc_) {
				j = ps->ssrc_->pnode[0]->v_node_index;
			}
			cvsrc = list_ + cellnum[j];
			if (!cvsrc->psl_th_) {
				cvsrc->psl_th_ = new PreSynList(1);
			}
		}
		cvsrc->psl_th_->append(ps);
	    }
	}
    }
}

void NetCvode::alloc_list() {
	int i;
	if (single_) {
		nlist_ = 1;
		list_ = new Cvode[1];
	}else{
		nlist_ = rootnodecount;
		list_ = new Cvode[nlist_];
		tq_ = new TQueue();
		for (i=0; i < nlist_; ++i) {
			TQItem* ti = tq_->insert(0., list_+i);
			list_[i].tqitem_ = ti;
		}
	}
	for (i=0; i < nlist_; ++i) {
		list_[i].ncv_ = this;
		list_[i].v_node_ = nil;
		list_[i].v_parent_ = nil;
		list_[i].cv_memb_list_ = nil;
	}
#if USENEOSIM
	if (p_nrn2neosim_send) for (i=0; i < nlist_; ++i) {
		list_[i].neosim_self_events_ = new TQueue();
	}
#endif
}

boolean NetCvode::init_global() {
	int i;
	CvMembList* cml;
	if (tree_changed) { setup_topology(); }
	if (v_structure_change) { v_setup_vectors(); }
	if (structure_change_cnt_ == structure_change_cnt) {
		return false;
	}
	if (diam_changed) { // need to guarantee that the matrix is allocated
		recalc_diam(); // for the present method
	}
//printf("NetCvode::init_global nrn_use_daspk_=%d single_=%d\n", nrn_use_daspk_, single_);
	structure_change_cnt_ = structure_change_cnt;
	matrix_change_cnt_ = -1;
	playrec_change_cnt_ = 0;
	if (hdp_) {
		delete hdp_;
		hdp_ = nil;
	}
	if (single_) {
		if (nlist_ != 1) {
			delete_list();
			alloc_list();
		}
		del_cv_memb_list();
		list_[0].rootnodecount_ = rootnodecount;
		list_[0].v_node_count_ = v_node_count;
		list_[0].v_node_ = v_node;
		list_[0].v_parent_ = v_parent;

		Cvode& cv = list_[0];
		for (i = n_memb_func - 1; i >= 0; --i) {
		  Memb_func* mf = memb_func + i;
		  Memb_list* ml = memb_list + i;
		  if (ml->nodecount
		    && (i == CAP
			|| mf->current || mf->ode_count || mf->ode_matsol
		        || mf->ode_spec || mf->state )
		  ) {
			// above maintains same order (not reversed) for
			// singly linked list built below
			cml = new CvMembList;
			cml->next = cv.cv_memb_list_;
			cv.cv_memb_list_ = cml;
			cml->index = i;
			cml->ml->nodecount = ml->nodecount;
			// assumes cell info grouped contiguously
			cml->ml->nodelist = ml->nodelist;
#if CACHEVEC
			cml->ml->nodeindices = ml->nodeindices;
#endif
			if (mf->hoc_mech) {
				cml->ml->prop = ml->prop;
			}else{
				cml->ml->data = ml->data;
				cml->ml->pdata = ml->pdata;
			}
		  }
		}
		fill_global_ba(BEFORE_BREAKPOINT, &cv.before_breakpoint_);
		fill_global_ba(AFTER_SOLVE, &cv.after_solve_);
		fill_global_ba(BEFORE_STEP, &cv.before_step_);
		distribute_dinfo(nil);
		// Every point process, but not artificial cells, cause at least a retreat.
		// All point processes, but not artificial cells,
		// have the global cvode as its nvi field
		for (i = 0; i < n_memb_func; ++i) {
			Memb_func* mf = memb_func + i;
			if (mf->is_point && !nrn_is_artificial_[i]) {
				Memb_list* ml = memb_list + i;
				int j;
				for (j = 0; j < ml->nodecount; ++j) {
					Point_process* pp;
					if (mf->hoc_mech) {
						pp = (Point_process*)ml->prop[j]->dparam[1]._pvoid;
					}else{
						pp = (Point_process*)ml->pdata[j][1]._pvoid;
					}
					pp->nvi_ = list_;
				}
			}
		}
		// share the main stelist
		list_->ste_list_ = StateTransitionEvent::stelist_;
	}else{
		if (nlist_ != rootnodecount) {
			delete_list();
			alloc_list();
		}
		del_cv_memb_list();
		// each node has a cell number
		int* cellnum = new int[v_node_count];
		for (i=0; i < rootnodecount; ++i) {
			cellnum[i] = i;
		}
		for (i=rootnodecount; i < v_node_count; ++i) {
			cellnum[i] = cellnum[v_parent[i]->v_node_index];
		}
		
		for (i=0; i < rootnodecount; ++i) {
			list_[i].v_node_count_ = 0;
		}
		for (i=0; i < v_node_count; ++i) {
			++list_[cellnum[i]].v_node_count_;
		}
		for (i=0; i < rootnodecount; ++i) {
			list_[cellnum[i]].v_node_ = new Node*[list_[cellnum[i]].v_node_count_];
			list_[cellnum[i]].v_parent_ = new Node*[list_[cellnum[i]].v_node_count_];
		}
		for (i=0; i < rootnodecount; ++i) {
			list_[i].v_node_count_ = 0;
			list_[i].rootnodecount_ = 1;
		}
		for (i=0; i < v_node_count; ++i) {
			list_[cellnum[i]].v_node_[list_[cellnum[i]].v_node_count_] = v_node[i];
			list_[cellnum[i]].v_parent_[list_[cellnum[i]].v_node_count_++] = v_parent[i];
		}
		// divide the memb_list info into per cell info
		// count
		for (i=n_memb_func-1; i >= 0; --i) {
		  Memb_func* mf = memb_func + i;
		  Memb_list* ml = memb_list + i;
		  if (ml->nodecount
		    && (mf->current || mf->ode_count || mf->ode_matsol
		        || mf->ode_spec || mf->state || i == CAP)
		  ) {
			// above maintains same order (not reversed) for
			// singly linked list built below
			int j;
			for (j = 0; j < ml->nodecount; ++j) {
				int inode = ml->nodelist[j]->v_node_index;
				Cvode& cv = list_[cellnum[inode]];
				if (cv.cv_memb_list_ && cv.cv_memb_list_->index == i) {
					++cv.cv_memb_list_->ml->nodecount;
				}else{
					cml = new CvMembList;
					cml->next = cv.cv_memb_list_;
					cv.cv_memb_list_ = cml;
					cml->index = i;
					cml->ml->nodecount = 1;
				}
			}
		  }
		}
		// allocate and re-initialize count
		CvMembList** cvml = new CvMembList*[nlist_];
		for (i=0; i < nlist_; ++i) {
			cvml[i] = list_[i].cv_memb_list_;
			for (cml = cvml[i]; cml; cml = cml->next) {
				Memb_list* ml = cml->ml;
				ml->nodelist = new Node*[ml->nodecount];
#if CACHEVEC
				ml->nodeindices = new int[ml->nodecount];
#endif
				if (memb_func[cml->index].hoc_mech) {
					ml->prop = new Prop*[ml->nodecount];
				}else{
					ml->data = new double*[ml->nodecount];
					ml->pdata = new Datum*[ml->nodecount];
				}
				ml->nodecount = 0;
			}
		}
		// fill pointers (and nodecount)
		// now list order is from 0 to n_memb_func
		for (i=0; i < n_memb_func; ++i) {
		  Memb_func* mf = memb_func + i;
		  Memb_list* ml = memb_list + i;
		  if (ml->nodecount
		    && (mf->current || mf->ode_count || mf->ode_matsol
		        || mf->ode_spec || mf->state || i == CAP)
		  ) {
			int j;
			for (j = 0; j < ml->nodecount; ++j) {
				int icell = cellnum[ml->nodelist[j]->v_node_index];
				Cvode& cv = list_[icell];
				if (cvml[icell]->index != i) {
					cvml[icell] = cvml[icell]->next;
					assert (cvml[icell] && cvml[icell]->index);
				}
				cml = cvml[icell];
				cml->ml->nodelist[cml->ml->nodecount] = ml->nodelist[j];
#if CACHEVEC
				cml->ml->nodeindices[cml->ml->nodecount] = ml->nodeindices[j];
#endif
				if (mf->hoc_mech) {
					cml->ml->prop[cml->ml->nodecount] = ml->prop[j];
				}else{
					cml->ml->data[cml->ml->nodecount] = ml->data[j];
					cml->ml->pdata[cml->ml->nodecount] = ml->pdata[j];
				}
				++cml->ml->nodecount;
			}
		  }
		}
		// do the above for the BEFORE/AFTER functions
		fill_local_ba(cellnum);

		distribute_dinfo(cellnum);
		// If a point process is not an artificial cell, fill its nvi_ field.
		// artifical cells have no integrator
		for (i = 0; i < n_memb_func; ++i) {
			Memb_func* mf = memb_func + i;
			if (mf->is_point) {
				Memb_list* ml = memb_list + i;
				int j;
				for (j = 0; j < ml->nodecount; ++j) {
					Point_process* pp;
					if (mf->hoc_mech) {
						pp = (Point_process*)ml->prop[j]->dparam[1]._pvoid;
					}else{
						pp = (Point_process*)ml->pdata[j][1]._pvoid;
					}
					if (nrn_is_artificial_[i] == 0) {
						int inode = ml->nodelist[j]->v_node_index;
						pp->nvi_ = list_ + cellnum[inode];
					}else{
						pp->nvi_ = nil;
					}
				}
			}
		}
		delete [] cellnum;
		delete [] cvml;
	}
	return true;
}

void NetCvode::fill_global_ba(int bat, BAMechList** baml) {
	BAMech* bam;
	for (bam = bamech_[bat]; bam; bam = bam->next) {
		BAMechList* ba = new BAMechList(baml);
		ba->bam = bam;
	}
}

void NetCvode::fill_local_ba(int* celnum) {
	fill_local_ba_cnt(BEFORE_BREAKPOINT, celnum);
	fill_local_ba_cnt(AFTER_SOLVE, celnum);
	fill_local_ba_cnt(BEFORE_STEP, celnum);
	fill_local_ba_alloc();
	fill_local_ba_indices(BEFORE_BREAKPOINT, celnum);
	fill_local_ba_indices(AFTER_SOLVE, celnum);
	fill_local_ba_indices(BEFORE_STEP, celnum);
}

void NetCvode::fill_local_ba_cnt(int bat, int* celnum) {
	BAMech* bam;
	int i;
	for (bam = bamech_[bat]; bam; bam = bam->next) {
		Memb_list* ml = memb_list + bam->type;
		for (i=0; i < ml->nodecount; ++i) {
			int inode = ml->nodelist[i]->v_node_index;
			Cvode* cv = list_ + celnum[inode];
			BAMechList* bl = cvbml(bat, bam, cv);
			++bl->cnt;
		}
	}
}
void NetCvode::fill_local_ba_alloc() {
	int i;
	for (i=0; i < nlist_; ++i) {
		BAMechList::alloc(list_[i].before_breakpoint_);
		BAMechList::alloc(list_[i].after_solve_);
		BAMechList::alloc(list_[i].before_step_);
	}
}
void NetCvode::fill_local_ba_indices(int bat, int* celnum) {
	BAMech* bam;
	int i;
	for (bam = bamech_[bat]; bam; bam = bam->next) {
		Memb_list* ml = memb_list + bam->type;
		for (i=0; i < ml->nodecount; ++i) {
			int inode = ml->nodelist[i]->v_node_index;
			Cvode* cv = list_ + celnum[inode];
			BAMechList* bl = cvbml(bat, bam, cv);
			bl->indices[bl->cnt] = i;
			++bl->cnt;
		}
	}
}

void BAMechList::alloc(BAMechList* first) {
	BAMechList* ba;
	for (ba = first; ba; ba = ba->next) {
		ba->indices = new int[ba->cnt];
		ba->cnt = 0; // counts up again in fill_local_ba_indices
	}
}

BAMechList* NetCvode::cvbml(int bat, BAMech* bam, Cvode* cv) {
	BAMechList** pbml;
	BAMechList* ba;
	if (bat == BEFORE_BREAKPOINT) {
		pbml = &cv->before_breakpoint_;
	}else if (bat == AFTER_SOLVE) {
		pbml = &cv->after_solve_;
	}else{
		pbml = &cv->before_step_;
	}
	if (!*pbml) {
		ba = new BAMechList(pbml);
	}else{
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

int NetCvode::solve(double tout) {
	int err = NVI_SUCCESS;
	if (nlist_ == 0) {
		if (tout >= 0.) {
			while (tqe_->least_t() <= tout && stoprun==0) {
				deliver_least_event();
			}
			if (stoprun==0) { t = tout; }
		} else {
			if (tqe_->least()) {
				t = tqe_->least_t();
				deliver_events(tqe_->least_t());
			}else{
				t += 1e6;
			}
		}
	}else if (single_) {
		if (tout >= 0.) {
			while (list_[0].t_ < tout || tqe_->least_t() < tout) {
				err = global_microstep();
				if (err != NVI_SUCCESS || stoprun) { return err; }
			}
			retreat(tout, list_);
		} else {
			// advance or initialized
			double tc = list_[0].t_;
			initialized_ = false;
			while (list_[0].t_ <= tc && !initialized_) {
				err = global_microstep();
				if (err != NVI_SUCCESS) { return err; }
			}
		}
	}else if (nlist_ > 0) {
		if (tout >= 0.) {
			time_t rt = time(nil);
//			int cnt = 0;
			while (tq_->least_t() < tout || tqe_->least_t() <= tout) {
				err = local_microstep();
				if (err != NVI_SUCCESS || stoprun) { return err; }
#if HAVE_IV
IFGUI
				if (rt < time(nil)) {
//				if (++cnt > 10000) {
//					cnt = 0;
					Oc oc; oc.notify();
					single_event_run();
					rt = time(nil);
				}
ENDGUI
#endif
			}
			for (int i=0; i < nlist_; ++i) {
				retreat(tout, list_ + i);
				list_[i].record_continuous();
			}
		} else {
			// an fadvance is not every microstep but
			// only when all the discontinuities at te take place or
			// tc increases.
			double tc = tq_->least_t();
			double te = tqe_->least_t();
			while (tq_->least_t() <= tc && tqe_->least_t() <= te){
				err = local_microstep();
				if (err != NVI_SUCCESS) { return err; }
			}
			// But make sure t is not past the least time.
			// fadvance and local step do not coexist seamlessly.
			t = tq_->least_t();
			if (te < t) {
				t = te;
			}
		}
	}else{
		t += 1e9;
	}
	return err;
}

void NetCvode::handle_tstop_event(double tt) {
	if (cvode_active_) {
		int i;
		for (i = 0; i < nlist_; ++i) {
			retreat(tt, list_ + i);
			list_[i].record_continuous();
		}
		initialized_ = true; // not really but we want to return from
		// fadvance after handling this.
	}
}

void NetCvode::deliver_least_event() {
	TQItem* q = tqe_->least();
	DiscreteEvent* de = (DiscreteEvent*)q->data_;
	tqe_->remove(q);
#if PRINT_EVENT
	if (print_event_) { de->pr("deliver", q->t_, this); }
#endif
	STATISTICS(deliver_cnt_);
	de->deliver(q->t_, this);
}

int NetCvode::local_microstep() {
	int err = NVI_SUCCESS;
	if (tqe_->least_t() <= tq_->least_t()) {
		deliver_least_event();
	}else{
		TQItem* q = tq_->least();
		Cvode* cv = (Cvode*)q->data_;
		err = cv->handle_step(this, 1e100);
		tq_->move_least(cv->t_);
	}
	return err;
}

int NetCvode::global_microstep() {
	int err = NVI_SUCCESS;
	if (tqe_->least_t() <= list_->t_) {
		deliver_events(tqe_->least_t());
	}else{
		err = list_->handle_step(this, tqe_->least_t());
	}
	if (tqe_->least_t() < list_->t_) {
		list_->interpolate(tqe_->least_t());
	}
	return err;
}

int Cvode::handle_step(NetCvode* ns, double te) {
	int err = NVI_SUCCESS;
	// first order correct condition evaluation goes here
	if (ns->condition_order() == 1) {
		t = t_; // for localstep method t is for a different cvode.fun call
		check_deliver();
		// done if the check puts a 0 delay event on queue
		if (ns->event_queue()->least_t() <= t_) {
			return err;
		}
	}
	if (initialize_) {
		err = init(t_);
		ns->initialized_ = true;
		// second order correct condition evaluation goes here
		if (ns->condition_order() == 2) {
			evaluate_conditions();
		}
	}else if (te <= tn_) {
		err = interpolate(te);
	}else if (t_ < tn_) {
		err = interpolate(tn_);
	}else{
		record_continuous();
		err = advance_tn();
		// second order correct condition evaluation goes here
		if (ns->condition_order() == 2) {
			evaluate_conditions();
		}
	}
	return err;
}

void net_move(void** v, Point_process* pnt, double tt) {
	if (!(*v)) {
		hoc_execerror( "No event with flag=1 for net_move in ", hoc_object_name(pnt->ob));
	}
	TQItem* q = (TQItem*)(*v);
//printf("net_move tt=%g %s *v=%lx\n", tt, hoc_object_name(pnt->ob), (long)(*v));
	if (tt < t) {
		SelfEvent* se = (SelfEvent*)q->data_;
		char buf[100];
		sprintf(buf, "net_move tt-t = %g", tt-t);
		se->pr(buf, tt, net_cvode_instance);
		hoc_execerror("net_move tt < t", 0);
	}
	net_cvode_instance->move_event(q, tt);
}

void artcell_net_move(void** v, Point_process* pnt, double tt) {
    if (nrn_use_selfqueue_) {
	if (!(*v)) {
		hoc_execerror( "No event with flag=1 for net_move in ", hoc_object_name(pnt->ob));
	}
	TQItem* q = (TQItem*)(*v);
//printf("artcell_net_move t=%g qt_=%g tt=%g %s *v=%lx\n", t, q->t_, tt, hoc_object_name(pnt->ob), (long)(*v));
	q->t_ = tt;
	if (tt < immediate_deliver_) {
//printf("artcell_net_move_ %s immediate %g %g %g\n", hoc_object_name(pnt->ob), t, tt, immediate_deliver_);
		SelfEvent* se = (SelfEvent*)q->data_;
		se->deliver(tt, net_cvode_instance);
	}
	if (tt < t) {
		SelfEvent* se = (SelfEvent*)q->data_;
		char buf[100];
		sprintf(buf, "net_move tt-t = %g", tt-t);
		se->pr(buf, tt, net_cvode_instance);
		hoc_execerror("net_move tt < t", 0);
	}
    }else{
	net_move(v, pnt, tt);
    }
}

void NetCvode::move_event(TQItem* q, double tnew) {
	STATISTICS(SelfEvent::selfevent_move_);
#if PRINT_EVENT
if (print_event_) {
	SelfEvent* se = (SelfEvent*)q->data_;
printf("NetCvode::move_event self event target %s t=%g, old=%g new=%g\n", hoc_object_name(se->target_->ob), t, q->t_, tnew);
}
#endif
#if USENEOSIM
	// only self events move
	if (neosim_entity_){
		cvode_instance->neosim_self_events_->move(q, tnew);
	}else{
		tqe_->move(q, tnew);
	}
#else
	tqe_->move(q, tnew);
#endif
}

void NetCvode::remove_event(TQItem* q) {
	tqe_->remove(q);
}

void net_send(void** v, double* weight, Point_process* pnt, double delay, double flag) {
	STATISTICS(SelfEvent::selfevent_send_);
	SelfEvent* se = SelfEvent::alloc();
	se->flag_ = flag;
	se->target_ = pnt;
	se->weight_ = weight;
	se->movable_ = v; // needed for SaveState
	assert(net_cvode_instance);
	++net_cvode_instance->unreffed_event_cnt_;
	if (delay < 0) {
		char buf[100];
		sprintf(buf, "net_send delay = %g", delay);
		se->pr(buf, t+delay, net_cvode_instance);
		hoc_execerror("net_send delay < 0", 0);
	}
	TQItem* q;
#if USENEOSIM
	if (neosim_entity_) {
		cvode_instance->neosim_self_events_->insert(t + delay, se);
	}else{
		q = net_cvode_instance->event(t + delay, se);
	}
#else
	q = net_cvode_instance->event(t + delay, se);
#endif
	if (flag == 1.0) {
		*v = (void*)q;
	}
//printf("net_send %g %s %g %lx\n", t+delay, hoc_object_name(pnt->ob), flag, (long)(*v));
}

void artcell_net_send(void** v, double* weight, Point_process* pnt, double delay, double flag) {
    if (nrn_use_selfqueue_ && flag == 1.0) {
	STATISTICS(SelfEvent::selfevent_send_);
	SelfEvent* se = SelfEvent::alloc();
	se->flag_ = flag;
	se->target_ = pnt;
	se->weight_ = weight;
	se->movable_ = v; // needed for SaveState
	assert(net_cvode_instance);
	++net_cvode_instance->unreffed_event_cnt_;
	if (delay < 0) {
		char buf[100];
		sprintf(buf, "net_send delay = %g", delay);
		se->pr(buf, t+delay, net_cvode_instance);
		hoc_execerror("net_send delay < 0", 0);
	}
	TQItem* q;
	q = selfqueue_->insert(se);
	q->t_ = t + delay;
	*v = (void*)q;
//printf("artcell_net_send %g %s %g %lx\n", t+delay, hoc_object_name(pnt->ob), flag, (long)(*v));
	if (q->t_ < immediate_deliver_) {
//printf("artcell_net_send_  %s immediate %g %g %g\n", hoc_object_name(pnt->ob), t, q->t_, immediate_deliver_);
		SelfEvent* se = (SelfEvent*)q->data_;
		selfqueue_->remove(q);
		se->deliver(q->t_, net_cvode_instance);
	}
    }else{
	net_send(v, weight, pnt, delay, flag);
    }
}

void net_event(Point_process* pnt, double time) {
	STATISTICS(net_event_cnt_);
	PreSyn* ps = (PreSyn*)pnt->presyn_;
	if (ps) {
		if (time < t) {
			char buf[100];
			sprintf(buf, "net_event time-t = %g", time-t);
			ps->pr(buf, time, net_cvode_instance);
			hoc_execerror("net_event time < t", 0);
		}
#if USENEOSIM
		if (neosim_entity_) {
			(*p_nrn2neosim_send)(neosim_entity_, t);
		}else{
#endif
		ps->send(time, net_cvode_instance);
#if USENEOSIM
		}
#endif
	}
}

void _nrn_watch_activate(Datum* d, double (*c)(Point_process*), int i, Point_process* pnt, int r, double flag) {
//	printf("_nrn_cond_activate %s flag=%g first return = %g\n", hoc_object_name(pnt->ob), flag, c(pnt));
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
				net_cvode_instance->remove_event(wc1->qthresh_);
				wc1->qthresh_ = nil;
			}
		}
		wl->remove_all();
	}
	WatchCondition* wc = (WatchCondition*)d[i]._pvoid;
	wl->append(wc);
	wc->activate(flag);
}

void _nrn_free_watch(Datum* d, int offset, int n) {
	int i;
	int nn = offset + n;
	for (i=offset; i < nn; ++i) {
		if (d[i]._pvoid) {
			WatchCondition* wc = (WatchCondition*)d[i]._pvoid;
			wc->Remove();
			delete wc;
		}
	}
}

void NetCvode::vec_event_store() {
	// not destroyed when vector destroyed.
	// should resize to 0 or remove before storing, just keeps incrementing
	if (vec_event_store_) {
		vec_event_store_ = nil;
	}
	if (ifarg(1)) {
		vec_event_store_ = vector_arg(1);
	}
}

#if BBTQ == 3 || BBTQ == 4
TQItem* NetCvode::fifo_event(double td, DiscreteEvent* db) {
    if (nrn_use_fifo_queue_) {
#if PRINT_EVENT
	if (print_event_) { db->pr("send", td, this); }
	if (vec_event_store_) {
		Vect* x = vec_event_store_;
		int n = x->capacity();
		x->resize_chunk(n+2);
		x->elem(n) = t;
		x->elem(n+1) = td;
	}
#endif
	return tqe_->insert_fifo(td, db);
    }else{
	return tqe_->insert(td, db);
    }
}
#else
#define fifo_event event
#endif

#if BBTQ == 5
TQItem* NetCvode::bin_event(double td, DiscreteEvent* db) {
    if (nrn_use_bin_queue_) {
#if PRINT_EVENT
	if (print_event_) {db->pr("binq send", td, this);}
	if (vec_event_store_) {
		Vect* x = vec_event_store_;
		int n = x->capacity();
		x->resize_chunk(n+2);
		x->elem(n) = t;
		x->elem(n+1) = td;
	}
#endif
	return tqe_->enqueue_bin(td, db);
    }else{
#if PRINT_EVENT
	if (print_event_) {db->pr("send", td, this);}
#endif
	return tqe_->insert(td, db);
    }
}
#else
#define bin_event event
#endif

TQItem* NetCvode::event(double td, DiscreteEvent* db) {
#if PRINT_EVENT
	if (print_event_) { db->pr("send", td, this); }
	if (vec_event_store_) {
		Vect* x = vec_event_store_;
		int n = x->capacity();
		x->resize_chunk(n+2);
		x->elem(n) = t;
		x->elem(n+1) = td;
	}
#endif
	return tqe_->insert(td, db);
}

void NetCvode::null_event(double tt) {
	if (tt - t < 0) { return; }
#if USENEOSIM
	if (neosim_entity_) {
		// ignore for neosim. There is no appropriate cvode_instance
		// cvode_instance->neosim_self_events_->insert(t + delay, null_event_);
	}else{
		event(tt, null_event_);
	}
#else
	event(tt, null_event_);
#endif
}

void NetCvode::tstop_event(double tt) {
	if (tt - t < 0) { return; }
#if USENEOSIM
	if (neosim_entity_) {
		// ignore for neosim. There is no appropriate cvode_instance
		// cvode_instance->neosim_self_events_->insert(t + delay, tstop_event_);
	}else{
		event(tt, tstop_event_);
	}
#else
	event(tt, tstop_event_);
#endif
}

void NetCvode::hoc_event(double tt, const char* stmt) {
	if (tt - t < 0) { return; }
	TQItem* q;
#if USENEOSIM
	if (neosim_entity_) {
		// ignore for neosim. There is no appropriate cvode_instance
		// cvode_instance->neosim_self_events_->insert(t + delay, null_event_);
	}else{
		event(tt, HocEvent::alloc(stmt));
	}
#else
	event(tt,  HocEvent::alloc(stmt));
#endif
}

void NetCvode::clear_events() {
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
	SelfEvent::reclaim();
	HocEvent::reclaim();
#if USENEOSIM
	int i;
	if (p_nrn2neosim_send) for (i=0; i < nlist_; ++i) {
		TQueue* tq = list_[i].neosim_self_events_;
		while(tq->least()) { tq->remove(tq->least());}
		// and have already been reclaimed by SelfEvent::reclaim()
	}
#endif
	unreffed_event_cnt_ = 0;
	delete tqe_;
	tqe_ = new TQueue();
	// I don't believe this is needed anymore since cvode not needed
	// til delivery.
	if (cvode_active_) { // in case there is a net_send from INITIAL cvode
		// then this needs to be done before INITIAL blocks are called
		init_global();
	}
	if (nrn_use_selfqueue_) {
		if (!selfqueue_) {
			selfqueue_ = new SelfQueue();
		}else{
			selfqueue_->remove_all();
		}
	}
#if BBTQ == 5
	tqe_->nshift_ = -1;
	tqe_->shift_bin(t);
#endif
}

void NetCvode::init_events() {
	hoc_Item* q;
	int i, j;
	double fifodelay;
#if BBTQ == 5
	tqe_->nshift_ = -1;
	tqe_->shift_bin(t);
#endif
	if (psl_) {
		ITERATE(q, psl_) {
			PreSyn* ps = (PreSyn*)VOIDITM(q);
			ps->init();
			ps->flag_ = false;
			NetConPList& dil = ps->dil_;
			ps->use_min_delay_ = 0;
#if USE_MIN_DELAY
			// also decide what to do about use_min_delay_
			// the rule for now is to use it if all delays are
			// the same and there are more than 2
#if BBTQ == 3 || BBTQ == 4
			// but
			// if we desire nrn_use_fifo_queue_ then use it
			// even if just one
			if (nrn_use_fifo_queue_) {
				if (dil.count()) {
					ps->use_min_delay_ = 1;
					ps->delay_ = dil.item(0)->delay_;
					fifodelay = ps->delay_;
				}
			}else
#endif // BBTQ
			{
				if (dil.count() > 2) {
					ps->use_min_delay_ = 1;
					ps->delay_ = dil.item(0)->delay_;
				}
			}
#endif // USE_MIN_DELAY

			for (i=dil.count()-1; i >= 0; --i) {
				NetCon* d = dil.item(i);
				if (d->target_) {
					int type = d->target_->prop->type;
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
#if BBTQ == 3 || BBTQ == 4
				if (nrn_use_fifo_queue_ && d->delay_ != fifodelay) {
hoc_warning("Use of the event fifo queue is turned off due to more than one value for NetCon.delay", 0);
					nrn_use_fifo_queue_ = false;
				}
#endif
			}
		}
	}
	for (i=0; i < nlist_; ++i) {
		if (list_[i].watch_list_) {
			list_[i].watch_list_->RemoveAll();
		}
	}
}

double PreSyn::mindelay() {
	double md = 1e9;
	int i;
	for (i=dil_.count()-1; i >= 0; --i) {
		NetCon* d = dil_.item(i);
		if (md > d->delay_) {
			md = d->delay_;
		}
	}
	return md;
}

void NetCvode::deliver_events(double til) {
	double t, dt;
//printf("deliver_events til %20.15g\n", til);
	while (tqe_->least_t() <= til) {
		deliver_least_event();
	}
}

static IvocVect* peqvec; //if not nil then the sorted times on the event queue.
static void peq(const TQItem*, int);
static void peq(const TQItem* q, int) {
	if (peqvec) {
		int n = peqvec->capacity();
		peqvec->resize_chunk(n+1);
		peqvec->elem(n) = q->t_;
	}else{
		DiscreteEvent* d = (DiscreteEvent*)q->data_;
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
	tqe_->forall_callback(peq);
	peqvec = nil;
}

static int event_info_type_;
static IvocVect* event_info_tvec_;
static IvocVect* event_info_flagvec_;
static OcList* event_info_list_; // netcon or point_process

static void event_info_callback(const TQItem*, int);
static void event_info_callback(const TQItem* q, int) {
	DiscreteEvent* d = (DiscreteEvent*)q->data_;
	NetCon* nc;
	PreSyn* ps;
	SelfEvent* se;
	int n = event_info_tvec_->capacity();
	switch(d->type()) {
	case NetConType:
		if (event_info_type_ == NetConType) {
			nc = (NetCon*)d;
			event_info_tvec_->resize_chunk(n+1);
			event_info_tvec_->elem(n) = q->t_;
			event_info_list_->append(nc->obj_);
		}
		break;
	case SelfEventType:
		if (event_info_type_ == SelfEventType) {
			se = (SelfEvent*)d;
			event_info_tvec_->resize_chunk(n+1);
			event_info_tvec_->elem(n) = q->t_;
			event_info_flagvec_->resize_chunk(n+1);
			event_info_flagvec_->elem(n) = se->flag_;
			event_info_list_->append(se->target_->ob);
		}
		break;
	case PreSynType:
		if (event_info_type_ == NetConType) {
			ps = (PreSyn*)d;
			for (int i = ps->dil_.count()-1; i >= 0; --i) {
				nc = ps->dil_.item(i);
				double td = nc->delay_ - ps->delay_;
				event_info_tvec_->resize_chunk(n+1);
				event_info_tvec_->elem(n) = q->t_ + td;
				event_info_list_->append(nc->obj_);
				++n;
			}
		}
		break;
	}
}

void NetCvode::event_queue_info() {
	// dangerous since many events can go out of existence after
	// a simulation and before NetCvode::clear at the next initialization
	int i = 1;
	event_info_type_ = (int)chkarg(i++, 2, 3);
	event_info_tvec_ = vector_arg(i++);
	event_info_tvec_->resize(0);
	if (event_info_type_ == SelfEventType) {
		event_info_flagvec_ = vector_arg(i++);
		event_info_flagvec_->resize(0);
	}
	Object* o = *hoc_objgetarg(i++);
	check_obj_type(o, "List");
	event_info_list_ = (OcList*)o->u.this_pointer;
	event_info_list_->remove_all();
	tqe_->forall_callback(event_info_callback);
}

void DiscreteEvent::send(double tt, NetCvode* ns) {
	STATISTICS(discretevent_send_);
	ns->event(tt, this);
}

void DiscreteEvent::deliver(double tt, NetCvode* ns) {
	STATISTICS(discretevent_deliver_);
}
void DiscreteEvent::pgvts_deliver(double tt, NetCvode* ns) {
	STATISTICS(discretevent_deliver_);
}

void DiscreteEvent::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s DiscreteEvent %.15g\n", s, tt);
}

void NetCon::send(double tt, NetCvode* ns) {
	if (active_ && target_) {
		STATISTICS(netcon_send_active_);
#if BBTQ == 5
		ns->bin_event(tt, this);
#else
		ns->event(tt, this);
#endif
	}else{
		STATISTICS(netcon_send_inactive_);
	}
}
	
void NetCon::deliver(double tt, NetCvode* ns) {
	assert(target_);
	Cvode* cv = (Cvode*)target_->nvi_;
	int type = target_->prop->type;
	if (nrn_use_selfqueue_ && nrn_is_artificial_[type]) {
		TQItem** pq = (TQItem**)(&target_->prop->dparam[nrn_artcell_qindex_[type]]._pvoid);
		TQItem* q;
		while ((q = *(pq)) != nil && q->t_ < tt) {
			SelfEvent* se = (SelfEvent*)selfqueue_->remove(q);
//printf("%d NetCon::deliver %g , earlier selfevent at %g\n", nrnmpi_myid, tt, q->t_);
			se->deliver(q->t_, ns);
		}	
	}
	if (cvode_active_ && cv) {
		ns->retreat(tt, cv);
		cv->set_init_flag();
	}else{
// no interpolation necessary for local step method and ARTIFICIAL_CELL
		t = tt;
	}

//printf("NetCon::deliver t=%g tt=%g %s\n", t, tt, hoc_object_name(target_->ob));
	STATISTICS(netcon_deliver_);
	(*pnt_receive[type])(target_, weight_, 0);
	if (errno) {
		if (nrn_errno_check(type)) {
hoc_warning("errno set during NetCon deliver to NET_RECEIVE", (char*)0);
		}
	}
}
void NetCon::pgvts_deliver(double tt, NetCvode* ns) {
	assert(target_);
	int type = target_->prop->type;
	STATISTICS(netcon_deliver_);
	(*pnt_receive[type])(target_, weight_, 0);
	if (errno) {
		if (nrn_errno_check(type)) {
hoc_warning("errno set during NetCon deliver to NET_RECEIVE", (char*)0);
		}
	}
}

void NetCon::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s %s", s, hoc_object_name(obj_));
	if (src_) {
		printf(" src=%s",  src_->osrc_ ? hoc_object_name(src_->osrc_):secname(src_->ssrc_));
	}else{
		printf(" src=nil");
	}
	printf(" target=%s %.15g\n", (target_?hoc_object_name(target_->ob):"nil"), tt);
}

void PreSyn::send(double tt, NetCvode* ns) {
	record(tt);
	if (use_min_delay_) {
		STATISTICS(presyn_send_mindelay_);
#if BBTQ == 3 || BBTQ == 4
		ns->fifo_event(tt+delay_, this);
#else
#if BBTQ == 5
		ns->bin_event(tt+delay_, this);
#else
		ns->event(tt+delay_, this);
#endif
#endif
	}else{
		STATISTICS(presyn_send_direct_);
		for (int i = dil_.count()-1; i >= 0; --i) {
			NetCon* d = dil_.item(i);
			if (d->active_ && d->target_) {
#if BBTQ == 5
				ns->bin_event(tt + d->delay_, d);
#else
				ns->event(tt + d->delay_, d);
#endif
			}
		}
	}
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
	
void PreSyn::deliver(double tt, NetCvode* ns) {
	if (qthresh_) {
		qthresh_ = nil;
//printf("PreSyn::deliver %s condition event tt=%20.15g\n", ssrc_?secname(ssrc_):"", tt);
		STATISTICS(deliver_qthresh_);
		send(tt, ns);
		return;
	}
	int i, n = dil_.count();
	STATISTICS(presyn_deliver_netcon_);
	for (i=0; i < n; ++i) {
		NetCon* d = dil_.item(i);
		if (d->active_ && d->target_) {
			double dtt = d->delay_ - delay_;
			if (dtt == 0.) {
				STATISTICS(presyn_deliver_direct_);
				STATISTICS(deliver_cnt_);
				d->deliver(tt, ns);
			}else if (dtt < 0.) {
hoc_execerror("internal error: Source delay is > NetCon delay", 0);
			}else{
				STATISTICS(presyn_deliver_ncsend_);
				ns->event(tt + dtt, d);
			}
		}
	}
}

void PreSyn::pgvts_deliver(double tt, NetCvode* ns) {
	if (qthresh_) {
		qthresh_ = nil;
//printf("PreSyn::deliver %s condition event tt=%20.15g\n", ssrc_?secname(ssrc_):"", tt);
		STATISTICS(deliver_qthresh_);
		send(tt, ns);
		return;
	}
	int i, n = dil_.count();
	STATISTICS(presyn_deliver_netcon_);
	for (i=0; i < n; ++i) {
		NetCon* d = dil_.item(i);
		if (d->active_ && d->target_) {
			double dtt = d->delay_ - delay_;
			if (0 && dtt == 0.) {
				STATISTICS(presyn_deliver_direct_);
				STATISTICS(deliver_cnt_);
				d->deliver(tt, ns);
			}else if (dtt < 0.) {
hoc_execerror("internal error: Source delay is > NetCon delay", 0);
			}else{
				STATISTICS(presyn_deliver_ncsend_);
				ns->event(tt + dtt, d);
			}
		}
	}
}

void PreSyn::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s", s);
	printf(" PreSyn src=%s",  osrc_ ? hoc_object_name(osrc_):secname(ssrc_));
	printf(" %.15g\n", tt);
}

SelfEvent::SelfEvent() {}
SelfEvent::~SelfEvent() {}
SelfEvent* SelfEvent::alloc() {
	if (!sepool_) {
		sepool_ = new SelfEventPool(1000);
	}
	return sepool_->alloc();
}

void SelfEvent::sefree() {
	sepool_->hpfree(this);
}

void SelfEvent::reclaim() {
	if (sepool_) {
		sepool_->free_all();
	}
}

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
	net_send(movable_, weight_, target_, tt - t, flag_);
}

DiscreteEvent* SelfEvent::savestate_read(FILE* f) {
	SelfEvent* se = new SelfEvent();
	char buf[300];
	char ppname[200];
	int ppindex, ncindex, moff, pptype, iml;
	double flag;
	Object* obj;
	fgets(buf, 300, f);
	assert(sscanf(buf, "%s %d %d %d %d %d %lf\n", ppname, &ppindex, &pptype, &iml, &ncindex, &moff, &flag) == 7);
#if 0
	// use of hoc_name2obj is way too inefficient
	se->target_ = ob2pntproc(hoc_name2obj(ppname, ppindex));
#else
	if (memb_func[pptype].hoc_mech) { // actually, this case does not exist
		se->target_ = (Point_process*)memb_list[pptype].prop[iml]->dparam[1]._pvoid;
	}else{
		se->target_ = (Point_process*)memb_list[pptype].pdata[iml][1]._pvoid;
	}
#endif
	se->weight_ = nil;
	if (ncindex >= 0) {	
#if 0
		// extremely inefficient. There are a LOT of NetCon.
		obj = hoc_name2obj("NetCon", ncindex);
		assert(obj);
		NetCon* nc = (NetCon*)obj->u.this_pointer;
#else
		NetCon* nc = NetConSave::index2netcon(ncindex);
#endif
		se->weight_ = nc->weight_;
	}
	se->flag_ = flag;
	se->movable_ = nil;
	if (moff >= 0) {
		se->movable_ = &(se->target_->prop->dparam[moff]._pvoid);
	}
	return se;
}

void SelfEvent::savestate_write(FILE* f) {
	fprintf(f, "%d\n", SelfEventType);
	int moff = -1;
	if (movable_) {
		moff = (Datum*)(movable_) - target_->prop->dparam;
		assert(movable_ == &(target_->prop->dparam[moff]._pvoid));
	}

	int ncindex = -1;
	// find the NetCon index for weight_
	if (weight_) {
		NetCon* nc = NetConSave::weight2netcon(weight_);
		assert(nc);
		ncindex = nc->obj_->index;
	}

	fprintf(f, "%s %d %d %d %d %d %g\n",
		target_->ob->ctemplate->sym->name, target_->ob->index,
		target_->prop->type, target_->iml_,
		ncindex,
		moff, flag_
	);
}

void SelfEvent::deliver(double tt, NetCvode* ns) {
	Cvode* cv = (Cvode*)target_->nvi_;
	int type = target_->prop->type;
	if (nrn_use_selfqueue_ && nrn_is_artificial_[type]) { // handle possible earlier flag=1 self event
		if (flag_ == 1.0) { *movable_ = 0; }
		TQItem* q;
		while ((q = (TQItem*)(*movable_)) != 0 && q->t_ <= tt) {
//printf("handle earlier %g selfqueue event from within %g SelfEvent::deliver\n", q->t_, tt);
			SelfEvent* se = (SelfEvent*)selfqueue_->remove(q);
			t = q->t_;
			se->call_net_receive(ns);
		}
	}
	if (cvode_active_ && cv) {
		ns->retreat(tt, cv);
		cv->set_init_flag();
	}else{
		t = tt;
	}
//printf("SelfEvent::deliver t=%g tt=%g %s\n", t, tt, hoc_object_name(target_->ob));
	call_net_receive(ns);
}
void SelfEvent::pgvts_deliver(double tt, NetCvode* ns) {
	call_net_receive(ns);
}
void SelfEvent::call_net_receive(NetCvode* ns) {
	STATISTICS(selfevent_deliver_);
	(*pnt_receive[target_->prop->type])(target_, weight_, flag_);
	if (errno) {
		if (nrn_errno_check(target_->prop->type)) {
hoc_warning("errno set during SelfEvent deliver to NET_RECEIVE", (char*)0);
		}
	}
	sefree();
	--(ns->unreffed_event_cnt_);
}
	
void SelfEvent::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s", s);
	printf(" SelfEvent target=%s %.15g flag=%g\n", hoc_object_name(target_->ob), tt, flag_);
}

void PlayRecordEvent::frecord_init(TQItem* q) {
	plr_->frecord_init(q);
}

void PlayRecordEvent::deliver(double tt, NetCvode* ns) {
	if (plr_->cvode_) {
		ns->retreat(tt, plr_->cvode_);
	}
	STATISTICS(playrecord_deliver_);
	plr_->deliver(tt, ns);
}

void PlayRecordEvent::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s PlayRecordEvent %.15g ", s, tt);
	plr_->pr();
}

// For localstep makes sure all cvode instances are at this time and 
// makes sure the continuous record records values at this time.
TstopEvent::TstopEvent() {}
TstopEvent::~TstopEvent() {}

void TstopEvent::deliver(double tt, NetCvode* ns) {
	STATISTICS(discretevent_deliver_);
	ns->handle_tstop_event(tt);
}
void TstopEvent::pgvts_deliver(double tt, NetCvode* ns) {
	deliver(tt, ns);
}

void TstopEvent::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s TstopEvent %.15g\n", s, tt);
}

DiscreteEvent* TstopEvent::savestate_save() {
//	pr("savestate_save", 0, net_cvode_instance);
	if (this != tstop_event_) {
		hoc_execerror("TstopEvent::savestate_save:", " is not the tstop_event_");
	}
	return new TstopEvent();
}

void TstopEvent::savestate_restore(double tt, NetCvode* nc) {
//	pr("savestate_restore", tt, nc);
	printf("tstop_event_ onto queue\n");
	nc->tstop_event(tt);
}

DiscreteEvent* TstopEvent::savestate_read(FILE* f) {
	return new TstopEvent();
}

void TstopEvent::savestate_write(FILE* f) {
	fprintf(f, "%d\n", TstopEventType);
}

#include <hocevent.cpp>

void NetCvode::retreat(double t, Cvode* cv) {
	if (!cvode_active_) { return; }
	if (tq_) {
#if PRINT_EVENT
		if (print_event_) {
printf("microstep retreat from %g (cvode_%lx is at %g) for event onset=%g\n", cv->tqitem_->t_, (long)cv, cv->t_, t);
		}
#endif
		cv->interpolate(t);
		tq_->move(cv->tqitem_, t);
#if PRINT_EVENT
		if (print_event_ > 1) {
printf("after target solve time for %lx is %g , dt=%g\n", (long)cv, cv->time(), dt);
		}
#endif
	}
}

#if USENEOSIM

boolean neosim_deliver_self_events(TQueue* tqe, double til);
boolean neosim_deliver_self_events(TQueue* tqe, double til) {
	boolean b;
	TQItem* q;
	DiscreteEvent* d;
	b = false;
	while (tqe->least_t() <= til + .5e-8) {
		b = true;
		q = tqe->least();
		t = q->t_;
		d = (DiscreteEvent*)q->data_;
		assert(d->type() == SelfEventType);
		tqe->remove(q);
		d->deliver(t, net_cvode_instance);
	}
}

void neosim2nrn_advance(void* e, void* v, double tout) {
	neosim_entity_ = e;
	NetCon* d = (NetCon*)v;
	TQueue* tqe;
		
	// now can integrate to tout since it is guaranteed there will
	// be no further real events to this cell before tout.
	// but we must handle self events. The implementation is
	// analogous to the NetCvode::solve with single and tout
	cvode_instance = (Cvode*)d->target_->nvi_; // so self event from INITIAL block
	tqe = cvode_instance->neosim_self_events_;
	// not a bug even if there is no BREAKPOINT block. I.e.
	// artificial cells will work.
	t = cvode_instance->time();
	while (tout > t) {
		do { 
			cvode_instance->check_deliver();
		}while (neosim_deliver_self_events(tqe, t));
		cvode_instance->solve();
	}
	cvode_instance->interpolate(tout);
	cvode_instance->check_deliver();
}

void neosim2nrn_deliver(void* e, void* v) {
	neosim_entity_ = e;
	NetCon* d = (NetCon*)v;
	Cvode* cv = (Cvode*)d->target_->nvi_;
	d->deliver(cv->t_, net_cvode_instance);
}

#endif

//parallel global variable time-step
int NetCvode::pgvts(double tstop) {
	int err = NVI_SUCCESS;
	double tt = t;
	while (tt < tstop && !stoprun && err == NVI_SUCCESS) {
		err = pgvts_event(tt);
	}
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
	int rank, op, err, init;
	DiscreteEvent* de;
	if (condition_order() == 1) {
		list_->check_deliver();
	}
	de = pgvts_least(tt, op, init);
	err = pgvts_cvode(tt, op);
	if (init) { list_->set_init_flag(); }
	if (de) { // handle the event
		de->pgvts_deliver(tt, this);
	}
	return err;
}

DiscreteEvent* NetCvode::pgvts_least(double& tt, int& op, int& init) {
	DiscreteEvent* de = nil;
#if PARANEURON
	TQItem* q = nil;
	if (list_->initialize_ && tqe_->least_t() > list_->t_) {
		tt = list_->t_;
		op = 3;
		init = 0;
	}else if (list_->tn_ < tqe_->least_t()) {
		tt = list_->tn_;
		op = 1;
		init = 0;
	}else{
		q = tqe_->least();
		if (q) {
			de = (DiscreteEvent*)q->data_;
			tt = q->t_;
			op = de->pgvts_op(init);
		}else{
			tt = 1e20;
			op = 1;
			init = 0;
		}
	}
	if (nrnmpi_pgvts_least(&tt, &op, &init)) {
		if (q) {
			tqe_->remove(q);
		}
	}else if (op == 4) {//NetParEvent need to be done all together
		tqe_->remove(q); 
	}else{
		de = nil;
	}
#endif
	return de;
}

int NetCvode::pgvts_cvode(double tt, int op) {
	int err = NVI_SUCCESS;
	// this is the only place where we can enter cvode
	switch (op) {
	case 1: // advance
		list_->record_continuous();
		err = list_->advance_tn();
		if (condition_order() == 2) {
			list_->evaluate_conditions();
		}
		break;
	case 2: // interpolate
		err = list_->interpolate(tt);
		break;
	case 3: // initialize
		err = list_->init(tt);
		initialized_ = true;
		if (condition_order() == 2) {
			list_->evaluate_conditions();
		}
		break;
	}
	return err;
}

void ncs2nrn_integrate(double tstop) {
	double ts;
	if (cvode_active_) {
#if PARANEURON
		if (net_cvode_instance->list()->use_partrans_) {
			net_cvode_instance->pgvts(tstop);
		}else
#endif
		{
			net_cvode_instance->solve(tstop);
		}
	}else{
#if NRNMPI
		ts = tstop - dt;
		assert(t <= tstop);
		// It may very well be the case that we do not advance at all
		while (t <= ts) {
#else
		ts = tstop - .5*dt;
		while (t < ts) {
#endif
			nrn_fixed_step();
			if (stoprun) {break;}
		}
	}
	// handle all the pending flag=1 self events
	nrn_pending_selfqueue(t);
}
void nrn_pending_selfqueue(double tt) {
	if (nrn_use_selfqueue_) {
		immediate_deliver_ = tt;
		double ts = t;
		TQItem* q1, *q2;
		for (q1 = selfqueue_->first(); q1; q1 = q2) {
			q2 = selfqueue_->next(q1);
			if (q1->t_ <= tt) {
				SelfEvent* se = (SelfEvent*)q1->data_;
				selfqueue_->remove(q1);
//printf("ncs2nrn_integrate %g SelfEvent for %s at %g\n", tstop, hoc_object_name(se->target_->ob), q1->t_);
				se->deliver(q1->t_, net_cvode_instance);
			}
		}
		assert(net_cvode_instance->event_queue()->least_t() >= tt);
		t = ts;
		immediate_deliver_ = -1e100;
	}
}

#if USENCS

void ncs2nrn_inputevent(int i, double tdeliver) {
	net_cvode_instance->event(tdeliver, ncs2nrn_input_->item(i));
}

// hoc tells us which are the input NetCons and which are the
// output NetCons

void nrn2ncs_netcons() {
	int i;
	Object* o;
	NetCon* nc;
	o = *hoc_objgetarg(1);
	check_obj_type(o, "List");
	OcList* list = (OcList*)(o->u.this_pointer);
	if (ncs2nrn_input_) {
		for (i=0; i < ncs2nrn_input_->count(); ++i) {
			hoc_obj_unref(ncs2nrn_input_->item(i)->obj_);
		}
		ncs2nrn_input_->remove_all();
	}else{
		ncs2nrn_input_ = new NetConPList(list->count());

	}
	for (i=0; i < list->count(); ++i) {
		hoc_obj_ref(list->object(i));
		nc = (NetCon*)(list->object(i)->u.this_pointer);
		ncs2nrn_input_->append(nc);
	}
	
	o = *hoc_objgetarg(2);
	check_obj_type(o, "List");
	list = (OcList*)(o->u.this_pointer);
	for (i=0; i < list->count(); ++i) {
		nc = (NetCon*)(list->object(i)->u.this_pointer);
		assert(nc->src_);
		nc->src_->output_index_ = i;
	}
}

#endif //USENCS

void NetCvode::statistics(int i) {
	if (i >= 0 && i < nlist_) {
		list_[i].statistics();
	}else{
		for (int j=0; j < nlist_; ++j) {
			list_[j].statistics();
		}
	}
	printf("NetCon active=%lu (not sent)=%lu delivered=%lu\n", NetCon::netcon_send_active_, NetCon::netcon_send_inactive_, NetCon::netcon_deliver_);
	printf("Condition O2 thresh detect=%lu via init=%lu effective=%lu abandoned=%lu (unnecesarily=%lu init+=%lu init-=%lu above=%lu below=%lu)\n",ConditionEvent::send_qthresh_, ConditionEvent::init_above_, ConditionEvent::deliver_qthresh_, ConditionEvent::abandon_, ConditionEvent::eq_abandon_, ConditionEvent::abandon_init_above_, ConditionEvent::abandon_init_below_, ConditionEvent::abandon_above_, ConditionEvent::abandon_below_);
	printf("PreSyn send: mindelay=%lu direct=%lu\n", PreSyn::presyn_send_mindelay_, PreSyn::presyn_send_direct_);
	printf("PreSyn deliver: O2 thresh=%lu  NetCon=%lu (send=%lu  deliver=%lu)\n", ConditionEvent::deliver_qthresh_, PreSyn::presyn_deliver_netcon_, PreSyn::presyn_deliver_ncsend_, PreSyn::presyn_deliver_direct_);
	printf("SelfEvent send=%lu move=%lu deliver=%lu\n", SelfEvent::selfevent_send_, SelfEvent::selfevent_move_, SelfEvent::selfevent_deliver_);
	printf("Watch send=%lu deliver=%lu\n", WatchCondition::watch_send_, WatchCondition::watch_deliver_);
	printf("PlayRecord send=%lu deliver=%lu\n", PlayRecordEvent::playrecord_send_, PlayRecordEvent::playrecord_deliver_);
	printf("HocEvent send=%lu deliver=%lu\n", HocEvent::hocevent_send_, HocEvent::hocevent_deliver_);
	printf("SingleEvent deliver=%lu move=%lu\n", KSSingle::singleevent_deliver_, KSSingle::singleevent_move_);
	printf("DiscreteEvent send=%lu deliver=%lu\n", DiscreteEvent::discretevent_send_, DiscreteEvent::discretevent_deliver_);
	printf("%lu total events delivered  net_event=%lu\n", deliver_cnt_, net_event_cnt_);
	printf("Discrete event TQueue\n");
	tqe_->statistics();
	if (tq_) {
		printf("Variable step integrator TQueue\n");
		tq_->statistics();
	}
}

void NetCvode::spike_stat() {
	Vect* v = vector_arg(1);
	v->resize(11);
	double* d = vector_vec(v);
	int i, n = 0;
	for (i = 0; i < nlist_; ++i) {
		n += list_[i].neq_;
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
	tqe_->spike_stat(d+8);
}

void NetCvode::solver_prepare() {
	int i;
	fornetcon_prepare();
  if (nrn_modeltype() == 0) {
		delete_list();
  }else{
	init_global();
    if (cvode_active_) {
	if (matrix_change_cnt_ != nrn_matrix_cnt_) {
		structure_change();
		matrix_change_cnt_ = nrn_matrix_cnt_;
	}
	for (i=0; i < nlist_; ++i) {
		list_[i].use_daspk_ = nrn_use_daspk_;
		list_[i].init_prepare();
	}
	// since there may be Vector.play events and INITIAL send events
	// at time 0 before actual initialization of integrators.
	for (i=0; i < nlist_; ++i) {
		list_[i].can_retreat_ = false;
	}
    }
  }
	if (playrec_change_cnt_ != structure_change_cnt_) {
		playrec_setup();
	}
}

void NetCvode::re_init(double t) {
	int i, j, k, l;
	if (nrn_modeltype() == 0) {
		for (i=0; i < nlist_; ++i) { list_[i].t_ = t; list_[i].tn_ = t;}
		return;
	}
	double dtsav = dt;
	solver_prepare();
	for (i=0; i < nlist_; ++i) {
		list_[i].stat_init();
		list_[i].init(t);
		if (!single_) {
			list_[i].tqitem_->t_ = t;
		}
		if (condition_order() == 2) {
			list_[i].evaluate_conditions();
		}
	}
	dt = dtsav;
}

void NetCvode::fornetcon_prepare() {
	if (fornetcon_change_cnt_ == structure_change_cnt) { return; }
	fornetcon_change_cnt_ = structure_change_cnt;
	if (nrn_fornetcon_cnt_ == 0) { return; }
	int i, j;
	// initialze a map from type to dparam index, -1 means no FOR_NETCONS statement
	int* t2i = new int[n_memb_func];
	for (i=0; i < n_memb_func; ++i) { t2i[i] = -1; }
	// create ForNetConsInfo in all the relevant point processes
	// and fill in the t2i map.
	for (i = 0; i < nrn_fornetcon_cnt_; ++i) {
		int index = nrn_fornetcon_index_[i];
		int type = nrn_fornetcon_type_[i];
		t2i[type] = index;
		Memb_list* m = memb_list + type;
		for (j = 0; j < m->nodecount; ++j) {
			void** v = &(m->pdata[j][index]._pvoid);
			_nrn_free_fornetcon(v);
			ForNetConsInfo* fnc = new ForNetConsInfo;
			*v = fnc;
			fnc->argslist = 0;
			fnc->size = 0;
		}
	}
	// two loops over all netcons. one to count, one to fill in argslist
	// count
	hoc_Item* q;
	ITERATE(q, psl_) {
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
		Memb_list* m = memb_list + nrn_fornetcon_type_[i];
		for (j = 0; j < m->nodecount; ++j) {
			ForNetConsInfo* fnc = (ForNetConsInfo*)m->pdata[j][index]._pvoid;
			if (fnc->size > 0) {
				fnc->argslist = new double*[fnc->size];
				fnc->size = 0;
			}
		}
	}
	// fill in argslist and count again
	ITERATE(q, psl_) {
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

extern "C" {
void record_init_clear(const TQItem* q, int) {
	DiscreteEvent* d = (DiscreteEvent*)q->data_;
	d->frecord_init((TQItem*)q);
}
};

void NetCvode::record_init() {
	int i, cnt = prl_->count();
	if (cnt) {
		// there may be some events on the queue descended from
		// finitialize that need to be removed
		record_init_items_->remove_all();
		tqe_->forall_callback(record_init_clear);
		int j, jcnt = record_init_items_->count();
		for (j=0; j < jcnt; ++j) {
			tqe_->remove(record_init_items_->item(j));
		}
		record_init_items_->remove_all();
	}
	for (i=0; i < cnt; ++i) {
		prl_->item(i)->record_init();
	}
}

void NetCvode::play_init() {
	int i, cnt = prl_->count();
	for (i=0; i < cnt; ++i) {
		prl_->item(i)->play_init();
	}
}

int NetCvode::cellindex() {
	Section* sec = chk_access();
	int i;
	if (single_) {
		return 0;
	}else{
		for (i=0; i < nlist_; ++i) {
			if (sec == list_[i].v_node_[list_[i].rootnodecount_]->sec) {
				return i;
			}
		}
	}
	hoc_execerror(secname(sec), " is not the root section for any local step cvode instance");
	return 0;
}

void NetCvode::states() {
	int i, j, n;
	Vect* v = vector_arg(1);
	if (!cvode_active_){
		v->resize(0);
		return;
	}
	double* vp;
	n = 0;
	for (i=0; i < nlist_; ++i) {
		n += list_[i].neq_;
	}
	v->resize(n);
	vp = vector_vec(v);
	j = 0;
	for (i=0; i < nlist_; ++i) {
		list_[i].states(vp+j);
		j += list_[i].neq_;
	}
}

void NetCvode::dstates() {
	int i, j, n;
	Vect* v = vector_arg(1);
	if (!cvode_active_){
		v->resize(0);
		return;
	}
	double* vp;
	n = 0;
	for (i=0; i < nlist_; ++i) {
		n += list_[i].neq_;
	}
	v->resize(n);
	vp = vector_vec(v);
	j = 0;
	for (i=0; i < nlist_; ++i) {
		list_[i].dstates(vp+j);
		j += list_[i].neq_;
	}
}

void NetCvode::error_weights() {
	int i, j, n;
	Vect* v = vector_arg(1);
	if (!cvode_active_){
		v->resize(0);
		return;
	}
	double* vp;
	n = 0;
	for (i=0; i < nlist_; ++i) {
		n += list_[i].neq_;
	}
	v->resize(n);
	vp = vector_vec(v);
	j = 0;
	for (i=0; i < nlist_; ++i) {
		list_[i].error_weights(vp+j);
		j += list_[i].neq_;
	}
}

void NetCvode::acor() {
	int i, j, n;
	Vect* v = vector_arg(1);
	if (!cvode_active_){
		v->resize(0);
		return;
	}
	double* vp;
	n = 0;
	for (i=0; i < nlist_; ++i) {
		n += list_[i].neq_;
	}
	v->resize(n);
	vp = vector_vec(v);
	j = 0;
	for (i=0; i < nlist_; ++i) {
		list_[i].acor(vp+j);
		j += list_[i].neq_;
	}
}

const char* NetCvode::statename(int is, int style) {
	int i, j, n, neq;
	if (!cvode_active_){
		hoc_execerror("Cvode is not active", 0);
	}
	double** pv;
	n = 0;
	for (i=0; i < nlist_; ++i) {
		n += list_[i].neq_;
	}
	if (is >= n) {
		hoc_execerror("Cvode::statename argument out of range", 0);
	}
	if (!hdp_ || hdp_->style() != style) {
		if (hdp_) {
			delete hdp_;
		}			
		hdp_ = new HocDataPaths(2*n, style);
		for (i=0; i < nlist_; ++i) {
			neq = list_[i].neq_;
			pv = list_[i].pv_;
			for (j=0; j < neq; ++j) {
				hdp_->append(pv[j]);
			}
		}
		hdp_->search();
	}
	j = 0;
	for (i=0; i < nlist_; ++i) {
		if (j + list_[i].neq_ > is) {
		    if (style == 2) {
			Symbol* sym = hdp_->retrieve_sym(list_[i].pv_[is - j]);
			assert(sym);
			return sym2name(sym);
		    }else{
			String* s = hdp_->retrieve(list_[i].pv_[is - j]);
			if (s) {
				return s->string();
			}else{
				return "unknown";
			}
		    }
		}
		j += list_[i].neq_;
	}
	return "unknown";
}

const char* NetCvode::sym2name(Symbol* sym) {
	if (sym->type == RANGEVAR && sym->u.rng.type > 1
	    && memb_func[sym->u.rng.type].is_point) {
		static char buf[200];
		sprintf(buf, "%s.%s", memb_func[sym->u.rng.type].sym->name, sym->name);
		return buf;
	}else{
		return sym->name;
	}
}

Symbol* NetCvode::name2sym(const char* name) {
	char* buf = new char[strlen(name)+1];
	strcpy(buf, name);
	char* cp;
	for (cp = buf; *cp; ++cp) {
		if (*cp == '.') {
			*cp = '\0';
			++cp;
			break;
		}
	}
	Symbol* sym = hoc_table_lookup(buf, hoc_built_in_symlist);
	if (!sym) {
		sym = hoc_table_lookup(buf, hoc_top_level_symlist);
	}
	if (sym && *cp == '\0' && (sym->type == RANGEVAR || strcmp(sym->name, "Vector") == 0)) {
		delete [] buf;
		return sym;
	}else if (sym && sym->type == TEMPLATE && *cp != '\0') {
		sym = hoc_table_lookup(cp, sym->u.ctemplate->symtable);
		if (sym) {
			delete [] buf;
			return sym;
		}
	}
	delete [] buf;
	hoc_execerror(name, "must be in form rangevar or Template.var");
	return nil;
}

void NetCvode::rtol(double x) {
	rtol_ = x;
}
void NetCvode::atol(double x) {
	atol_ = x;
}
void NetCvode::stiff(int x) {
	if ((stiff_ == 0) != (x == 0)) { // need to free if change between 0 and nonzero
		for (int i=0; i < nlist_; ++i) {
			list_[i].free_cvodemem();
		}
	}
	stiff_ = x;
}
void NetCvode::maxorder(int x) {
	maxorder_ = x;
	for (int i=0; i < nlist_; ++i) {
		list_[i].maxorder(maxorder_);
	}
}
int NetCvode::order(int i) {
	return list_[i].order();
}
void NetCvode::minstep(double x) {
	minstep_ = x;
	for (int i=0; i < nlist_; ++i) {
		list_[i].minstep(minstep_);
	}
}
void NetCvode::maxstep(double x) {
	maxstep_ = x;
	for (int i=0; i < nlist_; ++i) {
		list_[i].maxstep(maxstep_);
	}
}
void NetCvode::jacobian(int x) {
	jacobian_ = x;
}
void NetCvode::structure_change() {
	for (int i=0; i < nlist_; ++i) {
		list_[i].structure_change_ = true;
	}
}

NetCon* NetCvode::install_deliver(double* dsrc, Section* ssrc, Object* osrc,
	Object* target,	double threshold, double delay, double magnitude
    ) {
	PreSyn* ps = nil;
	double* psrc = nil;
	int i;
	if (ssrc) { consist_sec_pd("NetCon", ssrc, dsrc); }
	if (!pst_) {
		pst_ = new PreSynTable(1000);
		pst_cnt_ = 0;
	}
	if (!psl_) {
		psl_ = hoc_l_newlist();
		if (!psl_th_) {
			psl_th_ = hoc_l_newlist();
		}
	}
	if (osrc) {
		if (dsrc) {
			psrc = dsrc;
		}else{
			char buf[256];
			if (hoc_table_lookup("x", osrc->ctemplate->symtable)) {
				sprintf(buf, "%s.x", hoc_object_name(osrc));
				psrc = hoc_val_pointer(buf);
			}
		}
	}else{
		psrc = dsrc;
	}
	if (psrc) {
		if (!pst_->find(ps, psrc)) {
			ps = new PreSyn(psrc, osrc, ssrc);
			ps->hi_ = hoc_l_insertvoid(psl_, ps);
			ps->hi_th_ = hoc_l_insertvoid(psl_th_, ps);
			pst_->insert(psrc, ps);
			++pst_cnt_;
		}
		if (threshold != -1e9) { ps->threshold_ = threshold; }
	}else if (osrc){
		Point_process* pnt = ob2pntproc(osrc);
		if (pnt->presyn_) {
			ps = (PreSyn*)pnt->presyn_;
		}else{
			ps = new PreSyn(psrc, osrc, ssrc);
			if (threshold != -1e9) { ps->threshold_ = threshold; }
			ps->hi_ = hoc_l_insertvoid(psl_, ps);
			pnt->presyn_ = ps;
		}
	}else if (target) { // no source so use the special presyn
		if (!unused_presyn) {
			unused_presyn = new PreSyn(nil, nil, nil);
			unused_presyn->hi_ = hoc_l_insertvoid(psl_, unused_presyn);
		}
		ps = unused_presyn;
	}
	NetCon* d = new NetCon(ps, target);
	d->delay_ = delay;
	d->weight_[0] = magnitude;
	for (i=1;i<d->cnt_;i++) { d->weight_[i]=0.; }
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
		unused_presyn = nil;
	}
	hoc_l_delete(ps->hi_);
	if (ps->hi_th_) {
		hoc_l_delete(ps->hi_th_);
	}
	if (ps->thvar_) {
		--pst_cnt_;
		pst_->remove(ps->thvar_);
	}
	for (int i=0; i < nlist_; ++i) {
		PreSynList* psl = list_[i].psl_th_;
		if (psl) for (int j = 0; j < psl->count(); ++j) {
			if (psl->item(j) == ps) {
				psl->remove(j);
				return;
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
	printf("null_event_ onto queue\n");
	nc->null_event(tt);
}

DiscreteEvent* DiscreteEvent::savestate_read(FILE* f) {
	return new DiscreteEvent();
}

void DiscreteEvent::savestate_write(FILE* f) {
	fprintf(f, "%d\n", DiscreteEventType);
}

NetCon::NetCon() {
	cnt_ = 0; obj_ = nil; active_ = false; weight_ = nil;
	NetConSave::invalid();
}

NetCon::NetCon(PreSyn* src, Object* target) {
	NetConSave::invalid();
	obj_ = nil;
	src_ = src;
	if (src_) {
		src_->dil_.append((NetCon*)this);
		src_->use_min_delay_ = 0;
	}
	if (target == nil) {
		target_ = nil;
		active_ = false;
		cnt_ = 1;
		weight_ = new double[cnt_];
		return;
	}
	target_ = ob2pntproc(target);
	active_ = true;
#if DISCRETE_EVENT_OBSERVER
	ObjObservable::Attach(target, this);
#endif
	if (!pnt_receive[target_->prop->type]) {
hoc_execerror("No NET_RECEIVE in target PointProcess:", hoc_object_name(target));
	}
	cnt_ = pnt_receive_size[target_->prop->type];
	weight_ = nil;
	if (cnt_) {
		weight_ = new double[cnt_];
	}
}

NetCon::~NetCon() {
//printf("~NetCon\n");
	NetConSave::invalid();
	if (src_) {
		for (int i=0; i < src_->dil_.count(); ++i) {
			if (src_->dil_.item(i) == this) {
				src_->dil_.remove(i);
				if (src_->dil_.count() == 0 && src_->tvec_ == nil
				    && src_->idvec_ == nil) {
#if 1 || NRNMPI
	if (src_->output_index_ == -1)
#endif
					delete src_;
				}
				break;
			}
		}
	}
	if (cnt_) {
		delete [] weight_;
	}
#if DISCRETE_EVENT_OBSERVER
	if (target_) {
		ObjObservable::Detach(target_->ob, this);
	}
#endif
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
	nc->event(tt, netcon_);
}

DiscreteEvent* NetCon::savestate_read(FILE* f) {
	int index;
	char buf[200];
//	fscanf(f, "%d\n", &index);
	fgets(buf, 200, f);
	sscanf(buf, "%d\n", &index);
	NetCon* nc = NetConSave::index2netcon(index);
	assert(nc);
	return new NetConSave(nc);
}

void NetConSave::savestate_write(FILE* f) {
	fprintf(f, "%d\n", NetConType);
	fprintf(f, "%d\n", netcon_->obj_->index);
}

declareTable(NetConSaveWeightTable, void*, NetCon*)
implementTable(NetConSaveWeightTable, void*, NetCon*)
NetConSaveWeightTable* NetConSave::wtable_;

declareTable(NetConSaveIndexTable, long, NetCon*)
implementTable(NetConSaveIndexTable, long, NetCon*)
NetConSaveIndexTable* NetConSave::idxtable_;

void NetConSave::invalid() {
	if (wtable_) {
		delete wtable_;
		wtable_ = nil;
	}
	if (idxtable_) {
		delete idxtable_;
		idxtable_ = nil;
	}
}

NetCon* NetConSave::weight2netcon(double* pd) {
	NetCon* nc;
	if (!wtable_) {
		hoc_Item* q;
		Symbol* sym = hoc_lookup("NetCon");
		wtable_ = new NetConSaveWeightTable(2*sym->u.ctemplate->count);
		ITERATE (q, sym->u.ctemplate->olist) {
			Object* obj = OBJ(q);
			nc = (NetCon*)obj->u.this_pointer;
			if (nc->weight_) {
				wtable_->insert(nc->weight_, nc);
			}
		}
	}
	if (wtable_->find(nc, pd)) {
		assert(nc->weight_ == pd);
		return nc;
	}else{
		return nil;
	}
}

NetCon* NetConSave::index2netcon(long id) {
	NetCon* nc;
	if (!idxtable_) {
		hoc_Item* q;
		Symbol* sym = hoc_lookup("NetCon");
		idxtable_ = new NetConSaveIndexTable(2*sym->u.ctemplate->count);
		ITERATE (q, sym->u.ctemplate->olist) {
			Object* obj = OBJ(q);
			nc = (NetCon*)obj->u.this_pointer;
			if (nc->weight_) {
				idxtable_->insert(obj->index, nc);
			}
		}
	}
	if (idxtable_->find(nc, id)) {
		assert(nc->obj_->index == id);
		return nc;
	}else{
		return nil;
	}
}

PreSyn::PreSyn(double* src, Object* osrc, Section* ssrc) {
	PreSynSave::invalid();
	hi_index_ = -1;
	hi_th_ = nil;
	flag_ = false;
	valthresh_ = 0;
	thvar_ = src;
	osrc_ = osrc;
	ssrc_ = ssrc;
	threshold_ = 10.;
	use_min_delay_ = 0;
	tvec_ = nil;
	idvec_ = nil;
	stmt_ = nil;
	gid_ = -1;
#if 1 || USENCS || NRNMPI
	output_index_ = -1;
#endif
#if BGPDMA
	bgp.dma_send_ = 0;
#endif
#if DISCRETE_EVENT_OBSERVER
#if HAVE_IV
	Oc oc;
	if(thvar_) {
		oc.notify_when_freed(thvar_, this);
	}else if (osrc_) {
		oc.notify_when_freed(osrc_, this);
	}
#endif
#endif
}

PreSyn::~PreSyn() {
	PreSynSave::invalid();
//	printf("~PreSyn\n");
#if BGPDMA
	bgpdma_cleanup_presyn(this);
#endif
	if (stmt_) {
		delete stmt_;
	}
#if DISCRETE_EVENT_OBSERVER
	if (tvec_) {
		ObjObservable::Detach(tvec_->obj_, this);
		tvec_ = nil;
	}
	if (idvec_) {
		ObjObservable::Detach(idvec_->obj_, this);
		idvec_ = nil;
	}
#endif
	if (thvar_ || osrc_) {
#if DISCRETE_EVENT_OBSERVER
#if HAVE_IV
		Oc oc;
		oc.notify_pointer_disconnect(this);
#endif
#endif
		if (!thvar_) {
			Point_process* pnt = ob2pntproc(osrc_);
			if (pnt) {
				pnt->presyn_ = nil;
			}
		}
	}
	update(nil);
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
	nc->event(tt, presyn_);
}

DiscreteEvent* PreSyn::savestate_read(FILE* f) {
	PreSyn* ps = nil;
	char buf[200];
	int index;
	fgets(buf, 200, f);
	assert(sscanf(buf, "%d\n", &index));
	ps = PreSynSave::hindx2presyn(index);
	assert(ps);
	return new PreSynSave(ps);
}

void PreSynSave::savestate_write(FILE* f) {
	fprintf(f, "%d\n", PreSynType);
	fprintf(f, "%d\n", presyn_->hi_index_);
}

declareTable(PreSynSaveIndexTable, long, PreSyn*)
implementTable(PreSynSaveIndexTable, long, PreSyn*)
PreSynSaveIndexTable* PreSynSave::idxtable_;

void PreSynSave::invalid() {
	if (idxtable_) {
		delete idxtable_;
		idxtable_ = nil;
	}
}

PreSyn* PreSynSave::hindx2presyn(long id) {
	PreSyn* ps;
	if (!idxtable_) {
		hoc_Item* q;
		int cnt = 0;
		ITERATE (q, net_cvode_instance->psl_) {
			++cnt;
		}
//printf("%d PreSyn instances\n", cnt);
		idxtable_ = new PreSynSaveIndexTable(2*cnt);
		cnt = 0;
		ITERATE (q, net_cvode_instance->psl_) {
			ps = (PreSyn*)VOIDITM(q);
			assert(ps->hi_index_ == cnt);
			idxtable_->insert(ps->hi_index_, ps);
			++cnt;
		}
	}
	if (idxtable_->find(ps, id)) {
		assert(ps->hi_index_ == id);
		return ps;
	}else{
		return nil;
	}
}

void PreSyn::init() {
	qthresh_ = nil;
	if (tvec_) {
		tvec_->resize(0);
	}
	if (idvec_) {
		idvec_->resize(0);
	}
}

void PreSyn::record_stmt(const char* stmt) {
	if (stmt_) {
		delete stmt_;
		stmt_ = nil;
	}
	if (strlen(stmt) > 0) {
		stmt_ = new HocCommand(stmt);
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
	}
#endif
}

void PreSyn::record(double tt) {
	int i;
	if (tvec_) {
		i = tvec_->capacity();
		tvec_->resize_chunk(i+1);
		tvec_->elem(i) = tt;
	}
	if (idvec_) {
		i = idvec_->capacity();
		idvec_->resize_chunk(i+1);
		idvec_->elem(i) = rec_id_;
	}
	if (stmt_) {
		t = tt;
#if carbon
		stmt_->execute((unsigned int)0);
#else
		stmt_->execute(false);
#endif
	}
}

void PreSyn::disconnect(Observable* o) {
	if (tvec_ && tvec_->obj_ == ((ObjObservable*)o)->object()) {
		tvec_ = nil;
	}
	if (idvec_ && idvec_->obj_ == ((ObjObservable*)o)->object()) {
		idvec_ = nil;
	}
	if (dil_.count() == 0) {
		assert(tvec_ == nil && idvec_ == nil);
		delete this;
	}
}

void PreSyn::update(Observable* o) { // should be disconnect
//printf("PreSyn::disconnect\n");
	for (int i = 0; i < dil_.count(); ++i) {
#if 0 // osrc_ below is invalid
if (dil_.item(i)->obj_) {
	printf("%s disconnect from ", hoc_object_name(dil_.item(i)->obj_));
	printf("source %s\n", osrc_ ? hoc_object_name(osrc_) : secname(ssrc_));
}	
#endif
		dil_.item(i)->src_ = nil;
	}
	if (tvec_) {
#if DISCRETE_EVENT_OBSERVER
		ObjObservable::Detach(tvec_->obj_, this);
#endif
		tvec_ = nil;
	}
	if (idvec_) {
#if DISCRETE_EVENT_OBSERVER
		ObjObservable::Detach(idvec_->obj_, this);
#endif
		idvec_ = nil;
	}
	net_cvode_instance->presyn_disconnect(this);
	thvar_ = nil;	
	osrc_ = nil;
}

void PreSyn::update_ptr(double* pd) {
#if HAVE_IV
#if DISCRETE_EVENT_OBSERVER
	Oc oc;
	oc.notify_pointer_disconnect(this);
	oc.notify_when_freed(pd, this);
#endif
#endif
	thvar_ = pd;
}

void ConditionEvent::check(double teps) {
	if (value() > 0.0) {
		if (flag_ == false) {
			flag_ = true;
			valthresh_ = 0.;
#if USENEOSIM
		    if (neosim_entity_) {
			(*p_nrn2neosim_send)(neosim_entity_, t);
		    }else{
#endif
			send(t + teps, net_cvode_instance);
#if USENEOSIM
		    }
#endif
		}
	}else{
		flag_ = false;
	}
}

ConditionEvent::ConditionEvent() {}
ConditionEvent::~ConditionEvent() {}

void ConditionEvent::condition(Cvode* cv) { //logic for high order threshold detection
//printf("ConditionEvent::condition f=%d t=%20.15g v=%20.15g\n", flag_, t, value());
	if (qthresh_) { // the threshold event has not
		// been handled. i.e. the cell must have retreated to
		// a time not later  than the threshold time.
		assert (t <= qthresh_->t_);
		abandon_statistics(cv);
		// abandon the event
		STATISTICS(abandon_);
		net_cvode_instance->remove_event(qthresh_);
		qthresh_ = nil;
		valthresh_ = 0.;
		flag_ = false;
	}

	double val = value();
	if (flag_ == false && val >= 0.0) { // above threshold
		flag_ = true;
		valthresh_ = 0.;
		if (cv->t0_ == cv->tn_) { //inited
			// means immediate threshold event now.
			// no need for qthresh since there is
			// no question of abandoning it so instead
			// of a qthresh it is a send event.
			STATISTICS(init_above_);
			send(t, net_cvode_instance);
		}else{ // crossed somewhere in the told to t interval
			STATISTICS(send_qthresh_);
			// reset the flag_ when the value goes lower than
			// valold since value() when qthresh_ handled
			// may in fact be below 0.0
			valthresh_ = valold_;
			double th = -valold_/(val - valold_);
			th = th*t + (1. - th)*told_;
			assert(th >= cv->t0_ && th <= cv->tn_);
			qthresh_ = net_cvode_instance->event(th, this);
		}
	}else if (flag_ == true && valold_ < valthresh_ && val < valthresh_) {
		// below threshold
		// previous step crossed in negative direction
		// and there was not any retreat or initialization
		// to give spurious crossing.
		flag_ = false;
	}
	valold_ = val;
	told_ = t;
}

void ConditionEvent::abandon_statistics(Cvode* cv) {
#if 1
//printf("ConditionEvent::condition %s t=%20.15g abandon event at %20.15g\n", ssrc_?secname(ssrc_):"", t, qthresh_->t_);
	if (t == qthresh_->t_) {// it is not clear whether
		// this could happen and if it does it may
		// take fastidiousness to
		// an extreme
		STATISTICS(eq_abandon_);
		printf("abandon when t == qthresh_->t_ = %20.15g\n", t);
	}
	if (cv->t0_ == cv->tn_) { // inited
		if (value() > 0.0) { // above threshold
			STATISTICS(abandon_init_above_);
		}else{
			STATISTICS(abandon_init_below_);
		}
	}else{
		if (value() > 0.0) { // above threshold
			STATISTICS(abandon_above_);
		}else{
			STATISTICS(abandon_below_);
		}
	}
#endif
}

WatchCondition::WatchCondition(Point_process* pnt, double(*c)(Point_process*))
 : HTList(nil)
{
	pnt_ = pnt;
	c_ = c;
}

WatchCondition::~WatchCondition() {
printf("~WatchCondition\n");
	Remove();
}

void WatchCondition::activate(double flag) {
	qthresh_ = nil;
	flag_ = (value() > 0.) ? true: false;
	valthresh_ = 0.;
	nrflag_ = flag;
	Cvode* cv = (Cvode*)pnt_->nvi_;
	assert(cv);
	if (!cv->watch_list_) {
		cv->watch_list_ = new HTList(nil);
		net_cvode_instance->wl_list_->append(cv->watch_list_);
	}
	Remove();
	cv->watch_list_->Append(this);
}

void WatchCondition::asf_err() {
fprintf(stderr, "WATCH condition with flag=%g for %s\n",
	nrflag_, hoc_object_name(pnt_->ob));
}

void PreSyn::asf_err() {
fprintf(stderr, "PreSyn threshold for %s\n", osrc_ ? hoc_object_name(osrc_):secname(ssrc_));
}

void WatchCondition::send(double tt, NetCvode* nc) {
	qthresh_ = nc->event(tt, this);
	STATISTICS(watch_send_);
}

void WatchCondition::deliver(double tt, NetCvode* nc) {
	if (qthresh_) {
		qthresh_ = nil;
		STATISTICS(deliver_qthresh_);
	}
	Cvode* cv = (Cvode*)pnt_->nvi_;
	int type = pnt_->prop->type;
	if (cvode_active_ && cv) {
		nc->retreat(tt, cv);
		cv->set_init_flag();
	}else{
		t = tt;
	}
	STATISTICS(watch_deliver_);
	(*pnt_receive[type])(pnt_, nil, nrflag_);
	if (errno) {
		if (nrn_errno_check(type)) {
hoc_warning("errno set during WatchCondition deliver to NET_RECEIVE", (char*)0);
		}
	}
}

void WatchCondition::pgvts_deliver(double tt, NetCvode* nc) {
	if (qthresh_) {
		qthresh_ = nil;
		STATISTICS(deliver_qthresh_);
	}
	int type = pnt_->prop->type;
	STATISTICS(watch_deliver_);
	(*pnt_receive[type])(pnt_, nil, nrflag_);
	if (errno) {
		if (nrn_errno_check(type)) {
hoc_warning("errno set during WatchCondition deliver to NET_RECEIVE", (char*)0);
		}
	}
}

void WatchCondition::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s", s);
	printf(" WatchCondition %s %.15g flag=%g\n", hoc_object_name(pnt_->ob), tt, nrflag_);
}

void Cvode::ste_check() {
	int i;
	if (ste_list_) {
		boolean b = true;
		int cnt = ste_list_->count();
		double tstart = t_;
		while (b) { // until no more ste transitions
			StateTransitionEvent* ste;
			int itrans, itsav, isav;
			double tr = t_;
			double trsav = tr+1.;
			b = false;
			// earliest ste transition gets done first
			for (i=0; i < cnt; ++i) {
				ste = ste_list_->item(i);
				itrans = ste->condition(tr);
				if (itrans != -1 && tr < trsav) {
					b = true;
					isav = i;
					itsav = itrans;
					trsav = tr;
				}			
			}
			if (b) {
				ste = ste_list_->item(isav);
				if (trsav < tstart) {
					// second order implemented only for recording.
					interpolate(trsav);
					ste->execute(itsav);
					interpolate(tstart);
				}else{
					ste->execute(itsav);
				}
			}
		}
	}
}

void NetCvode::ste_check() { // for fixed step
	int i;
	STEList* stel = StateTransitionEvent::stelist_;
	if (stel) {
		boolean b = true;
		int cnt = stel->count();
		double tstart = t;
		while (b) { // until no more ste transitions
			StateTransitionEvent* ste;
			int itrans, itsav, isav;
			double tr = t;
			double trsav = tr+1.;
			b = false;
			// earliest ste transition gets done first
			for (i=0; i < cnt; ++i) {
				ste = stel->item(i);
				itrans = ste->condition(tr);
				if (itrans != -1 && tr < trsav) {
					b = true;
					isav = i;
					itsav = itrans;
					trsav = tr;
				}			
			}
			if (b) {
				ste = stel->item(isav);
				ste->execute(itsav);
			}
		}
	}
}

void Cvode::evaluate_conditions() {
	int i;
	net_cvode_instance->ste_check();
	if (psl_th_) {
		for (i = psl_th_->count()-1; i >= 0; --i) {
			psl_th_->item(i)->condition( this);
		}
	}
	if (watch_list_) {
		for (HTList* item = watch_list_->First(); item != watch_list_->End(); item = item->Next()) {
			((WatchCondition*)item)->condition(this);
		}
	}
}

void Cvode::check_deliver() {
	int i;
	net_cvode_instance->ste_check();
	if (psl_th_) {
		for (i = psl_th_->count()-1; i >= 0; --i) {
			psl_th_->item(i)->check();
		}
	}
	if (watch_list_) {
		for (HTList* item = watch_list_->First(); item != watch_list_->End(); item = item->Next()) {
			((WatchCondition*)item)->check();
		}
	}
}

void NetCvode::fixed_record_continuous() {
	int i, cnt;
	nrn_ba(BEFORE_STEP);
	cnt = fixed_record_->count();
	for (i=0; i < cnt; ++i) {
		fixed_record_->item(i)->continuous(t);
	}
}

void NetCvode::fixed_play_continuous() {
	int i, cnt;
	cnt = fixed_play_->count();
	for (i=0; i < cnt; ++i) {
		fixed_play_->item(i)->continuous(t);
	}
}

void NetCvode::deliver_net_events() { // for default method
	int i;
	TQItem* q;
	double tm, tt, tsav;
	tsav = t;
	tm = t + dt/2.;

	ste_check();

	if (psl_th_) { /* only look at ones with a threshold */
		hoc_Item* q1;
		ITERATE(q1, psl_th_) {
			PreSyn* ps = (PreSyn*)VOIDITM(q1);
			if (ps->thvar_) {
				ps->check(1e-10);
			}
		}
	}
	for (i=0; i < wl_list_->count(); ++i) {
		HTList* wl = wl_list_->item(i);
		for (HTList* item = wl->First(); item != wl->End(); item = item->Next()) {
			((WatchCondition*)item)->check();
		}
	}
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
		while ((q = tqe_->dequeue_bin()) != 0) {
			DiscreteEvent* db = (DiscreteEvent*)q->data_;
#if PRINT_EVENT
if (print_event_) {db->pr("binq deliver", t, this);}
#endif
			tqe_->release(q);
			db->deliver(t, this);
		}
//		assert(int(tm/dt)%1000 == tqe_->nshift_);
	}
#endif
	while ((tt = tqe_->least_t()) <= tm) {
		t = tt;
		q = tqe_->least();
		DiscreteEvent* db = (DiscreteEvent*)q->data_;
#if PRINT_EVENT
if (print_event_) {db->pr("deliver", tt, this);}
#endif
		tqe_->remove(q);
		db->deliver(tt, this);

	}
#if BBTQ == 5
	if (nrn_use_bin_queue_) {
		if (tqe_->top()) { goto tryagain; }
		tqe_->shift_bin(tm);
	}
#endif
	t = tsav;
}

implementPtrList(PlayRecList,PlayRecord)

void NetCvode::playrec_add(PlayRecord* pr) { // called by PlayRecord constructor
//printf("NetCvode::playrec_add %lx\n", (long)pr);
	playrec_change_cnt_ = 0;
	prl_->append(pr);
}

void NetCvode::playrec_remove(PlayRecord* pr) { // called by PlayRecord destructor
//printf("NetCvode::playrec_remove %lx\n", (long)pr);
	playrec_change_cnt_ = 0;
	int i, cnt = prl_->count();
	for (i=0; i < cnt; ++i) {
		if (prl_->item(i) == pr) {
			prl_->remove(i);
			break;
		}
	}
	cnt = fixed_play_->count();
	for (i=0; i < cnt; ++i) {
		if (fixed_play_->item(i) == pr) {
			fixed_play_->remove(i);
			break;
		}
	}
	cnt = fixed_record_->count();
	for (i=0; i < cnt; ++i) {
		if (fixed_record_->item(i) == pr) {
			fixed_record_->remove(i);
			break;
		}
	}
}

int NetCvode::playrec_item(PlayRecord* pr) {
	int i, cnt = prl_->count();
	for (i=0; i < cnt; ++i) {
		if (prl_->item(i) == pr) {
			return i;
		}
	}
	return -1;
}

PlayRecord* NetCvode::playrec_item(int i) {
	assert(i < prl_->count());
	return prl_->item(i);
}

PlayRecord* NetCvode::playrec_uses(void* v) {
	int i, cnt = prl_->count();
	for (i=0; i < cnt; ++i) {
		if (prl_->item(i)->uses(v)) {
			return prl_->item(i);
		}
	}
	return nil;
}

PlayRecord::PlayRecord(double* pd) {
//printf("PlayRecord::PlayRecord %lx\n", (long)this);
	pd_ = pd;
	cvode_ = nil;
#if HAVE_IV
	Oc oc;
	if (pd_) {
		oc.notify_when_freed(pd_, this);
	}
#endif
	net_cvode_instance->playrec_add(this);
}

PlayRecord::~PlayRecord() {
//printf("PlayRecord::~PlayRecord %lx\n", (long)this);
#if HAVE_IV
	Oc oc;
	oc.notify_pointer_disconnect(this);
#endif
	net_cvode_instance->playrec_remove(this);
}

void PlayRecord::update_ptr(double* pd) {
#if HAVE_IV
	Oc oc;
	oc.notify_pointer_disconnect(this);
	oc.notify_when_freed(pd, this);
#endif
	pd_ = pd;
}

void PlayRecord::disconnect(Observable*) {
//printf("PlayRecord::disconnect %ls\n", (long)this);
	delete this;
}

void PlayRecord::record_add(Cvode* cv) {
	cvode_ = cv;
	if (cv) {
		cv->record_add(this);
	}
	net_cvode_instance->fixed_record_->append(this);
}

void PlayRecord::play_add(Cvode* cv) {
	cvode_ = cv;
	if (cv) {
		cv->play_add(this);
	}
	net_cvode_instance->fixed_play_->append(this);
}

void PlayRecord::pr() {
	printf("PlayRecord\n");
}

TvecRecord::TvecRecord(Section* sec, IvocVect* t) : PlayRecord(&NODEV(sec->pnode[0])) {
//printf("TvecRecord\n");
	t_ = t;
	ObjObservable::Attach(t_->obj_, this);
}

TvecRecord::~TvecRecord() {
//printf("~TvecRecord\n");
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
	int j = t_->capacity();
	t_->resize_chunk(j + 1);
	t_->elem(j) = tt;
}

YvecRecord::YvecRecord(double* pd, IvocVect* y) : PlayRecord(pd) {
//printf("YvecRecord\n");
	y_ = y;
	ObjObservable::Attach(y_->obj_, this);
}

YvecRecord::~YvecRecord() {
//printf("~YvecRecord\n");
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
	y_->resize(0);
}

void YvecRecord::continuous(double tt) {
	int j = y_->capacity();
	y_->resize_chunk(j + 1);
	y_->elem(j) = *pd_;
}

VecRecordDiscrete::VecRecordDiscrete(double* pd, IvocVect* y, IvocVect* t) : PlayRecord(pd) {
//printf("VecRecordDiscrete\n");
	y_ = y;
	t_ = t;
	ObjObservable::Attach(y_->obj_, this);
	ObjObservable::Attach(t_->obj_, this);
	e_ = new PlayRecordEvent();
	e_->plr_ = this;
}

VecRecordDiscrete::~VecRecordDiscrete() {
//printf("~VecRecordDiscrete\n");
	ObjObservable::Detach(y_->obj_, this);
	ObjObservable::Detach(t_->obj_, this);
	delete e_;
}

PlayRecordSave* VecRecordDiscrete::savestate_save() {
	return new VecRecordDiscreteSave(this);
}

VecRecordDiscreteSave::VecRecordDiscreteSave(PlayRecord* prl) : PlayRecordSave(prl) {
	cursize_ = ((VecRecordDiscrete*)pr_)->y_->capacity();
}
VecRecordDiscreteSave::~VecRecordDiscreteSave() {
}
void VecRecordDiscreteSave::savestate_restore() {
	check();
	VecRecordDiscrete* vrd = (VecRecordDiscrete*)pr_;
	vrd->y_->resize(cursize_);
	assert(cursize_ <= vrd->t_->capacity());
}
void VecRecordDiscreteSave::savestate_write(FILE* f) {
	fprintf(f, "%d\n", cursize_);
}
void VecRecordDiscreteSave::savestate_read(FILE* f) {
	char buf[100];
	fgets(buf, 100, f);
	assert(sscanf(buf, "%d\n", &cursize_) == 1);
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
	if (t_->capacity() > 0) {
		e_->send(t_->elem(0), net_cvode_instance);
	}
}

void VecRecordDiscrete::frecord_init(TQItem* q) {
	record_init_items_->append(q);
}

void VecRecordDiscrete::deliver(double tt, NetCvode* nc) {
	int j = y_->capacity();
	y_->resize_chunk(j + 1);
	y_->elem(j) = *pd_;
	assert(Math::equal(t_->elem(j), tt, 1e-8));
	if (j+1 < t_->capacity()) {
		e_->send(t_->elem(j+1), nc);
	}
}

VecRecordDt::VecRecordDt(double* pd, IvocVect* y, double dt) : PlayRecord(pd) {
//printf("VecRecordDt\n");
	y_ = y;
	dt_ = dt;
	ObjObservable::Attach(y_->obj_, this);
	e_ = new PlayRecordEvent();
	e_->plr_ = this;
}

VecRecordDt::~VecRecordDt() {
//printf("~VecRecordDt\n");
	ObjObservable::Detach(y_->obj_, this);
	delete e_;
}

PlayRecordSave* VecRecordDt::savestate_save() {
	return new VecRecordDtSave(this);
}

VecRecordDtSave::VecRecordDtSave(PlayRecord* prl) : PlayRecordSave(prl) {
}
VecRecordDtSave::~VecRecordDtSave() {
}
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
	e_->send(0., net_cvode_instance);
}

void VecRecordDt::frecord_init(TQItem* q) {
	record_init_items_->append(q);
}

void VecRecordDt::deliver(double tt, NetCvode* nc) {
	int j = y_->capacity();
	y_->resize_chunk(j + 1);
	y_->elem(j) = *pd_;
	e_->send(tt + dt_, nc);
}

void NetCvode::vecrecord_add() {
	double* pd = hoc_pgetarg(1);
	consist_sec_pd("Cvode.record", chk_access(), pd);
	IvocVect* y = vector_arg(2);
	IvocVect* t = vector_arg(3);
	PlayRecord* pr = playrec_uses(y);
	if (pr) {
		delete pr;
	}
	pr = playrec_uses(t);
	if (pr) {
		delete pr;
	}
	boolean discrete = ( (ifarg(4) && (int)chkarg(4,0,1) == 1) ? true : false);
	if (discrete) {
		pr = new VecRecordDiscrete(pd, y, t);
	}else{
		pr = new TvecRecord(chk_access(), t);
		pr = new YvecRecord(pd, y);
	}
}

void NetCvode::vec_remove() {
	IvocVect* iv = vector_arg(1);
	PlayRecord* pr;
	while((pr = playrec_uses(vector_arg(1))) != nil) {
		delete pr;
	}
}

void NetCvode::playrec_setup() {
	long i, iprl, prlc;
	double* px;
	prlc = prl_->count();
	for (i = 0; i < nlist_; ++i) {
		Cvode& cv = list_[i];
		cv.delete_prl();
		fixed_record_->remove_all();
		fixed_play_->remove_all();
	}
	for (iprl = 0; iprl < prlc; ++iprl) {
		PlayRecord* pr = prl_->item(iprl);
		boolean b = false;
		if (single_) {
			pr->install(list_);
			b = true;
		}else for (i=0; i < nlist_; ++i) {
			if (list_[i].is_owner(pr->pd_)) {
				pr->install(list_ + i);
				b = true;
				break;
			}
		}
		if (b == false) {
hoc_execerror("A PlayRecord item cannot be associated with a RANGE variable", nil);
		}
	}
	playrec_change_cnt_ = structure_change_cnt_;
}

void StateTransitionEvent::stelist_change() {
	net_cvode_instance->stelist_change();
}

void NetCvode::stelist_change() {
	if (localstep()) {
		structure_change_cnt_ = 0;
	} else if (list_ && list_->ste_list_ == nil) {
		structure_change_cnt_ = 0;
	}
}

boolean Cvode::is_owner(double* pd) { // is a pointer to range variable in this cell
	int in;
	for (in=0; in < v_node_count_; ++in) {
		Node* nd = v_node_[in];
		if (&NODEV(nd) == pd) {
			return true;
		}
		Prop* p;
		for (p = nd->prop; p; p = p->next) {
			if (pd >= p->param && pd < (p->param + p->param_size)) {
				return true;
			}
		}
		if (nd->extnode) {
			if (pd >= nd->extnode->v && pd < (nd->extnode->v + nlayer)) {
				return true;
			}
		}
		// will need to check the line mechanisms when there is a cvode
		// specific list of them and IDA is allowed for local step method.
	}
	return false;
}

void NetCvode::consist_sec_pd(const char* msg, Section* sec, double* pd) {
	int in;
	Node* nd;
	for (in=-1; in < sec->nnode; ++in) {
		if (in == -1) {
			nd = sec->parentnode; // in case &v(0)
			if (!nd) { continue; }
		}else{
			nd = sec->pnode[in];
		}
		if (&NODEV(nd) == pd) {
			return;
		}
		Prop* p;
		for (p = nd->prop; p; p = p->next) {
			if (pd >= p->param && pd < (p->param + p->param_size)) {
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
	hoc_execerror(msg, " pointer not associated with currently accessed section\n\
Use section ... (&var(x)...) intead of ...(&section.var(x)...)\n");
}

double NetCvode::state_magnitudes() {
	if (hoc_is_double_arg(1)) {
		int on = (int)chkarg(1, 0, 2);
		int i;
		if (on == 2) {
			maxstate_analyse();
		}else{
			for (i=0; i < nlist_; ++i) {
				list_[i].activate_maxstate(on?true:false);
			}
		}
		return 0.;
	}else if (hoc_is_str_arg(1)) {
		Symbol* sym = name2sym(gargstr(1));
		double dummy;
		double* pamax = &dummy;
		if (ifarg(2)) {
			pamax = hoc_pgetarg(2);
		}
		return maxstate_analyse(sym, pamax);
	}else{
		int i, j, n;
		Vect* v = vector_arg(1);
		if (!cvode_active_){
			v->resize(0);
			return 0.;
		}
		double* vp;
		double* ms;
		n = 0;
		for (i=0; i < nlist_; ++i) {
			n += list_[i].neq_;
		}
		v->resize(n);
		vp = vector_vec(v);
		int getacor = 0;
		if (ifarg(2)) {
			getacor = (int)chkarg(2, 0, 1);
		}
		j = 0;
		if (list_ && list_[0].maxstate_) {
			for (i=0; i < nlist_; ++i) {
				if (getacor) {
					list_[i].maxacor(vp+j);
				}else{
					list_[i].maxstate(vp+j);
				}
				j += list_[i].neq_;
			}
		}
		return 0.;
	}
}

void NetCvode::maxstate_analyse() {
	int i, j, n;
	MaxStateItem* msi;
	Symbol* sym;
	double* ms;
	double* ma;
	if (!mst_) {
		int n = 0;
		for (sym = hoc_built_in_symlist->first; sym; sym = sym->next) {
			++n;
		}
		mst_ = new MaxStateTable(3*n);
	}
	{for (TableIterator(MaxStateTable) ti(*mst_); ti.more(); ti.next()) {
		msi = ti.cur_value();
		msi->max_ = -1e9;
		msi->amax_ = -1e9;
	}}
	if (!list_ || list_[0].neq_ == 0) {
		return;
	}
	statename(0,2);
	j = 0;
	for (i=0; i < nlist_; ++i) {
		n = list_[i].neq_;
		ms = list_[i].maxstate_;
		ma = list_[i].maxacor_;
		for (j=0; j < n; ++j) {
			sym = hdp_->retrieve_sym(list_[i].pv_[j]);
			if (!mst_->find(msi, (void*)sym)) {
				msi = new MaxStateItem();
				msi->sym_ = sym;
				msi->max_ = -1e9;
				msi->amax_ = -1e9;
				mst_->insert((void*)sym, msi);
			}
			if (msi->max_ < ms[j]) { msi->max_ = ms[j];}
			if (msi->amax_ < ma[j]) { msi->amax_ = ma[j];}
		}
	}
}

double NetCvode::maxstate_analyse(Symbol* sym, double* pamax) {
	MaxStateItem* msi;
	if (mst_ && mst_->find(msi, (void*)sym)) {
		*pamax = msi->amax_;
		return msi->max_;
	}
	*pamax = -1e9;
	return -1e9;
}

void NetCvode::recalc_ptrs(int nnode, double** oldvp, double* newv) {
#if CACHEVEC
	// update PlayRecord pointers to v
	int i, cnt = prl_->count();
	for (i=0; i < cnt; ++i) {
		PlayRecord* pr = prl_->item(i);
		if (pr->pd_ && nrn_isdouble(pr->pd_, 0, nnode-1)) {
			int k = (int)(*pr->pd_);
			if (oldvp[k] == pr->pd_) {
				pr->update_ptr(newv + k);
			}
		}
	}
	// update PreSyn pointers to v
	hoc_Item* q;
	if (psl_) ITERATE(q, psl_) {
		PreSyn* ps = (PreSyn*)VOIDITM(q);
		if (ps->thvar_ && nrn_isdouble(ps->thvar_, 0, nnode-1)) {
			int k = (int)(*ps->thvar_);
			if (oldvp[k] == ps->thvar_) {
				pst_->remove(ps->thvar_);
				pst_->insert(newv + k, ps);
				ps->update_ptr(newv + k);
			}
		}
	}
#endif
}
