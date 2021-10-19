/*
# =============================================================================
# Copyright (c) 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <utility>
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/core/nrnmpi.hpp"

namespace coreneuron {
extern void nrn_abort(int errcode);
template <typename... Args>
void nrn_fatal_error(const char* msg, Args&&... args) {
    if (nrnmpi_myid == 0) {
        printf(msg, std::forward<Args>(args)...);
    }
    nrn_abort(-1);
}
extern double nrn_wtime(void);
}  // namespace coreneuron
