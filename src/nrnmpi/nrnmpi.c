#include <../../nrnconf.h>
#include <nrnmpi.h>

int nrnmpi_numprocs = 1; /* size */
int nrnmpi_myid = 0; /* rank */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern double nrn_timeus();

#if NRNMPI
#include <mpi.h>
#define USE_HPM 0
#if USE_HPM
#include <libhpm.h>
#endif

int nrnmpi_use; /* NEURON does MPI init and terminate?*/
MPI_Comm nrnmpi_comm;
MPI_Comm nrn_bbs_comm;

extern void nrnmpi_spike_initialize();

#define nrnmpidebugleak 0
#if nrnmpidebugleak
extern void nrnmpi_checkbufleak();
#endif

static int nrnmpi_under_nrncontrol_;
#endif

void nrnmpi_init(int nrnmpi_under_nrncontrol, int* pargc, char*** pargv) {
#if NRNMPI
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
		int i, b;
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
		if (!b) {
			nrnmpi_use = 0;
			nrnmpi_under_nrncontrol_ = 0;
			return;
		}
#endif
		int flag;
		MPI_Initialized(&flag);

		if (flag || MPI_Init(pargc, pargv) == MPI_SUCCESS) ;
		else printf("MPI_INIT failed\n");

		MPI_Comm_dup(MPI_COMM_WORLD, &nrnmpi_comm);
	}
	MPI_Comm_dup(nrnmpi_comm, &nrn_bbs_comm);
	if (MPI_Comm_rank(nrnmpi_comm, &nrnmpi_myid) != MPI_SUCCESS) {
		printf("MPI_Comm_rank failed\n");
	}
	if (MPI_Comm_size(nrnmpi_comm, &nrnmpi_numprocs) != MPI_SUCCESS) {
		printf("MPI_Comm_size failed\n");
	}
	nrnmpi_spike_initialize();
	/*begin instrumentation*/
#if USE_HPM
	hpmInit( nrnmpi_myid, "mpineuron" );
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
		printf("numprocs=%d\n", nrnmpi_numprocs);
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
	return nrn_timeus();
}

void nrnmpi_terminate() {
#if NRNMPI
	if (nrnmpi_use) {
#if 0
		printf("%d nrnmpi_terminate\n", nrnmpi_myid);
#endif
#if USE_HPM
		hpmTerminate( nrnmpi_myid );
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
