#include "membfunc.h"

#include "multicore.h"
#include "section.h"

#include <cassert>

void Memb_func::invoke_initialize(neuron::model_sorted_token const& sorted_token,
                                  NrnThread* nt,
                                  Memb_list* ml,
                                  int mech_type) const {
    assert(has_initialize());
    if (ml->type() != mech_type) {
        throw std::runtime_error("Memb_func::invoke_initialize(nt[" + std::to_string(nt->id) +
                                 "], ml, " + std::to_string(mech_type) +
                                 "): type mismatch, ml->type()=" + std::to_string(ml->type()));
    }
    m_initialize(sorted_token, nt, ml, mech_type);
}
long& _nrn_mechanism_access_alloc_seq(Prop* prop) {
    return prop->_alloc_seq;
}
double& _nrn_mechanism_access_a(Node* node) {
    return node->a();
}
double& _nrn_mechanism_access_b(Node* node) {
    return node->b();
}
double& _nrn_mechanism_access_d(Node* node) {
    return node->d();
}
neuron::container::generic_data_handle*& _nrn_mechanism_access_dparam(Prop* prop) {
    return prop->dparam;
}
Extnode*& _nrn_mechanism_access_extnode(Node* node) {
    return node->extnode;
}
double& _nrn_mechanism_access_param(Prop* prop, int field, int array_index) {
    return prop->param(field, array_index);
}
double& _nrn_mechanism_access_rhs(Node* node) {
    return node->rhs();
}
double& _nrn_mechanism_access_voltage(Node* node) {
    return node->v();
}
neuron::container::data_handle<double> _nrn_mechanism_get_area_handle(Node* node) {
    if (node) {
        return node->area_handle();
    } else {
        return {};
    }
}
Section* _nrn_mechanism_get_child(Section* sec) {
    return sec->child;
}
int _nrn_mechanism_get_nnode(Section* sec) {
    return sec->nnode;
}
Node* _nrn_mechanism_get_node(Section* sec, int idx) {
    return sec->pnode[idx];
}
int _nrn_mechanism_get_num_vars(Prop* prop) {
    return prop->param_num_vars();
}
neuron::container::data_handle<double> _nrn_mechanism_get_param_handle(
    Prop* prop,
    neuron::container::field_index field) {
    return prop->param_handle(field);
}
NrnThread* _nrn_mechanism_get_thread(Node* node) {
    return node->_nt;
}
Section* _nrn_mechanism_get_sibling(Section* sec) {
    return sec->sibling;
}
int _nrn_mechanism_get_type(Prop* prop) {
    return prop ? prop->_type : -1;
}
int _nrn_mechanism_get_v_node_index(Node* node) {
    return node->v_node_index;
}
neuron::container::non_owning_identifier_without_container _nrn_get_prop_id(Prop* p) {
    return p->id();
}

namespace neuron::mechanism::_get {
std::size_t _current_row(Prop* prop) {
    return prop ? prop->current_row() : container::invalid_row;
}
std::vector<double* const*> const& _pdata_ptr_cache_data(
    neuron::model_sorted_token const& cache_token,
    int mech_type) {
    return cache_token.mech_cache(mech_type).pdata_ptr_cache;
}
}  // namespace neuron::mechanism::_get
