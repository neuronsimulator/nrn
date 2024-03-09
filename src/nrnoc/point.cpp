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

static double ppp_dummy = 42.0;
static int cppp_semaphore = 0; /* connect point process pointer semaphore */
static Datum* cppp_datum = nullptr;
static void free_one_point(Point_process* pnt);
static void create_artcell_prop(Point_process* pnt, short type);

Prop* nrn_point_prop_;
void (*nrnpy_o2loc_p_)(Object*, Section**, double*);
void (*nrnpy_o2loc2_p_)(Object*, Section**, double*);

void* create_point_process(int pointtype, Object* ho) {
    auto* const pp = new Point_process{};
    pp->ob = ho;
    if (nrn_is_artificial_[pointsym[pointtype]->subtype]) {
        create_artcell_prop(pp, pointsym[pointtype]->subtype);
        return pp;
    }
    if (ho && ho->ctemplate->steer && ifarg(1)) {
        loc_point_process(pointtype, (void*) pp);
    }
    return pp;
}

Object* nrn_new_pointprocess(Symbol* sym) {
    void* v;
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
        auto* const pp = static_cast<Point_process*>(v);
        free_one_point(pp);
        delete pp;
    }
}

void nrn_loc_point_process(int pointtype, Point_process* pnt, Section* sec, Node* node) {
    extern Prop* prop_alloc_disallow(Prop * *pp, short type, Node* nd);
    extern Section* nrn_pnt_sec_for_need_;
    assert(!nrn_is_artificial_[pointsym[pointtype]->subtype]);
    if (pnt->prop) {
        // Make the old Node forget about pnt->prop
        if (auto* const old_node = pnt->node; old_node) {
            auto* const p = pnt->prop;
            if (!nrn_is_artificial_[p->_type]) {
                auto* p1 = old_node->prop;
                if (p1 == p) {
                    old_node->prop = p1->next;
                } else {
                    for (; p1; p1 = p1->next) {
                        if (p1->next == p) {
                            p1->next = p->next;
                            break;
                        }
                    }
                }
            }
            v_structure_change = 1;  // needed?
        }
        // Tell the new Node about pnt->prop
        pnt->prop->next = node->prop;
        node->prop = pnt->prop;
        // Call the nrn_alloc function from the MOD file with nrn_point_prop_ set: this will skip
        // resetting parameter values and calling the CONSTRUCTOR block, but it *will* update ion
        // variables to point to the ion mechanism instance in the new Node
        prop_update_ion_variables(pnt->prop, node);
    } else {
        // Allocate a new Prop for this Point_process
        Prop* p;
        auto const x = nrn_arc_position(sec, node);
        nrn_point_prop_ = pnt->prop;
        nrn_pnt_sec_for_need_ = sec;
        // Both branches of this tell `node` about the new Prop `p`
        if (x == 0. || x == 1.) {
            p = prop_alloc_disallow(&(node->prop), pointsym[pointtype]->subtype, node);
        } else {
            p = prop_alloc(&(node->prop), pointsym[pointtype]->subtype, node);
        }
        nrn_pnt_sec_for_need_ = nullptr;
        nrn_point_prop_ = nullptr;
        pnt->prop = p;
        pnt->prop->dparam[1] = {neuron::container::do_not_search, pnt};
    }
    // Update pnt->sec with sec, unreffing the old value and reffing the new one
    nrn_sec_ref(&pnt->sec, sec);
    pnt->node = node;  // tell pnt which node it belongs to now
    pnt->prop->dparam[0] = node->area_handle();
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
                auto* pnt = p->dparam[1].get<Point_process*>();
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

neuron::container::data_handle<double> point_process_pointer(Point_process* pnt,
                                                             Symbol* sym,
                                                             int index) {
    if (!pnt->prop) {
        if (nrn_inpython_ == 1) { /* python will handle the error */
            hoc_warning("point process not located in a section", nullptr);
            nrn_inpython_ = 2;
            return {};
        } else {
            hoc_execerror("point process not located in a section", nullptr);
        }
    }
    if (sym->subtype == NRNPOINTER) {
        auto& datum = pnt->prop->dparam[sym->u.rng.index + index];
        if (cppp_semaphore) {
            ++cppp_semaphore;
            cppp_datum = &datum;  // we will store a value in `datum` later
            return neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                          &ppp_dummy};
        } else {
            // In case _p_somevar is being used as an opaque void* in a VERBATIM
            // block then then the Datum will hold a literal void* and will not
            // be convertible to double*. If instead somevar is used in the MOD
            // file and it was set from the interpreter (mech._ref_pv =
            // seg._ref_v), then the Datum will hold either data_handle<double>
            // or a literal double*. If we attempt to access a POINTER or
            // BBCOREPOINTER variable that is being used as an opaque void* from
            // the interpreter -- as is done in test_datareturn.py, for example
            // -- then we will reach this code when the datum holds a literal
            // void*. We don't know what type that pointer really refers to, so
            // in the first instance let's return nullptr in that case, not
            // void*-cast-to-double*.
            if (datum.holds<double*>()) {
                return static_cast<neuron::container::data_handle<double>>(datum);
            } else {
                return {};
            }
        }
    } else {
        if (pnt->prop->ob) {
            return neuron::container::data_handle<double>{
                pnt->prop->ob->u.dataspace[sym->u.rng.index].pval + index};
        } else {
            return pnt->prop->param_handle_legacy(sym->u.rng.index + index);
        }
    }
}

// put the right data handle on the stack
void steer_point_process(void* v) {
    auto* const pnt = static_cast<Point_process*>(v);
    auto* const sym = hoc_spop();
    auto const index = ISARRAY(sym) ? hoc_araypt(sym, SYMBOL) : 0;
    hoc_push(point_process_pointer(pnt, sym, index));
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
    *cppp_datum = hoc_pop_handle<double>();
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
    v_structure_change = 1;
    if (memb_func[p->_type].destructor) {
        memb_func[p->_type].destructor(p);
    }
    if (auto got = nrn_mech_inst_destruct.find(p->_type); got != nrn_mech_inst_destruct.end()) {
        (got->second)(p);
    }
    if (p->dparam) {
        nrn_prop_datum_free(p->_type, p->dparam);
    }
    delete p;
    pnt->prop = (Prop*) 0;
    pnt->node = (Node*) 0;
    if (pnt->sec) {
        section_unref(pnt->sec);
    }
    pnt->sec = (Section*) 0;
}

// called from prop_free
void clear_point_process_struct(Prop* p) {
    auto* const pnt = p->dparam[1].get<Point_process*>();
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
        if (p->dparam) {
            nrn_prop_datum_free(p->_type, p->dparam);
        }
        delete p;
    }
}

int is_point_process(Object* ob) {
    if (ob) {
        return ob->ctemplate->is_point_ != 0;
    }
    return 0;
}
