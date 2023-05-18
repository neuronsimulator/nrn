#pragma once

#ifdef __cplusplus
extern "C" {
#endif
typedef struct Symbol Symbol;
typedef struct Object Object;
typedef struct Section Section;
typedef struct SectionListIterator SectionListIterator;
typedef struct nrn_Item nrn_Item;
typedef struct SymbolTableIterator SymbolTableIterator;
typedef struct Symlist Symlist;

typedef enum {
    STACK_IS_STR = 1,
    STACK_IS_VAR = 2,
    STACK_IS_NUM = 3,
    STACK_IS_OBJVAR = 4,
    STACK_IS_OBJTMP = 5,
    STACK_IS_USERINT = 6,
    STACK_IS_SYM = 7,
    STACK_IS_OBJUNREF = 8,
    STACK_UNKNOWN = -1
} stack_types_t;

/****************************************
 * Initialization
 ****************************************/
int (*nrn_init)(int argc, const char** argv);
void (*nrn_redirect_stdout)(int (*myprint)(int, char*));

/****************************************
 * Sections
 ****************************************/
Section* (*nrn_new_section)(char const* name);
void (*nrn_connect_sections)(Section* child_sec,
                             double child_x,
                             Section* parent_sec,
                             double parent_x);
void (*nrn_set_section_length)(Section* sec, double length);
double (*nrn_get_section_length)(Section* sec);
double (*nrn_get_section_Ra)(Section* sec);
void (*nrn_set_section_Ra)(Section* sec, double val);
char const* (*nrn_secname)(Section* sec);
void (*nrn_push_section)(Section* sec);
void (*nrn_pop_section)(void);
void (*nrn_insert_mechanism)(Section* sec, Symbol* mechanism);
nrn_Item* (*nrn_get_allsec)(void);
nrn_Item* (*nrn_get_sectionlist_data)(Object* obj);

/****************************************
 * Segments
 ****************************************/
int (*nrn_get_nseg)(Section const* sec);
void (*nrn_set_nseg)(Section* sec, int nseg);
void (*nrn_set_segment_diam)(Section* sec, double x, double diam);
double* (*nrn_get_rangevar_ptr)(Section* sec, Symbol* sym, double x);

/****************************************
 * Functions, objects, and the stack
 ****************************************/
Symbol* (*nrn_get_symbol)(char const* name);
int (*nrn_get_symbol_type)(Symbol* sym);
double* (*nrn_get_symbol_ptr)(Symbol* sym);
void (*nrn_push_double)(double val);
double (*nrn_pop_double)(void);
void (*nrn_push_double_ptr)(double* addr);
double* (*nrn_pop_double_ptr)(void);
void (*nrn_push_str)(char** str);
char** (*nrn_pop_str)(void);
void (*nrn_push_int)(int i);
int (*nrn_pop_int)(void);
void (*nrn_push_object)(Object* obj);
Object* (*nrn_pop_object)(void);
stack_types_t (*nrn_stack_type)(void);
char const* const (*nrn_stack_type_name)(stack_types_t id);
Object* (*nrn_new_object)(Symbol* sym, int narg);
Symbol* (*nrn_get_method_symbol)(Object* obj, char const* name);
// TODO: the next two functions throw exceptions in C++; need a version that
//       returns a bool success indicator instead (this is actually the
//       classic behavior of OcJump)
void (*nrn_call_method)(Object* obj, Symbol* method_sym, int narg);
void (*nrn_call_function)(Symbol* sym, int narg);
void (*nrn_ref_object)(Object* obj);
void (*nrn_unref_object)(Object* obj);
char const* (*nrn_get_class_name)(Object* obj);

/****************************************
 * Miscellaneous
 ****************************************/
int (*nrn_call_hoc)(char const* command);
SectionListIterator* (*nrn_new_sectionlist_iterator)(nrn_Item* my_sectionlist);
void (*nrn_free_sectionlist_iterator)(SectionListIterator* sl);
Section* (*nrn_sectionlist_iterator_next)(SectionListIterator* sl);
int (*nrn_sectionlist_iterator_done)(SectionListIterator* sl);
SymbolTableIterator* (*nrn_new_symbol_table_iterator)(Symlist* my_symbol_table);
void (*nrn_free_symbol_table_iterator)(SymbolTableIterator* st);
char const* (*nrn_symbol_table_iterator_next)(SymbolTableIterator* st);
int (*nrn_symbol_table_iterator_done)(SymbolTableIterator* st);
int (*nrn_vector_capacity)(Object* vec);
double* (*nrn_vector_data_ptr)(Object* vec);
double* (*nrn_get_pp_property_ptr)(Object* pp, const char* name);
double* (*nrn_get_steered_property_ptr)(Object* obj, const char* name);
char const* (*nrn_get_symbol_name)(Symbol* sym);
Symlist* (*nrn_get_symbol_table)(Symbol* sym);
Symlist* (*nrn_get_global_symbol_table)(void);
// TODO: need shapeplot information extraction

#ifdef __cplusplus
}
#endif
