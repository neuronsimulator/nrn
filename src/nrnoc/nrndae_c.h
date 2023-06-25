#pragma once
#include "neuron/container/data_handle.hpp"

#include <vector>

extern void nrndae_alloc(void);
extern int nrndae_extra_eqn_count(void);
extern void nrndae_init(void);
extern void nrndae_rhs(NrnThread*); /* relative to c*dy/dt = -g*y + b */
extern void nrndae_lhs(void);
void nrndae_dkmap(std::vector<neuron::container::data_handle<double>>&,
                  std::vector<neuron::container::data_handle<double>>&);
extern void nrndae_dkres(double*, double*, double*);
extern void nrndae_dkpsol(double);
extern void nrndae_update(NrnThread*);
void nrn_matrix_node_free();
extern int nrndae_list_is_empty(void);

extern int nrn_use_daspk_;
