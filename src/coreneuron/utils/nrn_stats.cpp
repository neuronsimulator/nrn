/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

/**
 * @file nrn_stats.cpp
 * @date 25th Dec 2014
 * @brief Function declarations for the cell statistics
 *
 */

#include <algorithm>
#include <cstdio>
#include <climits>
#include <vector>
#include "coreneuron/utils/nrn_stats.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/io/output_spikes.hpp"
#include "coreneuron/apps/corenrn_parameters.hpp"
namespace coreneuron {
const int NUM_STATS = 13;

void report_cell_stats() {
    long stat_array[NUM_STATS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    for (int ith = 0; ith < nrn_nthread; ++ith) {
        stat_array[0] += nrn_threads[ith].ncell;           // number of cells
        stat_array[10] += nrn_threads[ith].end;            // number of compartments
        stat_array[1] += nrn_threads[ith].n_presyn;        // number of presyns
        stat_array[2] += nrn_threads[ith].n_input_presyn;  // number of input presyns
        stat_array[3] += nrn_threads[ith].n_netcon;        // number of netcons, synapses
        stat_array[4] += nrn_threads[ith].n_pntproc;       // number of point processes
        if (nrn_partrans::transfer_thread_data_) {
            size_t n = nrn_partrans::transfer_thread_data_[ith].tar_indices.size();
            stat_array[11] += n;  // number of transfer targets
            n = nrn_partrans::transfer_thread_data_[ith].src_indices.size();
            stat_array[12] += n;  // number of transfer sources
        }
    }
    stat_array[5] = spikevec_gid.size();  // number of spikes

    stat_array[6] = std::count_if(spikevec_gid.cbegin(), spikevec_gid.cend(), [](const int& s) {
        return s > -1;
    });  // number of non-negative gid spikes

#if NRNMPI
    long gstat_array[NUM_STATS];
    if (corenrn_param.mpi_enable) {
        nrnmpi_long_allreduce_vec(stat_array, gstat_array, NUM_STATS, 1);
    } else {
        assert(sizeof(stat_array) == sizeof(gstat_array));
        std::memcpy(gstat_array, stat_array, sizeof(stat_array));
    }
#else
    const long(&gstat_array)[NUM_STATS] = stat_array;
#endif

    if (nrnmpi_myid == 0) {
        printf("\n\n Simulation Statistics\n");
        printf(" Number of cells: %ld\n", gstat_array[0]);
        printf(" Number of compartments: %ld\n", gstat_array[10]);
        printf(" Number of presyns: %ld\n", gstat_array[1]);
        printf(" Number of input presyns: %ld\n", gstat_array[2]);
        printf(" Number of synapses: %ld\n", gstat_array[3]);
        printf(" Number of point processes: %ld\n", gstat_array[4]);
        printf(" Number of transfer sources: %ld\n", gstat_array[12]);
        printf(" Number of transfer targets: %ld\n", gstat_array[11]);
        printf(" Number of spikes: %ld\n", gstat_array[5]);
        printf(" Number of spikes with non negative gid-s: %ld\n", gstat_array[6]);
    }
}
}  // namespace coreneuron
