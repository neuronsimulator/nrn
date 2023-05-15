#pragma once

#ifdef __cplusplus
extern "C" {
#endif
struct Symbol;
struct Object;
struct Section;
struct SectionListIterator;
struct hoc_Item;
struct SymbolTableIterator;
struct Symlist;

/****************************************
 * Initialization
 ****************************************/
extern int (*nrn_init)(int argc, const char **argv);
extern void (*nrn_redirect_stdout)(int (*myprint)(int, char *));

/****************************************
 * Sections
 ****************************************/
extern Section *(*nrn_new_section)(char const *const name);
extern void (*nrn_connect_sections)(Section *child_sec, double child_x,
                                    Section *parent_sec, double parent_x);
extern void (*nrn_set_section_length)(Section *sec, double length);
extern double (*nrn_get_section_length)(Section *sec);
extern double (*nrn_get_section_Ra)(Section* sec);
extern void (*nrn_set_section_Ra)(Section* sec, double val);
extern char const *(*nrn_secname)(Section *sec);
extern void (*nrn_push_section)(Section *sec);
extern void (*nrn_pop_section)(void);
extern void (*nrn_insert_mechanism)(Section *sec, Symbol *mechanism);
extern hoc_Item* (*nrn_get_allsec)(void);
extern hoc_Item* (*nrn_get_sectionlist_data)(Object* obj);

/****************************************
 * Segments
 ****************************************/
extern int (*nrn_get_nseg)(Section const * const sec);
extern void (*nrn_set_nseg)(Section* const sec, const int nseg);
extern void (*nrn_set_segment_diam)(Section* const sec, const double x, const double diam);
extern double* (*nrn_get_rangevar_ptr)(Section* const sec, Symbol* const sym, double const x);

/****************************************
 * Functions, objects, and the stack
 ****************************************/
extern Symbol *(*nrn_get_symbol)(char const *const name);
extern int (*nrn_get_symbol_type)(Symbol *sym);
extern double *(*nrn_get_symbol_ptr)(Symbol *sym);
extern void (*nrn_push_double)(double val);
extern double (*nrn_pop_double)(void);
extern void (*nrn_push_double_ptr)(double *addr);
extern double *(*nrn_pop_double_ptr)(void);
extern void (*nrn_push_str)(char **str);
extern char **(*nrn_pop_str)(void);
extern void (*nrn_push_int)(int i);
extern int (*nrn_pop_int)(void);
extern void (*nrn_push_object)(Object* obj);
extern Object *(*nrn_pop_object)(void);
extern int (*nrn_stack_type)(void);
extern char const *const (*nrn_stack_type_name)(int id);
extern Object *(*nrn_new_object)(Symbol *sym, int narg);
extern Symbol *(*nrn_get_method_symbol)(Object *obj, char const *const name);
extern void (*nrn_call_method)(Object *obj, Symbol *method_sym, int narg);
extern void (*nrn_call_function)(Symbol *sym, int narg);
extern void (*nrn_ref_object)(Object *obj);
extern void (*nrn_unref_object)(Object *obj);
extern char const * (*nrn_get_class_name)(Object* obj);

/****************************************
 * Miscellaneous
 ****************************************/
extern int (*nrn_call_hoc)(char const *const command);
extern SectionListIterator *(*nrn_new_sectionlist_iterator)(hoc_Item *my_sectionlist);
extern void (*nrn_free_sectionlist_iterator)(SectionListIterator *sl);
extern Section *(*nrn_sectionlist_iterator_next)(SectionListIterator* sl);
extern int (*nrn_sectionlist_iterator_done)(SectionListIterator* sl);
extern SymbolTableIterator *(*nrn_new_symbol_table_iterator)(Symlist *my_symbol_table);
extern void (*nrn_free_symbol_table_iterator)(SymbolTableIterator *st);
extern char const *(*nrn_symbol_table_iterator_next)(SymbolTableIterator *st);
extern int (*nrn_symbol_table_iterator_done)(SymbolTableIterator *st);
extern int (*nrn_vector_capacity)(Object *vec);
extern double *(*nrn_vector_data_ptr)(Object *vec);
extern double* (*nrn_get_pp_property_ptr)(Object* pp, const char* name);
extern double* (*nrn_get_steered_property_ptr)(Object* obj, const char* name);
extern char const * (*nrn_get_symbol_name)(Symbol* sym);
extern Symlist * (*nrn_get_symbol_table)(Symbol* sym);
extern Symlist * (*nrn_get_global_symbol_table)(void);
// TODO: need shapeplot information extraction

#ifdef __cplusplus
}
#endif