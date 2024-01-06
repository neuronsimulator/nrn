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
#include "../ivoc/ocjump.h"
#include "../nrnoc/section.h"

#include "nrnmpiuse.h"
#include "parse.hpp"

/// A public face of hoc_Item
struct nrn_Item: public hoc_Item {};

/// A public interface of a Segment
struct Segment: public Node {};

struct SectionListIterator {
    SectionListIterator(nrn_Item*);
    Section* next(void);
    int done(void);

  private:
    hoc_Item* initial;
    hoc_Item* current;
};

struct SymbolTableIterator {
    SymbolTableIterator(Symlist*);
    char const* next(void);
    int done(void);

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

// Default implementation of modl_reg
extern "C" __attribute__((weak)) void modl_reg(){};

extern "C" {

/****************************************
 * Initialization
 ****************************************/

int nrn_init(int argc, const char** argv) {
    nrn_nobanner_ = 1;
    int exit_status = ivocmain_session(argc, argv, nullptr, 0);
#if NRNMPI
#if NRNMPI_DYNAMICLOAD
#ifdef nrnmpi_stubs
    nrnmpi_stubs();
#endif
#endif
#endif
    return exit_status;
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
    // TODO: check for memory leaks; should we free the symbol, pitm, etc?
    Symbol* symbol = new Symbol;
    symbol->name = strdup(name);
    symbol->type = 1;
    symbol->u.oboff = 0;
    symbol->arayinfo = 0;
    hoc_install_object_data_index(symbol);
    hoc_Item* itm;
    new_sections(nullptr, symbol, &itm, 1);
    return itm->element.sec;
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

double nrn_section_length_get(Section const* sec) {
    return section_length(const_cast<Section*>(sec));
}

double nrn_section_Ra_get(Section const* sec) {
    return nrn_ra(const_cast<Section*>(sec));
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

void nrn_mechanism_insert(Section* sec, Symbol* mechanism) {
    // TODO: throw exception if mechanism is not an insertable mechanism?
    mech_insert1(sec, mechanism->subtype);
}

/****************************************
 * Segments
 ****************************************/

int nrn_nseg_get(Section const* const sec) {
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

double nrn_rangevar_get(Symbol* sym, Section* sec, double x) {
    return *nrn_rangepointer(sec, sym, x);
}

void nrn_rangevar_set(Symbol* sym, Section* sec, double x, double value) {
    *nrn_rangepointer(sec, sym, x) = value;
}

RangeVar nrn_rangevar_new(Section* sec, double x, const char* var_name) {
    auto* node = static_cast<Segment*>(node_exact(sec, x));
    if (strcmp(var_name, "v") == 0) {
        // v doesnt requir symbol
        return {node, x, nullptr};
    } else {
        Symbol* sym = hoc_table_lookup(var_name, hoc_built_in_symlist);
        return {node, x, sym};
    }
}

nrn_Item* nrn_allsec(void) {
    return static_cast<nrn_Item*>(section_list);
}

nrn_Item* nrn_sectionlist_data(Object* obj) {
    // TODO: verify the obj is in fact a SectionList
    return (nrn_Item*) obj->u.this_pointer;
}

/****************************************
 * Functions, objects, and the stack
 ****************************************/

static constexpr int MAX_ERR_LEN = 200;
static char nrn_stack_error[MAX_ERR_LEN] = "";

const char* nrn_stack_err() {
    if (nrn_stack_error[0]) {
        return nrn_stack_error;
    } else {
        return NULL;
    }
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

void nrn_symbol_push(Symbol* sym) {
    hoc_pushpx(sym->u.pval);
}

/*double* nrn_get_symbol_ptr(Symbol* sym) {
    return sym->u.pval;
}*/

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

char** nrn_pop_str(void) {
    return hoc_strpop();
}

void nrn_int_push(int i) {
    hoc_pushi(i);
}

int nrn_int_pop(void) {
    return hoc_ipop();
}

void nrn_object_push(Object* obj) {
    hoc_push_object(obj);
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
    new_ob_ptr->refcount++;
    hoc_tobj_unref(obptr);
    return new_ob_ptr;
}

nrn_stack_types_t nrn_stack_type() {
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
        return STACK_IS_USERINT;
    case SYMBOL:
        return STACK_IS_SYM;
    case STKOBJ_UNREF:
        return STACK_IS_OBJUNREF;
    }
    return STACK_UNKNOWN;
}

char const* const nrn_stack_type_name(nrn_stack_types_t id) {
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
    case STACK_IS_USERINT:
        return "USERINT";
    case STACK_IS_SYM:
        return "SYMBOL";
    case STACK_IS_OBJUNREF:
        return "STKOBJ_UNREF";
    default:
        return "UNKNOWN";
    }
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

#define NRN_CHECK_SYMBOL(SYMB, ERR_RET)              \
    nrn_stack_error[0] = 0;                          \
    if (SYMB == nullptr) {                           \
        strcpy(nrn_stack_error, "Symbol not found"); \
        return ERR_RET;                              \
    }

/* NOTE
   We use this macro to deduplicate common code preparing arguments for a Neuron call
   It _must_ be a macro due to `va_start` and `return`
*/
#define NRN_CALL_CHECK_PREPARE(SYMB, ERR_RET) \
    NRN_CHECK_SYMBOL(SYMB, ERR_RET);          \
    va_list args;                             \
    va_start(args, format);


double nrn_function_call(const char* func_name, const char* format, ...) {
    auto sym = nrn_symbol(func_name);
    NRN_CALL_CHECK_PREPARE(sym,
                           -1);  // Important: initializes va_args. DON'T return before `va_end`
    double ret_val = -1.;        // default err. Happy path sets to 0

    try {
        int n_args = stack_push_args(format, args);
        OcJump::execute_throw_on_exception(sym, n_args);
        ret_val = hoc_xpop();
    } catch (const std::exception& e) {
        snprintf(nrn_stack_error, MAX_ERR_LEN, "nrn_function_call: %s", e.what());
    }

    va_end(args);  // mind we always need to `va_end` after a `va_start`
    return ret_val;
}


Object* nrn_object_new(const char* cls_name, const char* format, ...) {
    auto sym = nrn_symbol(cls_name);
    NRN_CALL_CHECK_PREPARE(sym, nullptr);
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
    auto sym = nrn_symbol(cls_name);
    NRN_CHECK_SYMBOL(sym, nullptr);
    Object* ret_val = nullptr;
    try {
        ret_val = hoc_newobj1(sym, 0);
    } catch (const std::exception& e) {
        snprintf(nrn_stack_error, MAX_ERR_LEN, "nrn_object_new: %s", e.what());
    }
    return ret_val;
}

int nrn_method_call(Object* obj, const char* method_name, const char* format, ...) {
    auto sym = nrn_method_symbol(obj, method_name);
    NRN_CALL_CHECK_PREPARE(sym, -1.);
    int ret_val = -1.;

    try {
        int n_args = stack_push_args(format, args);
        OcJump::execute_throw_on_exception(obj, sym, n_args);
        ret_val = 0;
    } catch (const std::exception& e) {
        snprintf(nrn_stack_error, MAX_ERR_LEN, "nrn_method_call: %s", e.what());
    }

    va_end(args);
    return ret_val;
}

// --- Helpers

double nrn_function_call_d(const char* func_name, double v) {
    return nrn_function_call(func_name, NRN_ARG_DOUBLE, v);
}

double nrn_function_call_s(const char* func_name, const char* v) {
    char* temp_str = strdup(v);
    auto x = nrn_function_call(func_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
}

Object* nrn_object_new_d(const char* cls_name, double v) {
    return nrn_object_new(cls_name, NRN_ARG_DOUBLE, v);
}

Object* nrn_object_new_s(const char* cls_name, const char* v) {
    char* temp_str = strdup(v);
    auto x = nrn_object_new(cls_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
}

int nrn_method_call_d(Object* obj, const char* method_name, double v) {
    return nrn_method_call(obj, method_name, NRN_ARG_DOUBLE, v);
}

int nrn_method_call_s(Object* obj, const char* method_name, const char* v) {
    char* temp_str = strdup(v);
    auto x = nrn_method_call(obj, method_name, NRN_ARG_STR_PTR, &temp_str);
    free(temp_str);
    return x;
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

/****************************************
 * Miscellaneous
 ****************************************/
int nrn_hoc_call(char const* const command) {
    return hoc_oc(command);
}

SectionListIterator::SectionListIterator(nrn_Item* my_sectionlist) {
    initial = my_sectionlist;
    current = my_sectionlist->next;
}

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

int SectionListIterator::done(void) {
    if (initial == current) {
        return 1;
    }
    return 0;
}

SymbolTableIterator::SymbolTableIterator(Symlist* list) {
    current = list->first;
}

char const* SymbolTableIterator::next(void) {
    auto result = current->name;
    current = current->next;
    return result;
}

int SymbolTableIterator::done(void) {
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
