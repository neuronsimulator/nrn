/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#include <math.h>
#include <stdlib.h>

#include "coreneuron/sim/scopmath/newton_thread.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {
NewtonSpace* nrn_cons_newtonspace(int n, int n_instance, bool compact_memory_layout) {
    NewtonSpace* ns = (NewtonSpace*) emalloc(sizeof(NewtonSpace));
    int m_instance = compact_memory_layout ? 1 : n_instance;
    ns->n = n;
    ns->n_instance = n_instance;
    ns->delta_x = makevector(n * m_instance * sizeof(double));
    ns->jacobian = makematrix(n, n*m_instance);
    ns->perm = (int*) emalloc((unsigned) (n * m_instance * sizeof(int)));
    ns->high_value = makevector(n * m_instance * sizeof(double));
    ns->low_value = makevector(n * m_instance * sizeof(double));
    ns->rowmax = makevector(n * m_instance * sizeof(double));
    ns->compact_memory_layout = compact_memory_layout;
    nrn_newtonspace_copyto_device(ns);
    return ns;
}

void nrn_destroy_newtonspace(NewtonSpace* ns) {
    nrn_newtonspace_delete_from_device(ns);
    free((char*) ns->perm);
    freevector(ns->delta_x);
    freematrix(ns->jacobian);
    freevector(ns->high_value);
    freevector(ns->low_value);
    freevector(ns->rowmax);
    free((char*) ns);
}
}  // namespace coreneuron
