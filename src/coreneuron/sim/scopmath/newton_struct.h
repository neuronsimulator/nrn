/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#pragma once
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"

namespace coreneuron {

/* avoid incessant alloc/free memory */
struct NewtonSpace {
    int n;
    int n_instance;
    double* delta_x;
    double** jacobian;
    int* perm;
    double* high_value;
    double* low_value;
    double* rowmax;
};

void nrn_newtonspace_copyto_device(NewtonSpace* ns);
void nrn_newtonspace_delete_from_device(NewtonSpace* ns);

}  // namespace coreneuron
