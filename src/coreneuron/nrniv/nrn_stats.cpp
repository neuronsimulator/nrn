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

