#include <../../nrnconf.h>
#undef check
#include <InterViews/resource.h>
#include <ctype.h>
#include "membfunc.h"
#include "nrnoc2iv.h"
#include "nrniv_mf.h"

#include "parse.hpp"
extern int point_reg_helper(Symbol*);
extern Object* hoc_newobj1(Symbol*, int);
extern Symlist* hoc_symlist;
extern void hoc_unlink_symbol(Symbol*, Symlist*);
extern void hoc_link_symbol(Symbol*, Symlist*);
extern void nrn_loc_point_process(int, Point_process*, Section*, Node*);
extern char* pnt_map;
extern Symbol** pointsym;
extern Prop* nrn_point_prop_;
extern void print_symlist(const char*, Symlist*);

extern void make_mechanism();
extern void make_pointprocess();
extern void hoc_construct_point(Object*, int);
extern Object* hoc_new_opoint(int);

static Object* last_created_pp_ob_;
static bool skip_;

static const char** make_m(bool, int&, Symlist*, char*, char*);

class HocMech {
  public:
    Symbol* mech;        // template name
    Symbol* initial;     // INITIAL  proc initial()
    Symbol* after_step;  // SOLVE ... METHOD after_cvode;proc after_step()
    Symlist* slist;      // point process range variables.
};

static void check(const char* s) {
    if (hoc_lookup(s)) {
        hoc_execerror(s, "already exists");
    }
}

static void check_list(const char* s, Symlist* sl) {
    if (hoc_table_lookup(s, sl)) {
        hoc_execerror(s, "already exists");
    }
}

void hoc_construct_point(Object* ob, int narg) {
    if (skip_) {
        // printf("skipped hoc_construct_point\n");
        return;
    }
    // printf("%s is a pointprocess\n", hoc_object_name(ob));
    int type = ob->ctemplate->symtable->last->subtype;
    int ptype = pnt_map[type];
    Point_process* pnt = (Point_process*) create_point_process(ptype, ob);
    ob->u.dataspace[ob->ctemplate->dataspace_size - 1]._pvoid = (void*) pnt;
    assert(last_created_pp_ob_ == NULL);
    last_created_pp_ob_ = ob;
    if (narg > 0) {
        auto const x = hoc_look_inside_stack<double>(narg - 1);
        // printf("x=%g\n", x);
        Section* sec = chk_access();
        Node* nd = node_exact(sec, x);
        // printf("ptype=%d pnt=%p %s nd=%p\n", ptype, pnt, secname(sec), nd);
        // printf("type=%d pointsym=%p\n", type, pointsym[ptype]);
        // printf("type=%d from pointsym %s = %d\n", type, pointsym[ptype]->name,
        // pointsym[ptype]->subtype);

        nrn_loc_point_process(ptype, pnt, sec, nd);
    }
}

Point_process* ob2pntproc_0(Object* ob) {
    Point_process* pp;
    if (ob->ctemplate->steer) {
        pp = (Point_process*) ob->u.this_pointer;
    } else {
        pp = (Point_process*) ob->u.dataspace[ob->ctemplate->dataspace_size - 1]._pvoid;
    }
    return pp;
}

Point_process* ob2pntproc(Object* ob) {
    Point_process* pp = ob2pntproc_0(ob);
    if (!pp || !pp->prop) {
        hoc_execerror(hoc_object_name(ob), "point process not located in a section");
    }
    return pp;
}

int special_pnt_call(Object* ob, Symbol* sym, int narg) {
    char* name = sym->name;
    if (strcmp(name, "loc") == 0) {
        int type = ob->ctemplate->symtable->last->subtype;
        int ptype = pnt_map[type];
        if (narg != 1) {
            hoc_execerror("no argument", 0);
        }
        auto const x = hoc_look_inside_stack<double>(narg - 1);
        Section* sec = chk_access();
        Node* node = node_exact(sec, x);
        nrn_loc_point_process(ptype, ob2pntproc(ob), sec, node);
        hoc_pushx(x);
        return 1;
    } else if (strcmp(name, "has_loc") == 0) {
        Point_process* p = ob2pntproc(ob);
        hoc_pushx(double(p != NULL && p->sec != NULL));
        return 1;
    } else if (strcmp(name, "get_loc") == 0) {
        hoc_pushx(get_loc_point_process(ob2pntproc(ob)));
        return 1;
    } else {
        return 0;
    }
}

static void alloc_mech(Prop* p) {
    Symbol* mech = ((HocMech*) memb_func[p->_type].hoc_mech)->mech;
    p->ob = hoc_newobj1(mech, 0);
    // printf("alloc_mech %s\n", hoc_object_name(p->ob));
}

static void alloc_pnt(Prop* p) {
    // this is complex because it can be called either before or
    // after the hoc object has been created. And so there
    // must be communication between alloc_pnt and hoc_construct_point.
    // need the prop->dparam[1]._pvoid
    if (nrn_point_prop_) {
        p->dparam = nrn_point_prop_->dparam;
        p->ob = nrn_point_prop_->ob;
        // printf("p->ob comes from nrn_point_prop_ %s\n", hoc_object_name(p->ob));
    } else {
        nrn_prop_datum_alloc(p->_type, 2, p);
        if (last_created_pp_ob_) {
            p->ob = last_created_pp_ob_;
            // printf("p->ob comes from last_created %s\n", hoc_object_name(p->ob));
        } else {
            Symbol* mech = ((HocMech*) memb_func[p->_type].hoc_mech)->mech;
            skip_ = true;
            // printf("p->ob comes from hoc_newobj1 %s\n", mech->name);
            p->ob = hoc_newobj1(mech, 0);
            skip_ = false;
        }
    }
    last_created_pp_ob_ = NULL;
}

Object* hoc_new_opoint(int type) {
    HocMech* hm = (HocMech*) memb_func[type].hoc_mech;
    return hoc_newobj1(hm->mech, 0);
}

static void call(Symbol* s, Node* nd, Prop* p) {
    Section* sec = nd->sec;
    Object* ob = p->ob;
    double x = nrn_arc_position(sec, nd);
    nrn_pushsec(sec);
    hoc_pushx(x);
    // printf("hoc_call_objfunc %s ob=%s\n", s->name, hoc_object_name(ob));
    hoc_call_objfunc(s, 1, ob);
    nrn_popsec();
}

static void initial(neuron::model_sorted_token const&, NrnThread* nt, Memb_list* ml, int type) {
    HocMech* hm = (HocMech*) memb_func[type].hoc_mech;
    int i, cnt = ml->nodecount;
    for (i = 0; i < cnt; ++i) {
        call(hm->initial, ml->nodelist[i], ml->prop[i]);
    }
}

static void after_step(neuron::model_sorted_token const&, NrnThread* nt, Memb_list* ml, int type) {
    HocMech* hm = (HocMech*) memb_func[type].hoc_mech;
    int i, cnt = ml->nodecount;
    for (i = 0; i < cnt; ++i) {
        call(hm->after_step, ml->nodelist[i], ml->prop[i]);
    }
}

// note that an sgi CC complained about the alloc token not being interpretable
// as std::alloc so we changed to hm_alloc
static HocMech* common_register(const char** m,
                                Symbol* classsym,
                                Symlist* slist,
                                void(hm_alloc)(Prop*),
                                int& type) {
    nrn_cur_t cur{};
    nrn_init_t initialize{};
    nrn_jacob_t jacob{};
    nrn_state_t stat{};
    HocMech* hm = new HocMech();
    hm->slist = NULL;
    hm->mech = classsym;
    hm->initial = hoc_table_lookup("initial", slist);
    hm->after_step = hoc_table_lookup("after_step", slist);
    if (hm->initial) {
        initialize = initial;
    }
    if (hm->after_step) {
        stat = after_step;
    }
    register_mech(m, hm_alloc, cur, jacob, stat, initialize, -1, 0);
    type = nrn_get_mechtype(m[1]);
    hoc_register_cvode(type, nullptr, nullptr, nullptr, nullptr);
    memb_func[type].hoc_mech = hm;
    return hm;
}

void make_mechanism() {
    char buf[256];
    int i, cnt;
    Symbol* sp;
    char* mname = gargstr(1);
    // printf("mname=%s\n", mname);
    check(mname);
    char* classname = gargstr(2);
    // printf("classname=%s\n", classname);
    char* parnames = NULL;
    if (ifarg(3)) {
        parnames = new char[strlen(gargstr(3)) + 1];
        strcpy(parnames, gargstr(3));
    }
    // if(parnames) printf("parnames=%s\n", parnames);
    Symbol* classsym = hoc_lookup(classname);
    if (classsym->type != TEMPLATE) {
        hoc_execerror(classname, "not a template");
    }
    cTemplate* tp = classsym->u.ctemplate;
    Symlist* slist = tp->symtable;
    const char** m = make_m(true, cnt, slist, mname, parnames);

    common_register(m, classsym, slist, alloc_mech, i);

    for (sp = slist->first; sp; sp = sp->next) {
        if (sp->type == VAR && sp->cpublic) {
            Sprintf(buf, "%s_%s", sp->name, m[1]);
            Symbol* sp1 = hoc_lookup(buf);
            sp1->u.rng.index = sp->u.oboff;
        }
    }
    for (i = 0; i < cnt; ++i) {
        if (m[i]) {
            delete[] m[i];
        }
    }
    delete[] m;
    delete[] parnames;
    hoc_retpushx(1.);
}

void make_pointprocess() {
    char buf[256];
    int i, cnt, type, ptype;
    Symbol *sp, *s2;
    char* classname = gargstr(1);
    // printf("classname=%s\n", classname);
    char* parnames = NULL;
    if (ifarg(2)) {
        parnames = new char[strlen(gargstr(2)) + 1];
        strcpy(parnames, gargstr(2));
    }
    // if(parnames) printf("parnames=%s\n", parnames);
    Symbol* classsym = hoc_lookup(classname);
    if (classsym->type != TEMPLATE) {
        hoc_execerror(classname, "not a template");
    }
    cTemplate* tp = classsym->u.ctemplate;
    Symlist* slist = tp->symtable;
    // increase the dataspace by 1 void pointer. The last element
    // is where the Point_process pointer can be found and when
    // the object dataspace is freed, so is the Point_process.
    if (tp->count > 0) {
        fprintf(stderr, "%d object(s) of type %s already exist.\n", tp->count, classsym->name);
        hoc_execerror("Can't make a template into a PointProcess when instances already exist", 0);
    }
    ++tp->dataspace_size;
    const char** m = make_m(false, cnt, slist, classsym->name, parnames);

    check_list("loc", slist);
    check_list("get_loc", slist);
    check_list("has_loc", slist);
    // so far we need only the name and type
    sp = hoc_install("loc", FUNCTION, 0., &slist);
    sp->cpublic = 1;
    sp = hoc_install("get_loc", FUNCTION, 0., &slist);
    sp->cpublic = 1;
    sp = hoc_install("has_loc", FUNCTION, 0., &slist);
    sp->cpublic = 1;

    Symlist* slsav = hoc_symlist;
    hoc_symlist = NULL;
    HocMech* hm = common_register(m, classsym, slist, alloc_pnt, type);
    hm->slist = hoc_symlist;
    hoc_symlist = slsav;
    s2 = hoc_table_lookup(m[1], hm->slist);
    assert(s2->subtype == type);
    //	type = s2->subtype;
    ptype = point_reg_helper(s2);
    // printf("type=%d pointtype=%d %s %p\n", type, ptype, s2->name, s2);
    classsym->u.ctemplate->is_point_ = ptype;

    // classsym->name is already in slist as an undef, Remove it and
    // move s2 out of HocMech->slist and into slist.
    // That is the one with the u.ppsym.
    // The only reason it needs to be in slist is to find the
    // mechanims type. And it needs to be LAST in that list.
    // The only reason for the u.ppsym is for ndatclas.cpp and we
    // need to fill those symbols with oboff.
    sp = hoc_table_lookup(classsym->name, slist);
    hoc_unlink_symbol(sp, slist);
    hoc_unlink_symbol(s2, hm->slist);
    hoc_link_symbol(s2, slist);
    hoc_link_symbol(sp, hm->slist);  // just so it isn't counted as leakage
    for (i = 0; i < s2->s_varn; ++i) {
        Symbol* sp = hoc_table_lookup(s2->u.ppsym[i]->name, slist);
        s2->u.ppsym[i]->cpublic = 2;
        s2->u.ppsym[1]->u.oboff = sp->u.oboff;
    }
    for (i = 0; i < cnt; ++i) {
        if (m[i]) {
            delete[] m[i];
        }
    }
    delete[] m;
    if (parnames) {
        delete[] parnames;
    }
    hoc_retpushx(1.);
}

static const char** make_m(bool suffix, int& cnt, Symlist* slist, char* mname, char* parnames) {
    char buf[256];
    char* cc;
    Symbol* sp;
    int i, imax;
    cnt = 0;
    for (sp = slist->first; sp; sp = sp->next) {
        if (sp->type == VAR) {
            ++cnt;
            // printf ("cnt=%d |%s|\n", cnt, sp->name);
        }
    }
    cnt += 6;
    // printf("cnt=%d\n", cnt);
    const char** m = new const char*[cnt];
    for (i = 0; i < cnt; ++i) {  // not all space is used since some variables
        m[i] = 0;                // are not public
    }
    i = 0;
    cc = new char[2];
    strcpy(cc, "0");
    m[i] = cc;
    // printf("m[%d]=%s\n", i, m[i]);
    ++i;
    cc = new char[strlen(mname) + 1];
    strcpy(cc, mname);
    m[i] = cc;
    // printf("m[%d]=%s\n", i, m[i]);
    ++i;

    // the remaining part of m must be a 0 separated list of
    // CONSTANT (actually PARAMETER), ASSIGNED, STATE
    // Normally these are contiguous in the p array.
    // At any rate the param array is not the normal representation
    // of scalar and array values in the object dataspace.
    // Since the object dataspace representtion is much more flexible
    // it will be the reponsibility of the allocation routine to
    // make sure that the style
    //	&(m->param[sym->u.rng.index])
    // has to actually execute the variant
    //	hoc_objectdata[sym->u.oboff].pval
    // when assigning and setting from the var_suffix form.

    // the PARAMETER names are space separated in parnames.
    char *cp, *csp = NULL;
    if (parnames)
        for (cp = parnames; cp && *cp; cp = csp) {
            csp = strchr(cp, ' ');
            if (csp) {
                *csp = '\0';
                ++csp;
                if (!isalpha(*csp)) {
                    hoc_execerror("Must be a space separated list of names\n", gargstr(3));
                }
            }
            if (suffix) {
                Sprintf(buf, "%s_%s", cp, m[1]);
                check(buf);
            } else {
                Sprintf(buf, "%s", cp);
            }
            if (!(sp = hoc_table_lookup(cp, slist)) || !sp->cpublic || !(sp->type == VAR)) {
                hoc_execerror(cp, "is not a public variable");
            }
            auto cc_size = strlen(cp) + strlen(m[1]) + 20;
            cc = new char[cc_size];
            // above 20 give enough room for _ and possible array size
            imax = hoc_total_array_data(sp, 0);
            if (imax > 1) {
                std::snprintf(cc, cc_size, "%s[%d]", buf, imax);
            } else {
                std::snprintf(cc, cc_size, "%s", buf);
            }
            m[i] = cc;
            // printf("m[%d]=%s\n", i, m[i]);
            ++i;
        }
    int j, jmax = i;
    m[i++] = 0;  // CONSTANT ASSIGNED separator
                 // printf("m[%d] = NULL\n", i);
    for (sp = slist->first; sp; sp = sp->next) {
        if (sp->type == VAR && sp->cpublic) {
            if (suffix) {
                Sprintf(buf, "%s_%s", sp->name, m[1]);
                check(buf);
            } else {
                Sprintf(buf, "%s", sp->name);
            }
            bool b = false;
            for (j = 1; j < jmax; ++j) {
                if (strstr(m[j], buf)) {
                    b = true;  // already a PARAMETER
                    break;
                }
            }
            if (b) {
                continue;
            }
            auto cc_size = strlen(buf) + 20;
            cc = new char[cc_size];
            // above 20 give enough room for possible array size
            imax = hoc_total_array_data(sp, 0);
            if (imax > 1) {
                std::snprintf(cc, cc_size, "%s[%d]", buf, imax);
            } else {
                std::snprintf(cc, cc_size, "%s", buf);
            }
            m[i] = cc;
            // printf("m[%d]=%s\n", i, m[i]);
            ++i;
        }
    }
    // printf("m[%d] = NULL\n", i);
    m[i++] = 0;  // ASSIGNED STATE separator
                 // printf("m[%d] = NULL\n", i);
    m[i++] = 0;  // STATE NRNPOINTER separator
                 // printf("m[%d] = NULL\n", i);
    m[i++] = 0;  // end
    return m;
}
