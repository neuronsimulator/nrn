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
#include "section.h"

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

double nrn_rangevar_get(Symbol* sym, Section* sec, double x) {
    return *nrn_rangepointer(sec, sym, x);
}

void nrn_rangevar_set(Symbol* sym, Section* sec, double x, double value) {
    *nrn_rangepointer(sec, sym, x) = value;
}

void nrn_rangevar_push(Symbol* sym, Section* sec, double x) {
    hoc_push(nrn_rangepointer(sec, sym, x));
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

Symbol* nrn_symbol(char const* const name) {
    return hoc_lookup(name);
}

int nrn_symbol_type(Symbol const* sym) {
    // TODO: these types are in parse.hpp and are not the same between versions,
    // so we really should wrap
    return sym->type;
}

void nrn_symbol_push(Symbol* sym) {
    hoc_pushpx(sym->u.pval);
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

Object* nrn_object_pop(void) {
    // NOTE: the returned object should be unref'd when no longer needed
    Object** obptr = hoc_objpop();
    Object* new_ob_ptr = *obptr;
    new_ob_ptr->refcount++;
    hoc_tobj_unref(obptr);
    return new_ob_ptr;
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

Symbol* nrn_method_symbol(Object* obj, char const* const name) {
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

SectionListIterator::SectionListIterator(nrn_Item* my_sectionlist)
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

int nrn_vector_capacity(Object const* vec) {
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
}
