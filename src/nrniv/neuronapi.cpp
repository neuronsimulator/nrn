#include "neuronapi.h"

#include "../../nrnconf.h"
#include "hocdec.h"
#include "cabcode.h"
#include "nrniv_mf.h"
#include "nrnmpi.h"
#include "nrnmpiuse.h"
#include "ocfunc.h"
#include "ocjump.h"
#include "parse.hpp"
#include "nrn_ansi.h"
#include "section.h"
#include "shapeplt.h"
#include <cstring>
#include <exception>
#include <limits>

/// A public face of hoc_Item
struct nrn_Item: public hoc_Item {};

struct SectionListIterator {
    explicit SectionListIterator(nrn_Item*);
    Section* next(void);
    int done(void) const;

  private:
    hoc_Item* initial;
    hoc_Item* current;
};

struct SymbolTableIterator {
    explicit SymbolTableIterator(Symlist*);
    Symbol* next(void);
    int done(void) const;

  private:
    Symbol* current;
};

/****************************************
 * Connections to the rest of NEURON
 ****************************************/
extern int nrn_nobanner_;
extern int diam_changed;
extern int nrn_try_catch_nest_depth;
extern "C" void nrnpy_set_pr_etal(int (*cbpr_stdoe)(int, char*), int (*cbpass)());
int ivocmain_session(int, const char**, const char**, int start_session);
void simpleconnectsection();
extern Object* hoc_newobj1(Symbol*, int);
extern std::tuple<int, const char**> nrn_mpi_setup(int argc, const char** argv);

extern "C" {

/****************************************
 * Initialization
 ****************************************/

int nrn_init(int argc, const char** argv) {
    nrn_nobanner_ = 1;
    auto [final_argc, final_argv] = nrn_mpi_setup(argc, argv);
    errno = 0;
    return ivocmain_session(final_argc, final_argv, nullptr, 0);
}

void nrn_stdout_redirect(int (*myprint)(int, char*)) {
    // the first argument of myprint is an integer indicating the output stream
    // if the int is 1, then stdout, else stderr
    // the char* is the message to display
    nrnpy_set_pr_etal(myprint, nullptr);
}

/****************************************
 * Sections
 ****************************************/

Section* nrn_section_new(char const* const name) {
    auto* symbol = new Symbol;
    auto pitm = new hoc_Item*;
    symbol->name = strdup(name);
    symbol->type = 1;
    symbol->u.oboff = 0;
    symbol->arayinfo = 0;
    hoc_install_object_data_index(symbol);
    hoc_top_level_data[symbol->u.oboff].psecitm = pitm;
    new_sections(nullptr, symbol, pitm, 1);
    return (*pitm)->element.sec;
}

void nrn_section_connect(Section* child_sec, double child_x, Section* parent_sec, double parent_x) {
    nrn_pushsec(child_sec);
    hoc_pushx(child_x);
    nrn_pushsec(parent_sec);
    hoc_pushx(parent_x);
    simpleconnectsection();
}

void nrn_section_length_set(Section* sec, const double length) {
    // TODO: call can_change_morph(sec) to check pt3dconst_; how should we handle
    // that?
    // TODO: is there a named constant so we don't have to use the magic number 2?
    sec->prop->dparam[2] = length;
    // nrn_length_change updates 3D points if needed
    nrn_length_change(sec, length);
    diam_changed = 1;
    sec->recalc_area_ = 1;
}

double nrn_section_length_get(Section* sec) {
    return section_length(sec);
}

double nrn_section_Ra_get(Section* sec) {
    return nrn_ra(sec);
}

void nrn_section_Ra_set(Section* sec, double const val) {
    // TODO: ensure val > 0
    // TODO: is there a named constant so we don't have to use the magic number 7?
    sec->prop->dparam[7] = val;
    diam_changed = 1;
    sec->recalc_area_ = 1;
}

double nrn_section_rallbranch_get(const Section* sec) {
    return sec->prop->dparam[4].get<double>();
}

void nrn_section_rallbranch_set(Section* sec, double const val) {
    // TODO: is there a named constant so we don't have to use the magic number 4?
    sec->prop->dparam[4] = val;
    diam_changed = 1;
    sec->recalc_area_ = 1;
}

char const* nrn_secname(Section* sec) {
    return secname(sec);
}

void nrn_section_push(Section* sec) {
    nrn_pushsec(sec);
}

void nrn_section_pop(void) {
    nrn_sec_pop();
}

void nrn_mechanism_insert(Section* sec, const Symbol* mechanism) {
    // TODO: throw exception if mechanism is not an insertable mechanism?
    mech_insert1(sec, mechanism->subtype);
}

bool nrn_section_is_active(const Section* sec) {
    if (!sec->prop) {
        return false;
    }
    return true;
}

void nrn_section_ref(Section* sec) {
    section_ref(sec);
}

void nrn_section_unref(Section* sec) {
    section_unref(sec);
}

Section* nrn_cas(void) {
    Section* sec = nrn_noerr_access();
    return sec;
}

/****************************************
 * Segments
 ****************************************/

int nrn_nseg_get(const Section* sec) {
    // always one more node than nseg
    return sec->nnode - 1;
}

void nrn_nseg_set(Section* const sec, const int nseg) {
    nrn_change_nseg(sec, nseg);
}

void nrn_segment_diam_set(Section* const sec, const double x, const double diam) {
    Node* const node = node_exact(sec, x);
    // TODO: this is fine if no 3D points; does it work if there are 3D points?
    for (auto prop = node->prop; prop; prop = prop->next) {
        if (prop->_type == MORPHOLOGY) {
            prop->param(0) = diam;
            diam_changed = 1;
            node->sec->recalc_area_ = 1;
            break;
        }
    }
}

double nrn_segment_diam_get(Section* const sec, const double x) {
    // Geometry from 3d points writes diam lazily, gated per-section on
    // recalc_area_. Mirror the range-variable read path (nrnpy_nrn.cpp) so a
    // diam read after pt3dadd returns the 3d-derived value, not a stale default.
    if (sec && sec->recalc_area_) {
        // nrn_area_ri normally reports a non-positive diameter with
        // hoc_execerror after clamping it to 1e-6. A C value accessor cannot
        // let that C++ exception unwind across its ABI, so complete the same
        // recompute without raising the HOC-level diagnostic.
        nrn_area_ri_no_diam_error(sec);
    }
    Node* const node = node_exact(sec, x);
    for (auto prop = node->prop; prop; prop = prop->next) {
        if (prop->_type == MORPHOLOGY) {
            return prop->param(0);
        }
    }
    return 0.0;
}

int nrn_segment_node_index(Section* const sec, const double x) {
    if (!sec || !sec->prop) {
        return -1;
    }
    return node_exact(sec, x)->v_node_index;
}

double nrn_rangevar_get(Symbol* sym, Section* sec, double x) {
    return *nrn_rangepointer(sec, sym, x);
}

void nrn_rangevar_set(Symbol* sym, Section* sec, double x, double value) {
    *nrn_rangepointer(sec, sym, x) = value;
}

void nrn_rangevar_push(Symbol* sym, Section* sec, double x) {
    hoc_push(nrn_rangepointer(sec, sym, x));
}

Object* nrn_segment_nmodlrandom_get(Section* sec, double x, Symbol* sym) {
    if (!sec || !nrn_section_is_active(sec) || !(x >= 0.0 && x <= 1.0) || !sym ||
        sym->type != RANGEOBJ || sym->subtype != NMODLRANDOM) {
        return nullptr;
    }
    Prop* prop = nrn_mechanism(sym->u.rng.type, node_exact(sec, x));
    if (!prop) {
        return nullptr;
    }
    Object* obj = nrn_nmodlrandom_wrap(prop, sym);
    hoc_obj_ref(obj);
    return obj;
}

Object* nrn_pntproc_nmodlrandom_get(Object* point_process, Symbol* sym) {
    if (!point_process || !point_process->ctemplate || !point_process->ctemplate->is_point_ ||
        !sym || sym->type != RANGEOBJ || sym->subtype != NMODLRANDOM ||
        hoc_table_lookup(sym->name, point_process->ctemplate->symtable) != sym) {
        return nullptr;
    }
    auto* pnt = ob2pntproc_0(point_process);
    if (!pnt || !pnt->prop) {
        return nullptr;
    }
    Object* obj = nrn_pntproc_nmodlrandom_wrap(pnt, sym);
    hoc_obj_ref(obj);
    return obj;
}

int nrn_setpointer_pop(Symbol* pointer_sym,
                       Section* sec,
                       double x,
                       char* error_msg,
                       size_t error_msg_size) {
    if (error_msg && error_msg_size > 0) {
        error_msg[0] = '\0';
    }
    auto fail = [&](const char* msg) {
        if (error_msg && error_msg_size > 0) {
            std::snprintf(error_msg, error_msg_size, "%s", msg);
        }
        return 1;
    };
    // The source pointer is whatever the caller pushed (e.g. via
    // nrn_rangevar_push), which reuses every existing way of obtaining one. Pop
    // it first, before the validations below, so the stack stays balanced even
    // on the error paths. It is the same data handle a dparam slot holds.
    neuron::container::data_handle<double> src = hoc_pop_handle<double>();
    // Only a mechanism POINTER range variable can be a setpointer target; the
    // slot it wires lives in the mechanism's dparam array (mirrors the guard in
    // nrn_pointer_assign / nrnpy_nrn.cpp).
    if (!pointer_sym || pointer_sym->type != RANGEVAR || pointer_sym->subtype != NRNPOINTER) {
        return fail("target is not a POINTER range variable");
    }
    // Locate the mechanism instance that owns this POINTER at (sec, x).
    Node* const nd = node_exact(sec, x);
    Prop* prop = nullptr;
    for (Prop* p = nd->prop; p; p = p->next) {
        if (p->_type == pointer_sym->u.rng.type) {
            prop = p;
            break;
        }
    }
    if (!prop) {
        return fail("the POINTER's mechanism is not present at the target segment");
    }
    // Wire the POINTER to the popped source handle (stable across data
    // permutation), the same assignment nrn_pointer_assign performs without the
    // PyObject* source.
    prop->dparam[pointer_sym->u.rng.index] = src;
    return 0;
}

int nrn_pp_setpointer_pop(Object* pp, const char* name, char* error_msg, size_t error_msg_size) {
    if (error_msg && error_msg_size > 0) {
        error_msg[0] = '\0';
    }
    auto fail = [&](const char* msg) {
        if (error_msg && error_msg_size > 0) {
            std::snprintf(error_msg, error_msg_size, "%s", msg);
        }
        return 1;
    };
    // Pop the source first (as nrn_setpointer_pop does) so the stack stays
    // balanced on every error path below. It is the same data handle a dparam
    // slot holds, obtained any way the stack supports (nrn_rangevar_push,
    // nrn_property_push, ...).
    neuron::container::data_handle<double> src = hoc_pop_handle<double>();
    // A point process is addressed by its instance object, not by (sec, x):
    // several point processes may share one location, so the segment alone
    // cannot say which instance owns the POINTER slot. This is the difference
    // from nrn_setpointer_pop, whose (sec, x) uniquely identifies the single
    // density-mechanism instance at a segment.
    if (!pp || !pp->ctemplate || !pp->ctemplate->is_point_) {
        return fail("object is not a point process");
    }
    // Resolve the POINTER by name in the point process's own symbol table, the
    // same lookup nrn_property_get uses (bare name, e.g. "vgap").
    Symbol* pointer_sym = hoc_table_lookup(name, pp->ctemplate->symtable);
    if (!pointer_sym || pointer_sym->type != RANGEVAR || pointer_sym->subtype != NRNPOINTER) {
        return fail("target is not a POINTER variable of this point process");
    }
    auto* const pnt = ob2pntproc_0(pp);
    if (!pnt || !pnt->prop) {
        return fail("point process is not located in a section");
    }
    // Wire the POINTER to the popped source handle -- the same slot assignment
    // nrn_setpointer_pop and nrn_pointer_assign perform, minus the PyObject*.
    pnt->prop->dparam[pointer_sym->u.rng.index] = src;
    return 0;
}

nrn_Item* nrn_allsec(void) {
    return static_cast<nrn_Item*>(section_list);
}

nrn_Item* nrn_sectionlist_data(const Object* obj) {
    // TODO: verify the obj is in fact a SectionList
    return (nrn_Item*) obj->u.this_pointer;
}

Section* nrn_section_parent(Section* sec) {
    // The section sec is connected to (SectionRef.parent), or NULL if sec is a
    // root. This is the raw connectivity; nrn_section_trueparent additionally
    // walks through parents joined at their 0 end.
    if (!sec) {
        return nullptr;
    }
    return sec->parentsec;
}

Section* nrn_section_trueparent(Section* sec) {
    // SectionRef.trueparent: the parent, except that a section connected to the
    // 0 end of its parent shares that parent's true parent, so the walk climbs
    // until the connection is not at the beginning. NULL if there is none.
    if (!sec) {
        return nullptr;
    }
    return nrn_trueparent(sec);
}

Section* nrn_section_child(Section* sec) {
    // First child connected to sec, or NULL if it has none. Walk the rest with
    // nrn_section_sibling; the order matches SectionRef.child[i].
    if (!sec) {
        return nullptr;
    }
    return sec->child;
}

Section* nrn_section_sibling(Section* sec) {
    // Next section sharing sec's parent, or NULL. With nrn_section_child this
    // iterates every child of a section:
    //   for (Section* c = nrn_section_child(p); c; c = nrn_section_sibling(c))
    if (!sec) {
        return nullptr;
    }
    return sec->sibling;
}

int nrn_sectionlist_to_array(nrn_Item* sl, Section** buf, int maxlen) {
    // Snapshot a section list (from nrn_allsec() or nrn_sectionlist_data(obj))
    // into buf in a single call -- the batched form of the section-list
    // iterator, which crosses the API boundary once per section. Building a
    // section array this way (an allsec gather, rebuilt whenever the topology
    // changes) becomes one crossing instead of one per section. The list is a
    // circular doubly-linked list of hoc_Item with sl as the sentinel, so walk
    // from sl->next back around to sl. Semi-deleted sections (no Section, or a
    // freed prop) are skipped, matching SectionListIterator; read-only, it does
    // not prune them.
    //
    // Fills up to maxlen entries and returns the TOTAL number of live sections,
    // which may exceed maxlen when buf is too small. Call with buf=NULL and
    // maxlen=0 to obtain just the count in one pass. Detecting whether a cached
    // snapshot has gone stale is a separate matter -- watch structure_change_cnt.
    if (!sl) {
        return 0;
    }
    int total = 0;
    for (hoc_Item* q = sl->next; q && q != sl; q = q->next) {
        Section* sec = q->element.sec;
        if (sec && sec->prop != nullptr) {
            if (buf && total < maxlen) {
                buf[total] = sec;
            }
            ++total;
        }
    }
    return total;
}

/****************************************
 * Functions, objects, and the stack
 ****************************************/

Symbol* nrn_symbol(char const* const name) {
    return hoc_lookup(name);
}

int nrn_symbol_type(const Symbol* sym) {
    // TODO: these types are in parse.hpp and are not the same between versions,
    // so we really should wrap
    return sym->type;
}

int nrn_symbol_subtype(const Symbol* sym) {
    return sym->subtype;
}

double* nrn_symbol_dataptr(const Symbol* sym) {
    // Only a scalar/array VAR has a real double* to hand back. Anything else --
    // a function (e.g. finitialize), an object, a string, a template (all with
    // type != VAR), or a section-level property (USERPROPERTY: nseg/L/Ra/
    // rallbranch, whose u member is a {membrane type, index} pair, not a
    // pointer) -- has no dataptr, so return nullptr instead of a reinterpreted
    // union member that the caller would dereference as garbage.
    if (!sym || sym->type != VAR) {
        return nullptr;
    }
    switch (sym->subtype) {
    case NOTUSER:
        // A NOTUSER runtime scalar (created in HOC by e.g. `x = 42`) does not
        // store its value at sym->u.pval -- for that subtype the union member
        // holds an object-data offset, not a pointer. The real storage is in
        // the top-level object-data array (see the NOTUSER branch of eval() in
        // oc/code.cpp, which reads *OPVAL(sym) ==
        // *hoc_top_level_data[sym->u.oboff].pval). Return that address so the
        // result is a dereferenceable double*, as the name promises.
        return hoc_top_level_data[sym->u.oboff].pval;
    case USERPROPERTY:
        return nullptr;
    default:
        // USERDOUBLE built-ins such as `t`, plus the typed USERINT/USERFLOAT
        // scalars whose storage aliases sym->u.pval (callers cast as needed).
        return sym->u.pval;
    }
}

Object* nrn_symbol_object_get(const Symbol* sym) {
    // A top-level objref (`objref o`) stores its Object* in the top-level
    // object-data array, not at sym->u.pval. Returns the bound object, or NULL
    // if the objref is nil or `sym` is not an objref. The object is returned
    // borrowed (its reference count is not incremented); call nrn_object_ref to
    // retain it past the next assignment to this objref.
    if (!sym || sym->type != OBJECTVAR) {
        return nullptr;
    }
    return hoc_top_level_data[sym->u.oboff].pobj[0];
}

bool nrn_symbol_object_set(Symbol* sym, Object* obj) {
    // Bind `obj` to a top-level objref, following HOC's assignment refcount
    // rules: release the previously bound object and retain the new one. A NULL
    // obj clears the objref (makes it nil). Returns true on success, false if
    // `sym` is not an objref.
    if (!sym || sym->type != OBJECTVAR) {
        return false;
    }
    Object** cell = hoc_top_level_data[sym->u.oboff].pobj;
    hoc_dec_refcount(cell);  // unref the old content and NULL the cell
    *cell = obj;
    hoc_obj_ref(obj);  // NULL-safe
    return true;
}

const char* nrn_symbol_str_get(const Symbol* sym) {
    // A top-level strdef (`strdef s`) stores its char* in the top-level
    // object-data array. Returns the string, or NULL if `sym` is not a strdef.
    if (!sym || sym->type != STRING) {
        return nullptr;
    }
    return hoc_top_level_data[sym->u.oboff].ppstr[0];
}

bool nrn_symbol_str_set(Symbol* sym, const char* value) {
    // Copy `value` into a top-level strdef's storage (freeing the previous
    // string), via the same helper HOC string assignment uses. Returns true on
    // success, false if `sym` is not a strdef.
    if (!sym || sym->type != STRING) {
        return false;
    }
    hoc_assign_str(hoc_top_level_data[sym->u.oboff].ppstr, value);
    return true;
}

bool nrn_symbol_is_array(const Symbol* sym) {
    return sym->arayinfo != nullptr;
}

void nrn_symbol_push(Symbol* sym) {
    hoc_pushpx(sym->u.pval);
}

Symbol* nrn_symbol_pop(void) {
    // Pop a Symbol (a STACK_IS_SYM entry) off the interpreter stack. Interpreter
    // frames for object-component access (e.g. reading or assigning pyobj.attr)
    // carry the attribute's Symbol on the stack; a binding that unwinds such a
    // frame needs to pop it. Public counterpart to the internal hoc_spop.
    return hoc_spop();
}

void nrn_double_push(double val) {
    hoc_pushx(val);
}

double nrn_double_pop(void) {
    return hoc_xpop();
}

void nrn_double_ptr_push(double* addr) {
    hoc_pushpx(addr);
}

double* nrn_double_ptr_pop(void) {
    return hoc_pxpop();
}

void nrn_str_push(char** str) {
    hoc_pushstr(str);
}

char** nrn_str_pop(void) {
    return hoc_strpop();
}

void nrn_int_push(int i) {
    hoc_pushi(i);
}

int nrn_int_pop(void) {
    if (hoc_stack_type_is_ndim()) {
        return hoc_pop_ndim();
    }
    return hoc_ipop();
}

void nrn_object_push(Object* obj) {
    hoc_push_object(obj);
}

void nrn_object_ptr_push(Object** obj_ref) {
    // Push a writable object-reference slot (the out-parameter form of
    // nrn_object_push). When a callee assigns to the corresponding $oN arg,
    // hoc assigns through this slot, updating *obj_ref in place. Unlike
    // nrn_object_push, which pushes an object by value, this exposes the
    // h.ref(obj) idiom (a callee that writes back into the caller's objref).
    // Named for the pointer it pushes (cf. nrn_double_ptr_push); the "ref" in
    // nrn_object_ref/unref is reference counting, a different concept.
    hoc_pushobj(obj_ref);
}

Object* nrn_object_pop(void) {
    // Returns NULL for a nil object reference (an unset objref) rather than
    // crashing: the ref-count bump that hands back a reference would otherwise
    // dereference NULL. This matters when unwinding a stack that may carry a nil
    // object -- e.g. the HOC-to-Python write-back path, where an objref RHS can
    // be nil. A non-NULL result is reference-counted and should be unref'd
    // (nrn_object_unref) when no longer needed.
    Object** obptr = hoc_objpop();
    Object* ob = *obptr;
    if (ob) {
        ob->refcount++;
    }
    hoc_tobj_unref(obptr);
    return ob;
}

nrn_stack_types_t nrn_stack_type(void) {
    switch (hoc_stack_type()) {
    case STRING:
        return STACK_IS_STR;
    case VAR:
        return STACK_IS_VAR;
    case NUMBER:
        return STACK_IS_NUM;
    case OBJECTVAR:
        return STACK_IS_OBJVAR;
    case OBJECTTMP:
        return STACK_IS_OBJTMP;
    case USERINT:
        return STACK_IS_INT;
    case SYMBOL:
        return STACK_IS_SYM;
    }
    return STACK_UNKNOWN;
}

char const* nrn_stack_type_name(nrn_stack_types_t id) {
    switch (id) {
    case STACK_IS_STR:
        return "STRING";
    case STACK_IS_VAR:
        return "VAR";
    case STACK_IS_NUM:
        return "NUMBER";
    case STACK_IS_OBJVAR:
        return "OBJECTVAR";
    case STACK_IS_OBJTMP:
        return "OBJECTTMP";
    case STACK_IS_INT:
        return "INT";
    case STACK_IS_SYM:
        return "SYMBOL";
    default:
        return "UNKNOWN";
    }
}

Object* nrn_object_new(Symbol* sym, int narg) {
    return hoc_newobj1(sym, narg);
}

Object* nrn_object_new_wrap(Symbol* sym, void* cpp_object) {
    // Wrap an existing C++ payload as a HOC Object of the class `sym` (which
    // must be a C++/CPLUSOBJECT template). Unlike nrn_object_new, which runs the
    // HOC constructor and pulls arguments off the stack, this backs the new
    // object directly with `cpp_object`, stored as its this_pointer. Pass
    // nullptr to construct an unbacked instance to fill in later. The returned
    // object has refcount 0; ref it (nrn_object_ref) to keep it alive.
    return hoc_new_object(sym, cpp_object);
}

int nrn_object_new_nothrow(Symbol* sym,
                           int narg,
                           Object** result,
                           char* error_msg,
                           size_t error_msg_size) {
    // Like nrn_object_new, but a HOC constructor error (bad arguments, a failing
    // INITIAL, etc.) is caught and reported instead of thrown, so a non-C++
    // caller (ctypes, MATLAB, ...) does not have a C++ exception propagate
    // across the FFI boundary. On success returns 0 with *result set; on error
    // returns nonzero with *result NULL and error_msg populated.
    if (error_msg && error_msg_size > 0) {
        error_msg[0] = '\0';
    }
    if (result) {
        *result = nullptr;
    }
    try {
        Object* obj = OcJump::newobj_throw_on_exception(sym, narg);
        if (result) {
            *result = obj;
        }
        return 0;
    } catch (const std::exception& e) {
        if (error_msg && error_msg_size > 0) {
            strncpy(error_msg, e.what(), error_msg_size - 1);
            error_msg[error_msg_size - 1] = '\0';
        }
        return 1;
    } catch (...) {
        if (error_msg && error_msg_size > 0) {
            strncpy(error_msg, "Unknown exception occurred", error_msg_size - 1);
            error_msg[error_msg_size - 1] = '\0';
        }
        return 1;
    }
}

Symbol* nrn_method_symbol(const Object* obj, char const* const name) {
    return hoc_table_lookup(name, obj->ctemplate->symtable);
}

void nrn_method_call(Object* obj, Symbol* method_sym, int narg) {
    OcJump::execute_throw_on_exception(obj, method_sym, narg);
}

void nrn_function_call(Symbol* sym, int narg) {
    // NOTE: this differs from hoc_call_func in that the response remains on the
    // stack
    OcJump::execute_throw_on_exception(sym, narg);
}

int nrn_method_call_nothrow(Object* obj,
                            Symbol* method_sym,
                            int narg,
                            char* error_msg,
                            size_t error_msg_size) {
    // Initialize error message buffer
    if (error_msg && error_msg_size > 0) {
        error_msg[0] = '\0';
    }

    try {
        OcJump::execute_throw_on_exception(obj, method_sym, narg);
        return 0;  // Success
    } catch (const std::exception& e) {
        if (error_msg && error_msg_size > 0) {
            strncpy(error_msg, e.what(), error_msg_size - 1);
            error_msg[error_msg_size - 1] = '\0';
        }
        return 1;  // Error
    } catch (...) {
        if (error_msg && error_msg_size > 0) {
            strncpy(error_msg, "Unknown exception occurred", error_msg_size - 1);
            error_msg[error_msg_size - 1] = '\0';
        }
        return 1;  // Error
    }
}

int nrn_function_call_nothrow(Symbol* sym, int narg, char* error_msg, size_t error_msg_size) {
    // Initialize error message buffer
    if (error_msg && error_msg_size > 0) {
        error_msg[0] = '\0';
    }

    try {
        OcJump::execute_throw_on_exception(sym, narg);
        return 0;  // Success
    } catch (const std::exception& e) {
        if (error_msg && error_msg_size > 0) {
            strncpy(error_msg, e.what(), error_msg_size - 1);
            error_msg[error_msg_size - 1] = '\0';
        }
        return 1;  // Error
    } catch (...) {
        if (error_msg && error_msg_size > 0) {
            strncpy(error_msg, "Unknown exception occurred", error_msg_size - 1);
            error_msg[error_msg_size - 1] = '\0';
        }
        return 1;  // Error
    }
}

void nrn_object_ref(Object* obj) {
    obj->refcount++;
}

void nrn_object_unref(Object* obj) {
    hoc_obj_unref(obj);
}

char const* nrn_class_name(const Object* obj) {
    return obj->ctemplate->sym->name;
}

bool nrn_prop_exists(const Object* obj) {
    return ob2pntproc_0(const_cast<Object*>(obj))->prop;
}

double nrn_distance(Section* sec0, double x0, Section* sec1, double x1) {
    Node* node0 = node_exact(sec0, x0);
    Node* node1 = node_exact(sec1, x1);
    Section* dummy_sec = nullptr;
    Node* dummy_node = nullptr;
    return topol_distance(sec0, node0, sec1, node1, &dummy_sec, &dummy_node);
}

/****************************************
 * Plot Shape
 ****************************************/

ShapePlotInterface* nrn_get_plotshape_interface(Object* ps) {
    ShapePlotInterface* spi;
    hoc_Item** my_section_list;
    spi = ((ShapePlotInterface*) ps->u.this_pointer);
    return spi;
}

Object* nrn_get_plotshape_section_list(ShapePlotInterface* spi) {
    return spi->neuron_section_list();
}

const char* nrn_get_plotshape_varname(ShapePlotInterface* spi) {
    return spi->varname();
}

float nrn_get_plotshape_low(ShapePlotInterface* spi) {
    return spi->low();
}

float nrn_get_plotshape_high(ShapePlotInterface* spi) {
    return spi->high();
}

/****************************************
 * Miscellaneous
 ****************************************/
int nrn_hoc_call(char const* const command) {
    return hoc_oc(command);
}

SectionListIterator::SectionListIterator(nrn_Item* my_sectionlist)
    : initial(my_sectionlist)
    , current(my_sectionlist->next) {}

Section* SectionListIterator::next() {
    if (!current) {
        return nullptr;
    }

    Section* sec = nullptr;
    while (current != initial) {
        // Save next pointer before possibly deleting current
        auto* q = current;
        current = current->next;
        sec = q->element.sec;

        // Check if the section is still valid
        if (!sec || sec->prop == nullptr) {
            // Unlink and delete invalid section
            if (q->prev) {
                q->prev->next = q->next;
            }
            if (q->next) {
                q->next->prev = q->prev;
            }
            delete q;
            continue;  // Try next
        }

        return sec;
    }

    return nullptr;
}

int SectionListIterator::done(void) const {
    if (initial == current) {
        return 1;
    }
    return 0;
}

SymbolTableIterator::SymbolTableIterator(Symlist* list)
    : current(list->first) {}

Symbol* SymbolTableIterator::next(void) {
    Symbol* result = current;
    current = current->next;
    return result;
}

int SymbolTableIterator::done(void) const {
    if (!current) {
        return 1;
    }
    return 0;
}

// copy semantics isn't great, but only two data items
// and is cleaner to use in a for loop than having to free memory at the end
SectionListIterator* nrn_sectionlist_iterator_new(nrn_Item* my_sectionlist) {
    return new SectionListIterator(my_sectionlist);
}

void nrn_sectionlist_iterator_free(SectionListIterator* sl) {
    delete sl;
}

Section* nrn_sectionlist_iterator_next(SectionListIterator* sl) {
    return sl->next();
}

int nrn_sectionlist_iterator_done(SectionListIterator* sl) {
    return sl->done();
}

SymbolTableIterator* nrn_symbol_table_iterator_new(Symlist* my_symbol_table) {
    return new SymbolTableIterator(my_symbol_table);
}

void nrn_symbol_table_iterator_free(SymbolTableIterator* st) {
    delete st;
}

Symbol* nrn_symbol_table_iterator_next(SymbolTableIterator* st) {
    return st->next();
}

int nrn_symbol_table_iterator_done(SymbolTableIterator* st) {
    return st->done();
}

int nrn_vector_capacity(const Object* vec) {
    // TODO: throw exception if vec is not a Vector
    return vector_capacity((IvocVect*) vec->u.this_pointer);
}

double* nrn_vector_data(Object* vec) {
    // TODO: throw exception if vec is not a Vector
    return vector_vec((IvocVect*) vec->u.this_pointer);
}

double nrn_property_get(const Object* obj, const char* name) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
        return *hoc_pxpop();
    } else {
        auto handle = point_process_pointer(ob2pntproc_0(const_cast<Object*>(obj)), sym, 0);
        return handle ? *handle : std::numeric_limits<double>::quiet_NaN();
    }
}

double nrn_property_array_get(const Object* obj, const char* name, int i) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
        return hoc_pxpop()[i];
    } else {
        auto handle = point_process_pointer(ob2pntproc_0(const_cast<Object*>(obj)), sym, i);
        return handle ? *handle : std::numeric_limits<double>::quiet_NaN();
    }
}

void nrn_property_set(Object* obj, const char* name, double value) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
        *hoc_pxpop() = value;
    } else {
        auto handle = point_process_pointer(ob2pntproc_0(obj), sym, 0);
        if (handle) {
            *handle = value;
        }
    }
}

void nrn_property_array_set(Object* obj, const char* name, int i, double value) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
        hoc_pxpop()[i] = value;
    } else {
        auto handle = point_process_pointer(ob2pntproc_0(obj), sym, i);
        if (handle) {
            *handle = value;
        }
    }
}

void nrn_property_push(Object* obj, const char* name) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
    } else {
        hoc_push(point_process_pointer(ob2pntproc_0(obj), sym, 0));
    }
}

void nrn_property_array_push(Object* obj, const char* name, int i) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
        hoc_pushpx(hoc_pxpop() + i);
    } else {
        hoc_push(point_process_pointer(ob2pntproc_0(obj), sym, i));
    }
}

bool nrn_property_data_handle_is_valid(const Object* obj, const char* name, int i) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        obj->ctemplate->steer(obj->u.this_pointer);
        return static_cast<bool>(hoc_pop_handle<double>());
    }
    return static_cast<bool>(point_process_pointer(ob2pntproc_0(const_cast<Object*>(obj)), sym, i));
}

char const* nrn_symbol_name(const Symbol* sym) {
    return sym->name;
}

Symlist* nrn_symbol_table(const Symbol* sym) {
    // TODO: ensure sym is an object or class
    // NOTE: to use with an object, call nrn_get_symbol(nrn_class_name(obj))
    return sym->u.ctemplate->symtable;
}

Symlist* nrn_global_symbol_table(void) {
    return hoc_built_in_symlist;
}

Symlist* nrn_top_level_symbol_table(void) {
    return hoc_top_level_symlist;
}

int nrn_symbol_array_length(const Symbol* sym) {
    return sym->arayinfo->sub[0];
}

// Function to register function/object in hoc
void nrn_register_function(void (*proc)(), const char* func_name, int type) {
    Symbol* sym;
    sym = hoc_install(func_name, type, 0, &hoc_top_level_symlist);
    sym->u.u_proc->defn.pf = proc;
    sym->u.u_proc->nauto = 0;
    sym->u.u_proc->nobjauto = 0;
}

void nrn_hoc_ret() {
    hoc_ret();
}

/****************************************
 * Parameter-reading functions
 ****************************************/
Object** nrn_objgetarg(int arg) {
    return hoc_objgetarg(arg);
}

char* nrn_gargstr(int arg) {
    return hoc_gargstr(arg);
}

double* nrn_getarg(int arg) {
    return hoc_getarg(arg);
}

std::FILE* nrn_obj_file_arg(int i) {
    return hoc_obj_file_arg(i);
}

bool nrn_ifarg(int arg) {
    return ifarg(arg);
}


bool nrn_is_object_arg(int arg) {
    return hoc_is_object_arg(arg);
}


bool nrn_is_str_arg(int arg) {
    return hoc_is_str_arg(arg);
}


bool nrn_is_double_arg(int arg) {
    return hoc_is_double_arg(arg);
}


bool nrn_is_pdouble_arg(int arg) {
    return hoc_is_pdouble_arg(arg);
}
}
