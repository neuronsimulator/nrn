/* a few calls to nrnmpi functions are made even when MPI not available */
/* these need enough implementation to work with nrnmpi_numprocs = 1 */

#include "nrnmpiuse.h"
#if NRNMPI_DYNAMICLOAD /* to end of file */

#include <stdio.h>
#include "nrnmpidec.h"

extern double nrn_timeus();

static void stub_nrnmpi_init(int nrnmpi_under_nrncontrol, int* pargc, char*** pargv) {}
static void stub_nrnmpi_terminate(){}
static double stub_nrnmpi_mindelay(double m) { return m; }
static int stub_nrnmpi_int_allmax(int x) { return x; }
static void stub_nrnmpi_barrier(){}

void nrnmpi_stubs() {
	p_nrnmpi_init = stub_nrnmpi_init;
	p_nrnmpi_terminate = stub_nrnmpi_terminate;
	p_nrnmpi_wtime = nrn_timeus;
	p_nrnmpi_mindelay = stub_nrnmpi_mindelay;
	p_nrnmpi_int_allmax = stub_nrnmpi_int_allmax;
	p_nrnmpi_barrier = stub_nrnmpi_barrier;
}

#endif
