/**
 * @file nrn_stats.cpp
 * @date 25th Dec 2014
 * @brief Function declarations for the cell statistics
 *
 */

#include <stdio.h>
#include "nrn_stats.h"
#include "corebluron/nrnmpi/nrnmpi.h"
#include "corebluron/nrnoc/multicore.h"

const int NUM_STATS = 2;

void report_cell_stats( void )
{
  long stat_array[NUM_STATS] = {0,0}, gstat_array[NUM_STATS];

  for (int ith=0; ith < nrn_nthread; ++ith)
  {
    stat_array[0] += (long)nrn_threads[ith].ncell;     // number of cells
    stat_array[1] += (long)nrn_threads[ith].n_netcon;  // number of netcons, synapses
  }

  nrnmpi_long_allreduce_vec( stat_array, gstat_array, NUM_STATS, 1 );

  if ( nrnmpi_myid == 0 )
  {
    printf("\n");
    printf(" Number of cells in the simulation: %ld\n", gstat_array[0]);
    printf(" Number of synapses in the simulation: %ld\n", gstat_array[1]);
    printf("\n");
  }
}

