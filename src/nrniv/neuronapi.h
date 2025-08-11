#pragma once

#include <stdbool.h>

#ifdef __cplusplus
#include <cstdio>
#include <cstddef>
using std::FILE;
extern "C" {
#else
#include <stdio.h>
#include <stddef.h>
#endif

// forward declarations (c++) and opaque c types
typedef struct Symbol Symbol;
typedef struct Object Object;
typedef struct Section Section;
typedef struct SectionListIterator SectionListIterator;
typedef struct nrn_Item nrn_Item;
typedef struct SymbolTableIterator SymbolTableIterator;
typedef struct Symlist Symlist;
typedef struct ShapePlotInterface ShapePlotInterface;

typedef enum {
    STACK_IS_STR = 1,
    STACK_IS_VAR = 2,
    STACK_IS_NUM = 3,
    STACK_IS_OBJVAR = 4,
    STACK_IS_OBJTMP = 5,
    STACK_IS_INT = 6,
    STACK_IS_SYM = 7,
    STACK_UNKNOWN = -1
} nrn_stack_types_t;

/****************************************
 * Initialization
 ****************************************/
int nrn_init(int argc, const char** argv);
void nrn_stdout_redirect(int (*myprint)(int, char*));

/****************************************
 * Sections
 ****************************************/
Section* nrn_section_new(const char* name);
void nrn_section_connect(Section* child_sec, double child_x, Section* parent_sec, double parent_x);
void nrn_section_length_set(Section* sec, double length);
double nrn_section_length_get(Section* sec);
double nrn_section_Ra_get(Section* sec);
void nrn_section_Ra_set(Section* sec, double val);
double nrn_section_rallbranch_get(const Section* sec);
void nrn_section_rallbranch_set(Section* sec, double val);
char const* nrn_secname(Section* sec);
void nrn_section_push(Section* sec);
void nrn_section_pop(void);
void nrn_mechanism_insert(Section* sec, const Symbol* mechanism);
nrn_Item* nrn_allsec(void);
nrn_Item* nrn_sectionlist_data(const Object* obj);
bool nrn_section_is_active(const Section* sec);
void nrn_section_ref(Section* sec);
void nrn_section_unref(Section* sec);
Section* nrn_cas(void);

/****************************************
 * Segments
 ****************************************/
int nrn_nseg_get(const Section* sec);
void nrn_nseg_set(Section* sec, int nseg);
void nrn_segment_diam_set(Section* sec, double x, double diam);
double nrn_segment_diam_get(Section* sec, double x);
void nrn_rangevar_push(Symbol* sym, Section* sec, double x);
double nrn_rangevar_get(Symbol* sym, Section* sec, double x);
void nrn_rangevar_set(Symbol* sym, Section* sec, double x, double value);

/****************************************
 * Functions, objects, and the stack
 ****************************************/
Symbol* nrn_symbol(const char* name);
void nrn_symbol_push(Symbol* sym);
int nrn_symbol_type(const Symbol* sym);
int nrn_symbol_subtype(const Symbol* sym);
double* nrn_symbol_dataptr(const Symbol* sym);
bool nrn_symbol_is_array(const Symbol* sym);
void nrn_double_push(double val);
double nrn_double_pop(void);
void nrn_double_ptr_push(double* addr);
double* nrn_double_ptr_pop(void);
void nrn_str_push(char** str);
char** nrn_str_pop(void);
void nrn_int_push(int i);
int nrn_int_pop(void);
void nrn_object_push(Object* obj);
Object* nrn_object_pop(void);
nrn_stack_types_t nrn_stack_type(void);
char const* nrn_stack_type_name(nrn_stack_types_t id);
Object* nrn_object_new(Symbol* sym, int narg);
Symbol* nrn_method_symbol(const Object* obj, const char* name);
// TODO: the next two functions throw exceptions in C++; need a version that
//       returns a bool success indicator instead (this is actually the
//       classic behavior of OcJump)
void nrn_method_call(Object* obj, Symbol* method_sym, int narg);
void nrn_function_call(Symbol* sym, int narg);
int nrn_method_call_nothrow(Object* obj,
                            Symbol* method_sym,
                            int narg,
                            char* error_msg,
                            size_t error_msg_size);
int nrn_function_call_nothrow(Symbol* sym, int narg, char* error_msg, size_t error_msg_size);
void nrn_object_ref(Object* obj);
void nrn_object_unref(Object* obj);
char const* nrn_class_name(const Object* obj);
bool nrn_prop_exists(const Object* obj);
double nrn_distance(Section* sec0, double x0, Section* sec1, double x1);

/****************************************
 * Shape Plot
 ****************************************/
ShapePlotInterface* nrn_get_plotshape_interface(Object* ps);
Object* nrn_get_plotshape_section_list(ShapePlotInterface* spi);
const char* nrn_get_plotshape_varname(ShapePlotInterface* spi);
float nrn_get_plotshape_low(ShapePlotInterface* spi);
float nrn_get_plotshape_high(ShapePlotInterface* spi);

/****************************************
 * Miscellaneous
 ****************************************/
int nrn_hoc_call(char const* command);
SectionListIterator* nrn_sectionlist_iterator_new(nrn_Item* my_sectionlist);
void nrn_sectionlist_iterator_free(SectionListIterator* sl);
Section* nrn_sectionlist_iterator_next(SectionListIterator* sl);
int nrn_sectionlist_iterator_done(SectionListIterator* sl);
SymbolTableIterator* nrn_symbol_table_iterator_new(Symlist* my_symbol_table);
void nrn_symbol_table_iterator_free(SymbolTableIterator* st);
Symbol* nrn_symbol_table_iterator_next(SymbolTableIterator* st);
int nrn_symbol_table_iterator_done(SymbolTableIterator* st);
int nrn_vector_capacity(const Object* vec);
double* nrn_vector_data(Object* vec);
double nrn_property_get(const Object* obj, const char* name);
double nrn_property_array_get(const Object* obj, const char* name, int i);
void nrn_property_set(Object* obj, const char* name, double value);
void nrn_property_array_set(Object* obj, const char* name, int i, double value);
void nrn_property_push(Object* obj, const char* name);
void nrn_property_array_push(Object* obj, const char* name, int i);
char const* nrn_symbol_name(const Symbol* sym);
Symlist* nrn_symbol_table(const Symbol* sym);
Symlist* nrn_global_symbol_table(void);
Symlist* nrn_top_level_symbol_table(void);
int nrn_symbol_array_length(const Symbol* sym);
void nrn_register_function(void (*proc)(), const char* func_name, int type);
void nrn_hoc_ret(void);

/****************************************
 * Parameter-reading functions
 ****************************************/
Object** nrn_objgetarg(int arg);
char* nrn_gargstr(int arg);
double* nrn_getarg(int arg);
FILE* nrn_obj_file_arg(int i);
bool nrn_ifarg(int arg);
bool nrn_is_object_arg(int arg);
bool nrn_is_str_arg(int arg);
bool nrn_is_double_arg(int arg);
bool nrn_is_pdouble_arg(int arg);

#ifdef __cplusplus
}
#endif
