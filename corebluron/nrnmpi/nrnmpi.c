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

#include <string.h>
#include "corebluron/nrnconf.h"
#include "corebluron/nrnmpi/nrnmpi.h"
#include "corebluron/nrnmpi/mpispike.h"
#include "corebluron/nrnmpi/nrnmpi_def_cinc.h"
#include "corebluron/nrniv/nrn_assert.h"

#if NRNMPI
#include <mpi.h>
#define USE_HPM 0
#if USE_HPM
#include <libhpm.h>
#endif

int nrnmusic;

MPI_Comm nrnmpi_world_comm;
MPI_Comm nrnmpi_comm;
MPI_Comm nrn_bbs_comm;
static MPI_Group grp_bbs;
static MPI_Group grp_net;

extern void nrnmpi_spike_initialize();

#define nrnmpidebugleak 0
#if nrnmpidebugleak
extern void nrnmpi_checkbufleak();
#endif

static int nrnmpi_under_nrncontrol_;
#endif

void nrnmpi_init(int nrnmpi_under_nrncontrol, int* pargc, char*** pargv) {
#if NRNMPI
	int i, b, flag;
	static int called = 0;
	if (called) { return; }
	called = 1;
	nrnmpi_use = 1;
	nrnmpi_under_nrncontrol_ = nrnmpi_under_nrncontrol;
	if( nrnmpi_under_nrncontrol_ ) {
#if 0
{int i;
printf("nrnmpi_init: argc=%d\n", *pargc);
for (i=0; i < *pargc; ++i) {
	printf("%d |%s|\n", i, (*pargv)[i]);
}
}
#endif

#if !ALWAYS_CALL_MPI_INIT
	/* this is not good. depends on mpirun adding at least one
	   arg that starts with -p4 but that probably is dependent
	   on mpich and the use of the ch_p4 device. We are trying to
	   work around the problem that MPI_Init may change the working
	   directory and so when not invoked under mpirun we would like to
	   NOT call MPI_Init.
	*/
		b = 0;
		for (i=0; i < *pargc; ++i) {
			if (strncmp("-p4", (*pargv)[i], 3) == 0) {
				b = 1;
				break;
			}
			if (strcmp("-mpi", (*pargv)[i]) == 0) {
				b = 1;
				break;
			}
		}
		if (nrnmusic) { b = 1; }
		if (!b) {
			nrnmpi_use = 0;
			nrnmpi_under_nrncontrol_ = 0;
			return;
		}
#endif
		MPI_Initialized(&flag);

		if (!flag && MPI_Init(pargc, pargv) != MPI_SUCCESS) {
		  printf("MPI_INIT failed\n");
		}

		{
			nrn_assert(MPI_Comm_dup(MPI_COMM_WORLD, &nrnmpi_world_comm));
		}
	}
	grp_bbs = MPI_GROUP_NULL;
	grp_net = MPI_GROUP_NULL;
	nrn_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrnmpi_comm));
	nrn_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrn_bbs_comm));
	nrn_assert(MPI_Comm_rank(nrnmpi_world_comm, &nrnmpi_myid_world));
	nrn_assert(MPI_Comm_size(nrnmpi_world_comm, &nrnmpi_numprocs_world));
	nrnmpi_numprocs = nrnmpi_numprocs_bbs = nrnmpi_numprocs_world;
	nrnmpi_myid = nrnmpi_myid_bbs = nrnmpi_myid_world;
	nrnmpi_spike_initialize();
	/*begin instrumentation*/
#if USE_HPM
	hpmInit( nrnmpi_myid_world, "mpineuron" );
#endif
#if 0
{int i;
printf("nrnmpi_init: argc=%d\n", *pargc);
for (i=0; i < *pargc; ++i) {
	printf("%d |%s|\n", i, (*pargv)[i]);
}
}
#endif
#if 1
	if (nrnmpi_myid == 0) {
		printf("numprocs=%d\n", nrnmpi_numprocs_world);
	}
#endif

#endif /* NRNMPI */



}

double nrnmpi_wtime() {
#if NRNMPI
	if (nrnmpi_use) {
		return MPI_Wtime();
	}
#endif
	return 0.0;
}


void nrnmpi_finalize(void) 
{
  MPI_Finalize();
}


void nrnmpi_terminate() {
#if NRNMPI
	if (nrnmpi_use) {
#if 0
		printf("%d nrnmpi_terminate\n", nrnmpi_myid_world);
#endif
#if USE_HPM
		hpmTerminate( nrnmpi_myid_world );
#endif
		if( nrnmpi_under_nrncontrol_ ) {
			MPI_Finalize();
		}
		nrnmpi_use = 0;
#if nrnmpidebugleak
		nrnmpi_checkbufleak();
#endif
	}
#endif /*NRNMPI*/
}

void nrnmpi_abort(int errcode) {
#if NRNMPI
	int flag;
	MPI_Initialized(&flag);
	if (flag) {
		MPI_Abort(MPI_COMM_WORLD, errcode);
	}else{
		abort();
	}
#else
	abort();
#endif
}

#if NRNMPI

/* so src/nrnpython/inithoc.cpp does not have to include a c++ mpi.h */
int nrnmpi_wrap_mpi_init(int* flag) {
	return MPI_Initialized(flag);
}
	
#endif
