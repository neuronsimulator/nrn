/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @file nrn_stats.cpp
 * @date 25th Dec 2014
 * @brief Function declarations for the cell statistics
 *
 */

#include <stdio.h>
#include <climits>
#include "nrn_stats.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/netcvode.h"
#include "coreneuron/nrniv/partrans.h"

extern int spikevec_size;
extern int* spikevec_gid;
extern NetCvode* net_cvode_instance;

const int NUM_STATS = 12;
#if COLLECT_TQueue_STATISTICS
const int NUM_EVENT_TYPES = 3;
#endif
enum event_type {enq=0, spike, ite};

void report_cell_stats( void )
{
    long stat_array[NUM_STATS] = {0,0,0,0,0,0,0,0,0,0,0,0}, gstat_array[NUM_STATS];

    for (int ith=0; ith < nrn_nthread; ++ith)
    {
        stat_array[0] += (long)nrn_threads[ith].ncell;           // number of cells
        stat_array[10] += (long)nrn_threads[ith].end;            // number of compartments
        stat_array[1] += (long)nrn_threads[ith].n_presyn;        // number of presyns
        stat_array[2] += (long)nrn_threads[ith].n_input_presyn;  // number of input presyns
        stat_array[3] += (long)nrn_threads[ith].n_netcon;        // number of netcons, synapses
        stat_array[4] += (long)nrn_threads[ith].n_pntproc;       // number of point processes
        if (nrn_partrans::transfer_thread_data_) {
          int ntar = nrn_partrans::transfer_thread_data_[ith].ntar;
          stat_array[11] += (long)ntar;  // number of transfer (gap) targets
        }
    }
    stat_array[5] = (long)spikevec_size;                         // number of spikes

    int spikevec_positive_gid_size = 0;
    for (int i=0; i < spikevec_size; ++i)
        if (spikevec_gid[i] > -1)
            spikevec_positive_gid_size++;

    stat_array[6] = (long)spikevec_positive_gid_size;            // number of non-negative gid spikes

    /// Event queuing statistics
#if COLLECT_TQueue_STATISTICS
//    long que_stat[3] = {0, 0, 0}, gmax_que_stat[3];
    /// Number of events for each thread, enqueued and spike enqueued
    std::vector<long> thread_vec_events[NUM_EVENT_TYPES];                               // Number of events throughout the simulation
    std::vector< std::pair<double, long> > thread_vec_max_num_events[NUM_EVENT_TYPES];   // Time and the maximum number of events
    std::vector<long> thread_vec_event_times[NUM_EVENT_TYPES];                          // Number of time intervals for events in the simulation
    for (int type = 0; type < NUM_EVENT_TYPES; ++type){
        thread_vec_events[type].resize(nrn_nthread);
        thread_vec_max_num_events[type].resize(nrn_nthread);
        thread_vec_event_times[type].resize(nrn_nthread);
    }

    std::map<double, long>::const_iterator mapit;

    /// Get the total number of enqueued events and enqueued with spike events
    /// time_map_events - maps from TQueue class in sptbinq.h, - a collector of events statistics
    for (int ith=0; ith < nrn_nthread; ++ith)
    {
        for (int type = 0; type < NUM_EVENT_TYPES; ++type) {
            thread_vec_event_times[type][ith] += (long)net_cvode_instance->p[ith].tqe_->time_map_events[type].size();
            thread_vec_max_num_events[type][ith].second = 0;
            mapit = net_cvode_instance->p[ith].tqe_->time_map_events[type].begin();
            for (; mapit != net_cvode_instance->p[ith].tqe_->time_map_events[type].end(); ++mapit) {
                thread_vec_events[type][ith] += mapit->second;
                if (mapit->second > thread_vec_max_num_events[type][ith].second) {
                    thread_vec_max_num_events[type][ith].second = mapit->second;
                    thread_vec_max_num_events[type][ith].first = mapit->first;
                }
            }
            stat_array[7+type] += thread_vec_events[type][ith];     // number of enqueued events and number of spike triggered events (enqueued after spike exchange)
        }
    }


    /// Maximum number of events and correspondent time
    long max_num_events[NUM_EVENT_TYPES] = {0,0,0}, gmax_num_events[NUM_EVENT_TYPES];
    /// Get the maximum number of events one between threads first
    for (int type = 0; type < NUM_EVENT_TYPES; ++type) {
        for (int ith = 0; ith < nrn_nthread; ++ith) {
            if (thread_vec_max_num_events[type][ith].second > max_num_events[type]) {
                max_num_events[type] = thread_vec_max_num_events[type][ith].second;
            }
        }
    }
    nrnmpi_long_allreduce_vec( max_num_events, gmax_num_events, NUM_EVENT_TYPES, 2 );

    long qmin[NUM_EVENT_TYPES] = {LONG_MAX, LONG_MAX, LONG_MAX}, qmax[NUM_EVENT_TYPES] = {0,0,0}, qsum[NUM_EVENT_TYPES] = {0,0,0}, qdiff[NUM_EVENT_TYPES];
    long gqmax[NUM_EVENT_TYPES], gqmin[NUM_EVENT_TYPES], gqsum[NUM_EVENT_TYPES], gqdiff_max[NUM_EVENT_TYPES], gqdiff_min[NUM_EVENT_TYPES];
    /// Max and min number of time intervals for the events and difference between threads
    for (int type = 0; type < NUM_EVENT_TYPES; ++type) {
        for (int ith = 0; ith < nrn_nthread; ++ith) {
            qsum[type] += thread_vec_event_times[type][ith];
            if (thread_vec_event_times[type][ith] > qmax[type])
                qmax[type] = thread_vec_event_times[type][ith];
            if (thread_vec_event_times[type][ith] < qmin[type])
                qmin[type] = thread_vec_event_times[type][ith];
        }
        qdiff[type] = qmax[type] - qmin[type];
    }
    nrnmpi_long_allreduce_vec( qsum, gqsum, NUM_EVENT_TYPES, 1 );
    nrnmpi_long_allreduce_vec( qmax, gqmax, NUM_EVENT_TYPES, 2 );
    nrnmpi_long_allreduce_vec( qmin, gqmin, NUM_EVENT_TYPES, 0 );
    nrnmpi_long_allreduce_vec( qdiff, gqdiff_max, NUM_EVENT_TYPES, 2 );
    nrnmpi_long_allreduce_vec( qdiff, gqdiff_min, NUM_EVENT_TYPES, 0 );
#endif

    nrnmpi_long_allreduce_vec( stat_array, gstat_array, NUM_STATS, 1 );

    if ( nrnmpi_myid == 0 )
    {
        printf("\n\n Simulation Statistics\n");
        printf(" Number of cells: %ld\n", gstat_array[0]);
        printf(" Number of compartments: %ld\n", gstat_array[10]);
        printf(" Number of presyns: %ld\n", gstat_array[1]);
        printf(" Number of input presyns: %ld\n", gstat_array[2]);
        printf(" Number of synapses: %ld\n", gstat_array[3]);
        printf(" Number of point processes: %ld\n", gstat_array[4]);
        printf(" Number of transfer (gap) targets: %ld\n", gstat_array[11]);
        printf(" Number of spikes: %ld\n", gstat_array[5]);
        printf(" Number of spikes with non negative gid-s: %ld\n", gstat_array[6]);
#if COLLECT_TQueue_STATISTICS
        printf(" Number of enqueued events: %ld\n", gstat_array[7]);
        printf(" Maximum number of time intervals for the events: %ld\n", gqmax[enq]);
        printf(" Number of after-spike enqueued events: %ld\n", gstat_array[8]);
        printf(" Number of inter-thread enqueued events: %ld\n", gstat_array[9]);
//        printf(" Maximum difference of time interval enqueued events between threads on a single MPI: %ld\n", gqdiff_max[enq]);
//        printf(" Maximum difference of time interval spike enqueued events between threads on a single MPI: %ld\n", gqdiff_max[spike]);
//        printf(" Minimum difference of time interval enqueued events between threads on a single MPI: %ld\n", gqdiff_min[enq]);
//        printf(" Minimum difference of time interval spike enqueued events between threads on a single MPI: %ld\n", gqdiff_min[spike]);
        printf(" Maximum number of enqueued events during specific time by one thread: %ld\n", gmax_num_events[enq]);
        printf(" Maximum number of spike enqueued events during specific time by one thread: %ld\n", gmax_num_events[spike]);
#endif
    }

#if COLLECT_TQueue_STATISTICS
    int q_detailed_stats = 0;
    if (q_detailed_stats) {
        nrnmpi_barrier();
        if ( nrnmpi_myid == 0 )
            printf("\n Times for maximum number of enqueued events: ");
        nrnmpi_barrier();
        for (int ith = 0; ith < nrn_nthread; ++ith) {
            if (thread_vec_max_num_events[enq][ith].second == gmax_num_events[enq])
                printf("%lf\n", thread_vec_max_num_events[enq][ith].first);
        }
        nrnmpi_barrier();

        if ( nrnmpi_myid == 0 )
            printf("\n\n Times for maximum number of spike enqueued events: ");
        nrnmpi_barrier();
        for (int ith = 0; ith < nrn_nthread; ++ith) {
            if (thread_vec_max_num_events[spike][ith].second == gmax_num_events[spike])
               printf("%lf\n", thread_vec_max_num_events[spike][ith].first);
        }
        nrnmpi_barrier();
    }
#endif
    if ( nrnmpi_myid == 0 )
        printf("\n\n");
}

