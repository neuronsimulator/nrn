/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <iostream>
#include <vector>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/io/nrn_setup.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/apps/corenrn_parameters.hpp"

namespace coreneuron {
/** display global mechanism count */
void write_mech_report() {
    /// mechanim count across all gids, local to rank
    const auto n_memb_func = corenrn.get_memb_funcs().size();
    std::vector<long> local_mech_count(n_memb_func, 0);
    std::vector<long> local_mech_size(n_memb_func, 0);

    /// each gid record goes on separate row, only check non-empty threads
    for (int i = 0; i < nrn_nthread; i++) {
        const auto& nt = nrn_threads[i];
        for (auto* tml = nt.tml; tml; tml = tml->next) {
            const int type = tml->index;
            const auto& ml = tml->ml;
            local_mech_count[type] += ml->nodecount;
            local_mech_size[type] = memb_list_size(tml, true);
        }
    }

    std::vector<long> total_mech_count(n_memb_func);
    std::vector<long> total_mech_size(n_memb_func);

#if NRNMPI
    if (corenrn_param.mpi_enable) {
        /// get global sum of all mechanism instances
        nrnmpi_long_allreduce_vec(&local_mech_count[0],
                                  &total_mech_count[0],
                                  local_mech_count.size(),
                                  1);
        nrnmpi_long_allreduce_vec(&local_mech_size[0],
                                  &total_mech_size[0],
                                  local_mech_size.size(),
                                  1);
    } else
#endif
    {
        total_mech_count = local_mech_count;
        total_mech_size = local_mech_size;
    }

    /// print global stats to stdout
    if (nrnmpi_myid == 0) {
        printf("\n============== MECHANISMS COUNT AND SIZE BY TYPE =============\n");
        printf("%4s %20s %10s %25s\n", "Id", "Name", "Count", "Total memory size (KiB)");
        for (size_t i = 0; i < total_mech_count.size(); i++) {
            if (total_mech_count[i] > 0) {
                printf("%4lu %20s %10ld %25.2lf\n",
                       i,
                       nrn_get_mechname(i),
                       total_mech_count[i],
                       static_cast<double>(total_mech_size[i]) / 1024);
            }
        }
        printf("==============================================================\n");
    }
}

}  // namespace coreneuron
