
#include <../../nrnconf.h>

#include <stdio.h>
#include <string.h>

#if HAVE_IV
#include "secbrows.h"
#include "ivoc.h"
#endif
#include "nrniv_mf.h"
#include "nrnoc2iv.h"
#include "nrnpy.h"
#include "nrnmenu.h"
#include "classreg.h"
#include "gui-redirect.h"

typedef void (*ReceiveFunc)(Point_process*, double*, double);
extern int hoc_return_type_code;
// from nrnoc
#include "membfunc.h"
#include "parse.hpp"
extern Symlist* hoc_built_in_symlist;
extern Symbol** pointsym;
extern ReceiveFunc* pnt_receive;
extern int nrn_has_net_event_cnt_;
extern int* nrn_has_net_event_;
extern short* nrn_is_artificial_;
extern int node_index(Section*, double);
extern char* pnt_map;
extern void nrn_parent_info(Section*);

// to nrnoc
void nrnallsectionmenu();
void nrnallpointmenu();
void nrnsecmenu();
void nrnglobalmechmenu();
void nrnmechmenu();
void nrnpointmenu();

int (*nrnpy_ob_is_seg)(Object*);

#if HAVE_IV
static void pnodemenu(Prop* p1, double, int type, const char* path, MechSelector* = NULL);
static void mech_menu(Prop* p1, double, int type, const char* path, MechSelector* = NULL);
static void point_menu(Object*, int);
#endif

void nrnallsectionmenu() {
    TRY_GUI_REDIRECT_DOUBLE("nrnallsectionmenu", NULL);

#if HAVE_IV
    IFGUI
    SectionBrowser::make_section_browser();
    ENDGUI
#endif

    hoc_retpushx(1.);
}

void nrnsecmenu() {
    TRY_GUI_REDIRECT_DOUBLE("nrnsecmenu", NULL);
#if HAVE_IV
    IFGUI
    double x;
    Section* sec = NULL;
    if (hoc_is_object_arg(1)) {  // x = -1 not allowed
        nrn_seg_or_x_arg(1, &sec, &x);
        nrn_pushsec(sec);
    } else {
        x = chkarg(1, -1., 1.);
    }
    section_menu(x, (int) chkarg(2, 1., 3.));
    if (sec) {
        nrn_popsec();
    }
    ENDGUI
#endif
    hoc_retpushx(1.);
}

#ifdef ultrix
char* strstr(const char*, const char*);
#endif

static bool has_globals(const char* name) {
    Symbol* sp;
    char suffix[100];
    Sprintf(suffix, "_%s", name);
    for (sp = hoc_built_in_symlist->first; sp; sp = sp->next) {
        if (sp->type == VAR && sp->subtype == USERDOUBLE && strstr(sp->name, suffix)) {
            return true;
        }
    }
    return false;
}

void nrnglobalmechmenu() {
    TRY_GUI_REDIRECT_DOUBLE("nrnglobalmechmenu", NULL);
#if HAVE_IV
    IFGUI
    Symbol* sp;
    char* s;
    char buf[200];
    char suffix[100];
    if (!ifarg(1)) {
        hoc_ivmenu("Mechanisms (Globals)");
        for (sp = hoc_built_in_symlist->first; sp; sp = sp->next) {
            if (sp->type == MECHANISM && sp->subtype != MORPHOLOGY && has_globals(sp->name)) {
                Sprintf(buf, "nrnglobalmechmenu(\"%s\")", sp->name);
                hoc_ivbutton(sp->name, buf);
            }
        }
        hoc_ivmenu(0);
        hoc_retpushx(1.);
        return;
    }
    char* name = gargstr(1);
    Sprintf(suffix, "_%s", name);
    if (ifarg(2) && *getarg(2) == 0.) {
        int cnt = 0;
        for (sp = hoc_built_in_symlist->first; sp; sp = sp->next) {
            if (sp->type == VAR && sp->subtype == USERDOUBLE &&
                (s = strstr(sp->name, suffix)) != 0 && s[strlen(suffix)] == '\0') {
                ++cnt;
            }
        }
        hoc_retpushx(double(cnt));
        return;
    }
    Sprintf(buf, "%s (Globals)", name);
    hoc_ivpanel(buf);
    for (sp = hoc_built_in_symlist->first; sp; sp = sp->next) {
        if (sp->type == VAR && sp->subtype == USERDOUBLE && (s = strstr(sp->name, suffix)) != 0 &&
            s[strlen(suffix)] == '\0') {
            if (ISARRAY(sp)) {
                char n[50];
                int i;
                Arrayinfo* a = sp->arayinfo;
                for (i = 0; i < a->sub[0]; i++) {
                    if (i > 5)
                        break;
                    Sprintf(buf, "%s[%d]", sp->name, i);
                    Sprintf(n, "%s[%d]", sp->name, i);
                    hoc_ivpvalue(n, hoc_val_handle(buf), false, sp->extra);
                }
            } else {
                hoc_ivvalue(sp->name, sp->name, 1);
            }
        }
    }
    hoc_ivpanelmap();
    ENDGUI
#endif
    hoc_retpushx(1.);
}

void nrnmechmenu() {
    hoc_retpushx(1.);
}

#if HAVE_IV
void section_menu(double x1, int type, MechSelector* ms) {
    char buf[200];
    const char* name;
    Section* sec;
    Prop* p;
    Node* node;
    double x;
    String btype;
    CopyString sname;

    switch (type) {
    case nrnocCONST:
        btype = "(Parameters)";
        break;
    case STATE:
        btype = "(States)";
        break;
    case 2:
        btype = "(Assigned)";
        break;
    }

    sec = chk_access();
    name = secname(sec);

    if (x1 >= 0) {
        node = node_exact(sec, x1);
        x = nrn_arc_position(sec, node);
        Sprintf(buf, "%s(%g) %s", name, x, btype.string());
    } else {
        Sprintf(buf, "%s(0 - 1) %s", name, btype.string());
        node = sec->pnode[0];
        x = nrn_arc_position(sec, node);
        sname = hoc_section_pathname(sec);
        // printf("returned %s\n", sname.string());
    }
    hoc_ivpanel(buf);
    hoc_ivlabel(buf);
    if (type == nrnocCONST) {
        if (x1 < 0) {
            Sprintf(buf, "nseg = %d", sec->nnode - 1);
            hoc_ivlabel(buf);
            Sprintf(buf, "%s.L", sname.string());
            if (sec->npt3d) {
                hoc_ivvaluerun("L", buf, "define_shape()", 1);
            } else {
                hoc_ivvalue("L", buf, 1);
            }
            Sprintf(buf, "%s.Ra += 0", sname.string());
            hoc_ivpvaluerun("Ra",
                            neuron::container::data_handle<double>{
                                neuron::container::do_not_search,
                                &(sec->prop->dparam[7].literal_value<double>())},
                            buf,
                            1,
                            0,
                            hoc_var_extra("Ra"));
            p = sec->prop;
            if (p->dparam[4].literal_value<double>() != 1) {
                hoc_ivpvaluerun("Rall",
                                neuron::container::data_handle<double>{
                                    neuron::container::do_not_search,
                                    &(sec->prop->dparam[4].literal_value<double>())},
                                "diam_changed = 1",
                                1,
                                0,
                                hoc_var_extra("rallbranch"));
            }
        }
    } else {
        if (x1 < 0) {
            Sprintf(buf, "%s.%s", sname.string(), "v");
            hoc_ivvalue("v", buf);
        } else {
            Sprintf(buf, "v(%g)", x);
            hoc_ivpvalue("v", hoc_val_handle(buf), false, hoc_lookup("v")->extra);
        }
    }

    p = node->prop;
    if (x1 < 0) {
        pnodemenu(p, x, type, sname.string(), ms);
    } else {
        pnodemenu(p, x, type, 0, ms);
    }
    hoc_ivpanelmap();
}

static void pnodemenu(Prop* p1, double x, int type, const char* path, MechSelector* ms) {
    if (!p1) {
        return;
    }
    pnodemenu(p1->next, x, type, path, ms); /*print in insert order*/
    if (memb_func[p1->_type].is_point) {
        return;
    } else {
        mech_menu(p1, x, type, path, ms);
    }
}
#endif

#if HAVE_IV
static bool nrn_is_const(const char* path, const char* name) {
    char buf[256];
    Sprintf(buf,
            "%s for (hoc_ac_) if (hoc_ac_ > 0 && hoc_ac_ < 1) if (%s(hoc_ac_) != %s(.5)) {hoc_ac_ "
            "= 0  break}\n",
            path,
            name,
            name);
    Oc oc;
    oc.run(buf);
    return (hoc_ac_ != 0.);
}
#endif

#if HAVE_IV
static void mech_menu(Prop* p1, double x, int type, const char* path, MechSelector* ms) {
    Symbol *sym, *vsym;
    int i, j;
    char buf[200];
    bool deflt;

    if (ms && !ms->is_selected(p1->_type)) {
        return;
    }
    if (type == nrnocCONST) {
        deflt = true;
    } else {
        deflt = false;
    }
    sym = memb_func[p1->_type].sym;
    if (sym->s_varn) {
        for (j = 0; j < sym->s_varn; j++) {
            vsym = sym->u.ppsym[j];
            if (nrn_vartype(vsym) == type) {
                if (vsym->type == RANGEVAR) {
                    if (ISARRAY(vsym)) {
                        char n[50];
                        Arrayinfo* a = vsym->arayinfo;
                        for (i = 0; i < a->sub[0]; i++) {
                            if (i > 5)
                                break;
                            Sprintf(n, "%s[%d]", vsym->name, i);
                            if (path) {
                                if (nrn_is_const(path, n)) {
                                    Sprintf(buf, "%s.%s", path, n);
                                    hoc_ivvalue(n, buf, deflt);
                                } else {
                                    Sprintf(buf, "%s is not constant", n);
                                    hoc_ivlabel(buf);
                                }
                            } else {
                                Sprintf(buf, "%s[%d](%g)", vsym->name, i, x);
                                hoc_ivpvalue(n, hoc_val_handle(buf), false, vsym->extra);
                            }
                        }
                    } else {
                        if (path) {
                            if (nrn_is_const(path, vsym->name)) {
                                Sprintf(buf, "%s.%s", path, vsym->name);
                                hoc_ivvalue(vsym->name, buf, deflt);
                            } else {
                                Sprintf(buf, "%s is not constant", vsym->name);
                                hoc_ivlabel(buf);
                            }
                        } else {
                            Sprintf(buf, "%s(%g)", vsym->name, x);
                            if (p1->_type == MORPHOLOGY) {
                                Section* sec = chk_access();
                                char buf2[200];
                                Sprintf(buf2, "%s.Ra += 0", secname(sec));
                                hoc_ivpvaluerun(
                                    vsym->name, hoc_val_handle(buf), buf2, 1, 0, vsym->extra);
                            } else {
                                hoc_ivpvalue(vsym->name, hoc_val_handle(buf), deflt, vsym->extra);
                            }
                        }
                    }
                }
            }
        }
    }
}
#endif

void nrnallpointmenu() {
    TRY_GUI_REDIRECT_DOUBLE("nrnallpointmenu", NULL);
#if HAVE_IV
    IFGUI
    int i;
    double x = n_memb_func - 1;
    Symbol *sp, *psym;
    char buf[200];
    hoc_Item* q;

    if (!ifarg(1)) {
        hoc_ivmenu("Point Processes");
        for (i = 1; (sp = pointsym[i]) != (Symbol*) 0; i++) {
            Sprintf(buf, "nrnallpointmenu(%d)", i);
            hoc_ivbutton(sp->name, buf);
        }
        hoc_ivmenu(0);
        hoc_retpushx(1.);
        return;
    }

    i = (int) chkarg(1, 0., x);
    if ((psym = pointsym[i]) != (Symbol*) 0) {
        hoc_ivpanel(psym->name);
        sp = hoc_table_lookup(psym->name, hoc_built_in_symlist);
        assert(sp && sp->type == TEMPLATE);

        bool locmenu = false;
        ITERATE(q, sp->u.ctemplate->olist) {  // are there any
            hoc_ivmenu("locations");
            locmenu = true;
            break;
        }

        bool are_globals = false;
        char suffix[100];
        Sprintf(suffix, "_%s", sp->name);
        for (Symbol* stmp = hoc_built_in_symlist->first; stmp; stmp = stmp->next) {
            if (stmp->type == VAR && stmp->subtype == USERDOUBLE && strstr(stmp->name, suffix)) {
                are_globals = true;
                break;
            }
        }

        ITERATE(q, sp->u.ctemplate->olist) {
            Object* ob = OBJ(q);
            Point_process* pp = ob2pntproc(ob);
            if (pp->sec) {
                Sprintf(buf, "nrnpointmenu(%p)", ob);
                hoc_ivbutton(sec_and_position(pp->sec, pp->node), buf);
            }
        }
        if (locmenu) {
            hoc_ivmenu(0);
        }
        if (are_globals) {
            Sprintf(buf, "nrnglobalmechmenu(\"%s\")", psym->name);
            hoc_ivbutton("Globals", buf);
        }
        hoc_ivpanelmap();
    }
    ENDGUI
#endif
    hoc_retpushx(1.);
}

void nrnpointmenu() {
    TRY_GUI_REDIRECT_DOUBLE("nrnpointmenu", NULL);
#if HAVE_IV
    IFGUI
    Object* ob;
    if (hoc_is_object_arg(1)) {
        ob = *hoc_objgetarg(1);
    } else {
        ob = (Object*) ((size_t) (*getarg(1)));
    }
    Symbol* sym = hoc_table_lookup(ob->ctemplate->sym->name, ob->ctemplate->symtable);
    if (!sym || sym->type != MECHANISM || !memb_func[sym->subtype].is_point) {
        hoc_execerror(ob->ctemplate->sym->name, "not a point process");
    }
    int make_label = 1;
    if (ifarg(2)) {
        make_label = int(chkarg(2, -1., 1.));
    }
    point_menu(ob, make_label);
    ENDGUI
#endif
    hoc_retpushx(1.);
}

#if HAVE_IV
static void point_menu(Object* ob, int make_label) {
    Point_process* pp = ob2pntproc(ob);
    int k, m;
    Symbol *psym, *vsym;
    char buf[200];
    bool deflt;

    if (pp->sec) {
        Sprintf(buf, "%s at ", hoc_object_name(ob));
        strcat(buf, sec_and_position(pp->sec, pp->node));
    } else {
        Sprintf(buf, "%s", hoc_object_name(ob));
    }
    hoc_ivpanel(buf);


    if (make_label == 1) {
        hoc_ivlabel(buf);
    } else if (make_label == 0) {
        hoc_ivlabel(hoc_object_name(ob));
    } else if (make_label == -1) {  // i.e. do neither
        k = 0;
    }
    psym = pointsym[pnt_map[pp->prop->_type]];

#if 0
        switch (type) {
        case nrnocCONST:
                Sprintf(buf,"%s[%d] (Parameters)", psym->name, j);
                break;
        case STATE:
                Sprintf(buf,"%s[%d] (States)", psym->name, j);
                break;
        case 2:
                Sprintf(buf,"%s[%d] (Assigned)", psym->name, j);
                break;
        }
#endif

    if (psym->s_varn) {
        for (k = 0; k < psym->s_varn; k++) {
            vsym = psym->u.ppsym[k];
            int vartype = nrn_vartype(vsym);
            if (vartype == NMODLRANDOM) {  // skip
                continue;
            }
            if (vartype == nrnocCONST) {
                deflt = true;

#if defined(MikeNeubig)
                deflt = false;
#endif  // end of hack
            } else {
                deflt = false;
            }
            if (ISARRAY(vsym)) {
                Arrayinfo* a = vsym->arayinfo;
                for (m = 0; m < vsym->arayinfo->sub[0]; m++) {
                    if (m > 5)
                        break;
                    Sprintf(buf, "%s[%d]", vsym->name, m);
                    auto pd = point_process_pointer(pp, vsym, m);
                    if (pd) {
                        hoc_ivpvalue(buf, pd, deflt, vsym->extra);
                    }
                }
            } else {
                hoc_ivpvalue(vsym->name, point_process_pointer(pp, vsym, 0), deflt, vsym->extra);
            }
        }
    }

    hoc_ivpanelmap();
}
#endif

//-----------------------
// MechanismStandard
static Symbol* ms_class_sym_;

static double ms_panel(void* v) {
    TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("MechanismStandard.panel", ms_class_sym_, v);
#if HAVE_IV
    IFGUI
    char* label = NULL;
    if (ifarg(1)) {
        label = gargstr(1);
    }
    ((MechanismStandard*) v)->panel(label);
    ENDGUI
#endif
    return 0.;
}
static double ms_action(void* v) {
    char* a = 0;
    Object* pyact = NULL;
    if (ifarg(1)) {
        if (hoc_is_str_arg(1)) {
            a = gargstr(1);
        } else {
            pyact = *hoc_objgetarg(1);
        }
    }
    ((MechanismStandard*) v)->action(a, pyact);
    return 0.;
}

static double ms_out(void* v) {
    MechanismStandard* m = (MechanismStandard*) v;
    if (ifarg(1)) {
        if (hoc_is_double_arg(1)) {
            double x = chkarg(1, 0, 1);
            m->out(chk_access(), x);
        } else {
            Object* o = *hoc_objgetarg(1);
            if (is_obj_type(o, "MechanismStandard")) {
                m->out((MechanismStandard*) o->u.this_pointer);
            } else if (is_point_process(o)) {
                m->out(ob2pntproc(o));
            } else if (nrnpy_ob_is_seg && (*nrnpy_ob_is_seg)(o)) {
                double x;
                Section* sec;
                nrn_seg_or_x_arg(1, &sec, &x);
                m->out(sec, x);
            } else {
                hoc_execerror("Object arg must be MechanismStandard or a Point Process, not",
                              hoc_object_name(o));
            }
        }
    } else {
        m->out(chk_access());
    }
    return 0.;
}

static double ms_in(void* v) {
    MechanismStandard* m = (MechanismStandard*) v;
    if (ifarg(1)) {
        if (hoc_is_double_arg(1)) {
            double x = chkarg(1, 0, 1);
            m->in(chk_access(), x);
        } else {
            Object* o = *hoc_objgetarg(1);
            if (is_obj_type(o, "MechanismStandard")) {
                m->in((MechanismStandard*) o->u.this_pointer);
            } else if (is_point_process(o)) {
                m->in(ob2pntproc(o));
            } else if (nrnpy_ob_is_seg && (*nrnpy_ob_is_seg)(o)) {
                double x;
                Section* sec;
                nrn_seg_or_x_arg(1, &sec, &x);
                m->in(sec, x);
            } else {
                hoc_execerror(
                    "Object arg must be MechanismStandard or a Point Process or a nrn.Segment, not",
                    hoc_object_name(o));
            }
        }
    } else {
        m->in(chk_access());
    }
    return 0.;
}

static double ms_set(void* v) {
    int i = 0;
    if (ifarg(3)) {  // array index
        i = int(*getarg(3));
    }
    ((MechanismStandard*) v)->set(gargstr(1), *getarg(2), i);
    return 0.;
}
static double ms_get(void* v) {
    int i = 0;
    if (ifarg(2)) {  // array index
        i = int(*getarg(2));
    }
    return ((MechanismStandard*) v)->get(gargstr(1), i);
}
static double ms_count(void* v) {
    hoc_return_type_code = 1;
    return ((MechanismStandard*) v)->count();
}
static double ms_name(void* v) {
    const char* n;
    int rval = 0;
    MechanismStandard* ms = (MechanismStandard*) v;
    if (ifarg(2)) {
        n = ms->name((int) chkarg(2, 0, ms->count() - 1), rval);
    } else {
        n = ms->name();
    }
    hoc_assign_str(hoc_pgargstr(1), n);
    hoc_return_type_code = 1;
    return double(rval);
}

static double ms_save(void* v) {
#if HAVE_IV
    std::ostream* o = Oc::save_stream;
    if (o) {
        ((MechanismStandard*) v)->save(gargstr(1), o);
    }
#endif
    return 0.;
}

static void* ms_cons(Object* ob) {
    int vartype = nrnocCONST;
    if (ifarg(2)) {
        // 0 means all
        vartype = int(chkarg(2, -1, STATE));
    }
    MechanismStandard* m = new MechanismStandard(gargstr(1), vartype);
    m->ref();
    m->msobj_ = ob;
    return (void*) m;
}

static void ms_destruct(void* v) {
    Resource::unref((MechanismStandard*) v);
}

static Member_func ms_members[] = {{"panel", ms_panel},
                                   {"action", ms_action},
                                   {"in", ms_in},
                                   {"_in", ms_in},
                                   {"out", ms_out},
                                   {"set", ms_set},
                                   {"get", ms_get},
                                   {"count", ms_count},
                                   {"name", ms_name},
                                   {"save", ms_save},
                                   {0, 0}};

void MechanismStandard_reg() {
    class2oc("MechanismStandard", ms_cons, ms_destruct, ms_members, NULL, NULL, NULL);
    ms_class_sym_ = hoc_lookup("MechanismStandard");
}

MechanismStandard::MechanismStandard(const char* name, int vartype) {
    msobj_ = NULL;
    glosym_ = NULL;
    np_ = new NrnProperty(name);
    name_cnt_ = 0;
    vartype_ = vartype;  // vartype=0 means all but not globals, -1 means globals
    offset_ = 0;
    if (vartype_ == -1) {
        char suffix[100];
        char* s;
        Sprintf(suffix, "_%s", name);
        Symbol* sp;
        for (sp = hoc_built_in_symlist->first; sp; sp = sp->next) {
            if (sp->type == VAR && sp->subtype == USERDOUBLE &&
                (s = strstr(sp->name, suffix)) != 0 && s[strlen(suffix)] == '\0') {
                ++name_cnt_;
            }
        }
        glosym_ = new Symbol*[name_cnt_];
        int i = 0;
        for (sp = hoc_built_in_symlist->first; sp; sp = sp->next) {
            if (sp->type == VAR && sp->subtype == USERDOUBLE &&
                (s = strstr(sp->name, suffix)) != 0 && s[strlen(suffix)] == '\0') {
                glosym_[i] = sp;
                ++i;
            }
        }
    } else {
        for (Symbol* sym = np_->first_var(); np_->more_var(); sym = np_->next_var()) {
            int type = np_->var_type(sym);
            if (type < vartype) {
                ++offset_;
            } else if (vartype == 0 || type == vartype) {
                ++name_cnt_;
            }
        }
    }
    action_ = "";
    pyact_ = NULL;
}
MechanismStandard::~MechanismStandard() {
    if (pyact_) {
        hoc_obj_unref(pyact_);
    }
    if (glosym_) {
        delete[] glosym_;
    }
    delete np_;
}
int MechanismStandard::count() {
    return name_cnt_;
}
const char* MechanismStandard::name() {
    return np_->name();
}
const char* MechanismStandard::name(int i, int& size) {
    Symbol* s;
    if (vartype_ == -1) {
        s = glosym_[i];
    } else {
        s = np_->var(i + offset_);
    }
    size = hoc_total_array_data(s, 0);
    return s->name;
}

void MechanismStandard::panel(const char* label) {
#if HAVE_IV
    mschk("panel");
    char buf[256];
    int i;
    Symbol* sym;
    hoc_ivpanel("MechanismStandard");
    if (label) {
        hoc_ivlabel(label);
    } else {
        hoc_ivlabel(np_->name());
    }
    for (sym = np_->first_var(), i = 0; np_->more_var(); sym = np_->next_var(), ++i) {
        if (vartype_ == 0 || np_->var_type(sym) == vartype_) {
            Object* pyactval = NULL;
            int size = hoc_total_array_data(sym, 0);
            if (pyact_) {
                assert(neuron::python::methods.callable_with_args);
                hoc_push_object(msobj_);
                hoc_pushx(double(i));
                hoc_pushx(0.0);
                pyactval = neuron::python::methods.callable_with_args(pyact_, 3);
            } else {
                Sprintf(buf, "hoc_ac_ = %d  %s", i, action_.c_str());
            }
            hoc_ivvaluerun_ex(sym->name,
                              NULL,
                              np_->prop_pval(sym),
                              NULL,
                              pyact_ ? NULL : buf,
                              pyactval,
                              true,
                              false,
                              true,
                              sym->extra);
            if (pyactval) {
                hoc_obj_unref(pyactval);
            }
            int j;
            for (j = 1; j < size; ++j) {
                ++i;
                if (pyact_) {
                    assert(neuron::python::methods.callable_with_args);
                    hoc_push_object(msobj_);
                    hoc_pushx(double(i));
                    hoc_pushx(double(j));
                    pyactval = neuron::python::methods.callable_with_args(pyact_, 3);
                } else {
                    Sprintf(buf, "hoc_ac_ = %d %s", i, action_.c_str());
                }
                char buf2[200];
                Sprintf(buf2, "%s[%d]", sym->name, j);
                hoc_ivvaluerun_ex(buf2,
                                  NULL,
                                  np_->prop_pval(sym, j),
                                  NULL,
                                  pyact_ ? NULL : buf,
                                  pyact_,
                                  true,
                                  false,
                                  true,
                                  sym->extra);
                if (pyactval) {
                    hoc_obj_unref(pyactval);
                }
            }
        }
    }
    hoc_ivpanelmap();
#endif
}
void MechanismStandard::action(const char* action, Object* pyact) {
    mschk("action");
    action_ = action ? action : "";
    if (pyact) {
        pyact_ = pyact;
        hoc_obj_ref(pyact);
    }
}
void MechanismStandard::set(const char* name, double val, int index) {
    mschk("set");
    Symbol* s = np_->find(name);
    if (s) {
        *np_->prop_pval(s, index) = val;
    } else {
        hoc_execerror(name, "not in this property");
    }
}
double MechanismStandard::get(const char* name, int index) {
    mschk("get");
    Symbol* s = np_->find(name);
    if (!s) {
        hoc_execerror(name, "not in this property");
    }
    auto const pval = np_->prop_pval(s, index);
    if (!pval) {
        return -1e300;
    }
    return *pval;
}

void MechanismStandard::in(Section* sec, double x) {
    mschk("in");
    int i = 0;
    if (x >= 0) {
        i = node_index(sec, x);
    }
    Prop* p = nrn_mechanism(np_->type(), sec->pnode[i]);
    NrnProperty::assign(p, np_->prop(), vartype_);
}
void MechanismStandard::in(Point_process* pp) {
    mschk("in");
    NrnProperty::assign(pp->prop, np_->prop(), vartype_);
}
void MechanismStandard::in(MechanismStandard* ms) {
    mschk("in");
    NrnProperty::assign(ms->np_->prop(), np_->prop(), vartype_);
}

void MechanismStandard::out(Section* sec, double x) {
    mschk("out");
    if (x < 0) {
        for (int i = 0; i < sec->nnode; ++i) {
            Prop* p = nrn_mechanism(np_->type(), sec->pnode[i]);
            NrnProperty::assign(np_->prop(), p, vartype_);
        }
    } else {
        int i = node_index(sec, x);
        Prop* p = nrn_mechanism(np_->type(), sec->pnode[i]);
        NrnProperty::assign(np_->prop(), p, vartype_);
    }
}
void MechanismStandard::out(Point_process* pp) {
    mschk("out");
    NrnProperty::assign(np_->prop(), pp->prop, vartype_);
}
void MechanismStandard::out(MechanismStandard* ms) {
    mschk("out");
    NrnProperty::assign(np_->prop(), ms->np_->prop(), vartype_);
}

void MechanismStandard::save(const char* obref, std::ostream* po) {
    mschk("save");
    std::ostream& o = *po;
    char buf[256];
    Sprintf(buf, "%s = new MechanismStandard(\"%s\")", obref, np_->name());
    o << buf << std::endl;
    for (Symbol* sym = np_->first_var(); np_->more_var(); sym = np_->next_var()) {
        if (vartype_ == 0 || np_->var_type(sym) == vartype_) {
            int i, cnt = hoc_total_array_data(sym, 0);
            for (i = 0; i < cnt; ++i) {
                Sprintf(
                    buf, "%s.set(\"%s\", %g, %d)", obref, sym->name, *np_->prop_pval(sym, i), i);
                o << buf << std::endl;
            }
        }
    }
}

void MechanismStandard::mschk(const char* s) {
    if (vartype_ == -1) {
        hoc_execerror(s, " MechanismStandard method not implemented for GLOBAL type");
    }
}

/*
help MembraneType
listin nrniv
Provides a way of iterating over all membrane mechanisms or point
processes and allows selection via a menu or under hoc control.

mt = new MembraneType(0)
The object can be considered a list of all the available continuous
membrane mechanisms. eg "hh", "pas", "extracellular". that can
be inserted into a section.

mt = new MembraneType(1)
The object can be considered a list of all available Point Processes.
eg. PulseStim, AlphaSynapse, VClamp.

To print the names of all mechanisms in this object list try:
strdef mname
for i=0,mt.count() {
    mt.select(i)
  mt.selected(mname)
    print mname
}

help select
mt.select("name")
mt.select(i)
selects either the named mechanism or the i'th mechanism in the list.

help selected
i = mt.selected([strdef])
returns the index of the current selection.  If present, strarg is assigned
to the name of the current selection.

help make
mt.make()
For continuous mechanisms. Inserts selected mechanism into currently
accessed section.

help remove
mt.remove()
For continuous mechanisms. Deletes selected mechanism from currently
accessed section. A nop if the mechanism is not in the section.

help make
mt.make(objectvar)
For point processes.  The arg becomes a reference to a new point process
of type given by the selection.
Note that the newly created point process is not located in any section.
Note that if objectvar was the only reference to another object then
that object is destroyed.

help count
i = mt.count()
The number of  different mechanisms in the list.

help menu
mt.menu()
Inserts a special menu into the currently open xpanel. The menu
label always reflects the current selection. Submenu items are indexed
according to position with the first item being item 0.  When the mouse
button is released on a submenu item that item becomes the selection
and the action (if any) is executed.

help action
mt.action("command")
The action to be executed when a submenu item is selected.
*/
static Symbol* mt_class_sym_;

static double mt_select(void* v) {
    MechanismType* mt = (MechanismType*) v;
    if (hoc_is_double_arg(1)) {
        mt->select(int(chkarg(1, -1, mt->count() - 1)));
    } else if (hoc_is_str_arg(1)) {
        mt->select(gargstr(1));
    }
    return 0.;
}
static double mt_selected(void* v) {
    MechanismType* mt = (MechanismType*) v;
    int i = mt->selected_item();
    if (ifarg(1)) {
        hoc_assign_str(hoc_pgargstr(1), mt->selected());
    }
    hoc_return_type_code = 1;
    return double(i);
}
static double mt_internal_type(void* v) {
    MechanismType* mt = (MechanismType*) v;
    return double(mt->internal_type());
}
static double mt_make(void* v) {
    MechanismType* mt = (MechanismType*) v;
    if (mt->is_point()) {
        mt->point_process(hoc_objgetarg(1));
    } else {
        mt->insert(chk_access());
    }
    return 0.;
}
static double mt_remove(void* v) {
    MechanismType* mt = (MechanismType*) v;
    mt->remove(chk_access());
    return 0.;
}
static double mt_count(void* v) {
    MechanismType* mt = (MechanismType*) v;
    hoc_return_type_code = 1;
    return double(mt->count());
}
static double mt_menu(void* v) {
    TRY_GUI_REDIRECT_METHOD_ACTUAL_DOUBLE("MechanismType.menu", mt_class_sym_, v);
#if HAVE_IV
    IFGUI
    MechanismType* mt = (MechanismType*) v;
    mt->menu();
    ENDGUI
#endif
    return 0.;
}
static double mt_action(void* v) {
    MechanismType* mt = (MechanismType*) v;
    if (hoc_is_str_arg(1)) {
        mt->action(gargstr(1), NULL);
    } else {
        mt->action(NULL, *hoc_objgetarg(1));
    }
    return 0.;
}
static double mt_is_target(void* v) {
    MechanismType* mt = (MechanismType*) v;
    hoc_return_type_code = 2;
    return double(mt->is_netcon_target(int(chkarg(1, 0, mt->count()))));
}
static double mt_has_net_event(void* v) {
    MechanismType* mt = (MechanismType*) v;
    hoc_return_type_code = 2;
    return double(mt->has_net_event(int(chkarg(1, 0, mt->count()))));
}
static double mt_is_artificial(void* v) {
    MechanismType* mt = (MechanismType*) v;
    hoc_return_type_code = 2;
    return double(mt->is_artificial(int(chkarg(1, 0, mt->count()))));
}
static double mt_is_ion(void* v) {
    auto* mt = static_cast<MechanismType*>(v);
    hoc_return_type_code = 2;
    return double(mt->is_ion());
}

static Object** mt_pp_begin(void* v) {
    MechanismType* mt = (MechanismType*) v;
    Point_process* pp = mt->pp_begin();
    Object* obj = NULL;
    if (pp) {
        obj = pp->ob;
    }
    return hoc_temp_objptr(obj);
}

static Object** mt_pp_next(void* v) {
    MechanismType* mt = (MechanismType*) v;
    Point_process* pp = mt->pp_next();
    Object* obj = NULL;
    if (pp) {
        obj = pp->ob;
    }
    return hoc_temp_objptr(obj);
}

extern const char** nrn_nmodl_text_;
static const char** mt_code(void* v) {
    static const char* nullstr = "";
    MechanismType* mt = (MechanismType*) v;
    int type = mt->internal_type();
    const char** p = nrn_nmodl_text_ + type;
    if (*p) {
        return p;
    }
    return &nullstr;
}

extern const char** nrn_nmodl_filename_;
static const char** mt_file(void* v) {
    static const char* nullstr = "";
    MechanismType* mt = (MechanismType*) v;
    int type = mt->internal_type();
    const char** p = nrn_nmodl_filename_ + type;
    if (*p) {
        return p;
    }
    return &nullstr;
}

static void* mt_cons(Object* obj) {
    MechanismType* mt = new MechanismType(int(chkarg(1, 0, 1)));
    mt->ref();
    mt->mtobj_ = obj;
    return (void*) mt;
}
static void mt_destruct(void* v) {
    MechanismType* mt = (MechanismType*) v;
    mt->unref();
}
static Member_func mt_members[] = {{"select", mt_select},
                                   {"selected", mt_selected},
                                   {"make", mt_make},
                                   {"remove", mt_remove},
                                   {"count", mt_count},
                                   {"menu", mt_menu},
                                   {"action", mt_action},
                                   {"is_netcon_target", mt_is_target},
                                   {"has_net_event", mt_has_net_event},
                                   {"is_artificial", mt_is_artificial},
                                   {"is_ion", mt_is_ion},
                                   {"internal_type", mt_internal_type},
                                   {0, 0}};
static Member_ret_obj_func mt_retobj_members[] = {{"pp_begin", mt_pp_begin},
                                                  {"pp_next", mt_pp_next},
                                                  {0, 0}};
static Member_ret_str_func mt_retstr_func[] = {{"code", mt_code}, {"file", mt_file}, {0, 0}};
void MechanismType_reg() {
    class2oc(
        "MechanismType", mt_cons, mt_destruct, mt_members, NULL, mt_retobj_members, mt_retstr_func);
    mt_class_sym_ = hoc_lookup("MechanismType");
}

/* static */ class MechTypeImpl {
  private:
    friend class MechanismType;
    bool is_point_;
    int* type_;
    int count_;
    int select_;
    std::string action_;
    Object* pyact_;
    Section* sec_iter_;
    int inode_iter_;
    Prop* p_iter_;
};

typedef Symbol* PSym;

MechanismType::MechanismType(bool point_process) {
    mti_ = new MechTypeImpl;
    mti_->is_point_ = point_process;
    mti_->count_ = 0;
    int i;
    for (i = 2; i < n_memb_func; ++i) {
        if (point_process == memb_func[i].is_point) {
            ++mti_->count_;
        }
    }
    mti_->type_ = new int[mti_->count_];
    int j = 0;
    for (i = 2; i < n_memb_func; ++i) {
        if (point_process == memb_func[i].is_point) {
            mti_->type_[j] = i;
            ++j;
        }
    }
    mti_->pyact_ = NULL;
    action("", NULL);
    select(0);
}
MechanismType::~MechanismType() {
    if (mti_->pyact_) {
        hoc_obj_unref(mti_->pyact_);
    }
    delete[] mti_->type_;
    delete mti_;
}
bool MechanismType::is_point() {
    return mti_->is_point_;
}

Point_process* MechanismType::pp_begin() {
    if (!mti_->is_point_) {
        hoc_execerror("Not a MechanismType(1)", 0);
    }
    mti_->sec_iter_ = chk_access();
    nrn_parent_info(mti_->sec_iter_);
    mti_->p_iter_ = 0;
    if (mti_->sec_iter_->parentnode) {
        mti_->inode_iter_ = -1;
        mti_->p_iter_ = mti_->sec_iter_->parentnode->prop;
    }
    if (!mti_->p_iter_) {
        mti_->inode_iter_ = 0;
        mti_->p_iter_ = mti_->sec_iter_->pnode[0]->prop;
    }
    Point_process* pp = pp_next();  // note that p_iter is the one looked at and then incremented
    return pp;
}

Point_process* MechanismType::pp_next() {
    Point_process* pp = NULL;
    bool done = mti_->p_iter_ == 0;
    while (!done) {
        if (mti_->p_iter_->_type == mti_->type_[mti_->select_]) {
            pp = mti_->p_iter_->dparam[1].get<Point_process*>();
            done = true;
            // but if it does not belong to this section
            if (pp->sec != mti_->sec_iter_) {
                pp = NULL;
                done = false;
            }
        }
        mti_->p_iter_ = mti_->p_iter_->next;
        while (!mti_->p_iter_) {
            ++mti_->inode_iter_;
            if (mti_->inode_iter_ >= mti_->sec_iter_->nnode) {
                done = true;
                break;  // really at the end
            } else {
                mti_->p_iter_ = mti_->sec_iter_->pnode[mti_->inode_iter_]->prop;
            }
        }
    }
    return pp;
}

bool MechanismType::is_netcon_target(int i) {
    int j = mti_->type_[i];
    return pnt_receive[j] ? true : false;
}

bool MechanismType::has_net_event(int i) {
    int j = mti_->type_[i];
    int k;
    for (k = 0; k < nrn_has_net_event_cnt_; ++k) {
        if (nrn_has_net_event_[k] == j) {
            return true;
        }
    }
    return false;
}

bool MechanismType::is_artificial(int i) {
    int j = mti_->type_[i];
    return (nrn_is_artificial_[j] ? true : false);
}

bool MechanismType::is_ion() {
    return nrn_is_ion(internal_type());
}

void MechanismType::select(const char* name) {
    for (int i = 0; i < mti_->count_; ++i) {
        if (strcmp(name, memb_func[mti_->type_[i]].sym->name) == 0) {
            select(i);
            break;
        }
    }
}
const char* MechanismType::selected() {
    Symbol* sym = memb_func[mti_->type_[selected_item()]].sym;
    return sym->name;
}
int MechanismType::internal_type() {
    return mti_->type_[selected_item()];
}
extern void mech_insert1(Section*, int);
extern void mech_uninsert1(Section*, Symbol*);
void MechanismType::insert(Section* sec) {
    if (!mti_->is_point_) {
        mech_insert1(sec, memb_func[mti_->type_[selected_item()]].sym->subtype);
    }
}
void MechanismType::remove(Section* sec) {
    if (!mti_->is_point_) {
        mech_uninsert1(sec, memb_func[mti_->type_[selected_item()]].sym);
    }
}

extern Object* nrn_new_pointprocess(Symbol*);

void MechanismType::point_process(Object** o) {
    Symbol* sym = memb_func[mti_->type_[selected_item()]].sym;
    hoc_dec_refcount(o);
    *o = nrn_new_pointprocess(sym);
    (*o)->refcount = 1;
}

void MechanismType::action(const char* action, Object* pyact) {
    mti_->action_ = action ? action : "";
    if (pyact) {
        hoc_obj_ref(pyact);
    }
    if (mti_->pyact_) {
        hoc_obj_unref(mti_->pyact_);
        mti_->pyact_ = NULL;
    }
    mti_->pyact_ = pyact;
}
void MechanismType::menu() {
#if HAVE_IV
    char buf[200];
    Oc oc;
    oc.run("{xmenu(\"MechType\")}\n");
    for (int i = 0; i < mti_->count_; ++i) {
        Symbol* s = memb_func[mti_->type_[i]].sym;
        if (s->subtype != MORPHOLOGY) {
            if (mti_->pyact_) {
                assert(neuron::python::methods.callable_with_args);
                hoc_push_object(mtobj_);
                hoc_pushx(double(i));
                Object* pyactval = neuron::python::methods.callable_with_args(mti_->pyact_, 2);
                hoc_ivbutton(s->name, NULL, pyactval);
                hoc_obj_unref(pyactval);
            } else {
                Sprintf(
                    buf, "xbutton(\"%s\", \"hoc_ac_=%d %s\")\n", s->name, i, mti_->action_.c_str());
                oc.run(buf);
            }
        }
    }
    oc.run("{xmenu()}\n");
#endif
}

int MechanismType::count() {
    return mti_->count_;
}
int MechanismType::selected_item() {
    return mti_->select_;
}
void MechanismType::select(int index) {
    if (index < 0) {
        mti_->select_ = index;
    } else if (index >= count()) {
        mti_->select_ = count() - 1;
    } else {
        mti_->select_ = index;
    }
}
