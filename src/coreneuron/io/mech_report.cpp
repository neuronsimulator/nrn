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
#include "coreneuron/mpi/nrnmpi.h"

namespace coreneuron {

/** display global mechanism count */
void write_mech_report() {
    /// mechanim count across all gids, local to rank
    const auto n_memb_func = corenrn.get_memb_funcs().size();
    std::vector<long> local_mech_count(n_memb_func, 0);

    /// each gid record goes on separate row, only check non-empty threads
    for (size_t i = 0; i < nrn_nthread; i++) {
        const auto& nt = nrn_threads[i];
        for (auto* tml = nt.tml; tml; tml = tml->next) {
            const int type = tml->index;
            const auto& ml = tml->ml;
            local_mech_count[type] += ml->nodecount;
        }
    }

    std::vector<long> total_mech_count(n_memb_func);

#if NRNMPI
    /// get global sum of all mechanism instances
    nrnmpi_long_allreduce_vec(&local_mech_count[0],
                              &total_mech_count[0],
                              local_mech_count.size(),
                              1);

#else
    total_mech_count = local_mech_count;
#endif
    /// print global stats to stdout
    if (nrnmpi_myid == 0) {
        printf("\n================ MECHANISMS COUNT BY TYPE ==================\n");
        printf("%4s %20s %10s\n", "Id", "Name", "Count");
        for (size_t i = 0; i < total_mech_count.size(); i++) {
            printf("%4lu %20s %10ld\n", i, nrn_get_mechname(i), total_mech_count[i]);
        }
        printf("=============================================================\n");
    }
}

}  // namespace coreneuron
