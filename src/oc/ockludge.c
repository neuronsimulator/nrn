#include <../../nrnconf.h>
#include <stdlib.h>
#include <nrnmpi.h>

#if NRNMPI && DARWIN && !defined(NRNMPI_DYNAMICLOAD)
// For DARWIN I do not really know the proper way to avoid
// dyld: lazy symbol binding failed: Symbol not found: _MPI_Init
// when the MPI functions are all used in the libnrnmpi.dylib
// but the libmpi.a is statically linked. Therefore I am forcing
// the linking here by listing all the MPI functions being used.
#include <mpi.h>
void work_around() {
	MPI_Comm c = MPI_COMM_WORLD;

	MPI_Abort(c, 0);
	MPI_Address(0,0);
	MPI_Allgather(0,0,0,0,0,0,c);
	MPI_Allgatherv(0,0,0,0,0,0,0,c);
	MPI_Allreduce(0,0,0,0,0,c);
	MPI_Barrier(c);
	MPI_Bcast(0,0,0,0,c);
	MPI_Comm_dup(c, 0);
	MPI_Comm_rank(c, 0);
	MPI_Comm_size(c, 0);
	MPI_Finalize();
	MPI_Gather(0,0,0,0,0,0,0,c);
	MPI_Gatherv(0,0,0,0,0,0,0,0,c);
	MPI_Get_count(0, 0, 0);
	MPI_Init(0, 0);
	MPI_Initialized(0);
	MPI_Iprobe(0,0,c,0,0);
	MPI_Irecv(0,0,0,0,0,c,0);
	MPI_Isend(0,0,0,0,0,c,0);
	MPI_Op_create(0, 0, 0);
	MPI_Pack(0, 0, 0, 0, 0, 0, c);
	MPI_Pack_size(0, 0, c, 0);
	MPI_Probe(0, 0, c, 0);
	MPI_Recv(0,0,0,0,0,c,0);
	MPI_Request_free(0);
	MPI_Send(0,0,0,0,0,c);
	MPI_Sendrecv(0,0,0,0,0,0,0,0,0,0,c,0);
	MPI_Type_commit(0);
	MPI_Type_struct(0,0,0,0,0);
	MPI_Unpack(0, 0, 0, 0, 0, 0, c);
	MPI_Wait(0, 0);
	MPI_Wtime();
}
#endif

