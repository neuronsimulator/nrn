#include <../../nrnconf.h>
#include <stdlib.h>
#include <classreg.h>
#include <vector>

#include "hocstr.h"
#include "parse.hpp"
#include "hocparse.h"
#include "code.h"
#include "hocassrt.h"
#include "hoclist.h"
#include "nrn_ansi.h"
#include "nrnmpi.h"
#include "nrnpy.h"
#include "nrnfilewrap.h"
#include "ocfunc.h"


#define PDEBUG 0

Symbol* nrnpy_pyobj_sym_{};
#include "section.h"
#include "nrniv_mf.h"
int section_object_seen;
struct Section* nrn_sec_pop();
static int connect_obsec_;

#define PUBLIC_TYPE   1
#define EXTERNAL_TYPE 2
static void call_constructor(Object*, Symbol*, int);
static void free_objectdata(Objectdata*, cTemplate*);

std::vector<const char*> py_exposed_classes{};

int hoc_print_first_instance = 1;
int hoc_max_builtin_class_id = -1;

static Symbol* hoc_obj_;

void hoc_install_hoc_obj(void) {
    /* see void hoc_objvardecl(void) */
    Object** pobj;
    Symbol* s = hoc_install("_pysec", OBJECTVAR, 0.0, &hoc_top_level_symlist);
    hoc_install_object_data_index(s);
    hoc_objectdata[s->u.oboff].pobj = pobj = (Object**) emalloc(sizeof(Object*));
    pobj[0] = nullptr;

    auto const code = hoc_oc("objref hoc_obj_[2]\n");
    assert(code == 0);
    hoc_obj_ = hoc_lookup("hoc_obj_");
}

Object* hoc_obj_get(int i) {
    Object** p = hoc_objectdata[hoc_obj_->u.oboff].pobj;
    if (p) {
        return p[i];
    } else {
        return nullptr;
    }
}

void hoc_obj_set(int i, Object* obj) {
    Object** p = hoc_objectdata[hoc_obj_->u.oboff].pobj;
    hoc_obj_ref(obj);
    hoc_dec_refcount(p + i);
    p[i] = obj;
}

char* hoc_object_name(Object* ob) {
    static char s[100];
    if (ob) {
        Sprintf(s, "%s[%d]", ob->ctemplate->sym->name, ob->index);
    } else {
        Sprintf(s, "NULLobject");
    }
    return s;
}

size_t hoc_total_array(Symbol* s) /* total number of elements in array pointer */
{
    int total = 1, i;
    Arrayinfo* a = OPARINFO(s);
    if (a) {
        for (i = a->nsub - 1; i >= 0; --i) {
            total *= a->sub[i];
        }
    }
    return total;
}

size_t hoc_total_array_data(Symbol* s,
                            Objectdata* obd) /* total number of elements in array pointer */
{
    Arrayinfo* a;
    int total = 1, i;

    if (!obd) {
        a = s->arayinfo;
    } else
        switch (s->type) {
        case RANGEVAR:
            a = s->arayinfo;
            break;
        default:
            a = obd[s->u.oboff + 1].arayinfo;
            break;
        }
    if (a) {
        for (i = a->nsub - 1; i >= 0; --i) {
            total *= a->sub[i];
        }
    }
    return total;
}

static int icntobjectdata = 0;
Object* hoc_thisobject;
Objectdata* hoc_objectdata = (Objectdata*) 0;
Objectdata* hoc_top_level_data;
static int icnttoplevel;
int hoc_in_template = 0;


void hoc_push_current_object(void) {
    hoc_push_object(hoc_thisobject);
}

Objectdata* hoc_objectdata_save(void) {
    /* hoc_top_level_data changes when new vars are introduced */
    if (hoc_objectdata == hoc_top_level_data) {
        /* a template starts out its Objectdata as 0. */
        return (Objectdata*) 1;
    } else {
        return hoc_objectdata;
    }
}

Objectdata* hoc_objectdata_restore(Objectdata* obdsav) {
    if (obdsav == (Objectdata*) 1) {
        return hoc_top_level_data;
        ;
    } else {
        return obdsav;
    }
}

void hoc_obvar_declare(Symbol* sym, int type, int pmes) {
    if (sym->type != UNDEF) {
        return;
    }
    assert(sym->cpublic != 2);
    if (pmes && hoc_symlist == hoc_top_level_symlist) {
        int b = 0;
        b = (hoc_fin == stdin);
        if (nrnmpi_myid_world == 0 && (hoc_print_first_instance && b)) {
            Printf("first instance of %s\n", sym->name);
        }
        sym->defined_on_the_fly = 1;
    }
    hoc_install_object_data_index(sym);
    sym->type = type;
    switch (type) {
    case VAR:
        /*printf("hoc_obvar_declare %s\n", sym->name);*/
        OPVAL(sym) = (double*) ecalloc(1, sizeof(double));
        break;
    case STRING:
        OPSTR(sym) = (char**) 0;
        break;
    case OBJECTVAR:
        break;
    case SECTION:
        OPSECITM(sym) = nullptr;  // TODO: whaa? (struct Item**)0;
        break;
    default:
        hoc_execerror(sym->name, "can't declare this in obvar_declare");
        break;
    }
}

/*-----------------------------------------------*/

/* template stack so nested templates are ok */
typedef union {
    Symbol* sym;
    Symlist* symlist;
    Objectdata* odata;
    Object* o;
    int i;
} Templatedatum;
#define NTEMPLATESTACK 20
static Templatedatum templatestack[NTEMPLATESTACK];
static Templatedatum* templatestackp = templatestack;

static Templatedatum* poptemplate(void) {
    if (templatestackp == templatestack) {
        hoc_execerror("templatestack underflow", (char*) 0);
    }
    return (--templatestackp);
}

#define pushtemplatesym(arg) \
    chktemplate();           \
    (templatestackp++)->sym = arg
#define pushtemplatesymlist(arg) \
    chktemplate();               \
    (templatestackp++)->symlist = arg
#define pushtemplatei(arg) \
    chktemplate();         \
    (templatestackp++)->i = arg
#define pushtemplateodata(arg) \
    chktemplate();             \
    (templatestackp++)->odata = arg
#define pushtemplateo(arg) \
    chktemplate();         \
    (templatestackp++)->o = arg

static void chktemplate(void) {
    if (templatestackp == templatestack + NTEMPLATESTACK) {
        templatestackp = templatestack;
        hoc_execerror("templatestack overflow", (char*) 0);
    }
}
/*------------------------------------------------*/

/* mostly to allow saving of objects */

#define OBJ_STACK_SIZE 10
static Object* obj_stack_[OBJ_STACK_SIZE + 1]; /* +1 so we can see the most recent pushed */
static int obj_stack_loc;

void hoc_object_push(void) {
    Object* ob = *hoc_objgetarg(1);
    if (ob->ctemplate->constructor) {
        hoc_execerror("Can't do object_push for built-in class", 0);
    }
    if (obj_stack_loc >= OBJ_STACK_SIZE) {
        hoc_execerror("too many object context stack depth", 0);
    }
    obj_stack_[obj_stack_loc++] = hoc_thisobject;
    obj_stack_[obj_stack_loc] = ob;
    hoc_thisobject = ob;
    if (ob) {
        hoc_symlist = ob->ctemplate->symtable;
        hoc_objectdata = ob->u.dataspace;
    } else {
        hoc_symlist = hoc_top_level_symlist;
        hoc_objectdata = hoc_top_level_data;
    }
    hoc_ret();
    pushx(0.);
}

void hoc_object_pushed(void) {
    Object* ob;
    int i = chkarg(1, 0., (double) obj_stack_loc);
    ob = obj_stack_[obj_stack_loc - i];
    hoc_ret();
    hoc_push_object(ob);
}

void hoc_object_pop(void) {
    Object* ob;
    if (obj_stack_loc < 1) {
        hoc_execerror("object context stack underflow", 0);
    }
    obj_stack_[obj_stack_loc] = nullptr;
    ob = obj_stack_[--obj_stack_loc];
    hoc_thisobject = ob;
    if (ob) {
        hoc_symlist = ob->ctemplate->symtable;
        hoc_objectdata = ob->u.dataspace;
    } else {
        hoc_symlist = hoc_top_level_symlist;
        hoc_objectdata = hoc_top_level_data;
    }
    hoc_ret();
    pushx(0.);
}
/*-----------------------------------------------*/
int hoc_resize_toplevel(int more) {
    if (more > 0) {
        icnttoplevel += more;
        hoc_top_level_data = (Objectdata*) erealloc((char*) hoc_top_level_data,
                                                    icnttoplevel * sizeof(Objectdata));
        if (templatestackp == templatestack) {
            hoc_objectdata = hoc_top_level_data;
        }
    }
    return icnttoplevel;
}

void hoc_install_object_data_index(Symbol* sp) {
    if (!hoc_objectdata) {
        icntobjectdata = 0;
    }
    sp->u.oboff = icntobjectdata;
    icntobjectdata += 2; /* data pointer and Arrayinfo */
    hoc_objectdata = (Objectdata*) erealloc((char*) hoc_objectdata,
                                            icntobjectdata * sizeof(Objectdata));
    hoc_objectdata[icntobjectdata - 1].arayinfo = sp->arayinfo;
    if (sp->arayinfo) {
        ++sp->arayinfo->refcount;
    }
    if (templatestackp == templatestack) {
        hoc_top_level_data = hoc_objectdata;
        icnttoplevel = icntobjectdata;
    }
}

int hoc_obj_run(const char* cmd, Object* ob) {
    int err;
    Object* objsave;
    Objectdata* obdsave;
    Symlist* slsave;
    int osloc;
    objsave = hoc_thisobject;
    obdsave = hoc_objectdata_save();
    slsave = hoc_symlist;
    osloc = obj_stack_loc;

    if (ob) {
        if (ob->ctemplate->constructor) {
            hoc_execerror("Can't execute in a built-in class context", 0);
        }
        hoc_thisobject = ob;
        hoc_objectdata = ob->u.dataspace;
        hoc_symlist = ob->ctemplate->symtable;
    } else {
        hoc_thisobject = 0;
        hoc_objectdata = hoc_top_level_data;
        hoc_symlist = hoc_top_level_symlist;
    }

    err = hoc_oc(cmd);

    hoc_thisobject = objsave;
    hoc_objectdata = hoc_objectdata_restore(obdsave);
    hoc_symlist = slsave;
    obj_stack_loc = osloc;

    return err;
}

void hoc_exec_cmd(void) { /* execute string from top level or within an object context */
    int err;
    char* cmd;
    char buf[256];
    char* pbuf;
    Object* ob = 0;
    HocStr* hs = 0;
    cmd = gargstr(1);
    pbuf = buf;
    auto pbuf_size = 256;
    if (strlen(cmd) > 256 - 10) {
        hs = hocstr_create(strlen(cmd) + 10);
        pbuf = hs->buf;
        pbuf_size = hs->size + 1;
    }
    if (cmd[0] == '~') {
        std::snprintf(pbuf, pbuf_size, "%s\n", cmd + 1);
    } else {
        std::snprintf(pbuf, pbuf_size, "{%s}\n", cmd);
    }
    if (ifarg(2)) {
        ob = *hoc_objgetarg(2);
    }
    err = hoc_obj_run(pbuf, ob);
    if (err) {
        hoc_execerror("execute error:", cmd);
    }
    if (pbuf != buf) {
        hocstr_delete(hs);
    }
    hoc_ret();
    pushx((double) (err));
}

/* call a function within the context of an object. Args must be on stack */
double hoc_call_objfunc(Symbol* s, int narg, Object* ob) {
    double d;  //, hoc_call_func();
    Object* objsave;
    Objectdata* obdsave;
    Symlist* slsave;
    objsave = hoc_thisobject;
    obdsave = hoc_objectdata_save();
    slsave = hoc_symlist;

    if (ob) {
        hoc_thisobject = ob;
        hoc_objectdata = ob->u.dataspace;
        hoc_symlist = ob->ctemplate->symtable;
    } else {
        hoc_thisobject = 0;
        hoc_objectdata = hoc_top_level_data;
        hoc_symlist = hoc_top_level_symlist;
    }

    d = hoc_call_func(s, narg);

    hoc_thisobject = objsave;
    hoc_objectdata = hoc_objectdata_restore(obdsave);
    hoc_symlist = slsave;

    return d;
}

void hoc_oop_initaftererror(void) {
    hoc_symlist = hoc_top_level_symlist;
    templatestackp = templatestack;
    icntobjectdata = icnttoplevel;
    hoc_objectdata = hoc_top_level_data;
    hoc_thisobject = nullptr;
    obj_stack_loc = 0;
    hoc_in_template = 0;
    connect_obsec_ = 0;
}

void oc_save_hoc_oop(Object** a1,
                     Objectdata** a2,
                     // a3 is missing, do not add it
                     int* a4,
                     Symlist** a5) {
    *a1 = hoc_thisobject;
    /* same style as hoc_objectdata_sav */
    if (hoc_objectdata == hoc_top_level_data) {
        *a2 = (Objectdata*) 1;
    } else {
        *a2 = hoc_objectdata;
    }
    *a4 = obj_stack_loc;
    *a5 = hoc_symlist;
}

void oc_restore_hoc_oop(Object** a1, Objectdata** a2, int* a4, Symlist** a5) {
    hoc_thisobject = *a1;
    if (*a2 == (Objectdata*) 1) {
        hoc_objectdata = hoc_top_level_data;
    } else {
        hoc_objectdata = *a2;
    }
    obj_stack_loc = *a4;
    hoc_symlist = *a5;
}

Object* hoc_new_object(Symbol* symtemp, void* v) {
    Object* ob;
#if PDEBUG
    printf("new object from template %s created.\n", symtemp->name);
#endif
    ob = (Object*) emalloc(sizeof(Object));
    ob->recurse = 0;
    ob->unref_recurse_cnt = 0;
    ob->refcount = 1; /* so template notify will not delete */
    ob->observers = nullptr;
    ob->ctemplate = symtemp->u.ctemplate;
    ob->aliases = nullptr;
    ob->itm_me = hoc_l_lappendobj(ob->ctemplate->olist, ob);
    ob->secelm_ = (hoc_Item*) 0;
    ob->ctemplate->count++;
    ob->index = ob->ctemplate->index++;
    if (symtemp->subtype & (CPLUSOBJECT | JAVAOBJECT)) {
        ob->u.this_pointer = v;
        if (v) {
            hoc_template_notify(ob, 1);
        }
    } else {
        ob->u.dataspace = 0;
    }
    ob->refcount = 0;
    return ob;
}

void hoc_new_object_asgn(Object** obp, Symbol* st, void* v) {
    hoc_dec_refcount(obp);
    *obp = hoc_new_object(st, v);
    hoc_obj_ref(*obp);
}

Object** hoc_temp_objvar(Symbol* symtemp, void* v) {
    return hoc_temp_objptr(hoc_new_object(symtemp, v));
}

struct guard_t {
    ~guard_t() {
        if (ob) {
            hoc_obj_unref(ob);
        }
    }
    Object* ob{};
};

Object* hoc_newobj1(Symbol* sym, int narg) {
    Object* ob;
    Objectdata* obd;
    Symbol* s;
    int i, total;

    guard_t guard{};  // unref the object we're creating if there is an exception before the end of
                      // this method
    guard.ob = ob = hoc_new_object(sym, nullptr);
    ob->refcount = 1;
    if (sym->subtype & (CPLUSOBJECT | JAVAOBJECT)) {
        call_constructor(ob, sym, narg);
    } else {
        ob->u.dataspace = obd = (Objectdata*) ecalloc(ob->ctemplate->dataspace_size,
                                                      sizeof(Objectdata));
        for (s = ob->ctemplate->symtable->first; s; s = s->next) {
            if (s->cpublic != 2) {
                switch (s->type) {
                case VAR:
                    if ((obd[s->u.oboff + 1].arayinfo = s->arayinfo) != (Arrayinfo*) 0) {
                        ++s->arayinfo->refcount;
                    }
                    total = hoc_total_array_data(s, obd);
                    obd[s->u.oboff].pval = (double*) emalloc(total * sizeof(double));
                    for (i = 0; i < total; i++) {
                        (obd[s->u.oboff].pval)[i] = 0.;
                    }
                    break;
                case STRING:
                    obd[s->u.oboff + 1].arayinfo = (Arrayinfo*) 0;
                    obd[s->u.oboff].ppstr = (char**) emalloc(sizeof(char*));
                    *obd[s->u.oboff].ppstr = (char*) emalloc(sizeof(char));
                    **(obd[s->u.oboff].ppstr) = '\0';
                    break;
                case OBJECTVAR:
                    if ((obd[s->u.oboff + 1].arayinfo = s->arayinfo) != (Arrayinfo*) 0) {
                        ++s->arayinfo->refcount;
                    }
                    total = hoc_total_array_data(s, obd);
                    obd[s->u.oboff].pobj = (Object**) emalloc(total * sizeof(Object*));
                    for (i = 0; i < total; i++) {
                        (obd[s->u.oboff].pobj)[i] = (Object*) 0;
                    }
                    if (strcmp(s->name, "this") == 0) {
                        obd[s->u.oboff].pobj[0] = ob;
                    }
                    break;
                case SECTION:
                    if ((obd[s->u.oboff + 1].arayinfo = s->arayinfo) != (Arrayinfo*) 0) {
                        ++s->arayinfo->refcount;
                    }
                    total = hoc_total_array_data(s, obd);
                    obd[s->u.oboff].psecitm = (hoc_Item**) emalloc(total * sizeof(hoc_Item*));
                    new_sections(ob, s, obd[s->u.oboff].psecitm, total);
                    break;
                }
            }
        }
        if (ob->ctemplate->is_point_) {
            hoc_construct_point(ob, narg);
        }
        if (ob->ctemplate->init) {
            call_ob_proc(ob, ob->ctemplate->init, narg);
        } else {
            for (i = 0; i < narg; ++i) {
                hoc_nopop();
            }
        }
    }
    hoc_template_notify(ob, 1);
    guard.ob = nullptr;  // do not unref, disable the guard
    return ob;
}

void hoc_newobj_arg(void) {
    Object* ob;
    Symbol* sym;
    int narg;
    sym = (pc++)->sym;
    narg = (pc++)->i;
    ob = hoc_newobj1(sym, narg);
    --ob->refcount; /*not necessarily 0 since init may reference 'this' */
    hoc_pushobj(hoc_temp_objptr(ob));
}

void hoc_newobj_ret(void) {
    hoc_newobj_arg();
}

void hoc_newobj(void) { /* template at pc+1 */
    Object *ob, **obp;
    Objectdata* obd;
    Symbol *sym, *s;
    int i, total;
    int narg;

    sym = (pc++)->sym;
    narg = (pc++)->i;
#if USE_PYTHON
    /* look inside stack because of limited number of temporary objects? */
    /* whatever. we will keep the strategy */
    if (hoc_inside_stacktype(narg) == OBJECTVAR) {
#endif
        obp = hoc_look_inside_stack<Object**>(narg);
        ob = hoc_newobj1(sym, narg);
        hoc_nopop(); /* the object pointer */
        hoc_dec_refcount(obp);
        *(obp) = ob;
        hoc_pushobj(obp);
#if USE_PYTHON
    } else { /* Assignment to OBJECTTMP not allowed */
        Object* o = hoc_obj_look_inside_stack(narg);
        hoc_execerror("Assignment to $o only allowed if caller arg was declared as objref", NULL);
    }
#endif
}

static void call_constructor(Object* ob, Symbol* sym, int narg) {
    Inst* pcsav;
    Symlist* slsav;
    Objectdata* obdsav;
    Object* obsav;

    slsav = hoc_symlist;
    obdsav = hoc_objectdata_save();
    obsav = hoc_thisobject;
    pcsav = pc;

    push_frame(sym, narg);
    ob->u.this_pointer = neuron::oc::invoke_method_that_may_throw(
        [ob]() -> std::string {
            std::string rval{hoc_object_name(ob)};
            rval.append(" constructor");
            return rval;
        },
        ob->ctemplate->constructor,
        ob);
    pop_frame();

    pc = pcsav;
    hoc_symlist = slsav;
    hoc_objectdata = hoc_objectdata_restore(obdsav);
    hoc_thisobject = obsav;
}

/* When certain methods of some Objects are called, the gui-redirect macros
   need Object* instead of Object*->u.this_pointer. Not worth changing the
   prototype of the call as it is used in so many places. So store the Object*
   to be obtained if needed.
*/

static Object* gui_redirect_obj_;
Object* nrn_get_gui_redirect_obj() {
    return gui_redirect_obj_;
}

void call_ob_proc(Object* ob, Symbol* sym, int narg) {
    Inst *pcsav, callcode[4];
    Symlist* slsav;
    Objectdata* obdsav;
    Object* obsav;

    slsav = hoc_symlist;
    obdsav = hoc_objectdata_save();
    obsav = hoc_thisobject;
    pcsav = pc;

    if (ob->ctemplate->sym->subtype & CPLUSOBJECT) {
        hoc_thisobject = ob;
        gui_redirect_obj_ = ob;
        push_frame(sym, narg);
        hoc_thisobject = obsav;
        auto const error_prefix_generator = [ob, sym]() {
            std::string rval{hoc_object_name(ob)};
            rval.append(2, ':');
            rval.append(sym->name);
            return rval;
        };
        if (sym->type == OBFUNCTION) {
            auto* const o = neuron::oc::invoke_method_that_may_throw(error_prefix_generator,
                                                                     sym->u.u_proc->defn.pfo_vp,
                                                                     ob->u.this_pointer);
            if (*o) {
                ++(*o)->refcount;
            } /* in case unreffed below */
            hoc_pop_frame();
            if (*o) {
                --(*o)->refcount;
            }
            hoc_pushobj(o);
        } else if (sym->type == STRFUNCTION) {
            auto* const s = const_cast<char**>(neuron::oc::invoke_method_that_may_throw(
                error_prefix_generator, sym->u.u_proc->defn.pfs_vp, ob->u.this_pointer));
            hoc_pop_frame();
            hoc_pushstr(s);
        } else {
            auto x = neuron::oc::invoke_method_that_may_throw(error_prefix_generator,
                                                              sym->u.u_proc->defn.pfd_vp,
                                                              ob->u.this_pointer);
            hoc_pop_frame();
            hoc_pushx(x);
        }
    } else if (ob->ctemplate->is_point_ && special_pnt_call(ob, sym, narg)) {
        ; /*empty since special_pnt_call did the work for get_loc, has_loc, and loc*/
    } else {
        callcode[0].pf = call;
        callcode[1].sym = sym;
        callcode[2].i = narg;
        callcode[3].in = STOP;

        hoc_objectdata = ob->u.dataspace;
        hoc_thisobject = ob;
        hoc_symlist = ob->ctemplate->symtable;
        execute(callcode);
        if (sym->type == PROCEDURE) {
            hoc_nopop();
        }
    }
    if (hoc_errno_check()) {
        char str[200];
        Sprintf(str, "%s.%s", hoc_object_name(ob), sym->name);
        hoc_warning("errno set during call of", str);
    }
    pc = pcsav;
    hoc_symlist = slsav;
    hoc_objectdata = hoc_objectdata_restore(obdsav);
    hoc_thisobject = obsav;
}

static void call_ob_iter(Object* ob, Symbol* sym, int narg) {
    Symlist* slsav;
    Objectdata* obdsav;
    Object* obsav;
    Object* stmtobj;
    Inst *stmtbegin, *stmtend;

    slsav = hoc_symlist;
    obdsav = hoc_objectdata_save();
    obsav = hoc_thisobject;

    hoc_objectdata = ob->u.dataspace;
    hoc_thisobject = ob;
    hoc_symlist = ob->ctemplate->symtable;

    stmtobj = hoc_look_inside_stack<Object*>(narg + 1);
    stmtbegin = pc + pc->i;
    pc++;
    stmtend = pc + pc->i;
    hoc_iterator_object(sym, narg, stmtbegin, stmtend, stmtobj);

    /* the stack was popped by hoc_iterator_object
    hoc_nopop();
    */

    hoc_symlist = slsav;
    hoc_objectdata = hoc_objectdata_restore(obdsav);
    hoc_thisobject = obsav;
}

void hoc_objvardecl(void) { /* symbol at pc+1, number of indices at pc+2 */
    Symbol* sym;
    int nsub, size, i;
    Object** pobj;

    sym = (pc++)->sym;
#if PDEBUG
    printf("declareing %s as objectvar\n", sym->name);
#endif
    if (sym->type == OBJECTVAR) {
        int total, i;
        total = hoc_total_array(sym);
        for (i = 0; i < total; i++) {
            hoc_dec_refcount((OPOBJ(sym)) + i);
        }
        free(hoc_objectdata[sym->u.oboff].pobj);
        hoc_freearay(sym);
    } else {
        sym->type = OBJECTVAR;
        hoc_install_object_data_index(sym);
    }
    nsub = (pc++)->i;
    if (nsub) {
        size = hoc_arayinfo_install(sym, nsub);
    } else {
        size = 1;
    }
    hoc_objectdata[sym->u.oboff].pobj = pobj = (Object**) emalloc(size * sizeof(Object*));
    for (i = 0; i < size; i++) {
        pobj[i] = (Object*) 0;
    }
}

void hoc_cmp_otype(void) { /* NUMBER, OBJECTVAR, or STRING must be the type */
    int type;
    type = (pc++)->i;
}

void hoc_known_type(void) {
    int type;
    type = ((pc++)->i);
}

void hoc_objectvar(void) { /* object variable symbol at pc+1. */
                           /* pointer to correct object left on stack */
    Objectdata* odsav;
    Object* obsav = 0;
    Symlist* slsav;
    Symbol* obs;
    Object** obp;
#if PDEBUG
    printf("code for hoc_objectvar()\n");
#endif
    obs = (pc++)->sym;
    if (obs->cpublic == 2) {
        obs = obs->u.sym;
        odsav = hoc_objectdata_save();
        obsav = hoc_thisobject;
        slsav = hoc_symlist;
        hoc_objectdata = hoc_top_level_data;
        hoc_thisobject = 0;
        hoc_symlist = hoc_top_level_symlist;
    }
    obp = OPOBJ(obs);
    if (ISARRAY(obs)) {
        hoc_pushobj(obp + araypt(obs, OBJECTVAR));
    } else {
        hoc_pushobj(obp);
    }
    if (obsav) {
        hoc_objectdata = hoc_objectdata_restore(odsav);
        hoc_thisobject = obsav;
        hoc_symlist = slsav;
    }
}

void hoc_objectarg(void) { /* object arg index at pc+1. */
                           /* pointer to correct object left on stack */
    int i;
    Object** obp;
#if PDEBUG
    printf("code for hoc_objectarg()\n");
#endif
    i = (pc++)->i;
    if (i == 0) {
        i = hoc_argindex();
    }
    obp = hoc_objgetarg(i);
    hoc_pushobj(obp);
}

void hoc_constobject(void) { /* template at pc, index at pc+1, objpointer left on stack*/
    char buf[200];
    Object* obj;
    hoc_Item* q;
    cTemplate* t = (pc++)->sym->u.ctemplate;
    int index = (int) hoc_xpop();
    ITERATE(q, t->olist) {
        obj = OBJ(q);
        if (obj->index == index) {
            hoc_pushobj(hoc_temp_objptr(obj));
            return;
        } else if (obj->index > index) {
            break;
        }
    }
    Sprintf(buf, "%s[%d]\n", t->sym->name, index);
    hoc_execerror("Object ID doesn't exist:", buf);
}

Object* hoc_name2obj(const char* name, int index) {
    char buf[200];
    Object* obj;
    hoc_Item* q;
    cTemplate* t;
    Symbol* sym;
    sym = hoc_table_lookup(name, hoc_top_level_symlist);
    if (!sym) {
        sym = hoc_table_lookup(name, hoc_built_in_symlist);
    }
    if (!sym || sym->type != TEMPLATE) {
        hoc_execerror(name, "is not a template");
    }
    t = sym->u.ctemplate;
    ITERATE(q, t->olist) {
        obj = OBJ(q);
        if (obj->index == index) {
            return obj;
        } else if (obj->index > index) {
            break;
        }
    }
    return nullptr;
}

void hoc_object_id(void) {
    Object* ob;

    ob = *(hoc_objgetarg(1));
    if (ifarg(2) && chkarg(2, 0., 1.) == 1.) {
        hoc_ret();
        if (ob) {
            pushx((double) ob->index);
        } else {
            pushx(-1.);
        }
    } else {
        hoc_ret();
        pushx((double) ((size_t) ob));
    }
}

static void range_suffix(Symbol* sym, int nindex, int narg) {
    int bdim = 0;
    if (ISARRAY(sym)) {
        if (nindex != sym->arayinfo->nsub) {
            bdim = 1;
        }
        /*
         It is a bit more difficult to push ndim here since the arc length, if
         specified, is top of stack before the index. However, waiting to fix up
         the stack til just before range_vec_indx calls hoc_araypt seems ill
         advised since range_vec_indx is called 6 places. And who knows what
         kinds of user errors are possible that we want to catch.
         So fix up below.
        */
    } else {
        if (nindex != 0) {
            bdim = 1;
        }
    }
    if (bdim) {
        hoc_execerror(sym->name, "wrong number of array dimensions");
    }

    if (sym->type == RANGEVAR) {
        if (ISARRAY(sym)) {  // fixup ndim here
            double x = -1.0;
            if (narg) {  // need to pop the arc length to push ndim
                if (narg > 1) {
                    hoc_execerr_ext("%s range variable can have only one arc length parameter",
                                    sym->name);
                }
                x = xpop();
            }
            if (!hoc_stack_type_is_ndim()) {
                hoc_push_ndim(nindex);
            }
            if (narg) {  // push back the arc length
                pushx(x);
            }
        }
        hoc_pushi(narg);
        hoc_pushs(sym);
    } else if (sym->subtype == USERPROPERTY) {
        if (narg) {
            hoc_execerror(sym->name, "section property can't have argument");
        }
        hoc_pushs(sym);
    } else if (sym->type == RANGEOBJ) {
        // must return NMODLObject on stack
        assert(sym->subtype == NMODLRANDOM);  // the only possibility at the moment
        double x{0.5};
        if (narg) {
            if (narg > 1) {
                hoc_execerr_ext("%s range object can have only one arg length parameter",
                                sym->name);
            }
            x = xpop();
        }
        Section* sec{nrn_sec_pop()};
        auto const i = node_index(sec, x);
        Prop* m = nrn_mechanism_check(sym->u.rng.type, sec, i);
        Object* ob = nrn_nmodlrandom_wrap(m, sym);
        hoc_push_object(ob);
    } else {
        hoc_execerror(sym->name, "suffix not a range variable or section property");
    }
}

void connect_obsec_syntax(void) {
    connect_obsec_ = 1;
}

// number of indices at pc+2, number of args at pc+3, symbol at pc+1
// object pointer on stack after indices
// if component turns out to be an object then make sure pointer to correct
// object, symbol, etc is left on stack for evaluation, assignment, etc.
void hoc_object_component() {
    Symbol *sym0, *sym = 0;
    int nindex, narg, cplus, isfunc;
    Object *obp, *obsav;
    Objectdata* psav;
    int* ptid;
    Symbol** psym;

#if PDEBUG
    printf("code for hoc_object_component()\n");
#endif
    sym0 = (pc++)->sym;
    nindex = (pc++)->i;
    narg = (pc++)->i;
    ptid = &(pc++)->i;
    psym = &(pc++)->sym;
    isfunc = (pc++)->i;

    if (section_object_seen) {
        section_object_seen = 0;
        range_suffix(sym0, nindex, narg);
        return;
    }
    if (connect_obsec_) {
        narg += nindex;
        nindex = 0;
    }
    int expect_stack_nsub{0};
    if (nindex) {
        if (narg) {
            hoc_execerror("[...](...) syntax only allowed for array range variables:", sym0->name);
        }
        if (!hoc_stack_type_is_ndim()) {
            hoc_push_ndim(nindex);
        }
        expect_stack_nsub = 1;
    } else {
        nindex = narg;
    }
    obp = hoc_obj_look_inside_stack(nindex + expect_stack_nsub);
    if (obp) {
#if USE_PYTHON
        if (obp->ctemplate->sym == nrnpy_pyobj_sym_) {
            if (isfunc & 2) {
                /* this is the final left hand side of an
                assignment to the method of a PythonObject
                and we need to put the PythonObject and the
                method with all its info onto the stack so that
                a proper __setattro__ or __setitem__ can be
                accomplished in the next hoc_object_asgn
                */
                if (isfunc & 1) {
                    hoc_execerror("Cannot assign to a PythonObject function call:", sym0->name);
                }
                pushi(nindex);
                pushs(sym0);
                hoc_push_object(obp);
                /* note obp is now on stack twice */
                /* hpoasgn will pop both */
            } else {
                neuron::python::methods.py2n_component(obp, sym0, nindex, isfunc);
            }
            return;
        }
#endif
        if (obp->ctemplate->id == *ptid) {
            sym = *psym;
        } else {
            if (obp->aliases == 0 || (sym = ivoc_alias_lookup(sym0->name, obp)) == 0) {
                /* lookup only has to be done once if the name is not an alias
                and the ptid of the object is still the same. */
                sym = hoc_table_lookup(sym0->name, obp->ctemplate->symtable);
                if (!sym || sym->cpublic != PUBLIC_TYPE) {
                    fprintf(stderr,
                            "%s not a public member of %s\n",
                            sym0->name,
                            obp->ctemplate->sym->name);
                    hoc_execerror(obp->ctemplate->sym->name, sym0->name);
                }
                *ptid = obp->ctemplate->id;
                *psym = sym;
            }
        }
    } else {
        hoc_execerror(sym0->name, ": object prefix is NULL");
    }

    psav = hoc_objectdata_save();
    obsav = hoc_thisobject;
    cplus = (obp->ctemplate->sym->subtype & (CPLUSOBJECT | JAVAOBJECT));
    if (!cplus) { /* c++ classes don't have a hoc dataspace */
        hoc_objectdata = obp->u.dataspace;
        hoc_thisobject = obp;
    }
    switch (sym->type) {
    case OBJECTVAR:
        if (nindex) {
            if (!ISARRAY(sym) || OPARINFO(sym)->nsub != nindex) {
                hoc_execerror(sym->name, ":not right number of subscripts");
            }
            nindex = araypt(sym, OBJECTVAR);
        }
        hoc_pop_defer();
        hoc_pushobj(OPOBJ(sym) + nindex);
        break;
    case VAR:
        if (cplus) {
            if (nindex) {
                if (!ISARRAY(sym) || sym->arayinfo->nsub != nindex) {
                    hoc_execerror(sym->name, ":not right number of subscripts");
                }
                if (narg) {
                    // there are 25 modeldb examples that use (index) instead
                    // of [index] syntax for an array in this context. So we
                    // have decided to keep allowing this legacy syntax for one
                    // dimensional arrays.
                    extern Symbol* nrn_matrix_sym;
                    if (narg == 1) {
                        hoc_push_ndim(1);
                    } else if (narg == 2 && obp->ctemplate->sym == nrn_matrix_sym) {
                        // Allow legacy syntax Matrix.x(i, j)
                        hoc_push_ndim(2);
                    } else {
                        hoc_execerr_ext("%s.%s is array not function. Use %s[...] syntax",
                                        hoc_object_name(obp),
                                        sym->name,
                                        sym->name);
                    }
                }
            }
            hoc_pushs(sym);
            (*obp->ctemplate->steer)(obp->u.this_pointer);
            double* pd = hoc_pxpop();
            /* cannot pop a temporary object til after the pd is used in
            case (e.g. Vector.x) the pointer is a field in the object
            (often the pd has nothing to do with the object)*/
            hoc_pop_defer(); /* corresponding unref_defer soon */
            hoc_pushpx(pd);
        } else {
            if (nindex) {
                if (!ISARRAY(sym) || OPARINFO(sym)->nsub != nindex) {
                    hoc_execerror(sym->name, ":not right number of subscripts");
                }
                if (narg) {
                    // there are a few modeldb examples that use (index) instead
                    // of [index] syntax for an array in this context. So we
                    // have decided to keep allowing this legacy syntax for one
                    // dimensional arrays.
                    if (narg == 1) {
                        hoc_push_ndim(1);
                    } else {
                        hoc_execerr_ext("%s.%s is array not function. Use %s[...] syntax",
                                        hoc_object_name(obp),
                                        sym->name,
                                        sym->name);
                    }
                }
                nindex = araypt(sym, OBJECTVAR);
            }
            hoc_pop_defer(); /*finally get rid of symbol */
            hoc_pushpx(OPVAL(sym) + nindex);
        }
        break;
    case STRING:
        if (nindex) {
            hoc_execerror(sym->name, ": string can't have function arguments or array indices");
        }
        hoc_pop_defer();
        hoc_pushstr(OPSTR(sym));
        break;
    case PROCEDURE:
    case FUNCTION: {
        if (expect_stack_nsub) {
            hoc_pop_ndim();
            hoc_execerr_ext("%s is a function not a %ddim array", sym->name, nindex);
        }
        double d = 0.;
        call_ob_proc(obp, sym, nindex);
        if (hoc_returning) {
            break;
        }
        if (sym->type == FUNCTION) {
            d = hoc_xpop();
        }
        hoc_pop_defer();
        hoc_pushx(d);
        break;
    }
    case HOCOBJFUNCTION:
    case OBFUNCTION: {
        Object** d;
        if (expect_stack_nsub) {
            hoc_pop_ndim();
            // for legacy reasons allow single arg [] format.
            // E.g. occasionally seen for List.object[index]
            if (nindex > 1) {
                hoc_execerr_ext("%s is a function not a %ddim array", sym->name, nindex);
            }
        }
        call_ob_proc(obp, sym, nindex);
        if (hoc_returning) {
            break;
        }
        d = hoc_objpop();
        if (*d) {
            (*d)->refcount++;
        } /* nopop may unref if temp obj.*/
        hoc_pop_defer();
        hoc_pushobj(d);
        if (*d) {
            (*d)->refcount--;
        } /* see the nopop may unref comment */
        hoc_tobj_unref(d);
        break;
    }
    case STRFUNCTION: {
        char** d;
        call_ob_proc(obp, sym, nindex);
        if (hoc_returning) {
            break;
        }
        d = hoc_strpop();
        hoc_pop_defer();
        hoc_pushstr(d);
        break;
    }
    case SECTIONREF: {
        extern Symbol* nrn_sec_sym;
        Section* sec;
        extern Section* nrn_sectionref_steer(Section * sec, Symbol * sym, int* pnindex);
        section_object_seen = 1;
        sec = (Section*) obp->u.this_pointer;
        if (sym != nrn_sec_sym) {
            sec = nrn_sectionref_steer(sec, sym, &nindex);
        }
        if (nrn_inpython_ == 2) {
            section_object_seen = 0;
            hoc_pop_defer();
            hoc_objectdata = hoc_objectdata_restore(psav);
            hoc_thisobject = obsav;
            return;
        }
        if (connect_obsec_) {
            double x = 0.0;
            connect_obsec_ = 0;
            if (nindex != 1) {
                hoc_execerror(sym->name, ": bad connect syntax");
            }
            x = hoc_xpop();
            hoc_pop_defer();
            hoc_pushx(x);
        } else {
            if (nindex) {
                hoc_execerror(sym->name, ":no subscript allowed");
            }
            hoc_pop_defer();
        }
        if (!sec->prop) {
            hoc_execerror("Section was deleted", (char*) 0);
        }
        nrn_pushsec(sec);
        break;
    }
    case SECTION: {
        double x = 0.0;
        section_object_seen = 1;
        if (connect_obsec_) {
            x = hoc_xpop();
            if (!nindex) {
                hoc_execerror(sym->name, ": bad connect syntax");
            }
            --nindex;
        }
        if (nindex) {
            if (!ISARRAY(sym) || OPARINFO(sym)->nsub != nindex) {
                hoc_execerror(sym->name, ":not right number of subscripts");
            }
            if (!hoc_stack_type_is_ndim()) {
                hoc_push_ndim(nindex);
            }
            nindex = araypt(sym, OBJECTVAR);
        }
        hoc_pop_defer();
        if (connect_obsec_) {
            hoc_pushx(x);
            connect_obsec_ = 0;
        }
        ob_sec_access_push(*(OPSECITM(sym) + nindex));
        break;
    }
    case ITERATOR: {
        if ((pc++)->i != ITERATOR) {
            hoc_execerror(sym->name, ":ITERATOR can only be used in a for statement");
        }
        call_ob_iter(obp, sym, nindex);
        if (hoc_returning) {
            break;
        }
        hoc_pop_defer();
        hoc_nopop(); /* get rid of iterator statement context */
        break;
    }
    case RANGEOBJ: {
        assert(sym->subtype == NMODLRANDOM);
        if (sym->subtype == NMODLRANDOM) {  // NMODL NEURON block RANDOM var
            // RANGE type. The void* is a nrnran123_State*. Wrap in a
            // NMODLRandom and push_object
            Object* o = nrn_pntproc_nmodlrandom_wrap(obp->u.this_pointer, sym);
            hoc_pop_defer();
            hoc_push_object(o);
        }
        break;
    }
    default:
        if (cplus) {
            if (nindex) {
                if (!ISARRAY(sym) || sym->arayinfo->nsub != nindex) {
                    hoc_execerror(sym->name, ":not right number of subscripts");
                }
                if (narg) {
                    // there are a few modeldb examples that use (index) instead
                    // of [index] syntax for an array in this context. So we
                    // have decided to keep allowing this legacy syntax for one
                    // dimensional arrays.
                    if (narg == 1) {
                        hoc_push_ndim(1);
                    } else {
                        hoc_execerr_ext("%s.%s is array not function. Use %s[...] syntax",
                                        hoc_object_name(obp),
                                        sym->name,
                                        sym->name);
                    }
                }
            }
            hoc_pushs(sym);
            (*obp->ctemplate->steer)(obp->u.this_pointer);
            auto dh = hoc_pop_handle<double>();
            hoc_pop_defer();
            hoc_push(std::move(dh));
        } else {
            hoc_execerror(sym->name, ": can't push that type onto stack");
        }
        break;
    case OBJECTALIAS:
        if (nindex) {
            hoc_execerror(sym->name, ": is an alias and cannot have subscripts");
        }
        hoc_pop_defer();
        hoc_push_object(sym->u.object_);
        break;
    case VARALIAS:
        if (nindex) {
            hoc_execerror(sym->name, ": is an alias and cannot have subscripts");
        }
        hoc_pop_defer();
        hoc_pushpx(sym->u.pval);
        break;
    }
    hoc_objectdata = hoc_objectdata_restore(psav);
    hoc_thisobject = obsav;
}

void hoc_object_eval(void) {
    int type;
#if PDEBUG
    printf("code for hoc_object_eval\n");
#endif
    type = hoc_stacktype();
    if (type == VAR) {
        hoc_pushx(*(hoc_pxpop()));
    } else if (type == SYMBOL) {
        auto* d_sym = hoc_look_inside_stack<Symbol*>(0);
        if (d_sym->type == RANGEVAR) {
            Symbol* sym = hoc_spop();
            int narg = hoc_ipop();
            struct Section* sec = nrn_sec_pop();
            double x;
            if (narg) {
                x = hoc_xpop();
            } else {
                x = .5;
            }
            hoc_pushx(*(nrn_rangepointer(sec, sym, x)));
        } else if (d_sym->type == VAR && d_sym->subtype == USERPROPERTY) {
            extern double cable_prop_eval(Symbol*);
            hoc_pushx(cable_prop_eval(hoc_spop()));
        }
    }
}

void hoc_ob_pointer(void) {
    int type;
    Symbol* sym;
#if PDEBUG
    printf("code for hoc_ob_pointer\n");
#endif
    type = hoc_stacktype();
    if (type == VAR) {
    } else if (type == SYMBOL) {
        auto* d_sym = hoc_look_inside_stack<Symbol*>(0);
        if (d_sym->type == RANGEVAR) {
            Symbol* sym = hoc_spop();
            int nindex = hoc_ipop();
            Section* sec = nrn_sec_pop();
            double x;
            if (nindex) {
                x = hoc_xpop();
            } else {
                x = .5;
            }
            hoc_push(nrn_rangepointer(sec, sym, x));
        } else if (d_sym->type == VAR && d_sym->subtype == USERPROPERTY) {
            hoc_pushpx(cable_prop_eval_pointer(hoc_spop()));
        } else {
            hoc_execerror("Not a double pointer", 0);
        }
    } else {
        hoc_execerror("Not a double pointer", 0);
    }
}

void hoc_asgn_obj_to_str(void) { /* string on stack */
    char *d, **pstr;
    d = *(hoc_strpop());
    pstr = hoc_strpop();
    hoc_assign_str(pstr, d);
}

void hoc_object_asgn() {
    int op = (pc++)->i;
    int type1 = hoc_stacktype();          // type of top entry
    int type2 = hoc_inside_stacktype(1);  // type of second-top entry
    if (type2 == SYMBOL) {
        auto* sym = hoc_look_inside_stack<Symbol*>(1);
        if (sym->type == RANGEVAR) {
            type2 = RANGEVAR;
        } else if (sym->type == VAR && sym->subtype == USERPROPERTY) {
            type2 = USERPROPERTY;
        }
    }
    if (type2 == RANGEVAR && type1 == NUMBER) {
        double d = hoc_xpop();
        Symbol* sym = hoc_spop();
        int nindex = hoc_ipop();
        Section* sec = nrn_sec_pop();
        if (nindex) {
            auto pd = nrn_rangepointer(sec, sym, hoc_xpop());
            if (op) {
                d = hoc_opasgn(op, *pd, d);
            }
            *pd = d;
        } else {
            nrn_rangeconst(sec,
                           sym,
                           neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                                  &d},
                           op);
        }
        hoc_pushx(d);
        return;
    } else if (type2 == USERPROPERTY && type1 == NUMBER) {
        double d = hoc_xpop();
        cable_prop_assign(hoc_spop(), &d, op);
        hoc_pushx(d);
        return;
    }
    switch (type2) {
    case VAR: {
        double d, *pd;
        d = hoc_xpop();
        pd = hoc_pxpop();
        if (op) {
            d = hoc_opasgn(op, *pd, d);
        }
        *pd = d;
        hoc_pushx(d);
    } break;
    case OBJECTVAR: {
        if (op) {
            hoc_execerror("Invalid assignment operator for object", nullptr);
        }
        Object** d = hoc_objpop();
        Object** pd = hoc_objpop();
        if (d != pd) {
            Object* tobj = *d;
            if (tobj) {
                (tobj)->refcount++;
            }
            hoc_tobj_unref(d);
            hoc_dec_refcount(pd);
            *pd = tobj;
        }
        hoc_pushobj(pd);
    } break;
    case STRING: {
        if (op) {
            hoc_execerror("Invalid assignment operator for string", nullptr);
        }
        char* d = *(hoc_strpop());
        char** pd = hoc_strpop();
        hoc_assign_str(pd, d);
        hoc_pushstr(pd);
    } break;
#if USE_PYTHON
    case OBJECTTMP: { /* should be PythonObject */
        Object* o = hoc_obj_look_inside_stack(1);
        assert(o->ctemplate->sym == nrnpy_pyobj_sym_);
        if (op) {
            hoc_execerror("Invalid assignment operator for PythonObject", nullptr);
        }
        neuron::python::methods.hpoasgn(o, type1);
    } break;
#endif
    default:
        hoc_execerror("Cannot assign to left hand side", nullptr);
    }
}

/* if the name isn't a template then look in the top level symbol table.
This allows objects to create objects of any class defined at the top level
*/
Symbol* hoc_which_template(Symbol* s) {
    if (s->type != TEMPLATE) {
        Symbol* s1;
        s1 = hoc_table_lookup(s->name, hoc_top_level_symlist);
        if (!s1 || s1->type != TEMPLATE) {
            hoc_execerror(s->name, "is not a template");
        }
        s = s1;
    }
    return s;
}

/* pushes old symtable and template name on template stack.
And creates new symtable. The new symtable
is used for all non-builtin names until an endtemplate is reached
*/
static int template_id;

void hoc_begintemplate(Symbol* t1) {
    Symbol* t;
    int type;
    t = hoc_decl(t1);

#if PDEBUG
    printf("begin template %s\n", t->name);
#endif
    type = t->type;
    if (type == TEMPLATE) {
        hoc_execerror(t->name, ": a template cannot be redefined");
        extern void hoc_free_symspace(Symbol*);
        hoc_free_symspace(t);
    } else if (type != UNDEF) {
        hoc_execerror(t->name, "already used as something besides template");
    }
    t->u.ctemplate = (cTemplate*) emalloc(sizeof(cTemplate));
    t->type = TEMPLATE;
    t->u.ctemplate->sym = t;
    t->u.ctemplate->symtable = (Symlist*) 0;
    t->u.ctemplate->dataspace_size = 0;
    t->u.ctemplate->constructor = 0;
    t->u.ctemplate->destructor = 0;
    t->u.ctemplate->is_point_ = 0;
    t->u.ctemplate->steer = 0;
    t->u.ctemplate->checkpoint = 0;
    t->u.ctemplate->id = ++template_id;
    pushtemplatei(icntobjectdata);
    pushtemplateodata(hoc_objectdata);
    pushtemplatei(hoc_in_template);
    pushtemplateo(hoc_thisobject);
    pushtemplatesymlist(hoc_symlist);
    pushtemplatesym(t);
    hoc_in_template = 1;
    hoc_objectdata = (Objectdata*) 0;
    hoc_thisobject = nullptr;
    hoc_symlist = t->u.ctemplate->symtable;
}

void hoc_endtemplate(Symbol* t) {
    Symbol *ts, *s;
#if PDEBUG
    printf("end template %s\n", t->name);
#endif
    ts = (poptemplate())->sym;
    if (strcmp(ts->name, t->name) != 0) {
        hoc_execerror(t->name, "- end template mismatched with begin");
    }
    ts->u.ctemplate->dataspace_size = icntobjectdata;
    ts->u.ctemplate->symtable = hoc_symlist;
    ts->u.ctemplate->count = 0;
    ts->u.ctemplate->index = 0;
    ts->u.ctemplate->olist = hoc_l_newlist();
    ts->u.ctemplate->observers = nullptr;
    hoc_symlist = (poptemplate())->symlist;
    free_objectdata(hoc_objectdata, ts->u.ctemplate);
    hoc_thisobject = (poptemplate())->o;
    hoc_in_template = (poptemplate())->i;
    hoc_objectdata = (poptemplate())->odata;
    icntobjectdata = (poptemplate())->i;
    ts->u.ctemplate->init = s = hoc_table_lookup("init", ts->u.ctemplate->symtable);
    if (s && s->type != PROCEDURE) {
        hoc_execerror("'init' can only be used as the initialization procedure for new objects",
                      nullptr);
    }
    ts->u.ctemplate->unref = s = hoc_table_lookup("unref", ts->u.ctemplate->symtable);
    if (s && s->type != PROCEDURE) {
        hoc_execerror(
            "'unref' can only be used as the callback procedure when the reference count is "
            "decremented",
            nullptr);
    }
}

void class2oc_base(const char* name,
                   void* (*cons)(Object*),
                   void (*destruct)(void*),
                   Member_func* m,
                   int (*checkpoint)(void**),
                   Member_ret_obj_func* mobjret,
                   Member_ret_str_func* strret) {
    extern int hoc_main1_inited_;
    Symbol *tsym, *s;
    cTemplate* t;
    int i;

    if (hoc_lookup(name)) {
        hoc_execerror(name, "already being used as a name");
    }

    tsym = hoc_install(name, UNDEF, 0.0, &hoc_symlist);
    tsym->subtype = CPLUSOBJECT;
    hoc_begintemplate(tsym);
    t = tsym->u.ctemplate;
    if (!hoc_main1_inited_ && t->id > hoc_max_builtin_class_id) {
        hoc_max_builtin_class_id = t->id;
    }
    t->constructor = cons;
    t->destructor = destruct;
    t->steer = 0;
    t->checkpoint = checkpoint;

    if (m)
        for (i = 0; m[i].name; ++i) {
            s = hoc_install(m[i].name, FUNCTION, 0.0, &hoc_symlist);
            s->u.u_proc->defn.pfd_vp = m[i].member;
            hoc_add_publiclist(s);
        }
    if (mobjret)
        for (i = 0; mobjret[i].name; ++i) {
            s = hoc_install(mobjret[i].name, OBFUNCTION, 0.0, &hoc_symlist);
            s->u.u_proc->defn.pfo_vp = mobjret[i].member;
            hoc_add_publiclist(s);
        }
    if (strret)
        for (i = 0; strret[i].name; ++i) {
            s = hoc_install(strret[i].name, STRFUNCTION, 0.0, &hoc_symlist);
            s->u.u_proc->defn.pfs_vp = strret[i].member;
            hoc_add_publiclist(s);
        }
    hoc_endtemplate(tsym);
}


void class2oc(const char* name,
              void* (*cons)(Object*),
              void (*destruct)(void*),
              Member_func* m,
              int (*checkpoint)(void**),
              Member_ret_obj_func* mobjret,
              Member_ret_str_func* strret) {
    class2oc_base(name, cons, destruct, m, checkpoint, mobjret, strret);
    py_exposed_classes.push_back(name);
}

Symbol* hoc_decl(Symbol* s) {
    Symbol* ss;
    if (templatestackp == templatestack) {
        if (s == hoc_table_lookup(s->name, hoc_built_in_symlist)) {
            hoc_execerror(s->name, ": Redeclaring at top level");
        }
        return s;
    }
    ss = hoc_table_lookup(s->name, hoc_symlist);
    if (!ss) {
        ss = hoc_install(s->name, UNDEF, 0.0, &hoc_symlist);
    }
    return ss;
}

void hoc_add_publiclist(Symbol* s) {
    Symbol* ss;
#if PDEBUG
    printf("public name %s with type %d\n", s->name, s->type);
#endif
    if (templatestackp == templatestack) {
        hoc_execerror("Not in a template\n", 0);
    }
    ss = hoc_decl(s);
    ss->cpublic = PUBLIC_TYPE;
}

void hoc_external_var(Symbol* s) {
    Symbol* s0;
    if (templatestackp == templatestack) {
        hoc_execerror("Not in a template\n", 0);
    }
    if (s->cpublic == PUBLIC_TYPE) {
        hoc_execerror(s->name, "can't be public and external");
    }
    s->cpublic = EXTERNAL_TYPE;
    s0 = hoc_table_lookup(s->name, hoc_top_level_symlist);
    if (!s0) {
        hoc_execerror(s->name, "not declared at the top level");
    }
    s->type = s0->type;
    s->subtype = s0->subtype;
    switch (s->type) {
    case FUNCTION:
    case PROCEDURE:
    case ITERATOR:
    case HOCOBJFUNCTION:
        s->u.u_proc = s0->u.u_proc;
        break;
    case TEMPLATE:
        s->u.ctemplate = s0->u.ctemplate;
        break;
    case STRING:
    case OBJECTVAR:
    case VAR:
    case SECTION:
        s->arayinfo = s0->arayinfo;
        s->u.sym = s0;
        break;
    default:
        hoc_execerror(s->name, "type is not allowed external");
        break;
    }
}

void hoc_ob_check(int type) {
    int t;
    t = ipop();
    if (type == -1) {
        if (t == OBJECTVAR) { /* don't bother to check */
            Code(hoc_cmp_otype);
            codei(0);
        }
    } else if (type) {
        if (t == OBJECTVAR) { /* must check dynamically */
#if PDEBUG
            printf("dymnamic checking of type=%d\n", type);
#endif
            Code(hoc_cmp_otype);
            codei(type);
        } else if (type != t) { /* static check */
            hoc_execerror("Type mismatch", (char*) 0);
        }
    } else {
        if (t != OBJECTVAR) {
            Code(hoc_known_type);
            codei(t);
        }
    }
}

void hoc_free_allobjects(cTemplate* ctemplate, Symlist* sl, Objectdata* data) {
    /* look in all object variables that point to
        objects with this template and null them */
    Symbol* s;
    int total, i;
    Object** obp;

    if (sl)
        for (s = sl->first; s; s = s->next) {
            if (s->type == OBJECTVAR && s->cpublic != 2) {
                total = hoc_total_array_data(s, data);
                for (i = 0; i < total; i++) {
                    obp = data[s->u.oboff].pobj + i;
                    if (*obp) {
#if 1
                        if ((*obp)->ctemplate == ctemplate) {
                            hoc_dec_refcount(obp);
                        } else if (s->subtype != CPLUSOBJECT) {
                            /* descend to look for more */
                            hoc_free_allobjects(ctemplate,
                                                (*obp)->ctemplate->symtable,
                                                (*obp)->u.dataspace);
                        }
#else
                        if (s->subtype != CPLUSOBJECT) {
                            /* descend to look for more */
                            hoc_free_allobjects(ctemplate,
                                                (*obp)->ctemplate->symtable,
                                                (*obp)->u.dataspace);
                        }
                        if ((*obp)->ctemplate == ctemplate) {
                            hoc_dec_refcount(obp);
                        }
#endif
                    }
                }
            }
        }
}

#define objectpath  hoc_objectpath_impl
#define pathprepend hoc_path_prepend

constexpr std::size_t hoc_object_pathname_bufsize = 512;
void pathprepend(char* path, const char* name, const char* indx) {
    char buf[200];
    if (path[0]) {
        strcpy(buf, path);
        std::snprintf(path, hoc_object_pathname_bufsize, "%s%s.%s", name, indx, buf);
    } else {
        std::snprintf(path, hoc_object_pathname_bufsize, "%s%s", name, indx);
    }
}

int objectpath(Object* ob, Object* oblook, char* path, int depth) {
    /* recursively build the pathname to the object */
    Symbol* s;
    Symlist* sl;
    int total, i;
    Objectdata* od;
    Object** obp;

    if (ob == oblook) {
        return 1;
    }
    if (oblook) {
        if (depth++ > 5) {
            hoc_warning("objectpath depth > 4 for", oblook->ctemplate->sym->name);
            return 0;
        }
        if (oblook->ctemplate->constructor) {
            return ivoc_list_look(ob, oblook, path, depth);
        } else {
            od = oblook->u.dataspace;
            sl = oblook->ctemplate->symtable;
        }
    } else {
        od = hoc_top_level_data;
        sl = hoc_top_level_symlist;
    }
    if (sl)
        for (s = sl->first; s; s = s->next) {
            if (s->type == OBJECTVAR && s->cpublic != 2) {
                total = hoc_total_array_data(s, od);
                for (i = 0; i < total; i++) {
                    obp = od[s->u.oboff].pobj + i;
                    if (*obp && *obp != oblook && objectpath(ob, *obp, path, depth)) {
                        pathprepend(path, s->name, hoc_araystr(s, i, od));
                        return 1;
                    }
                }
            }
        }
    return 0;
}

char* hoc_object_pathname(Object* ob) {
    static char path[hoc_object_pathname_bufsize];
    path[0] = '\0';
    if (objectpath(ob, nullptr, path, 0)) {
        return path;
    } else {
#if 0
		hoc_warning("Couldn't find a pathname to the object pointer",
			ob->ctemplate->sym->name);
		return (char*)0;
#else
        return hoc_object_name(ob);
#endif
    }
}

void hoc_obj_ref(Object* obj) {
    if (obj) {
        ++obj->refcount;
    }
}

void hoc_dec_refcount(Object** pobj) {
    Object* obj;

    obj = *pobj;
    if (obj == (Object*) 0) {
        return;
    }
    *pobj = (Object*) 0;
    assert(obj->refcount > 0);
    hoc_obj_unref(obj);
}

namespace {
struct helper_in_case_dtor_throws {
    helper_in_case_dtor_throws(Object* obj)
        : m_obj{obj} {}
    ~helper_in_case_dtor_throws() {
        if (--m_obj->ctemplate->count <= 0) {
            m_obj->ctemplate->index = 0;
        }
        m_obj->ctemplate = nullptr;
        /* for testing purposes we don't free the object in order
        to make sure no object variable ever uses a freed object */
        hoc_free_object(m_obj);
    }

  private:
    Object* m_obj;
};
}  // namespace

void hoc_obj_unref(Object* obj) {
    Object* obsav;

    if (!obj) {
        return;
    }
#if 0
printf("unreffing %s with refcount %d\n", hoc_object_name(obj), obj->refcount);
#endif
    --obj->refcount;
    if (obj->ctemplate->unref) {
        int i = obj->refcount;
        pushx((double) i);
        ++obj->unref_recurse_cnt;
        call_ob_proc(obj, obj->ctemplate->unref, 1);
        --obj->unref_recurse_cnt;
    }
    if (obj->refcount <= 0 && obj->unref_recurse_cnt == 0) {
        if (obj->aliases) {
            ivoc_free_alias(obj);
        }
        if (obj->observers) {
            hoc_obj_disconnect(obj);
        }
        hoc_l_delete(obj->itm_me);
        if (obj->ctemplate->observers) {
            hoc_template_notify(obj, 0);
        }
        // make sure that dereffing happens even if the destructor throws
        helper_in_case_dtor_throws _{obj};
        if (obj->ctemplate->sym->subtype & (CPLUSOBJECT | JAVAOBJECT)) {
            if (auto* const tp = obj->u.this_pointer; tp) {
                neuron::oc::invoke_method_that_may_throw(
                    [obj]() {
                        std::string rval{hoc_object_name(obj)};
                        rval.append(" destructor");
                        return rval;
                    },
                    obj->ctemplate->destructor,
                    tp);
            }
        } else {
            obsav = hoc_thisobject;
            hoc_thisobject = obj;
            free_objectdata(obj->u.dataspace, obj->ctemplate);
            obj->u.dataspace = (Objectdata*) 0;
            hoc_thisobject = obsav;
        }
    }
}

static void free_objectdata(Objectdata* od, cTemplate* ctemplate) {
    Symbol* s;
    int i, total;
    Objectdata* psav;
    Object** objp;

    psav = hoc_objectdata;
    hoc_objectdata = od;
    if (ctemplate->symtable)
        for (s = ctemplate->symtable->first; s; s = s->next) {
            if (s->cpublic != 2) {
                switch (s->type) {
                case VAR:
                    /*printf("free_objectdata %s\n", s->name);*/
                    hoc_free_val_array(OPVAL(s), hoc_total_array(s));
                    free_arrayinfo(OPARINFO(s));
                    break;
                case STRING:
                    hoc_free_pstring(OPSTR(s));
                    free_arrayinfo(OPARINFO(s));
                    break;
                case OBJECTVAR:
                    objp = OPOBJ(s);
                    if (strcmp("this", s->name) != 0) {
                        total = hoc_total_array(s);
                        for (i = 0; i < total; i++) {
                            hoc_dec_refcount(objp + i);
                        }
                    }
                    free_arrayinfo(OPARINFO(s));
                    free(objp);
                    break;
                case SECTION:
                    total = hoc_total_array(s);
                    for (i = 0; i < total; ++i) {
                        sec_free(*(OPSECITM(s) + i));
                    }
                    free(OPSECITM(s));
                    free_arrayinfo(OPARINFO(s));
                    break;
                }
            }
        }
    if (ctemplate->is_point_) {
        void* v = od[ctemplate->dataspace_size - 1]._pvoid;
        if (v) {
            /*			printf("Free point process\n");*/
            destroy_point_process(v);
        }
    }
    hoc_objectdata = psav;
    if (od) {
        free((char*) od);
    }
}


static void hoc_allobjects1(Symlist* sl, int nspace);
static void hoc_allobjects2(Symbol* s, int nspace);

void hoc_allobjects(void) {
    int n = 0;
    if (ifarg(1)) {
        if (hoc_is_str_arg(1)) {
            hoc_allobjects2(hoc_lookup(gargstr(1)), 0);
        } else {
            Object** o = hoc_objgetarg(1);
            if (*o) {
                n = (*o)->refcount;
            }
        }
    } else {
        hoc_allobjects1(hoc_built_in_symlist, 0);
        hoc_allobjects1(hoc_top_level_symlist, 0);
    }
    hoc_ret();
    pushx((double) n);
}

void hoc_allobjects1(Symlist* sl, int nspace) {
    Symbol* s;
    cTemplate* t;
    Object* o;
    hoc_Item* q;
    int i;
    if (sl)
        for (s = sl->first; s; s = s->next) {
            if (s->type == TEMPLATE) {
                t = s->u.ctemplate;
                ITERATE(q, t->olist) {
                    o = OBJ(q);
                    for (i = 0; i < nspace; ++i) {
                        Printf("   ");
                    }
                    Printf("%s with %d refs\n", hoc_object_name(o), o->refcount);
                }
                hoc_allobjects1(t->symtable, nspace + 1);
            }
        }
}

void hoc_allobjects2(Symbol* s, int nspace) {
    cTemplate* t;
    Object* o;
    hoc_Item* q;
    int i;
    if (s && s->type == TEMPLATE) {
        t = s->u.ctemplate;
        ITERATE(q, t->olist) {
            o = OBJ(q);
            for (i = 0; i < nspace; ++i) {
                Printf("   ");
            }
            Printf("%s with %d refs\n", hoc_object_name(o), o->refcount);
        }
    }
}

static void hoc_list_allobjref(Symlist*, Objectdata*, int);

void hoc_allobjectvars(void) {
    hoc_list_allobjref(hoc_top_level_symlist, hoc_top_level_data, 0);
    hoc_ret();
    pushx(0.);
}

static void hoc_list_allobjref(Symlist* sl, Objectdata* data, int depth) {
    /* look in all object variables that point to
        objects and print them */
    Symbol* s;
    int total, i, id;
    Object** obp;

    if (sl)
        for (s = sl->first; s; s = s->next) {
            if (s->type == OBJECTVAR && s->cpublic != 2) {
                total = hoc_total_array_data(s, data);
                for (i = 0; i < total; i++) {
                    obp = data[s->u.oboff].pobj + i;
                    for (id = 0; id < depth; id++) {
                        Printf("   ");
                    }
                    if (*obp) {
                        Printf("obp %s[%d] -> %s with %d refs.\n",
                               s->name,
                               i,
                               hoc_object_name(*obp),
                               (*obp)->refcount);
                    } else {
                        Printf("obp %s[%d] -> NULL\n", s->name, i);
                    }
                    if (*obp && !(*obp)->recurse && s->subtype != CPLUSOBJECT &&
                        (*obp)->u.dataspace != data /* not this */
                    ) {
                        /* descend to look for more */
                        (*obp)->recurse = 1;
                        hoc_list_allobjref((*obp)->ctemplate->symtable,
                                           (*obp)->u.dataspace,
                                           depth + 1);
                        (*obp)->recurse = 0;
                    }
                }
            }
        }
}

void check_obj_type(Object* obj, const char* type_name) {
    char buf[100];
    if (!obj || strcmp(obj->ctemplate->sym->name, type_name) != 0) {
        if (obj) {
            Sprintf(buf, "object type is %s instead of", obj->ctemplate->sym->name);
        } else {
            Sprintf(buf, "object type is nullptr instead of");
        }
        hoc_execerror(buf, type_name);
    }
}

int is_obj_type(Object* obj, const char* type_name) {
    if (obj && strcmp(obj->ctemplate->sym->name, type_name) == 0) {
        return 1;
    } else {
        return 0;
    }
}


void* nrn_opaque_obj2pyobj(Object* ho) {
    // The PyObject* reference is not incremented. Use only as last resort
    if (neuron::python::methods.opaque_obj2pyobj) {
        return neuron::python::methods.opaque_obj2pyobj(ho);
    }
    return nullptr;
}
