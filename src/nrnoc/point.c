#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/point.c,v 1.13 1999/03/23 16:12:09 hines Exp */

/* modl description via modlreg calls point_register_mech() and
saves the pointtype as later argument to create and loc */

#include <stdlib.h>
#include "section.h"
#include "membfunc.h"
#include "parse.h"

extern char* pnt_map;
extern Symbol** pointsym; /*list of variable symbols in s->u.ppsym[k]
				with number in s->s_varn */

extern short* nrn_is_artificial_;
extern double loc_point_process();
extern Prop* prop_alloc();

static int cppp_semaphore = 0;	/* connect point process pointer semaphore */
static double** cppp_pointer;
static void free_one_point();
static void create_artcell_prop(Point_process* pnt, short type);

Prop* nrn_point_prop_;
void (*nrnpy_o2loc_p_)(Object*, Section**, double*);

void* create_point_process(int pointtype, Object* ho)
{
	Point_process* pp;
	pp = (Point_process*)emalloc(sizeof(Point_process));
	pp->node = 0;
	pp->sec = 0;
	pp->prop = 0;
	pp->ob = ho;
	pp->presyn_ = 0;
	pp->nvi_ = 0;
	pp->_vnt = 0;
	
	if (nrn_is_artificial_[pointsym[pointtype]->subtype]) {
		create_artcell_prop(pp, pointsym[pointtype]->subtype);
		return pp;
	}
	if (ho && ho->template->steer && ifarg(1)) {
		loc_point_process(pointtype, (void*)pp);
	}
	return (void*)pp;
}

Object* nrn_new_pointprocess(Symbol* sym)
{
	void* v;
	Object* hoc_new_object(), *hoc_new_opoint();
	Object* ob;
	extern Symlist* hoc_built_in_symlist;
	Symbol* hoc_table_lookup();
	int pointtype;
	assert(sym->type == MECHANISM && memb_func[sym->subtype].is_point);
	pointtype = pnt_map[sym->subtype];
	if (memb_func[sym->subtype].hoc_mech) {
		ob = hoc_new_opoint(sym->subtype);
	}else{
		hoc_push_frame(sym, 0);
		v = create_point_process(pointtype, (Object*)0);
		hoc_pop_frame();
		sym = hoc_table_lookup(sym->name, hoc_built_in_symlist);
		ob =  hoc_new_object(sym, v);
		((Point_process*)v)->ob = ob;
	}
	return ob;
}

void destroy_point_process(void* v)
{
	Point_process* pp = (Point_process*)v;
	free_one_point(pp);
	free((char*)pp);
}

void nrn_loc_point_process(int pointtype, Point_process* pnt, Section* sec, Node* node)
{
	extern Prop *prop_alloc_disallow(), *prop_alloc();
	extern Section* nrn_pnt_sec_for_need_;
	Prop* p;
	double x, nrn_arc_position();
	
	assert(!nrn_is_artificial_[pointsym[pointtype]->subtype]);
	x = nrn_arc_position(sec, node);
	/* the problem the next fragment overcomes is that POINTER's become
	   invalid when a point process is moved (dparam and param were
	   allocated but then param was freed and replaced by old param) The
	   error that I saw, then, was when a dparam pointed to a param --
	   useful to give default value to a POINTER and then the param was
	   immediately freed. This was the tip of the iceberg since in general
	   when one moves a point process, some pointers are valid and
	   some invalid and this can only be known by the model in its
	   CONSTRUCTOR. Therefore, instead of copying the old param to
	   the new param (and therefore invalidating other pointers as in
	   menus) we flag the allocation routine for the model to
	   1) not allocate param and dparam, 2) don't fill param but
	   do the work for dparam (fill pointers for ions),
	   3) execute the constructor normally.
	*/
	if (pnt->prop) {
		nrn_point_prop_ = pnt->prop;
	}else{
		nrn_point_prop_ = (Prop*)0;
	}
	nrn_pnt_sec_for_need_ = sec;
	if (x == 0. || x == 1.) {
		p = prop_alloc_disallow(&(node->prop), pointsym[pointtype]->subtype, node);
	}else{
		p = prop_alloc(&(node->prop), pointsym[pointtype]->subtype, node);
	}
	nrn_pnt_sec_for_need_ = (Section*)0;

	nrn_point_prop_ = (Prop*)0;
	if (pnt->prop) {
		pnt->prop->param = (double*)0;
		pnt->prop->dparam = (Datum*)0;
		free_one_point(pnt);
	}
	nrn_sec_ref(&pnt->sec, sec);
	pnt->node = node;
	pnt->prop = p;
	pnt->prop->dparam[0].pval = &NODEAREA(node);
	pnt->prop->dparam[1]._pvoid = (void*)pnt;
	if (pnt->ob) {
		if (pnt->ob->observers) {
			hoc_obj_notify(pnt->ob);
		}
		if (pnt->ob->template->observers) {
			hoc_template_notify(pnt->ob, 2);
		}
	}
}

static void create_artcell_prop(Point_process* pnt, short type) {
	Prop* p = (Prop*)0;
	nrn_point_prop_ = (Prop*)0;
	pnt->prop = prop_alloc(&p, type, (Node*)0);
	pnt->prop->dparam[0].pval = (double*)0;
	pnt->prop->dparam[1]._pvoid = (void*)pnt;
	if (pnt->ob) {
		if (pnt->ob->observers) {
			hoc_obj_notify(pnt->ob);
		}
		if (pnt->ob->template->observers) {
			hoc_template_notify(pnt->ob, 2);
		}
	}
}

void nrn_relocate_old_points(Section* oldsec, Node* oldnode, Section* sec, Node* node)
{
	Point_process* pnt;
	Prop* p, *pn;
	if (oldnode) for (p = oldnode->prop; p; p = pn) {
		pn = p->next;
		if (memb_func[p->type].is_point) {
			pnt = (Point_process*)p->dparam[1]._pvoid;
			if (oldsec == pnt->sec) {
				if (oldnode == node) {
					nrn_sec_ref(&pnt->sec, sec);
				}else{
#if 0
double nrn_arc_position();
char* secname();
printf("relocating a %s to %s(%d)\n",
memb_func[p->type].sym->name,
secname(sec), nrn_arc_position(sec, node)
);
#endif
					nrn_loc_point_process(
					   pnt_map[p->type], pnt, sec, node
					);
				}
			}
		}
	}
}

double loc_point_process(int pointtype, void* v)
{
	extern int hoc_is_double_arg();
	extern double chkarg();
	extern Object** hoc_objgetarg();
	Point_process* pnt = (Point_process*)v;
	double x;
	Section *sec;
	Node *node, *node_exact();
	
	if (nrn_is_artificial_[pointsym[pointtype]->subtype]) {
		hoc_execerror("ARTIFICIAL_CELLs are not located in a section", (char*)0);
	}
	if (hoc_is_double_arg(1)) {
		x = chkarg(1, 0., 1.);
		sec = chk_access();
	}else{
		Object* o = *hoc_objgetarg(1);			
		sec = (Section*)0;
		if (nrnpy_o2loc_p_) {
			(*nrnpy_o2loc_p_)(o, &sec, &x);
		}
		if (!sec) {
			assert(0);
		}
	}
	node = node_exact(sec, x);
	nrn_loc_point_process(pointtype, pnt, sec, node);
	return x;
}

double get_loc_point_process(void* v)
{
#if METHOD3
	extern int _method3;
#endif
	double x, nrn_arc_position();
	Point_process *pnt = (Point_process*)v;
	Section* sec;
	
	if (pnt->prop == (Prop *)0) {
		hoc_execerror("point process not located in a section", (char*)0);
	}
	if (nrn_is_artificial_[pnt->prop->type]) {
		hoc_execerror("ARTIFICIAL_CELLs are not located in a section", (char*)0);
	}
	sec = pnt->sec;
	x = nrn_arc_position(sec, pnt->node);
	hoc_level_pushsec(sec);
	return x;
}

double has_loc_point(void* v)
{
	Point_process *pnt = (Point_process*)v;
	return (pnt->sec != 0);
}

double* point_process_pointer(Point_process* pnt, Symbol* sym, int index)
{
	static double dummy;
	double* pd;
	if (pnt->prop == (Prop *)0) {
		hoc_execerror("point process not located in a section", (char*)0);
	}
	if (sym->subtype == NRNPOINTER) {
		pd = (pnt->prop->dparam)[sym->u.rng.index + index].pval;
		if (cppp_semaphore) {
			++cppp_semaphore;
			cppp_pointer = &((pnt->prop->dparam)[sym->u.rng.index + index].pval);
			pd = &dummy;
		}else if(!pd) {
#if 0
		  hoc_execerror(sym->name, "wasn't made to point to anything");
#else
		  return (double*)0;
#endif
		}
	}else{
		if (pnt->prop->ob) {		
			pd = pnt->prop->ob->u.dataspace[sym->u.rng.index].pval + index;
		}else{
			pd = pnt->prop->param + sym->u.rng.index + index;
		}
	}
/*printf("point_process_pointer %s pd=%lx *pd=%g\n", sym->name, pd, *pd);*/
	return pd;
}

void steer_point_process(void* v) /* put the right double pointer on the stack */
{
	Symbol* sym, *hoc_spop();
	int index;
	Point_process *pnt = (Point_process*)v;
	sym = hoc_spop();
	if (ISARRAY(sym)) {
		index = hoc_araypt(sym, SYMBOL);
	}else{
		index = 0;
	}
	hoc_pushpx(point_process_pointer(pnt, sym, index));
}

void nrn_cppp(void) {
	cppp_semaphore = 1;
}

void connect_point_process_pointer(void) {
	double* hoc_pxpop();
	if (cppp_semaphore != 2) {
		cppp_semaphore = 0;
		hoc_execerror("not a point process pointer", (char*)0);
	}
	cppp_semaphore = 0;
	*cppp_pointer = hoc_pxpop();
	hoc_nopop();
}

static void free_one_point(Point_process* pnt) /* must unlink from node property list also */
{
	Prop *p, *p1;

	p = pnt->prop;
	if (!p) {
		return;
	}
	if (!nrn_is_artificial_[p->type]) {
		p1 = pnt->node->prop;
		if (p1 == p) {
			pnt->node->prop = p1->next;
		}else for (; p1; p1 = p1->next) {
			if (p1->next == p) {
				p1->next = p->next;
				break;
			}
		}
	}
#if VECTORIZE
	{ extern int v_structure_change;
	  v_structure_change = 1;
	}
#endif
	if (p->param) {
		if (memb_func[p->type].destructor) {
			memb_func[p->type].destructor(p);
		}
		notify_freed_val_array(p->param, p->param_size);
		nrn_prop_data_free(p->type, p->param);
	}
	if (p->dparam) {
		nrn_prop_datum_free(p->type, p->dparam);
	}
	free((char*)p);
	pnt->prop = (Prop *)0;
	pnt->node = (Node *)0;
	if (pnt->sec) {section_unref(pnt->sec);}
	pnt->sec = (Section *)0;
}

void clear_point_process_struct(Prop* p) /* called from prop_free */
{
	Point_process* pnt;
	pnt = (Point_process*)p->dparam[1]._pvoid;
	if (pnt) {
		free_one_point(pnt);	
		if (pnt->ob) {
			if (pnt->ob->observers) {
				hoc_obj_notify(pnt->ob);
			}
			if (pnt->ob->template->observers) {
				hoc_template_notify(pnt->ob, 2);
			}
		}
	} else {
		if (p->ob) {
			hoc_obj_unref(p->ob);
		}
		if (p->param) {
			notify_freed_val_array(p->param, p->param_size);
			nrn_prop_data_free(p->type, p->param);
		}
		if (p->dparam) {
			nrn_prop_datum_free(p->type, p->dparam);
		}
		free((char*)p);
	}
}

int is_point_process(Object* ob)
{
	if (ob) {
		return ob->template->is_point_ != 0;
	}
	return 0;
}
