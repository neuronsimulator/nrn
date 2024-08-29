#include "neuronapi.h"

#include <cstdarg>
#include <iostream>
#include <variant>

#include "../../nrnconf.h"
#include "../oc/hocdec.h"
#include "../nrnoc/nrniv_mf.h"
#include "../oc/nrnmpi.h"
#include "../oc/oc_ansi.h"
#include "../oc/ocfunc.h"
#include "ocjump.h"
#include "../nrnoc/section.h"

#include "nrnmpiuse.h"
#include "parse.hpp"

/// A public interface of hoc_Item
struct NrnListItem: public hoc_Item {};

/// A public interface of a Segment
struct Segment: public Node {};

struct SectionListIterator {
    explicit SectionListIterator(NrnListItem*);
    Section* next(void);
    int done(void) const;

  private:
    hoc_Item* initial;
    hoc_Item* current;
};

struct SymbolTableIterator {
    explicit SymbolTableIterator(Symlist*);
    char const* next(void);
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
extern void nrn_change_nseg(Section*, int);
extern Section* section_new(Symbol* sym);
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
    symbol->name = strdup(name);
    symbol->type = 1;
    symbol->u.oboff = 0;
    symbol->arayinfo = 0;
    hoc_install_object_data_index(symbol);
    return section_new(symbol);
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

/****************************************
 * Segments
 ****************************************/

int nrn_nseg_get(Section const* sec) {
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

double nrn_rangevar_get(Section* sec, double x, const char* var_name) {
    Symbol* sym = hoc_table_lookup(var_name, hoc_built_in_symlist);
    return *nrn_rangepointer(sec, sym, x);
}

void nrn_rangevar_set(Section* sec, double x, const char* var_name, double value) {
    Symbol* sym = hoc_table_lookup(var_name, hoc_built_in_symlist);
    *nrn_rangepointer(sec, sym, x) = value;
}

RangeVar nrn_rangevar_new(Section* sec, double x, const char* var_name) {
    auto* node = static_cast<Segment*>(node_exact(sec, x));
    if (strcmp(var_name, "v") == 0) {  // v doesnt require symbol
        return {node, x, nullptr};
    } else {
        return {node, x, hoc_table_lookup(var_name, hoc_built_in_symlist)};
    }
}

NrnList* nrn_allsec(void) {
    return static_cast<NrnList*>(section_list);
}

NrnList* nrn_sectionlist_data(Object* obj) {
    // TODO: verify the obj is in fact a SectionList
    return static_cast<NrnList*>(obj->u.this_pointer);
}

/****************************************
 * Functions, objects, and the stack
 ****************************************/

static constexpr int MAX_ERR_LEN = 200;
static char nrn_stack_error[MAX_ERR_LEN] = "";

char const* nrn_class_name(const Object* obj) {
    return obj->ctemplate->sym->name;
}

const char* nrn_stack_err() {
    if (nrn_stack_error[0]) {
        return nrn_stack_error;
    } else {
        return NULL;
    }
}

void clear_errors() {
    nrn_stack_error[0] = 0;
}

Symbol* nrn_symbol(char const* const name) {
    return hoc_lookup(name);
}

Symbol* nrn_method_symbol(Object* obj, char const* const name) {
    return hoc_table_lookup(name, obj->ctemplate->symtable);
}

int nrn_symbol_type(Symbol const* sym) {
    // TODO: these types are in parse.hpp and are not the same between versions,
    // so we really should wrap
    return sym->type;
}

Object* nrn_object_pop() {
    // NOTE: the returned object should be unref'd when no longer needed
    Object** obptr;
    try {
        obptr = hoc_objpop();
    } catch (const std::exception& e) {
        snprintf(nrn_stack_error, MAX_ERR_LEN, "nrn_object_pop: %s", e.what());
        return nullptr;
    }
    Object* new_ob_ptr = *obptr;
    hoc_obj_ref(new_ob_ptr);
    hoc_tobj_unref(obptr);
    return new_ob_ptr;
}

static int stack_push_args(const char arg_types[], va_list args) {
    for (int n_arg = 0; n_arg < 32; ++n_arg) {  // max 32 args
        switch (arg_types[n_arg]) {
        case (*NRN_ARG_DOUBLE): {
            auto d = va_arg(args, double);
            hoc_pushx(d);
        } break;
        case* NRN_ARG_DOUBLE_PTR:
            hoc_pushpx(va_arg(args, double*));
            break;
        case* NRN_ARG_STR_PTR:
            hoc_pushstr(va_arg(args, char**));
            break;
        case* NRN_ARG_SYMBOL:
            hoc_pushs(va_arg(args, Symbol*));
            break;
        case* NRN_ARG_OBJECT:
            hoc_push_object(va_arg(args, Object*));
            break;
        case* NRN_ARG_INT:
            hoc_pushi(va_arg(args, int));
            break;
        case* NRN_ARG_RANGEVAR: {
            auto* range_var = va_arg(args, RangeVar*);
            if (range_var->sym == nullptr) {
                // No symbol -> must be v
                hoc_push(range_var->node->v_handle());
            } else {
                hoc_push(nrn_rangepointer(range_var->node->sec, range_var->sym, range_var->x));
            }
        } break;
        case 0:
            return n_arg;
        default:
            throw std::runtime_error(std::string("Invalid argument type: ") + arg_types[n_arg]);
        }
    }
    throw std::runtime_error("Too many arguments or invalid format string");
}

static NrnResult stack_pop_result(Symbol* func) {
    NrnResult result{NrnResultType::NRN_ERR};
    switch (func->type) {
    case PROCEDURE:
        result.type = NrnResultType::NRN_VOID;
        break;
    case OBFUNCTION:
    case HOCOBJFUNCTION:
    case OBJECTFUNC:
        result.type = NrnResultType::NRN_OBJECT;
        result.o = nrn_object_pop();
        break;
    case STRFUNCTION:
    case STRINGFUNC:
        result.type = NrnResultType::NRN_STRING;
        result.s = hoc_strpop();
        break;
    default:
        result.type = NrnResultType::NRN_DOUBLE;
        result.d = hoc_xpop();
        break;
    }
    return result;
}

#define CHECK_SYMB_NOT_NULL(SYMB, SYMB_NAME, ERR_RET)                              \
    if (SYMB == nullptr) {                                                         \
        snprintf(nrn_stack_error, MAX_ERR_LEN, "Symbol not found: %s", SYMB_NAME); \
        return ERR_RET;                                                            \
    }

/* NOTE
   We use this macro to deduplicate common code preparing arguments for a Neuron call
   It _must_ be a macro due to `va_start` and `return`
*/
#define NRN_CALL_CHECK_PREPARE(SYMB, SYMB_NAME, ERR_RET) \
    CHECK_SYMB_NOT_NULL(SYMB, SYMB_NAME, ERR_RET);       \
    va_list args;                                        \
    va_start(args, format);


NrnResult nrn_function_call(const char* func_name, const char* format, ...) {
    clear_errors();
    auto sym = nrn_symbol(func_name);
    NrnResult ret_val{NrnResultType::NRN_ERR};
    NRN_CALL_CHECK_PREPARE(sym, func_name, ret_val);  // Initialize va_args. DON'T return before
                                                      // `va_end`
    try {
        if (sym->type == STRINGFUNC || sym->type == OBJECTFUNC) {
            throw std::runtime_error("Can't call non-numeric functions yet");
        }
        int n_args = stack_push_args(format, args);
        OcJump::execute_throw_on_exception(sym, n_args);
        ret_val = stack_pop_result(sym);
    } catch (const std::exception& e) {
        snprintf(nrn_stack_error, MAX_ERR_LEN, "nrn_function_call: %s", e.what());
    }

    va_end(args);  // mind we always need to `va_end` after a `va_start`
    return ret_val;
}


Object* nrn_object_new(const char* cls_name, const char* format, ...) {
    clear_errors();
    auto sym = nrn_symbol(cls_name);
    NRN_CALL_CHECK_PREPARE(sym, cls_name, nullptr);
    Object* ret_value = nullptr;

    try {
        int n_args = stack_push_args(format, args);
        ret_value = hoc_newobj1(sym, n_args);
    } catch (const std::exception& e) {
        snprintf(nrn_stack_error, MAX_ERR_LEN, "nrn_object_new: %s", e.what());
    }

    va_end(args);
    return ret_value;
}

Object* nrn_object_new_NoArgs(const char* cls_name) {
    clear_errors();
    auto sym = nrn_symbol(cls_name);
    CHECK_SYMB_NOT_NULL(sym, cls_name, nullptr);
    Object* ret_val = nullptr;
    try {
        ret_val = hoc_newobj1(sym, 0);
    } catch (const std::exception& e) {
        snprintf(nrn_stack_error, MAX_ERR_LEN, "nrn_object_new: %s", e.what());
    }
    return ret_val;
}

NrnResult nrn_method_call(Object* obj, const char* method_name, const char* format, ...) {
    clear_errors();
    auto sym = nrn_method_symbol(obj, method_name);
    NrnResult ret_val{NrnResultType::NRN_ERR};
    NRN_CALL_CHECK_PREPARE(sym, method_name, ret_val);

    try {
        int n_args = stack_push_args(format, args);
        OcJump::execute_throw_on_exception(obj, sym, n_args);
        ret_val = stack_pop_result(sym);
    } catch (const std::exception& e) {
        snprintf(nrn_stack_error, MAX_ERR_LEN, "nrn_method_call: %s", e.what());
    }

    va_end(args);
    return ret_val;
}


void nrn_object_incref(Object* obj) {
    obj->refcount++;
}

void nrn_object_decref(Object* obj) {
    hoc_obj_unref(obj);
}

double nrn_result_get_double(NrnResult* r) {
    if (r->type != NrnResultType::NRN_DOUBLE) {
        throw std::runtime_error("Result bad type");
    }
    return r->d;
}

Object* nrn_result_get_object(NrnResult* r) {
    if (r->type != NrnResultType::NRN_OBJECT) {
        throw std::runtime_error("Result bad type");
    }
    return r->o;
}

const char* nrn_result_get_string(NrnResult* r) {
    if (r->type != NrnResultType::NRN_STRING) {
        throw std::runtime_error("Result bad type");
    }
    return *r->s;
}

void nrn_result_drop(NrnResult* r) {
    switch (r->type) {
    case NrnResultType::NRN_OBJECT:
        nrn_object_decref(nrn_result_get_object(r));
        r->type = NrnResultType::NRN_ERR;
        break;
    default:
        // Nothing to do for others
        break;
    }
}

/****************************************
 * Miscellaneous
 ****************************************/
int nrn_hoc_call(char const* const command) {
    return hoc_oc(command);
}

SectionListIterator::SectionListIterator(NrnListItem* my_sectionlist)
    : initial(my_sectionlist)
    , current(my_sectionlist->next) {}

Section* SectionListIterator::next(void) {
    // NOTE: if no next element, returns nullptr
    while (true) {
        Section* sec = current->element.sec;

        if (sec->prop) {
            current = current->next;
            return sec;
        }
        hoc_l_delete(current);
        section_unref(sec);
        current = current->next;
        if (current == initial) {
            return nullptr;
        }
    }
}

int SectionListIterator::done(void) const {
    if (initial == current) {
        return 1;
    }
    return 0;
}

SymbolTableIterator::SymbolTableIterator(Symlist* list)
    : current(list->first) {}

char const* SymbolTableIterator::next(void) {
    auto result = current->name;
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
SectionListIterator* nrn_sectionlist_iterator_new(NrnListItem* my_sectionlist) {
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

char const* nrn_symbol_table_iterator_next(SymbolTableIterator* st) {
    return st->next();
}

int nrn_symbol_table_iterator_done(SymbolTableIterator* st) {
    return st->done();
}

int nrn_vector_size(Object const* vec) {
    // TODO: throw exception if vec is not a Vector
    return vector_capacity((IvocVect*) vec->u.this_pointer);
}

double* nrn_vector_data(Object* vec) {
    // TODO: throw exception if vec is not a Vector
    return vector_vec((IvocVect*) vec->u.this_pointer);
}

double nrn_property_get(Object const* obj, const char* name) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
        return *hoc_pxpop();
    } else {
        int index = sym->u.rng.index;
        return ob2pntproc_0(const_cast<Object*>(obj))->prop->param_legacy(index);
    }
}

double nrn_property_array_get(Object const* obj, const char* name, int i) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
        return hoc_pxpop()[i];
    } else {
        int index = sym->u.rng.index;
        return ob2pntproc_0(const_cast<Object*>(obj))->prop->param_legacy(index + i);
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
        int index = sym->u.rng.index;
        ob2pntproc_0(obj)->prop->param_legacy(index) = value;
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
        int index = sym->u.rng.index;
        ob2pntproc_0(obj)->prop->param_legacy(index + i) = value;
    }
}

void nrn_pp_property_array_set(Object* pp, const char* name, int i, double value) {
    int index = hoc_table_lookup(name, pp->ctemplate->symtable)->u.rng.index;
    ob2pntproc_0(pp)->prop->param_legacy(index + i) = value;
}

void nrn_property_push(Object* obj, const char* name) {
    auto sym = hoc_table_lookup(name, obj->ctemplate->symtable);
    if (!obj->ctemplate->is_point_) {
        hoc_pushs(sym);
        // put the pointer for the memory location on the stack
        obj->ctemplate->steer(obj->u.this_pointer);
    } else {
        int index = sym->u.rng.index;
        hoc_push(ob2pntproc_0(obj)->prop->param_handle_legacy(index));
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
        int index = sym->u.rng.index;
        hoc_push(ob2pntproc_0(obj)->prop->param_handle_legacy(index + i));
    }
}

char const* nrn_symbol_name(const Symbol* sym) {
    return sym->name;
}

Symlist* nrn_symbol_table(Symbol* sym) {
    // TODO: ensure sym is an object or class
    // NOTE: to use with an object, call nrn_get_symbol(nrn_class_name(obj))
    return sym->u.ctemplate->symtable;
}

Symlist* nrn_global_symbol_table(void) {
    return hoc_built_in_symlist;
}
}  // extern "C"
