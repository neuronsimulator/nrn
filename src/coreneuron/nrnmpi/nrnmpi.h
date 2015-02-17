/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef nrnmpi_h
#define nrnmpi_h
#include "coreneuron/nrnmpi/nrnmpiuse.h"

/* by default nrnmpi_numprocs_world = nrnmpi_numprocs = nrnmpi_numsubworlds and
   nrnmpi_myid_world = nrnmpi_myid and the bulletin board and network communication do
   not easily coexist. ParallelContext.subworlds(nsmall) divides the world into
   nrnmpi_numprocs_world/small subworlds of size nsmall.
*/
extern int nrnmpi_numprocs_world; /* size of entire world. total size of all subworlds */
extern int nrnmpi_myid_world; /* rank in entire world */
extern int nrnmpi_numprocs; /* size of subworld */
extern int nrnmpi_myid; /* rank in subworld */
extern int nrnmpi_numprocs_bbs; /* number of subworlds */
extern int nrnmpi_myid_bbs; /* rank in nrn_bbs_comm of rank 0 of a subworld */

typedef struct {
	int gid;
	double spiketime;
} NRNMPI_Spike;
	        
#if NRNMPI

#if defined(__cplusplus)
extern "C" {
#endif

extern int nrnmpi_use; /* NEURON does MPI init and terminate?*/

#if defined(__cplusplus)
}
#endif /*c++*/

#include "coreneuron/nrnmpi/nrnmpidec.h"

#endif /*NRNMPI*/
#endif /*nrnmpi_h*/
