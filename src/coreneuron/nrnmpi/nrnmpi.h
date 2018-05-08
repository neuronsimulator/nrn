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

#ifndef nrnmpi_h
#define nrnmpi_h
#include "coreneuron/nrnmpi/nrnmpiuse.h"

namespace coreneuron {
/* by default nrnmpi_numprocs_world = nrnmpi_numprocs = nrnmpi_numsubworlds and
   nrnmpi_myid_world = nrnmpi_myid and the bulletin board and network communication do
   not easily coexist. ParallelContext.subworlds(nsmall) divides the world into
   nrnmpi_numprocs_world/small subworlds of size nsmall.
*/
extern int nrnmpi_numprocs_world; /* size of entire world. total size of all subworlds */
extern int nrnmpi_myid_world;     /* rank in entire world */
extern int nrnmpi_numprocs;       /* size of subworld */
extern int nrnmpi_myid;           /* rank in subworld */
extern int nrnmpi_numprocs_bbs;   /* number of subworlds */
extern int nrnmpi_myid_bbs;       /* rank in nrn_bbs_comm of rank 0 of a subworld */

void nrn_abort(int errcode);
void nrn_fatal_error(const char* msg);
double nrn_wtime();
} //namespace coreneuron


#if NRNMPI

namespace coreneuron {
typedef struct {
    int gid;
    double spiketime;
} NRNMPI_Spike;

extern int nrnmpi_use; /* NEURON does MPI init and terminate?*/

} //namespace coreneuron
#include "coreneuron/nrnmpi/nrnmpidec.h"

#endif /*NRNMPI*/
#endif /*nrnmpi_h*/
