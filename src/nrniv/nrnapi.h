#pragma once

#include "hocdec.h"
#include "section.h"

// we define these here to allow the API to be independent of internal details
#define STACK_IS_STR 1
#define STACK_IS_VAR 2
#define STACK_IS_NUM 3
#define STACK_IS_OBJVAR 4
#define STACK_IS_OBJTMP 5
#define STACK_IS_USERINT 6
#define STACK_IS_SYM 7
#define STACK_IS_OBJUNREF 8
#define STACK_UNKNOWN -1

class SectionListIterator {
public:
  SectionListIterator(hoc_Item *);
  Section *next(void);
  int done(void);

private:
  hoc_Item *initial;
  hoc_Item *current;
};

extern "C" {
/****************************************
 * Initialization
 ****************************************/
int nrn_init(int argc, const char **argv);
void nrn_redirect_stdout(int (*myprint)(int, char *));

/****************************************
 * Sections
 ****************************************/
Section *nrn_new_section(char const *const name);
void nrn_connect_sections(Section *child_sec, double child_x,
                          Section *parent_sec, double parent_x);
void nrn_set_section_length(Section *sec, double length);
double nrn_get_section_length(Section *sec);
double nrn_get_section_Ra(Section* sec);
void nrn_set_section_Ra(Section* sec, double val);
char const *nrn_secname(Section *sec);
void nrn_push_section(Section *sec);
void nrn_pop_section(void);
void nrn_insert_mechanism(Section *sec, Symbol *mechanism);

/****************************************
 * Segments
 ****************************************/
int nrn_get_nseg(Section const * const sec);
void nrn_set_nseg(Section* const sec, const int nseg);
void nrn_set_segment_diam(Section* const sec, const double x, const double diam);
double* nrn_get_rangevar_ptr(Section* const sec, Symbol* const sym, double const x);


/****************************************
 * Functions, objects, and the stack
 ****************************************/
Symbol *nrn_get_symbol(char const *const name);
int nrn_get_symbol_type(Symbol *sym);
double *nrn_get_symbol_ptr(Symbol *sym);
void nrn_push_double(double val);
double nrn_pop_double(void);
void nrn_push_double_ptr(double *addr);
double *nrn_pop_double_ptr(void);
void nrn_push_str(char **str);
char **nrn_pop_str(void);
void nrn_push_int(int i);
int nrn_pop_int(void);
Object *nrn_pop_object(void);
int nrn_stack_type(void);
char const *const nrn_stack_type_name(int id);
Object *nrn_new_object(Symbol *sym, int narg);
Symbol *nrn_get_method_symbol(Object *obj, char const *const name);
void nrn_call_method(Object *obj, Symbol *method_sym, int narg);
void nrn_call_function(Symbol *sym, int narg);
// TODO: do we need a nrn_ref_object?
void nrn_unref_object(Object *obj);
// TODO: need a way to get the type of an object as a string

/****************************************
 * Miscellaneous
 ****************************************/
int nrn_call_hoc(char const *const command);
SectionListIterator *nrn_new_sectionlist_iterator(hoc_Item *my_sectionlist);
void nrn_free_sectionlist_iterator(SectionListIterator *sl);
Section *nrn_sectionlist_iterator_next(SectionListIterator* sl);
int nrn_sectionlist_iterator_done(SectionListIterator* sl);
int nrn_vector_capacity(Object *vec);
double *nrn_vector_data_ptr(Object *vec);
double* nrn_get_pp_property_ptr(Object* pp, const char* name);
double* nrn_get_steered_property_ptr(Object* obj, const char* name);
// TODO: need a way to get hoc_Item* corresponding to allsec, a given SectionList
// TODO: need a way to iterate over symbol trees (top level and per object)
// TODO: need a way of extracting information from a ShapePlotInterface
}
