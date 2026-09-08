#pragma once

#include "nrndlldef.h"
#include <stdbool.h>

#ifdef __cplusplus
#include <cstddef>
#include <cstdio>
using std::FILE;
extern "C" {
#else
#include <stddef.h>
#include <stdio.h>
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
NRN_DLLSYM int nrn_init(int argc, const char** argv);
NRN_DLLSYM void nrn_stdout_redirect(int (*myprint)(int, char*));

/****************************************
 * Sections
 ****************************************/
NRN_DLLSYM Section* nrn_section_new(const char* name);
NRN_DLLSYM void nrn_section_connect(Section* child_sec,
                                    double child_x,
                                    Section* parent_sec,
                                    double parent_x);
NRN_DLLSYM void nrn_section_length_set(Section* sec, double length);
NRN_DLLSYM double nrn_section_length_get(Section* sec);
NRN_DLLSYM double nrn_section_Ra_get(Section* sec);
NRN_DLLSYM void nrn_section_Ra_set(Section* sec, double val);
NRN_DLLSYM double nrn_section_rallbranch_get(const Section* sec);
NRN_DLLSYM void nrn_section_rallbranch_set(Section* sec, double val);
NRN_DLLSYM char const* nrn_secname(Section* sec);
NRN_DLLSYM void nrn_section_push(Section* sec);
NRN_DLLSYM void nrn_section_pop(void);
NRN_DLLSYM void nrn_mechanism_insert(Section* sec, const Symbol* mechanism);
NRN_DLLSYM nrn_Item* nrn_allsec(void);
NRN_DLLSYM nrn_Item* nrn_sectionlist_data(const Object* obj);
NRN_DLLSYM Section* nrn_section_parent(Section* sec);
NRN_DLLSYM Section* nrn_section_trueparent(Section* sec);
NRN_DLLSYM Section* nrn_section_child(Section* sec);
NRN_DLLSYM Section* nrn_section_sibling(Section* sec);
NRN_DLLSYM int nrn_sectionlist_to_array(nrn_Item* sl, Section** buf, int maxlen);
NRN_DLLSYM bool nrn_section_is_active(const Section* sec);
NRN_DLLSYM void nrn_section_ref(Section* sec);
NRN_DLLSYM void nrn_section_unref(Section* sec);
NRN_DLLSYM Section* nrn_cas(void);

/****************************************
 * Segments
 ****************************************/
NRN_DLLSYM int nrn_nseg_get(const Section* sec);
NRN_DLLSYM void nrn_nseg_set(Section* sec, int nseg);
NRN_DLLSYM void nrn_segment_diam_set(Section* sec, double x, double diam);
NRN_DLLSYM double nrn_segment_diam_get(Section* sec, double x);
NRN_DLLSYM int nrn_segment_node_index(Section* sec, double x);
NRN_DLLSYM void nrn_rangevar_push(Symbol* sym, Section* sec, double x);
NRN_DLLSYM double nrn_rangevar_get(Symbol* sym, Section* sec, double x);
NRN_DLLSYM void nrn_rangevar_set(Symbol* sym, Section* sec, double x, double value);
NRN_DLLSYM Object* nrn_segment_nmodlrandom_get(Section* sec, double x, Symbol* sym);
NRN_DLLSYM Object* nrn_pntproc_nmodlrandom_get(Object* point_process, Symbol* sym);
NRN_DLLSYM int nrn_setpointer_pop(Symbol* pointer_sym,
                                  Section* sec,
                                  double x,
                                  char* error_msg,
                                  size_t error_msg_size);
NRN_DLLSYM int nrn_pp_setpointer_pop(Object* pp,
                                     const char* name,
                                     char* error_msg,
                                     size_t error_msg_size);

/****************************************
 * Functions, objects, and the stack
 ****************************************/
NRN_DLLSYM Symbol* nrn_symbol(const char* name);
NRN_DLLSYM void nrn_symbol_push(Symbol* sym);
NRN_DLLSYM Symbol* nrn_symbol_pop(void);
NRN_DLLSYM int nrn_symbol_type(const Symbol* sym);
NRN_DLLSYM int nrn_symbol_subtype(const Symbol* sym);
NRN_DLLSYM double* nrn_symbol_dataptr(const Symbol* sym);
NRN_DLLSYM Object* nrn_symbol_object_get(const Symbol* sym);
NRN_DLLSYM bool nrn_symbol_object_set(Symbol* sym, Object* obj);
NRN_DLLSYM const char* nrn_symbol_str_get(const Symbol* sym);
NRN_DLLSYM bool nrn_symbol_str_set(Symbol* sym, const char* value);
NRN_DLLSYM bool nrn_symbol_is_array(const Symbol* sym);
NRN_DLLSYM void nrn_double_push(double val);
NRN_DLLSYM double nrn_double_pop(void);
NRN_DLLSYM void nrn_double_ptr_push(double* addr);
NRN_DLLSYM double* nrn_double_ptr_pop(void);
NRN_DLLSYM void nrn_str_push(char** str);
NRN_DLLSYM char** nrn_str_pop(void);
NRN_DLLSYM void nrn_int_push(int i);
NRN_DLLSYM int nrn_int_pop(void);
NRN_DLLSYM void nrn_object_push(Object* obj);
NRN_DLLSYM void nrn_object_ptr_push(Object** obj_ref);
NRN_DLLSYM Object* nrn_object_pop(void);
NRN_DLLSYM nrn_stack_types_t nrn_stack_type(void);
NRN_DLLSYM char const* nrn_stack_type_name(nrn_stack_types_t id);
NRN_DLLSYM Object* nrn_object_new(Symbol* sym, int narg);
NRN_DLLSYM Object* nrn_object_new_wrap(Symbol* sym, void* cpp_object);
NRN_DLLSYM int nrn_object_new_nothrow(Symbol* sym,
                                      int narg,
                                      Object** result,
                                      char* error_msg,
                                      size_t error_msg_size);
NRN_DLLSYM Symbol* nrn_method_symbol(const Object* obj, const char* name);
// TODO: the next two functions throw exceptions in C++; need a version that
//       returns a bool success indicator instead (this is actually the
//       classic behavior of OcJump)
NRN_DLLSYM void nrn_method_call(Object* obj, Symbol* method_sym, int narg);
NRN_DLLSYM void nrn_function_call(Symbol* sym, int narg);
NRN_DLLSYM int nrn_method_call_nothrow(Object* obj,
                                       Symbol* method_sym,
                                       int narg,
                                       char* error_msg,
                                       size_t error_msg_size);
NRN_DLLSYM int nrn_function_call_nothrow(Symbol* sym,
                                         int narg,
                                         char* error_msg,
                                         size_t error_msg_size);
NRN_DLLSYM void nrn_object_ref(Object* obj);
NRN_DLLSYM void nrn_object_unref(Object* obj);
NRN_DLLSYM char const* nrn_class_name(const Object* obj);
NRN_DLLSYM bool nrn_prop_exists(const Object* obj);
NRN_DLLSYM double nrn_distance(Section* sec0, double x0, Section* sec1, double x1);

/****************************************
 * Shape Plot
 ****************************************/
NRN_DLLSYM ShapePlotInterface* nrn_get_plotshape_interface(Object* ps);
NRN_DLLSYM Object* nrn_get_plotshape_section_list(ShapePlotInterface* spi);
NRN_DLLSYM const char* nrn_get_plotshape_varname(ShapePlotInterface* spi);
NRN_DLLSYM float nrn_get_plotshape_low(ShapePlotInterface* spi);
NRN_DLLSYM float nrn_get_plotshape_high(ShapePlotInterface* spi);

/****************************************
 * Miscellaneous
 ****************************************/
NRN_DLLSYM int nrn_hoc_call(char const* command);
NRN_DLLSYM SectionListIterator* nrn_sectionlist_iterator_new(nrn_Item* my_sectionlist);
NRN_DLLSYM void nrn_sectionlist_iterator_free(SectionListIterator* sl);
NRN_DLLSYM Section* nrn_sectionlist_iterator_next(SectionListIterator* sl);
NRN_DLLSYM int nrn_sectionlist_iterator_done(SectionListIterator* sl);
NRN_DLLSYM SymbolTableIterator* nrn_symbol_table_iterator_new(Symlist* my_symbol_table);
NRN_DLLSYM void nrn_symbol_table_iterator_free(SymbolTableIterator* st);
NRN_DLLSYM Symbol* nrn_symbol_table_iterator_next(SymbolTableIterator* st);
NRN_DLLSYM int nrn_symbol_table_iterator_done(SymbolTableIterator* st);
NRN_DLLSYM int nrn_vector_capacity(const Object* vec);
NRN_DLLSYM double* nrn_vector_data(Object* vec);
NRN_DLLSYM double nrn_property_get(const Object* obj, const char* name);
NRN_DLLSYM double nrn_property_array_get(const Object* obj, const char* name, int i);
NRN_DLLSYM void nrn_property_set(Object* obj, const char* name, double value);
NRN_DLLSYM void nrn_property_array_set(Object* obj, const char* name, int i, double value);
NRN_DLLSYM void nrn_property_push(Object* obj, const char* name);
NRN_DLLSYM void nrn_property_array_push(Object* obj, const char* name, int i);
NRN_DLLSYM bool nrn_property_data_handle_is_valid(const Object* obj, const char* name, int i);
NRN_DLLSYM char const* nrn_symbol_name(const Symbol* sym);
NRN_DLLSYM Symlist* nrn_symbol_table(const Symbol* sym);
NRN_DLLSYM Symlist* nrn_global_symbol_table(void);
NRN_DLLSYM Symlist* nrn_top_level_symbol_table(void);
NRN_DLLSYM int nrn_symbol_array_length(const Symbol* sym);
NRN_DLLSYM void nrn_register_function(void (*proc)(), const char* func_name, int type);
NRN_DLLSYM void nrn_hoc_ret(void);

/****************************************
 * Parameter-reading functions
 ****************************************/
NRN_DLLSYM Object** nrn_objgetarg(int arg);
NRN_DLLSYM char* nrn_gargstr(int arg);
NRN_DLLSYM double* nrn_getarg(int arg);
NRN_DLLSYM FILE* nrn_obj_file_arg(int i);
NRN_DLLSYM bool nrn_ifarg(int arg);
NRN_DLLSYM bool nrn_is_object_arg(int arg);
NRN_DLLSYM bool nrn_is_str_arg(int arg);
NRN_DLLSYM bool nrn_is_double_arg(int arg);
NRN_DLLSYM bool nrn_is_pdouble_arg(int arg);

#ifdef __cplusplus
}
#endif
