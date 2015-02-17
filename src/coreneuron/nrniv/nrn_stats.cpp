/**
 * @file nrn_stats.cpp
 * @date 25th Dec 2014
 * @brief Function declarations for the cell statistics
 *
 */

#include <stdio.h>
#include "nrn_stats.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrnoc/multicore.h"

extern int spikevec_size;

const int NUM_STATS = 3;

void report_cell_stats( void )
{
  long stat_array[NUM_STATS] = {0,0,0}, gstat_array[NUM_STATS];

  for (int ith=0; ith < nrn_nthread; ++ith)
  {
    stat_array[0] += (long)nrn_threads[ith].ncell;     // number of cells
    stat_array[1] += (long)nrn_threads[ith].n_netcon;  // number of netcons, synapses
  }
  stat_array[2] = (long)spikevec_size;

  nrnmpi_long_allreduce_vec( stat_array, gstat_array, NUM_STATS, 1 );

  if ( nrnmpi_myid == 0 )
  {
    printf("\n");
    printf(" Number of cells in the simulation: %ld\n", gstat_array[0]);
    printf(" Number of synapses in the simulation: %ld\n", gstat_array[1]);
    printf(" Number of spikes: %ld\n", gstat_array[2]);
    printf("\n");
  }
}

