#include <../../nrnconf.h>
#include <string.h>
#include <stdlib.h>
#include <OS/list.h>
#include <math.h>
#include "nrnoc2iv.h"
#include "classreg.h"
#include "kschan.h"
#include "kssingle.h"
#include "parse.h"
#include "nrniv_mf.h"

#define NSingleIndex 0
#if defined(__MWERKS__) && !defined(_MSC_VER)
#include <extras.h>
#define strdup _strdup
#endif

declarePtrList(KSChanList, KSChan)
implementPtrList(KSChanList, KSChan)

static KSChanList* channels;

extern "C" {
extern char* hoc_symbol_units(Symbol*, const char*);
extern void nrn_mk_table_check();
}

static Symbol* ksstate_sym;
static Symbol* ksgate_sym;
static Symbol* kstrans_sym;

#define nt_dt nrn_threads->_dt

static void check_objtype(Object* o, Symbol* s) {
	if (o->ctemplate->sym != s) {
		char buf[200];
		sprintf(buf, "%s is not a %s", o->ctemplate->sym->name, s->name);
		hoc_execerror(buf,0);
	}
	if (!o->u.this_pointer) {
		hoc_execerror(hoc_object_name(o), " was deleted by KSChan");
	}
}

static void unref(Object* obj) {
	if (obj) {
		obj->u.this_pointer = 0;
		hoc_obj_unref(obj);
	}
}

static void chkobj(void* v) {
	if (!v) {
		hoc_execerror("This object was deleted by KSChan", 0);
	}
}

static void check_table_thread_(double* p, Datum* ppvar, Datum* thread, void* vnt, int type) {
	KSChan* c = channels->item(type);
	c->check_table_thread((NrnThread*)vnt);
}

static void nrn_alloc(Prop* prop) {
	KSChan* c = channels->item(prop->type);
	c->alloc(prop);
}

static void nrn_init(NrnThread* nt, Memb_list* ml, int type) {
//printf("nrn_init\n");
	KSChan* c = channels->item(type);
	c->init(ml->nodecount, ml->nodelist, ml->data, ml->pdata, nt);
}

static void nrn_cur(NrnThread* nt, Memb_list* ml, int type) {
//printf("nrn_cur\n");
	KSChan* c = channels->item(type);
#if CACHEVEC
	if (use_cachevec) {
		c->cur(ml->nodecount, ml->nodeindices, ml->data, ml->pdata, nt);
	}else
#endif /* CACHEVEC */
	{
		c->cur(ml->nodecount, ml->nodelist, ml->data, ml->pdata);
	}
}

static void nrn_jacob(NrnThread* nt, Memb_list* ml, int type) {
//printf("nrn_jacob\n");
	KSChan* c = channels->item(type);
#if CACHEVEC
	if (use_cachevec) {
		c->jacob(ml->nodecount, ml->nodeindices, ml->data, ml->pdata, nt);
	}else
#endif /* CACHEVEC */
	{
		c->jacob(ml->nodecount, ml->nodelist, ml->data, ml->pdata);
	}
}

static void nrn_state(NrnThread* nt, Memb_list* ml, int type) {
//printf("nrn_state\n");
	KSChan* c = channels->item(type);
#if CACHEVEC
	if (use_cachevec) {
		c->state(ml->nodecount, ml->nodeindices, ml->nodelist, ml->data,
				ml->pdata, nt);
	}else
#endif /* CACHEVEC */
	{
		c->state(ml->nodecount, ml->nodelist, ml->data, ml->pdata, nt);
	}
}

static int ode_count(int type){
//printf("ode_count\n");
	KSChan* c = channels->item(type);
	return c->count();
}
static void ode_map(int ieq, double** pv, double** pvdot,
    double* p, Datum* pd, double* atol, int type){
//printf("ode_map\n");
	KSChan* c = channels->item(type);
	c->map(ieq, pv, pvdot, p, pd, atol);
}
static void ode_spec(NrnThread*, Memb_list* ml, int type){
//printf("ode_spec\n");
	KSChan* c = channels->item(type);
	c->spec(ml->nodecount, ml->nodelist, ml->data, ml->pdata);
}
static void ode_matsol(NrnThread* nt, Memb_list* ml, int type){
//printf("ode_matsol\n");
	KSChan* c = channels->item(type);
	c->matsol(ml->nodecount, ml->nodelist, ml->data, ml->pdata, nt);
}
static void singchan(NrnThread* nt, Memb_list* ml, int type){
//printf("singchan_\n");
	KSChan* c = channels->item(type);
	c->cv_sc_update(ml->nodecount, ml->nodelist, ml->data, ml->pdata, nt);
}
static void* hoc_create_pnt(Object* ho) {
	return create_point_process(ho->ctemplate->is_point_, ho);
}
static void hoc_destroy_pnt(void* v) {
	// first free the KSSingleNodeData if it exists.
	Point_process* pp = (Point_process*)v;
	if (pp->prop) {
		KSChan* c = channels->item(pp->prop->type);
		c->destroy_pnt(pp);
	}
}

void KSChan::destroy_pnt(Point_process* pp) {
	if (single_ && pp->prop->dparam[2]._pvoid) {
//printf("deleteing KSSingleNodeData\n");
		KSSingleNodeData* snd = (KSSingleNodeData*) pp->prop->dparam[2]._pvoid;
		delete snd;
		pp->prop->dparam[2]._pvoid = NULL;
	}
	destroy_point_process(pp);
}
static double hoc_loc_pnt(void* v) {
	Point_process* pp = (Point_process*)v;
	return loc_point_process(pp->ob->ctemplate->is_point_, pp);
}
static double hoc_has_loc(void* v) {
	return has_loc_point(v);
}
static double hoc_get_loc_pnt(void* v) {
	return get_loc_point_process(v);
}
static double hoc_nsingle(void* v) {
	Point_process* pp = (Point_process*)v;
	KSChan* c = channels->item(pp->prop->type);
	if (ifarg(1)) {
		c->nsingle(pp, (int)chkarg(1, 1, 1e9));
	}
	return (double) c->nsingle(pp);
}
static Member_func member_func[] = {
        "loc", hoc_loc_pnt,
        "has_loc", hoc_has_loc,
        "get_loc", hoc_get_loc_pnt,
	"nsingle", hoc_nsingle,
	0, 0
};

void kschan_cvode_single_update() {
}

// hoc interface

static double ks_setstructure(void* v) {
	KSChan* ks = (KSChan*)v;
	ks->setstructure(vector_arg(1));
	return 1;
}

static double ks_remove_state(void* v) {
	KSChan* ks = (KSChan*)v;
	int is;
	if (hoc_is_double_arg(1)) {
		is = (int)chkarg(1, 0, ks->nstate_ - 1);
	}else{
		Object* obj = *hoc_objgetarg(1);
		check_objtype(obj, ksstate_sym);
		KSState* kss = (KSState*)obj->u.this_pointer;
		is = kss->index_;
	}
	ks->remove_state(is);
	return 0.;
}

static double ks_remove_transition(void* v) {
	KSChan* ks = (KSChan*)v;
	int it;
	if (hoc_is_double_arg(1)) {
		it = (int)chkarg(1, ks->ivkstrans_, ks->ntrans_ - 1);
	}else{
		Object* obj = *hoc_objgetarg(1);
		check_objtype(obj, kstrans_sym);
		KSTransition* kst = (KSTransition*)obj->u.this_pointer;
		it = kst->index_;
		assert(it >= ks->ivkstrans_ && it < ks->ntrans_);
	}
	ks->remove_transition(it);
	return 0.;
}

static double ks_ngate(void* v) {
	KSChan* ks = (KSChan*)v;
	return (double)ks->ngate_;
}

static double ks_nstate(void* v) {
	KSChan* ks = (KSChan*)v;
	return (double)ks->nstate_;
}

static double ks_ntrans(void* v) {
	KSChan* ks = (KSChan*)v;
	return (double)ks->ntrans_;
}

static double ks_nligand(void* v) {
	KSChan* ks = (KSChan*)v;
	return (double)ks->nligand_;
}

static double ks_is_point(void* v) {
	KSChan* ks = (KSChan*)v;
	return (ks->is_point() ? 1. : 0.);
}

static double ks_single(void* v) {
	KSChan* ks = (KSChan*)v;
	if (ifarg(1)) {
		ks->set_single(((int)chkarg(1, 0, 1)) != 0);
	}
	return (ks->is_single() ? 1. : 0.);
}

static double ks_iv_type(void* v) {
	KSChan* ks = (KSChan*)v;
	if (ifarg(1)) {
		ks->cond_model_ = (int)chkarg(1, 0, 2);
		ks->setcond();
	}
	if (ks->ion_sym_) {
		return (double)ks->cond_model_;
	}
	return 0.;
}

static double ks_gmax(void* v) {
	KSChan* ks = (KSChan*)v;
	if (ifarg(1)) {
		ks->gmax_deflt_ = chkarg(1, 0., 1e9);
	}
	return ks->gmax_deflt_;
}

static double ks_erev(void* v) {
	KSChan* ks = (KSChan*)v;
	if (ifarg(1)) {
		ks->erev_deflt_ = chkarg(1, -1e9, 1e9);
	}
	return ks->erev_deflt_;
}

static double ks_vres(void* v) {
	KSChan* ks = (KSChan*)v;
	
	if (ifarg(1)) {
		KSSingle::vres_ = chkarg(1, 1e-9, 1e9);
	}
	return KSSingle::vres_;
}

static double ks_rseed(void* v) {
	KSChan* ks = (KSChan*)v;
	
	if (ifarg(1)) {
		KSSingle::idum_ = (unsigned int)chkarg(1, 0, 1e9);
	}
	return (double)KSSingle::idum_;
}

static double ks_usetable(void* v) {
	KSChan* ks = (KSChan*)v;
	if (ifarg(1)) {
		if (hoc_is_pdouble_arg(1)) {
			int n;
			n = ks->usetable(hoc_pgetarg(1), hoc_pgetarg(2));
			return double(n);
		}else{
			bool use = ((int)chkarg(1, 0, 1)) ? true : false;
			if (ifarg(2)) {
				ks->usetable(use, (int)chkarg(2, 2, 10000),
					*getarg(3), *getarg(4));
			}else{
				ks->usetable(use);
			}
		}
	}
	return ks->usetable() ? 1. : 0.;
}

static Object** temp_objvar(const char* name, void* v, Object** obp) {
	Object** po;
	if (*obp) {
		po = hoc_temp_objptr(*obp);
	}else{
		po = hoc_temp_objvar(hoc_lookup(name), v);
		*obp = *po;
		hoc_obj_ref(*po);
	}
	return po;
}

static Object** ks_add_hhstate(void* v) {
	KSChan* ks = (KSChan*)v;
	KSState* kss = ks->add_hhstate(gargstr(1));
	return temp_objvar("KSState", kss, &kss->obj_);
}

static Object** ks_add_ksstate(void* v) {
	KSChan* ks = (KSChan*)v;
	Object* obj = *hoc_objgetarg(1);
	int ig = ks->ngate_;
	if (obj) {
		check_objtype(obj, ksgate_sym);
		KSGateComplex* ksg = (KSGateComplex*)obj->u.this_pointer;
		assert(ksg && ksg->index_ < ks->ngate_);
		ig = ksg->index_;
	}
	KSState* kss = ks->add_ksstate(ig, gargstr(2));
	return temp_objvar("KSState", kss, &kss->obj_);
}

static Object** ks_add_transition(void* v) {
	KSChan* ks = (KSChan*)v;
	const char* lig = NULL;
	if (ifarg(3)) {
		lig = gargstr(3);
	}
	int src, target;
	if (hoc_is_double_arg(1)) {
		src = (int)chkarg(1, ks->nhhstate_, ks->nstate_-1);
		target = (int)chkarg(2, ks->nhhstate_, ks->nstate_-1);
	}else{
		Object* obj = *hoc_objgetarg(1);
		check_objtype(obj, ksstate_sym);
		src = ((KSState*)obj->u.this_pointer)->index_;
		obj = *hoc_objgetarg(2);
		check_objtype(obj, ksstate_sym);
		target = ((KSState*)obj->u.this_pointer)->index_;
	}
	KSTransition* kst = ks->add_transition(src, target, lig);
	return temp_objvar("KSTrans", kst, &kst->obj_);
}

static Object** ks_trans(void* v) {
	KSChan* ks = (KSChan*)v;
	KSTransition* kst;
	if (hoc_is_double_arg(1)) {
		kst = ks->trans_ + (int)chkarg(1, 0, ks->ntrans_ - 1);
	}else{
		int src, target;
		Object* obj = *hoc_objgetarg(1);
		check_objtype(obj, ksstate_sym);
		src = ((KSState*)obj->u.this_pointer)->index_;
		obj = *hoc_objgetarg(2);
		check_objtype(obj, ksstate_sym);
		target = ((KSState*)obj->u.this_pointer)->index_;
		kst = ks->trans_ + ks->trans_index(src, target);
	}
	return temp_objvar("KSTrans", kst, &kst->obj_);
}

static Object** ks_state(void* v) {
	KSChan* ks = (KSChan*)v;
	KSState* kss = ks->state_ + (int)chkarg(1, 0, ks->nstate_ - 1);
	return temp_objvar("KSState", kss, &kss->obj_);
}

static Object** ks_gate(void* v) {
	KSChan* ks = (KSChan*)v;
	KSGateComplex* ksg = ks->gc_ + (int)chkarg(1, 0, ks->ngate_ - 1);
	return temp_objvar("KSGate", ksg, &ksg->obj_);
}

static const char** ks_name(void* v) {
	KSChan* ks = (KSChan*)v;
	if (ifarg(1)) {
		ks->setname(gargstr(1));
	}
	char** ps = hoc_temp_charptr();
	*ps = (char*)ks->name_.string();
	return (const char**)ps;
}

static const char** ks_ion(void* v) {
	KSChan* ks = (KSChan*)v;
	if (ifarg(1)) {
		ks->setion(gargstr(1));
	}
	char** ps = hoc_temp_charptr();
	*ps = (char*)ks->ion_.string();
	return (const char**)ps;
}

static const char** ks_ligand(void* v) {
	KSChan* ks = (KSChan*)v;
	char** ps = hoc_temp_charptr();
	*ps = (char*)ks->ligands_[(int)chkarg(1, 0, ks->nligand_ - 1)]->name;
	return (const char**)ps;
}

static double kss_frac(void* v) {
	chkobj(v);
	KSState* kss = (KSState*)v;
	if (ifarg(1)) {
		kss->f_ = chkarg(1, 0., 1e9);
	}
	return kss->f_;
}

static double kss_index(void* v) {
	chkobj(v);
	KSState* kss = (KSState*)v;
	return kss->index_;
}

static Object** kss_gate(void* v) {
	chkobj(v);
	KSState* kss = (KSState*)v;
	KSChan* ks = kss->ks_;
	int ig = ks->gate_index(kss->index_);
	KSGateComplex* ksg = ks->gc_ + ig;
	return temp_objvar("KSGate", ksg, &ksg->obj_);
}

static const char** kss_name(void* v) {
	chkobj(v);
	KSState* kss = (KSState*)v;
	if (ifarg(1)) {
		kss->ks_->setsname(kss->index_, gargstr(1));
	}
	char** ps = hoc_temp_charptr();
	*ps = (char*)kss->string();
	return (const char**)ps;
}

static double ksg_nstate(void* v) {
	chkobj(v);
	KSGateComplex* ksg = (KSGateComplex*)v;
	return (double)ksg->nstate_;
}

static double ksg_power(void* v) {
	chkobj(v);
	KSGateComplex* ksg = (KSGateComplex*)v;
	if (ifarg(1)) {
		// could affect validity of single channel style
		ksg->ks_->power(ksg, (int)chkarg(1, 0, 1e6));
	}
	return (double)ksg->power_;
}

static double  ksg_sindex(void* v) {
	chkobj(v);
	KSGateComplex* ksg = (KSGateComplex*)v;
	return (double)ksg->sindex_;
}

static double  ksg_index(void* v) {
	chkobj(v);
	KSGateComplex* ksg = (KSGateComplex*)v;
	return (double)ksg->index_;
}

static double kst_set_f(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	int i = (int)chkarg(1, 0, 1);
	int type = (int)chkarg(2, 0, 7);
	Vect* vec = vector_arg(3);
	double vmin = -100;
	double vmax = 50;
	if (type == 7) { // table, optional vmin, vmax
		if (ifarg(4)) {
			vmin = *getarg(4);
			vmax = *getarg(5);
		}
	}
	kst->setf(i, type, vec, vmin, vmax);
	return 0;
}

static double kst_index(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	return (double)kst->index_;
}

static double kst_type(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	if (ifarg(1)) {
		int type = (int)chkarg(1, 0, 3);
		char* s = NULL;
		if (type >= 2) {
			s = gargstr(2);
		}
		Object* o = kst->obj_;
		kst->ks_->settype(kst, type, s); // kst may change
		kst = (KSTransition*)o->u.this_pointer;
	}
	return (double)kst->type_;
}

static double kst_ftype(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	KSChanFunction* f;
	if ((int)chkarg(1, 0, 1) == 0) {
		f = kst->f0;
	}else{
		f = kst->f1;
	}
	if (f) {
		return (double)f->type();
	}
	return -1.;
}

static double kst_ab(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	Vect* x = vector_arg(1);
	Vect* a = vector_arg(2);
	Vect* b = vector_arg(3);
	kst->ab(x, a, b);
	return 0;
}

static double kst_inftau(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	Vect* x = vector_arg(1);
	Vect* a = vector_arg(2);
	Vect* b = vector_arg(3);
	kst->inftau(x, a, b);
	return 0;
}

static double kst_f(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	int i = (int)chkarg(1, 0, 1);
	KSChanFunction* f = (i ? kst->f1 : kst->f0);
	if (!f) { return 0.; }
	double x = *getarg(2);
	return f->f(x);
}

static Object** kst_src(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	KSState* kss = kst->ks_->state_ + kst->src_;
	return temp_objvar("KSState", kss, &kss->obj_);
}

static Object** kst_target(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	KSState* kss = kst->ks_->state_ + kst->target_;
	return temp_objvar("KSState", kss, &kss->obj_);
}

static Object** kst_parm(void* v) {
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	KSChanFunction* f;
	if ((int)chkarg(1, 0, 1) == 0) {
		f = kst->f0;
	}else{
		f = kst->f1;
	}
	Vect* vec = NULL;
	if (f) {
		vec = f->gp_;
		if (f->type() == 7) {
			if (ifarg(2)) {
				double* px;
				px = hoc_pgetarg(2);
				*px = ((KSChanTable*)f)->vmin_;
				px = hoc_pgetarg(3);
				*px = ((KSChanTable*)f)->vmax_;
			}
		}
	}
	return vector_temp_objvar(vec);
};

static const char** kst_ligand(void* v) {
	static char s[20];;
	s[0] = '\0';
	chkobj(v);
	KSTransition* kst = (KSTransition*)v;
	if (kst->type_ >= 2) {
		strncpy(s, kst->ks_->ligands_[kst->ligand_index_]->name, 20);
		s[strlen(s) - 4] = (kst->type_==3) ? 'i' : 'o';
		s[strlen(s) - 3] = '\0';
	}	
	char** ps = hoc_temp_charptr();
	*ps = s;
	return (const char**)ps;
}

static double kst_stoichiometry(void* v) {
	KSTransition* kst = (KSTransition*)v;
	if (ifarg(1)) {
		kst->stoichiom_ = (int)chkarg(1, 1, 1e9);
	}
	return double(kst->stoichiom_);
}

static double ks_pr(void* v) {
	KSChan* ks = (KSChan*)v;
	KSTransition* kt;
	Symbol* s;

	int i, j;
	Printf("%s type properties\n", hoc_object_name(ks->obj_));
Printf("name=%s is_point_=%s ion_=%s cond_model_=%d\n", ks->name_.string(), (ks->is_point() ? "true" : "false"), ks->ion_.string(), ks->cond_model_);
Printf("  ngate=%d nstate=%d nhhstate=%d nligand=%d ntrans=%d ivkstrans=%d iligtrans=%d\n",
ks->ngate_, ks->nstate_, ks->nhhstate_, ks->nligand_, ks->ntrans_,
ks->ivkstrans_, ks->iligtrans_);
	Printf("  default gmax=%g erev=%g\n", ks->gmax_deflt_, ks->erev_deflt_);
	for (i=0; i < ks->ngate_; ++i) {
Printf("    gate %d index=%d nstate=%d power=%d\n",
i, ks->gc_[i].sindex_, ks->gc_[i].nstate_, ks->gc_[i].power_);
	}
	for (i=0; i < ks->nligand_; ++i) {
		Printf("    ligand %d %s\n", i, ks->ligands_[i]->name);
	}
	for (i=0; i < ks->iligtrans_; ++i) {
		kt = ks->trans_ + i;
Printf("    trans %d src=%d target=%d type=%d\n", i, kt->src_, kt->target_, kt->type_);
Printf("        f0 type=%d   f1 type=%d\n", kt->f0?kt->f0->type():-1, kt->f1?kt->f1->type():-1);
	}
	for (i=ks->iligtrans_; i < ks->ntrans_; ++i) {
		kt = ks->trans_ + i;
Printf("    trans %d src=%d target=%d type=%d ligindex=%d\n", i, kt->src_, kt->target_, kt->type_, kt->ligand_index_);
Printf("        f0 type=%d   f1 type=%d\n", kt->f0?kt->f0->type():-1, kt->f1?kt->f1->type():-1);
	}
	Printf("    state names and fractional conductance\n");
	for (i=0; i < ks->nstate_; ++i) {
		Printf("    %d %s %g\n", i, ks->state_[i].string(), ks->state_[i].f_);
	}
	return 1;
}

static Member_func ks_dmem[] = {
	// keeping c++ consistent with java
	"setstructure", ks_setstructure,

	"remove_state", ks_remove_state,
	"remove_transition", ks_remove_transition,

	"ngate", ks_ngate,
	"nstate", ks_nstate,
	"ntrans", ks_ntrans,
	"nligand", ks_nligand,
	"is_point", ks_is_point,
	"single", ks_single,
	"pr", ks_pr,

	"iv_type", ks_iv_type,
	"gmax", ks_gmax,
	"erev", ks_erev,
	"vres", ks_vres,
	"rseed", ks_rseed,
	"usetable", ks_usetable,
	0, 0
};

static Member_ret_obj_func ks_omem[] = {
	"add_hhstate", ks_add_hhstate,
	"add_ksstate", ks_add_ksstate,
	"add_transition", ks_add_transition,
	"trans", ks_trans,
	"state", ks_state,
	"gate", ks_gate,
	0,0
};

static Member_ret_str_func ks_smem[] = {
	"name", ks_name,
	"ion", ks_ion,
	"ligand", ks_ligand,
	0, 0
};

static Member_func kss_dmem[] = {
	"frac", kss_frac,
	"index", kss_index,
	0,0
};

static Member_ret_obj_func kss_omem[] = {
	"gate", kss_gate,
	0,0
};

static Member_ret_str_func kss_smem[] = {
	"name", kss_name,
	0,0
};

static Member_func ksg_dmem[] = {
	"nstate", ksg_nstate,
	"power", ksg_power,
	"sindex", ksg_sindex,
	"index", ksg_index,
	0,0
};

static Member_ret_obj_func ksg_omem[] = {
	0,0
};

static Member_ret_str_func ksg_smem[] = {
	0,0
};

static Member_func kst_dmem[] = {
	"set_f", kst_set_f,
	"index", kst_index,
	"type", kst_type,
	"ftype", kst_ftype,
	"ab", kst_ab,
	"inftau", kst_inftau,
	"f", kst_f,
	"stoichiometry", kst_stoichiometry,
	0,0
};

static Member_ret_obj_func kst_omem[] = {
	"src", kst_src,
	"target", kst_target,
	"parm", kst_parm,
	0,0
};

static Member_ret_str_func kst_smem[] = {
	"ligand", kst_ligand,
	0,0
};

static void* ks_cons(Object* o) {
/*
	hoc_obj_ref(o); // so never destroyed
	char* suffix = gargstr(1);
	check(suffix);
	char* ion = gargstr(2);
	Object* t1 = *hoc_objgetarg(4);
	Object* t2 = *hoc_objgetarg(7);
	check_obj_type(t1, "VGateTransRate");
	check_obj_type(t2, "VGateTransRate");
	hoc_obj_ref(t1); // never destroyed
	hoc_obj_ref(t2); // never destroyed
*/
	bool isp = false;
	if (ifarg(1)) {
		isp = ((int)chkarg(1, 0, 1)) != 0;
	}
	KSChan* c = new KSChan(o, isp);
	return c;
}
static void ks_destruct(void*) {
	assert(0);
}

// construction of KSState, KSGateComplex, and KSTransition are 
// handled by KSChan in order for it to maintain its slightly more
// computationally efficient lists
static void* kss_cons(Object* o) {
	hoc_execerror("Cannot create a KSState except through KSChan", 0);
	return NULL;
}
static void kss_destruct(void*) {
}
static void* ksg_cons(Object* o) {
	hoc_execerror("Cannot create a KSGate except through KSChan", 0);
	return NULL;
}
static void ksg_destruct(void*) {
}
static void* kst_cons(Object* o) {
	hoc_execerror("Cannot create a KSTransition except through KSChan", 0);
	return NULL;
}
static void kst_destruct(void*) {
}

void KSChan_reg() {
	class2oc("KSChan", ks_cons, ks_destruct, ks_dmem, NULL, ks_omem, ks_smem);
	class2oc("KSGate", ksg_cons, ksg_destruct, ksg_dmem, NULL, ksg_omem, ksg_smem);
	class2oc("KSState", kss_cons, kss_destruct, kss_dmem, NULL, kss_omem, kss_smem);
	class2oc("KSTrans", kst_cons, kst_destruct, kst_dmem, NULL, kst_omem, kst_smem);
	ksstate_sym = hoc_lookup("KSState");
	ksgate_sym = hoc_lookup("KSGate");
	kstrans_sym = hoc_lookup("KSTrans");
	KSSingle::vres_ = 0.1;
	KSSingle::idum_ = 0;
}

// param is gmax, g, i --- if change then change numbers below
// state names are handled individually
static const char* m_kschan_pat[] = { "0", "kschan", "gmax", 0, "g", "i", 0, 0, 0 };
static const char* m_kschan[9];
// gmax=0 g=1 i=1 state names will be modltype 2, there are no pointer variables

void KSChan::add_channel(const char** m) {
	KSChan* c = (KSChan*)this;
	Symlist* sav = hoc_symlist;
	hoc_symlist = hoc_built_in_symlist;
	hoc_built_in_symlist = 0;
	if (is_point()) {
		pointtype_ = point_register_mech(m, nrn_alloc, nrn_cur, nrn_jacob, nrn_state, nrn_init, -1, 1,
			hoc_create_pnt, hoc_destroy_pnt, member_func);
	}else{
		register_mech(m, nrn_alloc, nrn_cur, nrn_jacob, nrn_state, nrn_init, -1, 1);
	}
	hoc_built_in_symlist = hoc_symlist;
	hoc_symlist = sav;
	mechtype_ = nrn_get_mechtype(m[1]);
//printf("mechanism type is %d\n", mechtype_);
	hoc_register_cvode(mechtype_, ode_count, ode_map, ode_spec, ode_matsol);
	if (!channels) {
		channels = new KSChanList(50);
	}
	while(channels->count() < mechtype_) {
		channels->append(NULL);
	}
	channels->append(c);
}

KSChan::KSChan(Object* obj, bool is_p) {
//printf("KSChan created\n");
	int i;
	nhhstate_ = 0;
	mechtype_ = -1;
	usetable(false, 0, 1., 0.);;
	is_point_ = is_p;
	is_single_ = false;
	single_ = NULL;
	ppoff_ = (is_point() ? (is_single() ? 3 : 2) : 0) ; // area, pnt, single
	gmaxoffset_ = (is_single() ? 1 : 0); // and Nsingle is the first
	obj_ = obj;
	hoc_obj_ref(obj_);
	gc_ = NULL; state_ = NULL; trans_ = NULL; iv_relation_ = NULL;
	state_size_ = gate_size_ = trans_size_ = 0;
	ngate_ = nligand_ = nstate_ = nksstate_ = 0;
	ntrans_ = iligtrans_ = ivkstrans_ = 0;
	cond_model_ = 0;
	ion_sym_ = NULL;
	ligands_ = NULL;
	mechsym_ = NULL;
	rlsym_ = NULL;
	char buf[50];
	sprintf(buf, "Chan%d", obj_->index);
	name_ = buf;
	ion_ = "NonSpecific";
	mat_ = NULL;
	elms_ = NULL;	
	diag_ = NULL;
	gmax_deflt_ = 0.;
	erev_deflt_ = 0.;
	soffset_ = 4; // gmax, e, g, i before the first state in p array
	build();
}

KSChan::~KSChan(){}

void KSChan::build() {
	if (mechsym_) { return; }
	int i;
	char buf[100];
	if (strcmp(ion_.string(), "NonSpecific") != 0) {
		ion_reg(ion_.string(), -10000.);
		sprintf(buf, "%s_ion", ion_.string());
		ion_sym_ = looksym(buf);
		if (!ion_sym_) {
			hoc_execerror(buf, " is not an ion mechanism");
		}
	}
	const char* suffix = name_.string();
	char unsuffix[100];
	if (is_point()) {
		unsuffix[0] = '\0';
	}else{
		sprintf(unsuffix, "_%s", name_.string());
	}
	if (looksym(suffix)) {
		hoc_execerror(suffix, "already exists");
	}
	nrn_assert((m_kschan[0] = strdup(m_kschan_pat[0])) != 0);
	nrn_assert((m_kschan[1] = strdup(suffix)) != 0);
	nrn_assert(snprintf(buf, 100,  "gmax%s", unsuffix) < 100);
	nrn_assert((m_kschan[2] = strdup(buf)) != 0);
	int aoff = 0;
	if (!ion_sym_) {
		nrn_assert(snprintf(buf, 100, "e%s", unsuffix) < 100);
		nrn_assert((m_kschan[3] = strdup(buf)) != 0);
		aoff = 1;
	}
	m_kschan[3+aoff] = 0;
	nrn_assert(snprintf(buf, 100, "g%s", unsuffix) < 100);
	nrn_assert((m_kschan[4+aoff] = strdup(buf)) != 0);
	nrn_assert(snprintf(buf, 100, "i%s", unsuffix) < 100);
	nrn_assert((m_kschan[5+aoff] = strdup(buf)) != 0);
	m_kschan[6+aoff] = 0;
	m_kschan[7+aoff] = 0;
	soffset_ = 3+aoff; // first state points here in p array
	add_channel(m_kschan);
	for (i=0; i < 9; ++i) if (m_kschan[i]) { free((void*)m_kschan[i]); }
	mechsym_ = looksym(suffix);
	if (is_point()) {
		rlsym_ = looksym(suffix, mechsym_);
	}else{
		rlsym_ = mechsym_;
	}
	setcond();
	sname_install();
//	printf("%s allowed in insert statement\n", name_.string());
}

void KSChan::setname(const char* s) {
//printf("KSChan::setname\n");
	int i;
	if (strcmp(s, name_.string()) == 0) { return; }
	name_ = s;
	if (mechsym_) {
		char old_suffix[100];
		i = 0;
		while (strcmp(mechsym_->name, name_.string()) != 0
		    &&	looksym(name_.string())){
			Printf("KSChan::setname %s already in use\n", name_.string());
			sprintf(old_suffix, "%s%d", s, i);
			name_ = old_suffix;
			++i;
// if want original name use if statement and something like this
//			name_ = mechsym_->name
//			return;
		}
		sprintf(old_suffix, "_%s", mechsym_->name);
		const char* suffix = name_.string();
		free(mechsym_->name);
		mechsym_->name = strdup(suffix);
		if (is_point()) {
			free(rlsym_->name);
			rlsym_->name = strdup(suffix);
		}
		
		Symbol* sp;
		if (!is_point()) for (i=0; i < rlsym_->s_varn; ++i) {
			sp = rlsym_->u.ppsym[i];
			char* cp = strstr(sp->name, old_suffix);
			if (cp) {
				int nbase = cp - sp->name;
				int n = nbase + strlen(suffix) + 2;
				char* s1 = (char*)hoc_Emalloc(n); hoc_malchk();
				strncpy(s1, sp->name, nbase);
				sprintf(s1 + nbase, "_%s", suffix);
//printf("KSChan::setname change %s to %s\n", sp->name, s1);
				free(sp->name);
				sp->name = s1;
			}
		}
//	printf("%s renamed to %s\n", old_suffix+1, name_.string());
	}
}

int KSChan::state(const char* s) {
	int i;
	for (i = 0; i < nstate_; ++i) {
		if (strcmp(state_[i].string(), s) == 0) {
			return i;
		}
	}
	return -1;
}

void KSChan::power(KSGateComplex* gc, int p) {
	if (is_single() && p != 1) {
		set_single(false);
	}
	gc->power_ = p;
}

void KSChan::set_single(bool b, bool update) {
	if (!is_point()) {
		b = false;
		return;
	}
	if (b && (ngate_ != 1 || gc_[0].power_ != 1 || nhhstate_ > 0  || nksstate_ < 2)) {
		b = false;
hoc_warning("KSChan single channel mode implemented only for single ks gating complex to first power", 0);
	}
	if (is_single()) {
		memb_func[mechtype_].singchan_ = NULL;
		delete_schan_node_data();
		delete single_;
		single_ = NULL;
	}
	is_single_ = b;
	if (update) {
		update_prop();
	}
	if (b) {
		single_ = new KSSingle(this);
		memb_func[mechtype_].singchan_ = singchan;
		alloc_schan_node_data();
	}
}

const char* KSChan::state(int i) {
	return state_[i].string();
}

int KSChan::trans_index(const char* s, const char* t) {
	int i;
	for (i = 0; i < ntrans_; ++i) {
		if (strcmp(state_[trans_[i].src_].string(), s) == 0
		  && strcmp(state_[trans_[i].target_].string(), t) == 0){
			return i;
		}
	}
	return -1;
}

int KSChan::trans_index(int s, int t) {
	int i;
	for (i = 0; i < ntrans_; ++i) {
		if (trans_[i].src_ == s && trans_[i].target_ == t){
			return i;
		}
	}
	return -1;
}

int KSChan::gate_index(int is) {
	int i;
	for (i=1; i < ngate_; ++i) {
		if (is < gc_[i].sindex_) {
			return i-1;
		}
	}
	return ngate_ - 1;
}

void KSChan::update_prop() {
	// prop.param is [Nsingle], gmax, [e], g, i, states
	// prop.dparam for density is [4ion], [4ligands]
	// prop.dparam for point is area, pnt, [singledata], [4ion], [4ligands]
	int i;
	Symbol* searchsym = (is_point() ? mechsym_ : NULL);
	
	// some old sym pointers
	Symbol* gmaxsym = rlsym_->u.ppsym[gmaxoffset_];
	Symbol* gsym = rlsym_->u.ppsym[soffset_ - 2];
	Symbol* isym = rlsym_->u.ppsym[soffset_ - 1];
	Symbol* esym = ion_sym_ ? NULL : rlsym_->u.ppsym[gmaxoffset_ + 1];
	int old_gmaxoffset = gmaxoffset_;
	int old_soffset = soffset_;
	int old_svarn = rlsym_->s_varn;
	
	// sizes and offsets
	psize_ = 3; // prop->param: gmax, g, i
	dsize_ = 0; // prop->dparam: empty
	ppoff_ = 0;
	soffset_ = 3;
	gmaxoffset_ = 0;
	if (is_single()) {
		psize_ += 1; // Nsingle exists
		dsize_ += 1; // KSSingleNodeData* exists
		gmaxoffset_ = 1;
		ppoff_ += 1;
		soffset_ += 1;
	}
	if (is_point()) {
		dsize_ += 2; // area, Point_process* exists
		ppoff_ += 2;
	}
	if (ion_sym_ == NULL ) {
		psize_ += 1; // e exists
		soffset_ += 1;
	}else{
		dsize_ += 4; // ion current
	}
	dsize_ += 4*nligand_;
	psize_ += nstate_;

	// range variable names associated with prop->param
	rlsym_->s_varn = psize_; // problem here
	Symbol** ppsym = newppsym(rlsym_->s_varn);
	if (is_point()) {
		Symbol* sym = looksym("Nsingle", searchsym);
		if (is_single()) { // Nsingle exists with offset 0
			if (!sym) {
				sym = installsym("Nsingle", RANGEVAR, searchsym);
			}
			ppsym[NSingleIndex] = sym;
			sym->subtype = nrnocCONST; // PARAMETER
			sym->u.rng.type = rlsym_->subtype;
			sym->u.rng.index = NSingleIndex;
		}else if (sym) { // eliminate if Nsingle exists
			freesym(sym, searchsym);
		}
	}
	ppsym[gmaxoffset_] = gmaxsym;
	gmaxsym->u.rng.index = gmaxoffset_;
	ppsym[soffset_ - 2] = gsym;
	gsym->u.rng.index = soffset_ - 2;
	ppsym[soffset_ - 1] = isym;
	isym->u.rng.index = soffset_ - 1;
	if (esym) {
		ppsym[gmaxoffset_ + 1] = esym;
		esym->u.rng.index = gmaxoffset_ + 1;
	}
	int j;
	for (j=soffset_, i=old_soffset; i < old_svarn; ++i, ++j) {
		ppsym[j] = rlsym_->u.ppsym[i];
		ppsym[j]->u.rng.index = j;
	}
	free(rlsym_->u.ppsym);
	rlsym_->u.ppsym = ppsym;
	setcond();
	state_consist(gmaxoffset_ - old_gmaxoffset);
	ion_consist();
}

void KSChan::setion(const char* s) {
//printf("KSChan::setion\n");
	int i;
	if (strcmp(ion_.string(), s) == 0) { return; }
	Symbol* searchsym = (is_point() ? mechsym_ : NULL);
	if (strlen(s) == 0) {
		ion_ = "NonSpecific";
	}else{
		ion_ = s;
	}
	char buf[100];
	int pdoff = ppoff_;
	int io = gmaxoffset_;
	if( strcmp(ion_.string(), "NonSpecific") == 0) { // non-specific
		if (ion_sym_) { // switch from useion to non-specific
printf("switch from useion to non-specific\n");
			rlsym_->s_varn += 1;
			Symbol** ppsym = newppsym(rlsym_->s_varn);
			for (i=0; i <= io; ++i) {
				ppsym[i] = rlsym_->u.ppsym[i];
			}
			ion_sym_ = NULL;
			if (is_point()) {
				sprintf(buf, "e");
			}else{
				sprintf(buf, "e_%s", rlsym_->name);
			}
			if (looksym(buf, searchsym)) {
				hoc_execerror(buf, "already exists");
			}
			ppsym[1 + io] = installsym(buf, RANGEVAR, searchsym);
			ppsym[1 + io]->subtype = 0;
			ppsym[1 + io]->u.rng.type = rlsym_->subtype;
			ppsym[1 + io]->cpublic = 1;
			ppsym[1 + io]->u.rng.index = 1 + io;
			for (i=2+io; i <  rlsym_->s_varn; ++i) {
				ppsym[i] = rlsym_->u.ppsym[i-1];
				ppsym[i]->u.rng.index += 1;
			}
			free(rlsym_->u.ppsym);
			rlsym_->u.ppsym = ppsym;
			soffset_ += 1;
			setcond();
			state_consist();
			ion_consist();
		}
	}else{ // want useion
		pdoff = 5 + ppoff_;
		sprintf(buf, "%s_ion", s);
		// is it an ion
		Symbol* sym = looksym(buf);
		if (!sym || sym->type != MECHANISM ||
memb_func[sym->subtype].alloc != memb_func[looksym("na_ion")->subtype].alloc) {
			Printf("%s is not an ion mechanism", sym->name);
		}		
		if (ion_sym_) { // there already is an ion
			if (strcmp(ion_sym_->name, buf) != 0) { //is it different
//				printf(" mechanism %s now uses %s instead of %s\n",
//					name_.string(), sym->name, ion_sym_->name);
				ion_sym_ = sym;
				state_consist();
				ion_consist();
			}
			// if same do nothing
		}else{ // switch from non-specific to useion
			Symbol* searchsym = (is_point() ? mechsym_ : NULL);
			ion_sym_ = sym;
			rlsym_->s_varn -= 1;
			Symbol** ppsym = newppsym(rlsym_->s_varn);
			for (i=0; i <= io; ++i) {
				ppsym[i] = rlsym_->u.ppsym[i];
			}
			freesym(rlsym_->u.ppsym[1 + io], searchsym);
			for (i=1 + io; i <  rlsym_->s_varn; ++i) {
				ppsym[i] = rlsym_->u.ppsym[i+1];
				ppsym[i]->u.rng.index -= 1;
			}
			free(rlsym_->u.ppsym);
			rlsym_->u.ppsym = ppsym;
			--soffset_;
			setcond();
			state_consist();
			ion_consist();
		}
	}
	for (i = iligtrans_; i < ntrans_; ++i) {
		trans_[i].lig2pd(pdoff);
	}
}

void KSChan::setsname(int i, const char* s) {
	state_[i].name_ = s;
	sname_install();
}

void KSChan::free1() {
	int i;
	for (i=0; i < nstate_; ++i) { unref(state_[i].obj_); }
	for (i=0; i < ngate_; ++i) { unref(gc_[i].obj_); }
	for (i=0; i < ntrans_; ++i) { unref(trans_[i].obj_); }
	if (gc_) {delete [] gc_; gc_ = NULL;}
	if (state_) { delete [] state_; state_ = NULL;}
	if (trans_) { delete [] trans_; trans_ = NULL;}
	if (iv_relation_) { delete iv_relation_; iv_relation_ = NULL; }
	if (ligands_) { delete [] ligands_; ligands_ = NULL; }
	if (mat_) {
		spDestroy(mat_);
		delete [] elms_;
		delete [] diag_;
		mat_ = NULL;
	}
	ngate_ = 0;
	nstate_ = 0;
	ntrans_ = 0;
	state_size_ = 0;
	gate_size_ = 0;
	trans_size_ = 0;
}

void KSChan::setcond() {
//printf("KSChan::setcond\n");
	int i;
	if (iv_relation_) { delete iv_relation_; }
	if (ion_sym_) {
		if (cond_model_ == 2) {
			if (is_point()) {
				iv_relation_ = new KSPPIvghk();
				((KSPPIvghk*)iv_relation_)->z = nrn_ion_charge(ion_sym_);
			}else{
				iv_relation_ = new KSIvghk();
				((KSIvghk*)iv_relation_)->z = nrn_ion_charge(ion_sym_);
			}
			for (i=gmaxoffset_; i < 2+gmaxoffset_; ++i) {
				rlsym_->u.ppsym[i]->name[0] = 'p';
hoc_symbol_units(rlsym_->u.ppsym[i], (is_point() ? "cm3/s" : "cm/s"));
			}
		}else{
			if (is_point()) {
				iv_relation_ = new KSPPIv();
			}else{
				iv_relation_ = new KSIv();
			}
			for (i=gmaxoffset_; i < 2 + gmaxoffset_; ++i) {
				rlsym_->u.ppsym[i]->name[0] = 'g';
hoc_symbol_units(rlsym_->u.ppsym[i], is_point() ? "uS" : "S/cm2");
			}
		}
hoc_symbol_units(rlsym_->u.ppsym[2 + gmaxoffset_], is_point() ? "nA" : "mA/cm2");
	}else{
		if (is_point()) {
			iv_relation_ = new KSPPIvNonSpec();
		}else{
			iv_relation_ = new KSIvNonSpec();
		}
		for (i=gmaxoffset_; i < 3 + gmaxoffset_; i += 2) {
			rlsym_->u.ppsym[i]->name[0] = 'g';
hoc_symbol_units(rlsym_->u.ppsym[i], is_point() ? "uS" : "S/cm2");
		}
		hoc_symbol_units(rlsym_->u.ppsym[1 + gmaxoffset_], "mV");
hoc_symbol_units(rlsym_->u.ppsym[3 + gmaxoffset_], is_point() ? "nA" : "mA/cm2");
	}
	if (is_point()) { ((KSPPIv*)iv_relation_)->ppoff_ = ppoff_; }
}

void KSChan::setligand(int i, const char* lig) {
	char buf[100];
//printf("KSChan::setligand %d %s\n", i, lig);
	sprintf(buf, "%s_ion", lig);
	Symbol* s = looksym(buf);
	if (!s) {
		ion_reg(lig, 0);
		s = looksym(buf);
	}
	if (s->type != MECHANISM ||
memb_func[s->subtype].alloc != memb_func[looksym("na_ion")->subtype].alloc) {
		hoc_execerror(buf, "is already in use and is not an ion.");
	}
	ligands_[i] = s;
	if (mechsym_) {
		state_consist();		
		ion_consist();
	}
}

void KSChan::settype(KSTransition* t, int type, const char* lig) {
	int i, j;
	// if no ligands involved then it is just a type change.
	usetable(false);
	if (type < 2 && t->type_ < 2) {
		t->type_ = type;
		return;
	}
	set_single(false);
	int ilig = -1;
	// is t already using a ligand
	int iligold = -2;
	if (t->type_ >= 2) {
		iligold = t->ligand_index_;	
//printf("t already using a ligand index %d\n", iligold);
		if (type < 2) { // from having to not having
			// what is to be done with existing ligand
			// is anybody else using it
			bool remove = true;
			for (i = iligtrans_; i < ntrans_; ++i) {
				if (trans_[i].ligand_index_ == iligold && iligold != i) {
					remove = false; // old is still needed
				}
			}
			if (remove) { // unneeded
				Symbol** ligands = NULL;
				--nligand_;
				if (nligand_ > 0) {
					ligands = new Symbol*[nligand_];
				}
				for (i=0, j=0; j <  nligand_; ++i, ++j) {
					if (i == iligold) { ++j; }
					ligands[i] = ligands_[j];
				}
				delete [] ligands_;
				ligands_ = ligands;
			}
			// transitions with ligands may get decremented
			for (i = iligtrans_; i < ntrans_; ++i) {
				if (trans_[i].ligand_index_ > iligold) {
					--trans_[i].ligand_index_;
				}
			}
			// the transition may have to be moved forward
			assert(t->index_ >= iligtrans_);
			KSTransition tt = *t;
			t->obj_ = NULL;
			trans_remove(t->index_);
			trans_insert(iligtrans_, tt.src_, tt.target_);
			t = trans_ + iligtrans_ - 1;
			t->type_ = type;
			t->ligand_index_ = -1;
			t->obj_ = tt.obj_;
			if (t->obj_) { t->obj_->u.this_pointer = t; }
			t->f0 = tt.f0;
			t->f1 = tt.f1;
			tt.f0 = NULL;
			tt.f1 = NULL;
			check_struct();
			state_consist();
			ion_consist();
			setupmat();
			return;
		}
	}
	// there is a ligand, handle the last two cases
	// is ligand valid
	char buf[100];
//printf("KSChan::settype %s %d %s\n", hoc_object_name(t->obj_), type, lig);
//printf("old t->ligand_index_=%d type=%d\n", t->ligand_index_, t->type_);
	strcpy(buf, lig);
	strcpy(buf+strlen(buf)-1, "_ion");
//printf("ion name %s\n", buf);
	Symbol* s = looksym(buf);
	if (!s) {
		hoc_execerror(buf, "does not exist");
		ion_reg(lig, 0);
		s = looksym(buf);
	}
	if (s->type != MECHANISM ||
memb_func[s->subtype].alloc != memb_func[looksym("na_ion")->subtype].alloc) {
		hoc_execerror(buf, "is already in use and is not an ion.");
	}
	// is ligand in list
	for (i=0; i < nligand_; ++i) {
		if (ligands_[i] == s) {
			ilig = i;
			break;
		}
	}
//printf("ilig=%d iligold=%d\n", ilig, iligold);
	bool add2list = true;
	bool move = true;
	// if t already using a ligand, what is to be done with it
	if (t->type_ >= 2) {
		move = false; // do not need to reorder the transition
		add2list = false;
		for (i = iligtrans_; i < ntrans_; ++i) {
			if (trans_[i].ligand_index_ == iligold && t->index_ != i) {
				add2list = true; // old is still needed
			}
		}
	}
//printf("add2list=%d\n", add2list);
	if (add2list) { // add it to list
		Symbol** ligands = new Symbol*[nligand_+1];
		for (i=0; i < nligand_; ++i) {
			ligands[i] = ligands_[i];
		}
		if (nligand_) { delete [] ligands_; }
		ilig = nligand_;
		++nligand_;
		ligands_ = ligands;
	}else{ // replace
		ilig = iligold;
	}
	ligands_[ilig] = s;
#if 0
printf("ligands\n");
for (i=0; i < nligand_; ++i) {printf("%s\n", ligands_[ilig]->name);}
#endif
	// update the transition
	t->ligand_index_ = ilig;
	t->type_ = type;
//printf("new t->ligand_index_=%d type=%d\n", t->ligand_index_, t->type_);
	// if switch from no ligand to ligand
	// then transition must be moved to iligtrans or above
	if (iligold < 0 && ilig >= 0) {
#if 0
printf("old transition order\n");
for (i=0; i < ntrans_; ++i) {
printf("i=%d index=%d type=%d ligand_index=%d %s<->%s\n", i,
trans_[i].index_, trans_[i].type_, trans_[i].ligand_index_,
state_[trans_[i].src_].string(), state_[trans_[i].target_].string());
}
#endif
		assert(t->index_ < iligtrans_);
		KSTransition tt = *t;
		t->obj_ = NULL;
		t->f0 = NULL;
		t->f1 = NULL;
		trans_remove(t->index_);
		trans_insert(ntrans_, tt.src_, tt.target_);
		t = trans_ + ntrans_ - 1;
		t->obj_ = tt.obj_;
		t->ligand_index_ = tt.ligand_index_;
		t->type_ = tt.type_;
		t->f0 = tt.f0;
		t->f1 = tt.f1;
		tt.f0 = NULL;
		tt.f1 = NULL;
		if (t->obj_) { t->obj_->u.this_pointer = t; }
		if (iligtrans_ == ntrans_) {
			--iligtrans_;
		}
#if 0
printf("new transition order\n");
for (i=0; i < ntrans_; ++i) {
printf("i=%d index=%d type=%d ligand_index=%d %s<->%s %s\n", i,
trans_[i].index_, trans_[i].type_, trans_[i].ligand_index_,
state_[trans_[i].src_].string(), state_[trans_[i].target_].string(),
hoc_object_name(trans_[i].obj_));
}
#endif
	}
	check_struct();
	state_consist();
	ion_consist();
	setupmat();
}

KSState* KSChan::add_hhstate(const char* name) {
	int i;
	usetable(false);
	// new state, transition, gate, and f
	int is = nhhstate_;
	state_insert(is, name, 1.);
	gate_insert(is, is, 1);
	trans_insert(is, is, is);
	trans_[is].ligand_index_ = -1;
	trans_[is].type_ = 0;
	// adjust gate indices
	for (i=nhhstate_; i < ngate_; ++i) {
		++gc_[i].sindex_;
	}
	// adjust transition indices	
	for (i=ivkstrans_; i < ntrans_; ++i) {
		++trans_[i].src_;
		++trans_[i].target_;
	}
	set_single(false);
	check_struct();
	sname_install();
	state_consist();
	setupmat();
	return state_ + is;
}

KSState* KSChan::add_ksstate(int ig, const char* name) {
	// states must be added so that the gate states are in sequence
	int i, is;
	usetable(false);
	if (ig == ngate_) {
		is = nstate_;
		gate_insert(ig, is, 1);
	}else{
		is = gc_[ig].sindex_ + gc_[ig].nstate_;
		++gc_[ig].nstate_;
	}
	state_insert(is, name, 0.);
	if (nksstate_ == 0) {
		--nhhstate_; ++nksstate_;
	}
	// update gate indices
	for (i = ig+1; i < ngate_; ++i) {
		++gc_[i].sindex_;
	}
	// update transition indices
	for (i = ivkstrans_; i < ntrans_; ++i) {
		if (trans_[i].src_ > is) { --trans_[i].src_; }
		if (trans_[i].target_ > is) { --trans_[i].target_; }
	}
	check_struct();
	sname_install();
	set_single(false);
	state_consist();
	setupmat();
	return state_ + is;
}

void KSChan::remove_state(int is) {
	int i;
	usetable(false);
	if (is < nhhstate_) {
		state_remove(is);
		gate_remove(is);
		trans_remove(is);
		// adjust gate indices
		for (i=is; i < ngate_; ++i) {
			--gc_[i].sindex_;
		}
		// adjust transition indices	
		for (i=is; i < ntrans_; ++i) {
			--trans_[i].src_;
			--trans_[i].target_;
		}
	}else{ // remove a kinetic scheme state
		state_remove(is);
		// remove all the transitions involving this state
		for (i = ntrans_ - 1; i >= ivkstrans_; --i) {
			if (trans_[i].src_ == is || trans_[i].target_ == is) {
				trans_remove(i);
			}
		}
		// adjust transition indices
		for (i = ivkstrans_; i < ntrans_; ++i) {
			if (trans_[i].src_ > is) { --trans_[i].src_; }
			if (trans_[i].target_ > is) { --trans_[i].target_; }
		}
		// If this is the only state
		// the gate is removed. Otherwise the index and nstate
		// are updated. Note that it is not inconsistent for
		// the state graph of a gate to be multiple.
		for (i = nhhstate_; i < ngate_; ++i) {
			if (is >= gc_[i].sindex_ && is < (gc_[i].sindex_ + gc_[i].nstate_)) {
				if (gc_[i].nstate_ == 1) {// remove gate
					gate_remove(i);
				}else{
					--gc_[i].nstate_;
					if (is == gc_[i].sindex_) {
						++gc_[i].sindex_;
					}
				}
				break;
			}
		}
		for (i = nhhstate_; i < ngate_; ++i) {
			if (gc_[i].sindex_ > is) {
				--gc_[i].sindex_;
			}
		}
	}
	set_single(false);
	check_struct();
	sname_install();
	state_consist();
	setupmat();
}

KSTransition* KSChan::add_transition(int src, int target, const char* ligand) {
	usetable(false);
	assert(ligand == NULL);
	int it = (ligand ? ntrans_ : iligtrans_);
	trans_insert(it, src, target);
	trans_[it].ligand_index_ = -1;
	trans_[it].type_ = 0;
	set_single(false);
	check_struct();
	setupmat();
	return trans_ + it;
}

void KSChan::remove_transition(int it) {
	usetable(false);
	assert(it >= ivkstrans_);
	set_single(false);
	trans_remove(it);
	check_struct();
	setupmat();
}

//#undef assert
//#define assert(arg) if (!(arg)) { abort(); }

void KSChan::check_struct() {
	int i;
	assert(ngate_ >= nhhstate_);
	assert(ivkstrans_ == nhhstate_);
	assert(nstate_ == nhhstate_ + nksstate_);
	for (i=0; i < nhhstate_; ++i) {
		assert(trans_[i].src_ == i);
		assert(trans_[i].target_ == i);
		assert(gc_[i].sindex_ == i);
		assert(gc_[i].nstate_ == 1);
	}
	for (i=1; i < ngate_; ++i) {
		assert(gc_[i].index_ == i);
		assert(gc_[i].sindex_ == gc_[i-1].sindex_ + gc_[i-1].nstate_);
	}
	for (i=ivkstrans_; i < ntrans_; ++i) {
		assert(trans_[i].src_ >= nhhstate_);
		assert(trans_[i].target_ >= nhhstate_);
	}
	for (i=0; i < iligtrans_; ++i) {
		assert(trans_[i].type_ < 2);
if (trans_[i].ligand_index_ != -1) {
printf("trans_ %d ligand_index_=%d\n", i, trans_[i].ligand_index_);
}
		assert(trans_[i].ligand_index_ == -1);
	}
	for (i=iligtrans_; i < ntrans_; ++i) {
		int j = trans_[i].ligand_index_;
		assert(j >= 0 && j < nligand_);
		assert(trans_[i].type_ >= 2);
	}
	for (i=0; i < nstate_; ++i) {
		assert(state_[i].ks_ == this);
		assert(state_[i].index_ == i);
		Object* o = state_[i].obj_;
		if (o) {
			assert(o->u.this_pointer == state_ + i);
		}
	}
	for (i=0; i < ntrans_; ++i) {
		assert(trans_[i].ks_ == this);
		assert(trans_[i].index_ == i);
		Object* o = trans_[i].obj_;
		if (o) {
			assert(o->u.this_pointer == trans_ + i);
		}
	}
}

KSState* KSChan::state_insert(int i, const char* n, double d) {
	int j;
	usetable(false);
	if (nstate_ >= state_size_) {
		state_size_ += 5;
		KSState* state = new KSState[state_size_];
		for (j = 0; j < nstate_; ++j) {
			state[j] = state_[j];
		}
		delete [] state_;
		for (j = 0; j < state_size_; ++j) {
			state[j].ks_ = this;
		}
		state_ = state;
	}
	for (j = i; j < nstate_; ++j) {
		state_[j+1] = state_[j];
	}
	state_[i].f_ = d;
	state_[i].name_ = n;
	if (i <= nhhstate_) {
		++nhhstate_;
	}else{
		++nksstate_;
	}
	++nstate_;
	for (j = 0; j < nstate_; ++j) {
		state_[j].index_ = j;
		if (state_[j].obj_) { state_[j].obj_->u.this_pointer = state_ + j; }
	}
	return state_ + i;
}

void KSChan::state_remove(int i) {
	int j;
	usetable(false);
	unref(state_[i].obj_);
	for (j = i+1; j < nstate_; ++j) {
		state_[j-1] = state_[j];
		if (state_[j-1].obj_) { state_[j-1].obj_->u.this_pointer = state_ + j - 1; }
	}
	if (i < nhhstate_) {
		--nhhstate_;
	}else{
		--nksstate_;
	}
	--nstate_;
	state_[nstate_].obj_ = NULL;
	for (j = 0; j < nstate_; ++j) {
		state_[j].index_ = j;
		if (state_[j].obj_) { state_[j].obj_->u.this_pointer = state_ + j; }
	}
}

KSGateComplex* KSChan::gate_insert(int ig, int is, int power) {
	int j;
	usetable(false);
	if (ngate_ >= gate_size_) {
		gate_size_ += 5;
		KSGateComplex* gc = new KSGateComplex[gate_size_];
		for (j = 0; j < ngate_; ++j) {
			gc[j] = gc_[j];
		}
		delete [] gc_;
		gc_ = gc;			
		for (j = 0; j < gate_size_; ++j) {
			gc_[j].ks_ = this;
		}
	}
	for (j = ig; j < ngate_; ++j) {
		gc_[j+1] = gc_[j];
	}
	gc_[ig].sindex_ = is;
	gc_[ig].nstate_ = 1;
	gc_[ig].power_ = power;
	++ngate_;
	for (j = 0; j < ngate_; ++j) {
		gc_[j].index_ = j;
		if (gc_[j].obj_) { gc_[j].obj_->u.this_pointer = gc_ + j; }
	}
	return gc_ + ig;
}

void KSChan::gate_remove(int i) {
	int j;
	usetable(false);
	unref(gc_[i].obj_);
	for (j = i+1; j < ngate_; ++j) {
		gc_[j-1] = gc_[j];
		if (gc_[j-1].obj_) { gc_[j-1].obj_->u.this_pointer = gc_ + j - 1; }
	}
	--ngate_;
	gc_[ngate_].obj_ = NULL;
	for (j = 0; j < ngate_; ++j) {
		gc_[j].index_ = j;
		if (gc_[j].obj_) { gc_[j].obj_->u.this_pointer = gc_ + j; }
	}
}

KSTransition* KSChan::trans_insert(int i, int src, int target) {
	int j;
	usetable(false);
	if (ntrans_ >= trans_size_) {
		trans_size_ += 5;
		KSTransition* trans = new KSTransition[trans_size_];
		for (j = 0; j < ntrans_; ++j) {
			trans[j] = trans_[j];
			trans_[j].f0 = NULL;
			trans_[j].f1 = NULL;
		}
		delete [] trans_;
		trans_ = trans;			
	}
	for (j = i; j < ntrans_; ++j) {
		trans_[j+1] = trans_[j];
	}
	trans_[i].src_ = src;
	trans_[i].target_ = target;
	trans_[i].f0 = NULL;
	trans_[i].f1 = NULL;
	ivkstrans_ = nhhstate_;
	if (i <= iligtrans_) {
		++iligtrans_;
	}
	++ntrans_;
	for (j = 0; j < ntrans_; ++j) {
		trans_[j].index_ = j;
		trans_[j].ks_ = this;
		if (trans_[j].obj_) { trans_[j].obj_->u.this_pointer = trans_ + j; }
	}
	return trans_ + i;
}

void KSChan::trans_remove(int i) {
	int j;
	usetable(false);
	unref(trans_[i].obj_);
	for (j = i+1; j < ntrans_; ++j) {
		trans_[j-1] = trans_[j];
		if (trans_[j-1].obj_) { trans_[j-1].obj_->u.this_pointer = trans_ + j - 1; }
	}
	if (i < ivkstrans_) {
		--ivkstrans_;
	}
	if (i < iligtrans_) {
		--iligtrans_;
	}
	--ntrans_;
	for (j = 0; j < ntrans_; ++j) {
		trans_[j].index_ = j;
		if (trans_[j].obj_) { trans_[j].obj_->u.this_pointer = trans_ + j; }
	}
	trans_[ntrans_].obj_ = NULL;
}

void KSChan::setstructure(Vect* vec) {
	int i, j, ii, idx, ns;
//printf("setstructure called for KSChan %p %s\n", this, name_.string());
#if 0
for (i=0; i < vec->capacity(); ++i) {
	printf("%d %g\n", i, vec->elem(i));
}
#endif
	usetable(false);
	int nstate_old = nstate_;
	KSState* state_old = state_;
	nstate_ = 0;
	state_ = 0;
	free1();
	j = 0;
	cond_model_ = (int)vec->elem(j++);
	setcond();
	ngate_ = (int)vec->elem(j++);
	nstate_ = (int)vec->elem(j++);
	nhhstate_ = (int)vec->elem(j++);
	nksstate_ = nstate_ - nhhstate_;
	ivkstrans_ = nhhstate_;
	ntrans_ = (int)vec->elem(j++);
	nligand_ = (int)vec->elem(j++);
	iligtrans_ = (int)vec->elem(j++);
	if (ngate_) {
		gc_ = new KSGateComplex[ngate_];
		gate_size_ = ngate_;
	}
	if (ntrans_) {
		trans_ = new KSTransition[ntrans_];
		trans_size_ = ntrans_;
	}
	if (nstate_) {
		state_ = new KSState[nstate_];
		state_size_ = nstate_;
		// preserve old names as much as possible
		for (i = 0; i < nstate_; ++i) {
			state_[i].index_ = i;
			state_[i].ks_ = this;
			if (i < nstate_old) {
				state_[i].name_ = state_old[i].name_;
			}else{
				char buf[20];
				sprintf(buf, "s%d", i);
				state_[i].name_ = buf;
			}
		}
		if (state_old) {
			for (i=0; i < nstate_old; ++i) { unref(state_old[i].obj_); }
			delete [] state_old;
		}
	}
	for (i=0; i < nstate_; ++i) { state_[i].f_ = 0.; }
	for (i=0; i < ngate_; ++i) {
		gc_[i].ks_ = this;
		gc_[i].index_ = i;
		gc_[i].sindex_ = idx = (int)vec->elem(j++);
		gc_[i].nstate_ = ns = (int)vec->elem(j++);
		gc_[i].power_ = (int)vec->elem(j++);
		for (ii=0; ii < ns; ++ii) {
			state_[ii + idx].f_ = vec->elem(j++);
		}
	}
	// assert that order is all vtrans and then all lig trans
	int pdoff = ppoff_ + (ion_sym_ ? 5 : 0);
	for (i=0; i < ntrans_; ++i) {
		trans_[i].index_ = i;
		trans_[i].ks_ = this;
		trans_[i].src_ = (int)vec->elem(j++);
		trans_[i].target_ = (int)vec->elem(j++);
		trans_[i].type_ = (int)vec->elem(j++);
		trans_[i].ligand_index_ = (int)vec->elem(j++);
		if (i >= iligtrans_) {
			trans_[i].lig2pd(pdoff);
		}
	}
	if (nligand_) {
		ligands_ = new Symbol*[nligand_];
		for (i=0; i < nligand_; ++i) { ligands_[i] = NULL; }
	}
	check_struct();
	if (mechsym_) {
		set_single(false, false);
		sname_install();
		state_consist();
		setupmat();
	}
}

void KSChan::sname_install() {
	Symbol* searchsym = is_point() ? mechsym_ : NULL;
	char unsuffix[100];
	if (is_point()) {
		unsuffix[0] = '\0';
	}else{
		sprintf(unsuffix, "_%s", mechsym_->name);
	}
	// there need to be symbols for nstate_ states
	int i;
	int nold = rlsym_->s_varn;
	int nnew = soffset_ + nstate_;
	Symbol** snew;
	Symbol** sold = rlsym_->u.ppsym;
	// the ones that exist and new symbols get a temporary name of "".

	snew = newppsym(nnew);
	for (i=0; i < nnew; ++i) {
		if (i < nold) {
			snew[i] = sold[i];
			if (i >= soffset_) {
				snew[i]->name[0] = '\0'; // will be freed below
			}
		}else{
			// if not enough make more with a name of ""
			snew[i] = installsym("", RANGEVAR, searchsym);
			snew[i]->subtype = 3;
			snew[i]->u.rng.type = rlsym_->subtype;
			snew[i]->u.rng.index = i;
		}
	}
	// if too many then free the unused symbols and hope they really are unused
	for (i = nnew; i < nold; ++i) {
		freesym(sold[i], searchsym);
	}
	rlsym_->s_varn = nnew;
	free(rlsym_->u.ppsym);
	rlsym_->u.ppsym = snew;

	// fill the names checking for conflicts
	char buf[100], buf1[100];
	for (i=0; i < nstate_; ++i) {
		sprintf(buf, "%s%s", state_[i].string(), unsuffix);
		int j = 0;
		buf1[0]='\0';
		while (looksym(buf, searchsym)) {
			sprintf(buf1, "%s%d", state_[i].string(), j++);
			nrn_assert(snprintf(buf, 100, "%s%s", buf1, unsuffix) < 100);
		}
		free(snew[i + soffset_]->name);
		snew[i + soffset_]->name = strdup(buf);
		if (strlen(buf1) > 0) {
			state_[i].name_ = buf1;
		}
	}
}

// check at the top and built-in level, or, if top!= NULL check the template
Symbol* KSChan::looksym(const char* name, Symbol* top) {
	if (top) {
if (top->type != TEMPLATE) { printf("%s type=%d\n", top->name, top->type); abort();}
		assert(top->type == TEMPLATE);
		return hoc_table_lookup(name, top->u.ctemplate->symtable);
	}
	Symbol* sp = hoc_table_lookup(name, hoc_top_level_symlist);
	if (sp) { return sp; }
	sp = hoc_table_lookup(name, hoc_built_in_symlist);
	return sp;
}

// install in the built-in list or the template list
Symbol* KSChan::installsym(const char* name, int type, Symbol* top) {
	if (top) {
		assert (top->type == TEMPLATE);
		Symbol* s = hoc_install(name, type, 0.0, &top->u.ctemplate->symtable);
		s->cpublic = 1;
		return s;
	}
	return hoc_install(name, type, 0.0, &hoc_built_in_symlist);
}

Symbol** KSChan::newppsym(int n) {
	Symbol** spp = (Symbol**) hoc_Emalloc(n*sizeof(Symbol*)); hoc_malchk();
	return  spp;
}

void KSChan::freesym(Symbol* s, Symbol* top) {
	if (top) {
		assert(top->type == TEMPLATE);
		hoc_unlink_symbol(s, top->u.ctemplate->symtable);
	}else{
		hoc_unlink_symbol(s, hoc_built_in_symlist);
	}
	free(s->name);
	if (s->extra) {
		if (s->extra->parmlimits) {
			free(s->extra->parmlimits);
		}
		if (s->extra->units) {
			free(s->extra->units);
		}
		free(s->extra);
	}
	free(s);
}

void KSChan::setupmat() {
	int i, j, err;
//printf("KSChan::setupmat nksstate=%d\n", nksstate_);
	if (mat_) {
		spDestroy(mat_);
		delete [] elms_;
		delete [] diag_;
		mat_ = NULL;
	}
	if (!nksstate_) { return; }
	mat_ = spCreate(nksstate_, 0, &err);
	if (err != spOKAY) {
		hoc_execerror("Couldn't create sparse matrix", 0);
	}
	spFactor(mat_); // will fail but creates an internal vector needed by
			// mulmat which might be called prior to initialization
			// when switching to cvode active.
	elms_ = new double*[4*(ntrans_ - ivkstrans_)];
	diag_ = new double*[nksstate_];
	for (i=ivkstrans_, j=0; i < ntrans_; ++i) {
		int s, t;
		s = trans_[i].src_ - nhhstate_ + 1;
		t = trans_[i].target_ - nhhstate_ + 1;
		elms_[j++] = spGetElement(mat_, s, s);
		elms_[j++] = spGetElement(mat_, s, t);
		elms_[j++] = spGetElement(mat_, t, t);
		elms_[j++] = spGetElement(mat_, t, s);
	}
	for (i=0; i < nksstate_; ++i) {
		diag_[i] = spGetElement(mat_, i+1, i+1);
	}
}

void KSChan::fillmat(double v, Datum* pd) {
	int i, j;
	double a, b;
	spClear(mat_);
	for (i=ivkstrans_, j=0; i < iligtrans_; ++i) {
		trans_[i].ab(v, a, b);
//printf("trans %d v=%g a=%g b=%g\n", i, v, a, b);
		*elms_[j++] -= a;
		*elms_[j++] += b;
		*elms_[j++] -= b;
		*elms_[j++] += a;
	}
	for (i=iligtrans_; i < ntrans_; ++i) {
		a = trans_[i].alpha(pd);
		b = trans_[i].beta();
		*elms_[j++] -= a;
		*elms_[j++] += b;
		*elms_[j++] -= b;
		*elms_[j++] += a;
	}
//printf("after fill\n");
//spPrint(mat_, 0, 1, 0);
}

void KSChan::mat_dt(double dt, double* p) {
	// y' = m*y  this part add the dt for the form ynew/dt - yold/dt =m*ynew
	// the matrix ends up as (m-1/dt)ynew = -1/dt*yold
	int i;
	double dt1 = -1./dt;
	for (i=0; i < nksstate_; ++i) {
		*(diag_[i]) += dt1;
		p[i] *= dt1;
	}
}

void KSChan::solvemat(double* s) {
	int e;
	e = spFactor(mat_);
	if (e != spOKAY) {
		switch (e) {
		case spZERO_DIAG:
			hoc_execerror("spFactor error:", "Zero Diagonal");
		case spNO_MEMORY:
			hoc_execerror("spFactor error:", "No Memory");
		case spSINGULAR:
			hoc_execerror("spFactor error:", "Singular");
		}  
	}
	spSolve(mat_, s-1, s-1);
}

void KSChan::mulmat(double* s, double* ds) {
	spMultiply(mat_, ds-1, s-1);
}

void KSChan::alloc(Prop* prop) {
//printf("KSChan::alloc nstate_=%d nligand_=%d\n", nstate_, nligand_);
//printf("KSChan::alloc %s param=%p\n", name_.string(), prop->param);
	int j;
	prop->param_size = soffset_ + 2*nstate_;
    if (is_point() && nrn_point_prop_) {
	assert(nrn_point_prop_->param_size == prop->param_size);
	prop->param = nrn_point_prop_->param;
	prop->dparam = nrn_point_prop_->dparam;
    }else{
	prop->param = nrn_prop_data_alloc(prop->type, prop->param_size, prop);
	prop->param[gmaxoffset_] = gmax_deflt_;
	if (is_point()) {
		prop->param[NSingleIndex] = 1.;
	}
	if (!ion_sym_) {
		prop->param[1 + gmaxoffset_] = erev_deflt_;
	}
    }
	int ppsize = ppoff_;
	if (ion_sym_) {
		ppsize += 5 + 2*nligand_;
	}else{
		ppsize += 2*nligand_;
	}
    if (!is_point() || nrn_point_prop_ == 0) {
	if (ppsize > 0) {
		prop->dparam = nrn_prop_datum_alloc(prop->type, ppsize, prop);
		if (is_point()) {
			prop->dparam[2]._pvoid = NULL;
		}
	}else{
		prop->dparam = 0;
	}
    }
	Datum* pp = prop->dparam;
	int poff = ppoff_;
	if (ion_sym_) {
		Prop* prop_ion = need_memb(ion_sym_);
		if (cond_model_ == 0) { //ohmic
			nrn_promote(prop_ion, 0, 1);
		}else if (cond_model_ == 1) { // nernst
			nrn_promote(prop_ion, 1, 0);
		}else{ // ghk
			nrn_promote(prop_ion, 1, 0);
		}
		pp[ppoff_+0].pval = prop_ion->param + 0; //ena
		pp[ppoff_+1].pval = prop_ion->param + 3; //ina
		pp[ppoff_+2].pval = prop_ion->param + 4; //dinadv
		pp[ppoff_+3].pval = prop_ion->param + 1; //nai
		pp[ppoff_+4].pval = prop_ion->param + 2; //nao
		poff += 5;
	}
	for (j=0; j < nligand_; ++j) {
		Prop* pion = need_memb(ligands_[j]);
		nrn_promote(pion, 1, 0);
		pp[poff+2*j].pval = pion->param + 2; // nao
		pp[poff+2*j+1].pval = pion->param + 1; // nai
	}
	if (single_ && prop->dparam[2]._pvoid == NULL) {
		single_->alloc(prop, soffset_);
	}
}

Prop* KSChan::needion(Symbol* s, Node* nd, Prop* pm) {
	Prop* p, *pion;
	int type = s->subtype;
	for (p = nd->prop; p; p = p->next) {
		if (p->type == type) {
			break;
		}
	}
	pion = p;
//printf("KSChan::needion %s\n", s->name);
//printf("before ion rearrangement\n");
//for (p=nd->prop; p; p=p->next) {printf("\t%s\n", memb_func[p->type].sym->name);}
	if (!pion) {
		pion = prop_alloc(&nd->prop, type, nd);
	}else{ // if after then move to beginning
		for (p = pm; p; p = p->next) {
			if (p->next == pion) {
				p->next = p->next->next;
				pion->next = nd->prop;
				nd->prop = pion;
				break;
			}
		}						
	}
//printf("after ion rearrangement\n");
//for (p=nd->prop; p; p=p->next) {printf("\t%s\n", memb_func[p->type].sym->name);}
	return pion;
}

void KSChan::ion_consist() {
//printf("KSChan::ion_consist\n");
	int i, j;
	Section* sec;
	Node* nd;
	hoc_Item* qsec;
	int mtype = rlsym_->subtype;
	int poff = ppoff_;
	if (ion_sym_) {
		poff += 5;
	}
	for (i = iligtrans_; i < ntrans_; ++i) {
		trans_[i].lig2pd(poff);
	}
	int ppsize = poff + 2*nligand_;
	ForAllSections(sec)
		for (i = 0; i < sec->nnode; ++i) {
			nd = sec->pnode[i];
			Prop* p, *pion;
			for (p = nd->prop; p; p = p->next) {
				if (p->type == mtype) {
					break;
				}
			}
			if (!p) { continue; }
			p->dparam = (Datum*)erealloc(p->dparam, ppsize*sizeof(Datum));
//printf("KSChan::ion_consist %s node %d mtype=%d ion_type=%d\n",
//secname(sec), i, mtype, ion_sym_->subtype);
			if (ion_sym_) {
				pion = needion(ion_sym_, nd, p);
				if (cond_model_ == 0) { //ohmic
					nrn_promote(pion, 0, 1);
				}else if (cond_model_ == 1) { // nernst
					nrn_promote(pion, 1, 0);
				}else{ // ghk
					nrn_promote(pion, 1, 0);
				}
				Datum* pp = p->dparam;
				pp[ppoff_+0].pval = pion->param + 0; //ena
				pp[ppoff_+1].pval = pion->param + 3; //ina
				pp[ppoff_+2].pval = pion->param + 4; //dinadv
				pp[ppoff_+3].pval = pion->param + 1; //nai
				pp[ppoff_+4].pval = pion->param + 2; //nao
			}
			for (j=0; j < nligand_; ++j) {
				ligand_consist(j, poff, p, nd);
			}
		}
	}
}

void KSChan::ligand_consist(int j, int poff, Prop* p, Node* nd) {
	Prop* pion;
	pion = needion(ligands_[j], nd, p);
	nrn_promote(pion, 1, 0);
	p->dparam[poff+2*j].pval = pion->param + 2; // nao
	p->dparam[poff+2*j+1].pval = pion->param + 1; // nai
}

void KSChan::state_consist(int shift) { // shift when Nsingle winks in and out of existence
//printf("KSChan::state_consist\n");
	int i, j, ns;
	Section* sec;
	Node* nd;
	hoc_Item* qsec;
	int mtype = rlsym_->subtype;
	ns = soffset_ + 2*nstate_;
	ForAllSections(sec)
		for (i = 0; i < sec->nnode; ++i) {
			nd = sec->pnode[i];
			Prop* p;
			for (p = nd->prop; p; p = p->next) {
				if (p->type == mtype) {
					if (p->param_size != ns) {
		v_structure_change = 1;
		double* oldp = p->param;
		p->param = (double*)erealloc(oldp, ns*sizeof(double));
		if (oldp != p->param || shift != 0) {
//printf("KSChan::state_consist realloc changed location\n");
			notify_freed_val_array(oldp, p->param_size);
		}
		p->param_size = ns;
		if (shift == 1) {
			for (j = ns-1; j > 0; --j) {
				p->param[j] = p->param[j-1];
			}
			p->param[0] = 1;
		}else if (shift == -1) {
			for (j = 1; j < ns; ++j) {
				p->param[j-1] = p->param[j];
			}
		}
					}
					break;
				}
			}
		}
	}
}

void KSChan::delete_schan_node_data() {
	hoc_List* list  = mechsym_->u.ctemplate->olist;
	hoc_Item* q;
	ITERATE(q, list) {
		Point_process* pnt = (Point_process*)(OBJ(q)->u.this_pointer);
		if (pnt && pnt->prop && pnt->prop->dparam[2]._pvoid) {
			KSSingleNodeData* snd = (KSSingleNodeData*)pnt->prop->dparam[2]._pvoid;
			delete snd;
			pnt->prop->dparam[2]._pvoid = NULL;
		}
	}
}

void KSChan::alloc_schan_node_data() {
	hoc_List* list  = mechsym_->u.ctemplate->olist;
	hoc_Item* q;
	ITERATE(q, list) {
		Point_process* pnt = (Point_process*)(OBJ(q)->u.this_pointer);
		if (pnt && pnt->prop) {
			single_->alloc(pnt->prop, soffset_);
		}
	}
}

void KSChan::init(int n, Node** nd, double** pp, Datum** ppd, NrnThread* nt) {
	int i, j;
	if (nstate_) for (i=0; i < n; ++i) {
		double v = NODEV(nd[i]);
		double* s = pp[i] + soffset_;
		for (j=0; j < nstate_; ++j) {
			s[j] = 0;
		}
		for (j=0; j < ngate_; ++j) {
			s[gc_[j].sindex_] = 1;
		}
		for (j = 0; j < nhhstate_; ++j) {
			s[j] = trans_[j].inf(v);
		}
		if (nksstate_) {
			s += nhhstate_;
			fillmat(v, ppd[i]);
			mat_dt(1e9, s);
			solvemat(s);
		}
		if (is_single()) {
			KSSingleNodeData* snd = (KSSingleNodeData*)ppd[i][2]._pvoid;
			snd->nsingle_ = int(pp[i][NSingleIndex] + .5);
			pp[i][NSingleIndex] = double(snd->nsingle_);
			if (snd->nsingle_ > 0) {
				// replace population fraction with integers.
				single_->init(v, s, snd, nt);
			}
		}
//printf("KSChan::init\n");
//s = pp[i] + soffset_;
//for (j=0; j < nstate_; ++j) {
//	printf("%d %g\n", j, s[j]);
//}
	}
}

void KSChan::state(int n, Node** nd, double** pp, Datum** ppd, NrnThread* nt) {
	int i, j;
	double* s;
	if (nstate_) {
	    for (i=0; i < n; ++i) {
		if (is_single() && pp[i][NSingleIndex] > .999) {
			single_->state(nd[i], pp[i], ppd[i], nt);
			continue;
		}
		double v = NODEV(nd[i]);
		s = pp[i] + soffset_;
		if (usetable_) {
			double inf, tau;
			int k; double x, y;
			x = (v - vmin_)*dvinv_;
			y = floor(x);
			k = int(y);
			x -= y;
		    if(k < 0) {
			for (j = 0; j < nhhstate_; ++j) {
				trans_[j].inftau_hh_table(0, inf, tau);
				s[j] += (inf - s[j])*tau;
			}
		    }else if (k >= hh_tab_size_) {
			for (j = 0; j < nhhstate_; ++j) {
				trans_[j].inftau_hh_table(hh_tab_size_-1, inf, tau);
				s[j] += (inf - s[j])*tau;
			}
		    }else{
			for (j = 0; j < nhhstate_; ++j) {
				trans_[j].inftau_hh_table(k, x, inf, tau);
				s[j] += (inf - s[j])*tau;
			}
		    }
		}else{
			for (j = 0; j < nhhstate_; ++j) {
				double inf, tau;
				trans_[j].inftau(v, inf, tau);
				tau = 1. - KSChanFunction::Exp(-nt->_dt/tau);
				s[j] += (inf - s[j])*tau;
			}
		}
		if (nksstate_) {
			s += nhhstate_;
			fillmat(v, ppd[i]);
			mat_dt(nt->_dt, s);
			solvemat(s);
		}
	    }
	}
}

#if CACHEVEC
void KSChan::state(int n, int *ni, Node** nd, double** pp, Datum** ppd, NrnThread* _nt) {
	int i, j;
	double* s;
	if (nstate_) {
	    for (i=0; i < n; ++i) {
		if (is_single() && pp[i][NSingleIndex] > .999) {
			single_->state(nd[i], pp[i], ppd[i], _nt);
			continue;
		}
		double v = VEC_V(ni[i]);
		s = pp[i] + soffset_;
		if (usetable_) {
			double inf, tau;
			int k; double x, y;
			x = (v - vmin_)*dvinv_;
			y = floor(x);
			k = int(y);
			x -= y;
		    if(k < 0) {
			for (j = 0; j < nhhstate_; ++j) {
				trans_[j].inftau_hh_table(0, inf, tau);
				s[j] += (inf - s[j])*tau;
			}
		    }else if (k >= hh_tab_size_) {
			for (j = 0; j < nhhstate_; ++j) {
				trans_[j].inftau_hh_table(hh_tab_size_-1, inf, tau);
				s[j] += (inf - s[j])*tau;
			}
		    }else{
			for (j = 0; j < nhhstate_; ++j) {
				trans_[j].inftau_hh_table(k, x, inf, tau);
				s[j] += (inf - s[j])*tau;
			}
		    }
		}else{
			for (j = 0; j < nhhstate_; ++j) {
				double inf, tau;
				trans_[j].inftau(v, inf, tau);
				tau = 1. - KSChanFunction::Exp(-_nt->_dt/tau);
				s[j] += (inf - s[j])*tau;
			}
		}
		if (nksstate_) {
			s += nhhstate_;
			fillmat(v, ppd[i]);
			mat_dt(_nt->_dt, s);
			solvemat(s);
		}
	    }
	}
}
#endif /* CACHEVEC */

void KSChan::cur(int n, Node** nd, double** pp, Datum** ppd) {
	int i;
	for (i=0; i < n; ++i) {
		double g, ic;
		g = conductance(pp[i][gmaxoffset_], pp[i]+soffset_);
		ic = iv_relation_->cur(g, pp[i]+gmaxoffset_, ppd[i], NODEV(nd[i]));
		NODERHS(nd[i]) -= ic;
	}
}

#if CACHEVEC
void KSChan::cur(int n, int *nodeindices, double** pp, Datum** ppd, NrnThread* _nt) {
	int i;
	for (i=0; i < n; ++i) {
		double g, ic;
		int ni = nodeindices[i];
		g = conductance(pp[i][gmaxoffset_], pp[i]+soffset_);
		ic = iv_relation_->cur(g, pp[i]+gmaxoffset_, ppd[i], VEC_V(ni));
		VEC_RHS(ni) -= ic;
	}
}
#endif /* CACHEVEC */

void KSChan::jacob(int n, Node** nd, double** pp, Datum** ppd) {
	int i;
	for (i=0; i < n; ++i) {
		NODED(nd[i]) += iv_relation_->jacob(pp[i]+gmaxoffset_, ppd[i], NODEV(nd[i]));
	}
}

#if CACHEVEC
void KSChan::jacob(int n, int *nodeindices, double** pp, Datum** ppd, NrnThread* _nt) {
	int i;
	for (i=0; i < n; ++i) {
		int ni = nodeindices[i];
		VEC_D(ni) += iv_relation_->jacob(pp[i]+gmaxoffset_, ppd[i], VEC_V(ni));
	}

}
#endif /* CACHEVEC */

double KSIv::cur(double g, double* p, Datum* pd, double v) {
	double i, ena;
	ena = *pd[0].pval;
	p[1] = g;
	i =  g*(v - ena);
	p[2] = i;
	*pd[1].pval += i; //iion
	return i;
}

double KSIv::jacob(double* p, Datum* pd, double) {
	*pd[2].pval += p[1]; // diion/dv
	return p[1];
}

double KSIvghk::cur(double g, double* p, Datum* pd, double v) {
	double i, ci, co;
	ci = *pd[3].pval;
	co = *pd[4].pval;
	p[1] = g;
	i = g*nrn_ghk(v, ci, co, z);
	p[2] = i;
	*pd[1].pval += i;
	return i;
}

double KSIvghk::jacob(double* p, Datum* pd, double v) {
	double i1, ci, co, didv;
	ci = *pd[3].pval;
	co = *pd[4].pval;
	i1 = p[1]*nrn_ghk(v + .001, ci, co, z); // g is p[1]
	didv = (i1 - p[2])*1000.;
	*pd[2].pval += didv;
	return didv;
}

double KSIvNonSpec::cur(double g, double* p, Datum* pd, double v) {
	double i;
	p[2] = g; // gmax, e, g
	i = g*(v - p[1]);
	p[3] = i;
	return i;
}

double KSIvNonSpec::jacob(double* p, Datum* pd, double) {
	return p[2];
}

double KSPPIv::cur(double g, double* p, Datum* pd, double v) {
	double afac = 1.e2/(*pd[0].pval);
	pd += ppoff_;
	double i, ena;
	ena = *pd[0].pval;
	p[1] = g;
	i =  g*(v - ena);
	p[2] = i;
	i *= afac;
	*pd[1].pval += i; //iion
	return i;
}

double KSPPIv::jacob(double* p, Datum* pd, double) {
	double afac = 1.e2/(*pd[0].pval);
	pd += ppoff_;
	double g = p[1]*afac;
	*pd[2].pval += g; // diion/dv
	return g;
}

double KSPPIvghk::cur(double g, double* p, Datum* pd, double v) {
	double afac = 1.e2/(*pd[0].pval);
	pd += ppoff_;
	double i, ci, co;
	ci = *pd[3].pval;
	co = *pd[4].pval;
	p[1] = g;
	i = g*nrn_ghk(v, ci, co, z)*1e6;
	p[2] = i;
	i *= afac;
	*pd[1].pval += i;
	return i;
}

double KSPPIvghk::jacob(double* p, Datum* pd, double v) {
	double afac = 1.e2/(*pd[0].pval);
	pd += ppoff_;
	double i1, ci, co, didv;
	ci = *pd[3].pval;
	co = *pd[4].pval;
	i1 = p[1]*nrn_ghk(v + .001, ci, co, z)*1e6; // g is p[1]
	didv = (i1 - p[2])*1000.;
	didv *= afac;
	*pd[2].pval += didv;
	return didv;
}

double KSPPIvNonSpec::cur(double g, double* p, Datum* pd, double v) {
	double afac = 1.e2/(*pd[0].pval);
	double i;
	p[2] = g; // gmax, e, g
	i = g*(v - p[1]);
	p[3] = i;
	return i*afac;
}

double KSPPIvNonSpec::jacob(double* p, Datum* pd, double) {
	double afac = 1.e2/(*pd[0].pval);
	return p[2]*afac;
}

double KSChan::conductance(double gmax, double* s) {
	double g = 1.;
	int i;
	for (i=0; i < ngate_; ++i) {
		g *= gc_[i].conductance(s, state_);
	}
	return gmax*g;
}

KSTransition::KSTransition(){
	obj_ = NULL;
	f0 = NULL;
	f1 = NULL;
	size1_ = 0;
	inftab_ = NULL;
	tautab_ = NULL;
	stoichiom_ = 1;
//	f0 = new KSChanFunction();
//	f1 = new KSChanFunction();
}

KSTransition::~KSTransition(){
	if (f0) {
		delete f0;
	}
	if (f1) {
		delete f1;
	}
	hh_table_make(0., 0);
}

void KSTransition::setf(int i, int type, Vect* vec, double vmin, double vmax) {
	ks_->usetable(false);
	if (i == 0) {
		if (f0) { delete f0; }
		f0 = KSChanFunction::new_function(type, vec, vmin, vmax);
	}else{
		if (f1) { delete f1; }
		f1 = KSChanFunction::new_function(type, vec, vmin, vmax);
	}
}

void KSTransition::lig2pd(int poff) {
	ks_->usetable(false);
	if (type_ == 2) {
		pd_index_ = poff + 2*ligand_index_;
	}else if (type_ == 3){
		pd_index_ = poff + 2*ligand_index_ + 1;
	}else{
		assert(0);
	}
}

void KSTransition::ab(double v, double& a, double& b) {
	a = f0->f(v);
	if (f0->type() == 5 && f1->type() == 6) {
		b = ((KSChanBGinf*)f0)->tau;
	}else{
		b = f1->f(v);
	}
	if (type_ == 1) {
		double t = a;
		a = t/b;
		b = (1. - t)/b;
	}
}

void KSTransition::ab(Vect* v, Vect* a, Vect* b) {
	int i, n = v->capacity();
	a->resize(n);
	b->resize(n);
	if (f0->type() == 5 && f1->type() == 6) {
		for (i=0; i < n; ++i) {
			a->elem(i) = f0->f(v->elem(i));
			b->elem(i) = ((KSChanBGinf*)f0)->tau;
		}
	}else{
		for (i=0; i < n; ++i) {
			a->elem(i) = f0->f(v->elem(i));
			b->elem(i) = f1->f(v->elem(i));
		}
	}
	if (type_ == 1) {
		for (i=0; i < n; ++i) {
			double t = a->elem(i);
			a->elem(i) = t/b->elem(i);
			b->elem(i) = (1. - t)/b->elem(i);
		}
	}
}

void KSTransition::inftau(double v, double& a, double& b) {
	a = f0->f(v);
	if (f0->type() == 5 && f1->type() == 6) {
		b = ((KSChanBGinf*)f0)->tau;
	}else{
		b = f1->f(v);
	}
	if (type_ != 1) {
		double t  = 1./(a + b);
		a = a * t;
		b = t;
	}
}

void KSTransition::inftau(Vect* v, Vect* a, Vect* b) {
	int i, n = v->capacity();
	a->resize(n);
	b->resize(n);
	if (f0->type() == 5 && f1->type() == 6) {
		for (i=0; i < n; ++i) {
			a->elem(i) = f0->f(v->elem(i));
			b->elem(i) = ((KSChanBGinf*)f0)->tau;
		}
	}else{
		for (i=0; i < n; ++i) {
			a->elem(i) = f0->f(v->elem(i));
			b->elem(i) = f1->f(v->elem(i));
		}
	}
	if (type_ != 1) {
		for (i=0; i < n; ++i) {
			double t  = 1./(a->elem(i) + b->elem(i));
			a->elem(i) = a->elem(i) * t;
			b->elem(i) = t;
		}
	}
}

double KSTransition::alpha(Datum* pd) {
	double x = *(pd[pd_index_].pval);
	switch (stoichiom_) {
	case 1: return x * f0->c(0);
	case 2: return x * x * f0->c(0);
	case 3: return x * x * x * f0->c(0);
	case 4: {x *= x; return x * x * f0->c(0); }
	default: return f0->c(0)* pow(x, double(stoichiom_));
	}
}

double KSTransition::beta() {
	return f1->c(0);
}

KSGateComplex::KSGateComplex() {
	power_ = 0;
	obj_ = NULL;
}

KSGateComplex::~KSGateComplex() {}
double KSGateComplex::conductance(double* s, KSState* st) {
	double g = 0.;
	int i;
	s += sindex_;
	st += sindex_;
	for (i=0; i < nstate_; ++i) {
		g += s[i] * st[i].f_;
	}
#if 1
	switch (power_) { // 14.42
	case 1: return g;
	case 2: return g*g;
	case 3: return g*g*g;
	case 4: g = g*g; return g*g;
	}
#endif
	return pow(g, (double)power_); // 18.74

}

KSState::KSState() {
	obj_ = NULL;
	f_ = 0.;
}

KSState::~KSState() {
}

int KSChan::count() { return nstate_; }

void KSChan::map(int ieq, double** pv, double** pvdot,
    double* p, Datum* pd, double* atol){
	int i;
	double *p1 = p + soffset_;
	double *p2 = p1 + nstate_;
	for (i=0; i < nstate_; ++i) {
		pv[i] = p1 + i;
		pvdot[i] = p2 + i;
	}
}

void KSChan::spec(int n, Node** nd, double** p, Datum** ppd){
	int i, j;
	if (nstate_) for (i=0; i < n; ++i) {
//for (j=0; j < nstate_; ++j) {
//printf("KSChan spec before j=%d s=%g ds=%g\n", j, p[i][soffset_+j], p[i][soffset_+nstate_+j]);
//}
		double v = NODEV(nd[i]);
		double *p1 = p[i] + soffset_;
		double *p2 = p1 + nstate_;
		if (is_single() && p[i][NSingleIndex] > .999) {
			for (j = 0; j < nstate_; ++j) {
				p2[j] = 0.;
			}
			continue;
		}
		for (j = 0; j < nhhstate_; ++j) {
			double inf, tau;
			trans_[j].inftau(v, inf, tau);
			p2[j] = (inf - p1[j])/tau;
		}
		if (nksstate_) {
			fillmat(v, ppd[i]);
			mulmat(p1 + nhhstate_, p2 + nhhstate_);
		}
//for (j=0; j < nstate_; ++j) {
//printf("KSChan spec after j=%d s=%g ds=%g\n", j, p[i][soffset_+j], p[i][soffset_+nstate_+j]);
//}
	}
}

void KSChan::matsol(int n, Node** nd, double** p, Datum** ppd, NrnThread* nt){
	int i, j;
	double* p2;
	if (nstate_) for (i=0; i < n; ++i) {
		if (is_single() && p[i][NSingleIndex] > .999) {
			continue;
		}
		double v = NODEV(nd[i]);
		p2 = p[i] + soffset_ + nstate_;
		for (j = 0; j < nhhstate_; ++j) {
			double tau;
			tau = trans_[j].tau(v);
			p2[j] /= (1 + nt->_dt/tau);
		}
		if (nksstate_) {
			p2 += nhhstate_;
			fillmat(v, ppd[i]);
			mat_dt(nt->_dt, p2);
			solvemat(p2);
		}
	}
}

// from Cvode::do_nonode
void KSChan::cv_sc_update(int n, Node** nd, double** pp, Datum** ppd, NrnThread* nt) {
	int i, j;
	double* s;
	if (nstate_) {
		for (i=0; i < n; ++i) {
			if (pp[i][NSingleIndex] > .999) {
				single_->cv_update(nd[i], pp[i], ppd[i], nt);
			}
		}
	}
}

KSChanFunction::KSChanFunction() {
	gp_ = NULL;
}

KSChanFunction::~KSChanFunction() {
	if (gp_) {
		hoc_obj_unref(gp_->obj_);
	}
}

KSChanFunction* KSChanFunction::new_function(int type, Vect* vec, double vmin, double vmax) {
	KSChanFunction* f;
	switch (type) {
	case 1:
		f = new KSChanConst();
		break;
	case 2:
		f = new KSChanExp();
		break;
	case 3:
		f = new KSChanLinoid();
		break;
	case 4:
		f = new KSChanSigmoid();
		break;
	case 5:
		f = new KSChanBGinf();
		break;
	case 6:
		f = new KSChanBGtau();
		break;
	case 7:
		f = new KSChanTable(vec, vmin, vmax);
		break;
	default:
		f = new KSChanFunction();
		break;
	}
	f->gp_ = vec;
	hoc_obj_ref(f->gp_->obj_);
	return f;
}

KSChanTable::KSChanTable(Vect* vec, double vmin, double vmax) {
	vmin_ = vmin;
	vmax_ = vmax;
	assert(vmax > vmin);
	assert(vec->capacity() > 1);
	dvinv_ = (vec->capacity() - 1)/(vmax - vmin);
}

double KSChanTable::f(double v) {
	double x;
	if (v <= vmin_) {
		x = c(0);
	}else if (v >= vmax_) {
		x = c(gp_->capacity() - 1);
	}else{
		x = (v - vmin_)*dvinv_;
		int i = (int)x;
		x -= floor(x);
		x = c(i) + (c(i+1) - c(i))*x;
	}
	return x;
}

void KSTransition::hh_table_make(double dt, int size, double vmin, double vmax) {
	int i;
	double dv, tau;
	if (size < 1 || vmin >= vmax || size - size1_ != 1) {
		if (size1_) {
			delete [] inftab_;
			delete [] tautab_;
			size1_ = 0;
			inftab_ = NULL;
			tautab_ = NULL;
		}
		if (size < 1) {
			return;
		}
	}
	if (inftab_ == NULL) {
		inftab_ = new double[size];
		tautab_ = new double[size];
	}
	size1_ = size - 1;
	dv = (vmax - vmin)/size1_;
	for (i=0; i < size; ++i) {
		inftau(vmin + i*dv, inftab_[i], tau);
		tautab_[i] = 1. - KSChanFunction::Exp(-dt/tau);
	}
}

void KSChan::usetable(bool use, int size, double vmin, double vmax) {
	if (vmin >= vmax) {
		vmin_ = -100;
		vmax_ = 50;
	}else{
		vmin_ = vmin;
		vmax_ = vmax;
	}
	if (size < 2) {
		hh_tab_size_ = 200;
	}else{
		hh_tab_size_ = size;
	}
	dvinv_ = (hh_tab_size_-1)/(vmax_ - vmin_);
	usetable(use);
}

static int ksusing(int type) {
	for (int i=0; i < nrn_nthread; ++i) {
		for (NrnThreadMembList* tml = nrn_threads[i].tml; tml; tml = tml->next) {
			if (tml->index == type) {
				return 1;
			}
		}
	}
	return 0;
}

void KSChan::usetable(bool use) {
	int i;
	if (nhhstate_ == 0) { use = false; }
	usetable_ = use;
	if (mechtype_ == -1) { return; }
	if (usetable_) {
		dtsav_ = -1.0;
		check_table_thread(nrn_threads);
		if (memb_func[mechtype_].thread_table_check_ != check_table_thread_) {
			memb_func[mechtype_].thread_table_check_ = check_table_thread_;
			if (ksusing(mechtype_)) {
				nrn_mk_table_check();
			}
		}
	}else{
		if (memb_func[mechtype_].thread_table_check_) {
			memb_func[mechtype_].thread_table_check_ = 0;
			if (ksusing(mechtype_)) {
				nrn_mk_table_check();
			}
		}
	}
}

int KSChan::usetable(double* vmin, double* vmax) {
	*vmin = vmin_;
	*vmax = vmax_;
	return hh_tab_size_;
}

void KSChan::check_table_thread(NrnThread* nt) {
	int i;
	if (usetable_ && nt->_dt != dtsav_) {
		for (i = 0; i < nhhstate_; ++i) {
			trans_[i].hh_table_make(nt->_dt, hh_tab_size_, vmin_, vmax_);
		}
		dtsav_ = nt->_dt;
	}	
}
