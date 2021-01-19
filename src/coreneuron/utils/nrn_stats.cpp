/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
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

#include <cstdio>
#include <climits>
#include <vector>
#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/utils/nrn_stats.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/io/output_spikes.hpp"
namespace coreneuron {
extern corenrn_parameters corenrn_param;

const int NUM_STATS = 13;
enum event_type { enq = 0, spike, ite };

void report_cell_stats(void) {
    long stat_array[NUM_STATS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, gstat_array[NUM_STATS];

    for (int ith = 0; ith < nrn_nthread; ++ith) {
        stat_array[0] += (long) nrn_threads[ith].ncell;           // number of cells
        stat_array[10] += (long) nrn_threads[ith].end;            // number of compartments
        stat_array[1] += (long) nrn_threads[ith].n_presyn;        // number of presyns
        stat_array[2] += (long) nrn_threads[ith].n_input_presyn;  // number of input presyns
        stat_array[3] += (long) nrn_threads[ith].n_netcon;        // number of netcons, synapses
        stat_array[4] += (long) nrn_threads[ith].n_pntproc;       // number of point processes
        if (nrn_partrans::transfer_thread_data_) {
            size_t n = nrn_partrans::transfer_thread_data_[ith].tar_indices.size();
            stat_array[11] += (long) n;  // number of transfer targets
            n = nrn_partrans::transfer_thread_data_[ith].src_indices.size();
            stat_array[12] += (long) n;  // number of transfer sources
        }
    }
    stat_array[5] = (long) spikevec_gid.size();  // number of spikes

    int spikevec_positive_gid_size = 0;
    for (std::size_t i = 0; i < spikevec_gid.size(); ++i) {
        if (spikevec_gid[i] > -1) {
            spikevec_positive_gid_size++;
        }
    }

    stat_array[6] = (long) spikevec_positive_gid_size;  // number of non-negative gid spikes

#if NRNMPI
    nrnmpi_long_allreduce_vec(stat_array, gstat_array, NUM_STATS, 1);
#else
    assert(sizeof(stat_array) == sizeof(gstat_array));
    memcpy(gstat_array, stat_array, sizeof(stat_array));
#endif

    if (nrnmpi_myid == 0 && !corenrn_param.is_quiet()) {
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
