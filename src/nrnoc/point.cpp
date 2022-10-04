#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/point.cpp,v 1.13 1999/03/23 16:12:09 hines Exp */

/* modl description via modlreg calls point_register_mech() and
saves the pointtype as later argument to create and loc */

#include <stdlib.h>

#include "membfunc.h"
#include "nrniv_mf.h"
#include "ocnotify.h"
#include "parse_with_deps.hpp"
#include "section.h"


extern char* pnt_map;
extern Symbol** pointsym; /*list of variable symbols in s->u.ppsym[k]
                with number in s->s_varn */

extern short* nrn_is_artificial_;
extern Prop* prop_alloc(Prop**, int, Node*);

static int cppp_semaphore = 0; /* connect point process pointer semaphore */
static double** cppp_pointer;
static void free_one_point(Point_process* pnt);
static void create_artcell_prop(Point_process* pnt, short type);

Prop* nrn_point_prop_;
void (*nrnpy_o2loc_p_)(Object*, Section**, double*);
void (*nrnpy_o2loc2_p_)(Object*, Section**, double*);

void* create_point_process(int pointtype, Object* ho) {
    Point_process* pp;
    pp = (Point_process*) emalloc(sizeof(Point_process));
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
    if (ho && ho->ctemplate->steer && ifarg(1)) {
        loc_point_process(pointtype, (void*) pp);
    }
    return (void*) pp;
}

Object* nrn_new_pointprocess(Symbol* sym) {
    void* v;
    extern Object* hoc_new_object(Symbol*, void*);
    extern Object* hoc_new_opoint(int);
    Object* ob;
    extern Symlist* hoc_built_in_symlist;
    int pointtype;
    assert(sym->type == MECHANISM && memb_func[sym->subtype].is_point);
    pointtype = pnt_map[sym->subtype];
    if (memb_func[sym->subtype].hoc_mech) {
        ob = hoc_new_opoint(sym->subtype);
    } else {
        hoc_push_frame(sym, 0);
        v = create_point_process(pointtype, nullptr);
        hoc_pop_frame();
        sym = hoc_table_lookup(sym->name, hoc_built_in_symlist);
        ob = hoc_new_object(sym, v);
        ((Point_process*) v)->ob = ob;
    }
    return ob;
}

void destroy_point_process(void* v) {
    // might be NULL if error handling because of error in construction
    if (v) {
        Point_process* pp = (Point_process*) v;
        free_one_point(pp);
        free(pp);
    }
}

void nrn_loc_point_process(int pointtype, Point_process* pnt, Section* sec, Node* node) {
    extern Prop* prop_alloc_disallow(Prop * *pp, short type, Node* nd);
    extern Prop* prop_alloc(Prop**, int, Node*);
    extern Section* nrn_pnt_sec_for_need_;
    Prop* p;
    double x;

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
    } else {
        nrn_point_prop_ = (Prop*) 0;
    }
    nrn_pnt_sec_for_need_ = sec;
    if (x == 0. || x == 1.) {
        p = prop_alloc_disallow(&(node->prop), pointsym[pointtype]->subtype, node);
    } else {
        p = prop_alloc(&(node->prop), pointsym[pointtype]->subtype, node);
    }
    nrn_pnt_sec_for_need_ = (Section*) 0;

    nrn_point_prop_ = (Prop*) 0;
    if (pnt->prop) {
        pnt->prop->param = nullptr;
        pnt->prop->dparam = nullptr;
        free_one_point(pnt);
    }
    nrn_sec_ref(&pnt->sec, sec);
    pnt->node = node;
    pnt->prop = p;
    pnt->prop->dparam[0] = &NODEAREA(node);
    pnt->prop->dparam[1] = pnt;
    if (pnt->ob) {
        if (pnt->ob->observers) {
            hoc_obj_notify(pnt->ob);
        }
        if (pnt->ob->ctemplate->observers) {
            hoc_template_notify(pnt->ob, 2);
        }
    }
}

static void create_artcell_prop(Point_process* pnt, short type) {
    Prop* p = (Prop*) 0;
    nrn_point_prop_ = (Prop*) 0;
    pnt->prop = prop_alloc(&p, type, (Node*) 0);
    pnt->prop->dparam[0] = static_cast<double*>(nullptr);
    pnt->prop->dparam[1] = pnt;
    if (pnt->ob) {
        if (pnt->ob->observers) {
            hoc_obj_notify(pnt->ob);
        }
        if (pnt->ob->ctemplate->observers) {
            hoc_template_notify(pnt->ob, 2);
        }
    }
}

void nrn_relocate_old_points(Section* oldsec, Node* oldnode, Section* sec, Node* node) {
    if (oldnode)
        for (Prop *p = oldnode->prop, *pn; p; p = pn) {
            pn = p->next;
            if (memb_func[p->_type].is_point) {
                using std::get;
                auto* pnt = get<Point_process*>(p->dparam[1]);
                if (oldsec == pnt->sec) {
                    if (oldnode == node) {
                        nrn_sec_ref(&pnt->sec, sec);
                    } else {
                        nrn_loc_point_process(pnt_map[p->_type], pnt, sec, node);
                    }
                }
            }
        }
}

void nrn_seg_or_x_arg(int iarg, Section** psec, double* px) {
    if (hoc_is_double_arg(iarg)) {
        *px = chkarg(iarg, 0., 1.);
        *psec = chk_access();
    } else {
        Object* o = *hoc_objgetarg(iarg);
        *psec = (Section*) 0;
        if (nrnpy_o2loc_p_) {
            (*nrnpy_o2loc_p_)(o, psec, px);
        }
        if (!(*psec)) {
            assert(0);
        }
    }
}

void nrn_seg_or_x_arg2(int iarg, Section** psec, double* px) {
    if (hoc_is_double_arg(iarg)) {
        *px = chkarg(iarg, 0., 1.);
        *psec = chk_access();
    } else {
        Object* o = *hoc_objgetarg(iarg);
        *psec = (Section*) 0;
        if (nrnpy_o2loc2_p_) {
            (*nrnpy_o2loc2_p_)(o, psec, px);
        }
        if (!(*psec)) {
            assert(0);
        }
    }
}

double loc_point_process(int pointtype, void* v) {
    Point_process* pnt = (Point_process*) v;
    double x;
    Section* sec;
    Node* node;

    if (nrn_is_artificial_[pointsym[pointtype]->subtype]) {
        hoc_execerror("ARTIFICIAL_CELLs are not located in a section", (char*) 0);
    }
    nrn_seg_or_x_arg(1, &sec, &x);
    node = node_exact(sec, x);
    nrn_loc_point_process(pointtype, pnt, sec, node);
    return x;
}

double get_loc_point_process(void* v) {
    double x;
    Point_process* pnt = (Point_process*) v;
    Section* sec;

    if (pnt->prop == (Prop*) 0) {
        hoc_execerror("point process not located in a section", (char*) 0);
    }
    if (nrn_is_artificial_[pnt->prop->_type]) {
        hoc_execerror("ARTIFICIAL_CELLs are not located in a section", (char*) 0);
    }
    sec = pnt->sec;
    x = nrn_arc_position(sec, pnt->node);
    hoc_level_pushsec(sec);
    return x;
}

double has_loc_point(void* v) {
    Point_process* pnt = (Point_process*) v;
    return (pnt->sec != 0);
}

double* point_process_pointer(Point_process* pnt, Symbol* sym, int index) {
    static double dummy;
    double* pd;
    if (!pnt->prop) {
        if (nrn_inpython_ == 1) { /* python will handle the error */
            hoc_warning("point process not located in a section", nullptr);
            nrn_inpython_ = 2;
            return nullptr;
        } else {
            hoc_execerror("point process not located in a section", nullptr);
        }
    }
    if (sym->subtype == NRNPOINTER) {
        // In case _p_somevar is being used as an opaque void* then the active
        // member of the datum will be generic_data_handle, but its type will
        // be void and not double
        auto& datum = pnt->prop->dparam[sym->u.rng.index + index];
        if (datum.holds<double*>()) {
            using std::get;
            pd = get<double*>(datum);
        } else {
            pd = nullptr;
        }
        if (cppp_semaphore) {
            ++cppp_semaphore;
            assert(false);
            // old code: cppp_pointer = &((pnt->prop->dparam)[sym->u.rng.index + index].pval);
            pd = &dummy;
        } else if (!pd) {
            return nullptr;
        }
    } else {
        if (pnt->prop->ob) {
            pd = pnt->prop->ob->u.dataspace[sym->u.rng.index].pval + index;
        } else {
            pd = &(pnt->prop->param[sym->u.rng.index + index]);
        }
    }
    /*printf("point_process_pointer %s pd=%lx *pd=%g\n", sym->name, pd, *pd);*/
    return pd;
}

/* put the right double pointer on the stack */
void steer_point_process(void* v) {
    Symbol* sym;
    int index;
    Point_process* pnt = (Point_process*) v;
    sym = hoc_spop();
    if (ISARRAY(sym)) {
        index = hoc_araypt(sym, SYMBOL);
    } else {
        index = 0;
    }
    hoc_pushpx(point_process_pointer(pnt, sym, index));
}

void nrn_cppp(void) {
    cppp_semaphore = 1;
}

void connect_point_process_pointer(void) {
    if (cppp_semaphore != 2) {
        cppp_semaphore = 0;
        hoc_execerror("not a point process pointer", (char*) 0);
    }
    cppp_semaphore = 0;
    *cppp_pointer = hoc_pxpop();
    hoc_nopop();
}

// must unlink from node property list also
static void free_one_point(Point_process* pnt) {
    auto* p = pnt->prop;
    if (!p) {
        return;
    }
    if (!nrn_is_artificial_[p->_type]) {
        auto* p1 = pnt->node->prop;
        if (p1 == p) {
            pnt->node->prop = p1->next;
        } else
            for (; p1; p1 = p1->next) {
                if (p1->next == p) {
                    p1->next = p->next;
                    break;
                }
            }
    }
    { v_structure_change = 1; }
    if (p->param) {
        if (memb_func[p->_type].destructor) {
            memb_func[p->_type].destructor(p);
        }
        notify_freed_val_array(p->param, p->param_size);
        nrn_prop_data_free(p->_type, p->param);
    }
    if (p->dparam) {
        nrn_prop_datum_free(p->_type, p->dparam);
    }
    free(p);
    pnt->prop = (Prop*) 0;
    pnt->node = (Node*) 0;
    if (pnt->sec) {
        section_unref(pnt->sec);
    }
    pnt->sec = (Section*) 0;
}

// called from prop_free
void clear_point_process_struct(Prop* p) {
    auto* const pnt = [](auto& datum) -> Point_process* {
        if (datum.template holds<Point_process*>()) {
            using std::get;
            return get<Point_process*>(datum);
        } else {
            return nullptr;
        }
    }(p->dparam[1]);
    if (pnt) {
        free_one_point(pnt);
        if (pnt->ob) {
            if (pnt->ob->observers) {
                hoc_obj_notify(pnt->ob);
            }
            if (pnt->ob->ctemplate->observers) {
                hoc_template_notify(pnt->ob, 2);
            }
        }
    } else {
        if (p->ob) {
            hoc_obj_unref(p->ob);
        }
        if (p->param) {
            notify_freed_val_array(p->param, p->param_size);
            nrn_prop_data_free(p->_type, p->param);
        }
        if (p->dparam) {
            nrn_prop_datum_free(p->_type, p->dparam);
        }
        free(p);
    }
}

int is_point_process(Object* ob) {
    if (ob) {
        return ob->ctemplate->is_point_ != 0;
    }
    return 0;
}
