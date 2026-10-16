#pragma once

#include "neuron/container/generic_data_handle.hpp"
#include "options.h"  // for EXTRACELLULAR
#include "neuron/container/data_handle.hpp"

struct Node;
struct Object;
struct Section;
struct Symbol;
struct Prop;
struct hoc_Item;

double cable_prop_eval(Symbol* sym);

// section on stack will be popped
double* cable_prop_eval_pointer(Symbol* sym);

void oc_save_cabcode(int* a1, int* a2);
void oc_restore_cabcode(int* a1, int* a2);

void nrn_initcode();
void nrn_pushsec(Section*);
Object* nrn_sec2cell(Section* sec);
int nrn_sec2cell_equals(Section* sec, Object* obj);
void new_sections(Object* ob, Symbol* sym, hoc_Item** pitm, int size);
Section* section_new(Symbol* sym);

#if USE_PYTHON
struct NPySecObj;
Section* nrnpy_newsection(NPySecObj*);
#endif

int arc0at0(Section*);
double nrn_ra(Section*);
void cab_alloc(Prop*);
void morph_alloc(Prop* p);
double nrn_diameter(Node* nd);
double section_length(Section* sec);
Section* chk_access();

/// return 0 if no accessed section
Section* nrn_noerr_access();

void nrn_disconnect(Section* sec);
Section* nrn_sec_pop();
void ob_sec_access_push(hoc_Item* qsec);
void mech_insert1(Section* sec, int type);
void mech_uninsert1(Section* sec, Symbol* s);
void nrn_rangeconst(Section*, Symbol*, neuron::container::data_handle<double> value, int op);
Prop* nrn_mechanism(int type, Node* nd);

/// returns prop given mech type, section, and inode error if mech not at this position
Prop* nrn_mechanism_check(int type, Section* sec, int inode);
Prop* hoc_getdata_range(int type);
int nrn_exists(Symbol* s, Node* node);
neuron::container::data_handle<double> nrn_rangepointer(Section* sec, Symbol* s, double d);

/// return nullptr if failure instead of hoc_execerror and return pointer to the 0 element if an
/// array
neuron::container::generic_data_handle nrnpy_rangepointer(Section*, Symbol*, double, int*, int);

/// returns nearest index to x
int node_index(Section* sec, double x);

/// return -1 if x at connection end, nnode-1 if at other end
int node_index_exact(Section* sec, double x);
void nrn_change_nseg(Section* sec, int n);
void cable_prop_assign(Symbol* sym, double* pd, int op);

/* x of parent for this section */
double nrn_connection_position(Section*);

/* x=0,1 end connected to parent */
double nrn_section_orientation(Section*);

int nrn_at_beginning(Section* sec);
Section* nrn_trueparent(Section*);
void nrn_parent_info(Section* s);
void setup_topology();

/// name of section (for use in error messages)
const char* secname(Section* sec);

char* hoc_section_pathname(Section* sec);
double nrn_arc_position(Section* sec, Node* node);
const char* sec_and_position(Section* sec, Node* nd);
int segment_limits(double*);

/// like node_index but give proper node when x is 0 or 1 as well as in between
Node* node_exact(Section* sec, double x);
Node* node_ptr(Section* sec, double x, double* parea);

/// returns location of property symbol
neuron::container::data_handle<double> dprop(Symbol* s, int indx, Section* sec, short inode);

/// returns location of property symbol, return nullptr instead of hoc_execerror
neuron::container::generic_data_handle nrnpy_dprop(Symbol* s,
                                                   int indx,
                                                   Section* sec,
                                                   short inode,
                                                   int* err);
int has_membrane(char* mechanism_name, Section* sec);

/// turn off section stack fixing (in case of return, continue, break in a section statement)
/// between explicit user level push_section, etc and pop_section
void hoc_level_pushsec(Section* sec);

Section* nrn_section_exists(char* name, int indx, Object* cell);

#if EXTRACELLULAR
double* nrn_vext_pd(Symbol* s, int indx, Node* nd);
#endif
