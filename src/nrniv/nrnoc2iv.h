#ifndef nrnoc2iv_h
#define nrnoc2iv_h
// common things in nrnoc that can be used by nrniv

#include "oc2iv.h"

extern "C" {
#include "section.h"
}

extern "C" {

void v_setup_vectors();
void section_ref(Section*);
void section_unref(Section*);
char* secname(Section*);
void nrn_pushsec(Section*);
void nrn_popsec();
Section* chk_access();
void nrn_rangeconst(Section*, Symbol*, double* value, int op = 0);
Prop* nrn_mechanism(int type, Node*);
boolean nrn_exists(Symbol*, Node*);
double* nrn_rangepointer(Section*, Symbol*, double x);
double* cable_prop_eval_pointer(Symbol*); // section on stack will be popped
char* hoc_section_pathname(Section*);
double nrn_arc_position(Section*, Node*);
double node_dist(Section*, Node*); // distance of node to parent position
Node* node_exact(Section*, double);
double nrn_section_orientation(Section*);
double nrn_connection_position(Section*);
Section* nrn_trueparent(Section*);
double topol_distance(Section*, Node*, Section*, Node*, Section**, Node**);
int arc0at0(Section*);
void nrn_clear_mark();
short nrn_increment_mark(Section*);
short nrn_value_mark(Section*);
boolean is_point_process(Object*);
int nrn_vartype(Symbol*); // nrnocCONST, DEP, STATE
void recalc_diam();
}

#include "ndatclas.h"

#endif

