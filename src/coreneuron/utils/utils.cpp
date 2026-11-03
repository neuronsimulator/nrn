/*
# =============================================================================
# Copyright (c) 2021-22 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include <chrono>
#include "utils.hpp"
#include "coreneuron/apps/corenrn_parameters.hpp"

namespace coreneuron {
[[noreturn]] void nrn_abort(int errcode) {
#if NRNMPI
    if (corenrn_param.mpi_enable && nrnmpi_initialized()) {
        nrnmpi_abort(errcode);
    }
#endif
    std::abort();
}

double nrn_wtime() {
#if NRNMPI
    if (corenrn_param.mpi_enable) {
        return nrnmpi_wtime();
    } else
#endif
    {
        const auto now = std::chrono::time_point_cast<std::chrono::duration<double>>(
            std::chrono::system_clock::now());
        return now.time_since_epoch().count();
    }
}

extern "C" {
void (*nrn2core_subworld_info_)(int&, int&, int&);
}


}  // namespace coreneuron
