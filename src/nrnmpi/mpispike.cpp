#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

/* do not want the redef in the dynamic load case */
#include <nrnmpiuse.h>

#if NRNMPI_DYNAMICLOAD
#include <nrnmpi_dynam.h>
#endif

#include <nrnmpi.h>
#include <hocdec.h>

#if NRNMPI
#include "nrnmpidec.h"
#include "nrnmpi_impl.h"
#include "mpispike.h"
#include <mpi.h>

#include <limits>
#include <string>

#define nrn_mpi_assert(arg) nrn_assert(arg == MPI_SUCCESS)

extern void nrnbbs_context_wait();

static int np;
static int* displs;
static int* byteovfl; /* for the compressed transfer method */
static MPI_Datatype spike_type;

static void pgvts_op(double* in, double* inout, int* len, MPI_Datatype* dptr);
static MPI_Op mpi_pgvts_op;

static void make_spike_type() {
    NRNMPI_Spike s;
    int block_lengths[2];
    MPI_Aint displacements[2];
    MPI_Aint addresses[3];
    MPI_Datatype typelist[2];

    typelist[0] = MPI_INT;
    typelist[1] = MPI_DOUBLE;

    block_lengths[0] = block_lengths[1] = 1;

    MPI_Get_address(&s, &addresses[0]);
    MPI_Get_address(&(s.gid), &addresses[1]);
    MPI_Get_address(&(s.spiketime), &addresses[2]);

    displacements[0] = addresses[1] - addresses[0];
    displacements[1] = addresses[2] - addresses[0];

    MPI_Type_create_struct(2, block_lengths, displacements, typelist, &spike_type);
    MPI_Type_commit(&spike_type);

    MPI_Op_create((MPI_User_function*) pgvts_op, 1, &mpi_pgvts_op);
}

void nrnmpi_spike_initialize() {
    make_spike_type();
}

#if nrn_spikebuf_size > 0

static MPI_Datatype spikebuf_type;

static void make_spikebuf_type(int* nout_) {
    NRNMPI_Spikebuf s;
    int block_lengths[3];
    MPI_Aint displacements[3];
    MPI_Aint addresses[4];
    MPI_Datatype typelist[3];

    typelist[0] = MPI_INT;
    typelist[1] = MPI_INT;
    typelist[2] = MPI_DOUBLE;

    block_lengths[0] = 1;
    block_lengths[1] = nrn_spikebuf_size;
    block_lengths[2] = nrn_spikebuf_size;

    MPI_Get_address(&s, &addresses[0]);
    MPI_Get_address(&(s.nspike), &addresses[1]);
    MPI_Get_address(&(s.gid[0]), &addresses[2]);
    MPI_Get_address(&(s.spiketime[0]), &addresses[3]);

    displacements[0] = addresses[1] - addresses[0];
    displacements[1] = addresses[2] - addresses[0];
    displacements[2] = addresses[3] - addresses[0];

    MPI_Type_create_struct(3, block_lengths, displacements, typelist, &spikebuf_type);
    MPI_Type_commit(&spikebuf_type);
}
#endif

int nrnmpi_spike_exchange(int* ovfl,
                          int* nout_,
                          int* nin_,
                          NRNMPI_Spike* spikeout_,
                          NRNMPI_Spike** spikein_,
                          int* icapacity_) {
    int i, n, novfl, n1;
    if (!displs) {
        np = nrnmpi_numprocs;
        displs = (int*) hoc_Emalloc(np * sizeof(int));
        hoc_malchk();
        displs[0] = 0;
#if nrn_spikebuf_size > 0
        make_spikebuf_type(nout_);
#endif
    }
    nrnbbs_context_wait();
#if nrn_spikebuf_size == 0
    MPI_Allgather(nout_, 1, MPI_INT, nin_, 1, MPI_INT, nrnmpi_comm);
    n = nin_[0];
    for (i = 1; i < np; ++i) {
        displs[i] = n;
        n += nin_[i];
    }
    if (n) {
        if (*icapacity_ < n) {
            *icapacity_ = n + 10;
            free(*spikein_);
            *spikein_ = (NRNMPI_Spike*) hoc_Emalloc(*icapacity_ * sizeof(NRNMPI_Spike));
            hoc_malchk();
        }
        MPI_Allgatherv(
            spikeout_, *nout_, spike_type, *spikein_, nin_, displs, spike_type, nrnmpi_comm);
    }
#else
    MPI_Allgather(spbufout_, 1, spikebuf_type, spbufin_, 1, spikebuf_type, nrnmpi_comm);
    novfl = 0;
    n = spbufin_[0].nspike;
    if (n > nrn_spikebuf_size) {
        nin_[0] = n - nrn_spikebuf_size;
        novfl += nin_[0];
    } else {
        nin_[0] = 0;
    }
    for (i = 1; i < np; ++i) {
        displs[i] = novfl;
        n1 = spbufin_[i].nspike;
        n += n1;
        if (n1 > nrn_spikebuf_size) {
            nin_[i] = n1 - nrn_spikebuf_size;
            novfl += nin_[i];
        } else {
            nin_[i] = 0;
        }
    }
    if (novfl) {
        if (*icapacity_ < novfl) {
            *icapacity_ = novfl + 10;
            free(*spikein_);
            *spikein_ = (NRNMPI_Spike*) hoc_Emalloc(*icapacity_ * sizeof(NRNMPI_Spike));
            hoc_malchk();
        }
        n1 = (*nout_ > nrn_spikebuf_size) ? *nout_ - nrn_spikebuf_size : 0;
        MPI_Allgatherv(spikeout_, n1, spike_type, *spikein_, nin_, displs, spike_type, nrnmpi_comm);
    }
    *ovfl = novfl;
#endif
    return n;
}

/*
The compressed spike format is restricted to the fixed step method and is
a sequence of unsigned char.
nspike = buf[0]*256 + buf[1]
a sequence of spiketime, localgid pairs. There are nspike of them.
    spiketime is relative to the last transfer time in units of dt.
    note that this requires a mindelay < 256*dt.
    localgid is an unsigned int, unsigned short,
    or unsigned char in size depending on the range and thus takes
    4, 2, or 1 byte respectively. To be machine independent we do our
    own byte coding. When the localgid range is smaller than the true
    gid range, the gid->PreSyn are remapped into
    hostid specific	maps. If there are not many holes, i.e just about every
    spike from a source machine is delivered to some cell on a
    target machine, then instead of	a hash map, a vector is used.
The allgather sends the first part of the buf and the allgatherv buffer
sends any overflow.
*/
int nrnmpi_spike_exchange_compressed(int localgid_size,
                                     int ag_send_size,
                                     int ag_send_nspike,
                                     int* ovfl_capacity,
                                     int* ovfl,
                                     unsigned char* spfixout,
                                     unsigned char* spfixin,
                                     unsigned char** spfixin_ovfl,
                                     int* nin_) {
    int i, novfl, n, ntot, idx, bs, bstot; /* n is #spikes, bs is #byte overflow */
    if (!displs) {
        np = nrnmpi_numprocs;
        displs = (int*) hoc_Emalloc(np * sizeof(int));
        hoc_malchk();
        displs[0] = 0;
    }
    if (!byteovfl) {
        byteovfl = (int*) hoc_Emalloc(np * sizeof(int));
        hoc_malchk();
    }
    nrnbbs_context_wait();

    MPI_Allgather(spfixout, ag_send_size, MPI_BYTE, spfixin, ag_send_size, MPI_BYTE, nrnmpi_comm);
    novfl = 0;
    ntot = 0;
    bstot = 0;
    for (i = 0; i < np; ++i) {
        displs[i] = bstot;
        idx = i * ag_send_size;
        n = spfixin[idx++] * 256;
        n += spfixin[idx++];
        ntot += n;
        nin_[i] = n;
        if (n > ag_send_nspike) {
            bs = 2 + n * (1 + localgid_size) - ag_send_size;
            byteovfl[i] = bs;
            bstot += bs;
            novfl += n - ag_send_nspike;
        } else {
            byteovfl[i] = 0;
        }
    }
    if (novfl) {
        if (*ovfl_capacity < novfl) {
            *ovfl_capacity = novfl + 10;
            free(*spfixin_ovfl);
            *spfixin_ovfl = (unsigned char*) hoc_Emalloc(*ovfl_capacity * (1 + localgid_size) *
                                                         sizeof(unsigned char));
            hoc_malchk();
        }
        bs = byteovfl[nrnmpi_myid];
        /*
        note that the spfixout buffer is one since the overflow
        is contiguous to the first part. But the spfixin_ovfl is
        completely separate from the spfixin since the latter
        dynamically changes its size during a run.
        */
        MPI_Allgatherv(spfixout + ag_send_size,
                       bs,
                       MPI_BYTE,
                       *spfixin_ovfl,
                       byteovfl,
                       displs,
                       MPI_BYTE,
                       nrnmpi_comm);
    }
    *ovfl = novfl;
    return ntot;
}

double nrnmpi_mindelay(double m) {
    double result;
    if (!nrnmpi_use) {
        return m;
    }
    nrnbbs_context_wait();
    MPI_Allreduce(&m, &result, 1, MPI_DOUBLE, MPI_MIN, nrnmpi_comm);
    return result;
}

int nrnmpi_int_allmax(int x) {
    int result;
    if (nrnmpi_numprocs < 2) {
        return x;
    }
    nrnbbs_context_wait();
    MPI_Allreduce(&x, &result, 1, MPI_INT, MPI_MAX, nrnmpi_comm);
    return result;
}

#define ALLTOALLV_SPARSE_TAG 101980

/* Code derived from MPI_Alltoallv_sparse in MP-Gadget: https://github.com/MP-Gadget */

static int MPI_Alltoallv_sparse(void* sendbuf,
                                int* sendcnts,
                                int* sdispls,
                                MPI_Datatype sendtype,
                                void* recvbuf,
                                int* recvcnts,
                                int* rdispls,
                                MPI_Datatype recvtype,
                                MPI_Comm comm) {
    int myrank;
    int nranks;
    nrn_mpi_assert(MPI_Comm_rank(comm, &myrank));
    nrn_mpi_assert(MPI_Comm_size(comm, &nranks));

    int rankp;
    for (rankp = 0; nranks > (1 << rankp); rankp++)
        ;

    ptrdiff_t lb;
    ptrdiff_t send_elsize;
    ptrdiff_t recv_elsize;

    nrn_mpi_assert(MPI_Type_get_extent(sendtype, &lb, &send_elsize));
    nrn_mpi_assert(MPI_Type_get_extent(recvtype, &lb, &recv_elsize));

    MPI_Request* requests = (MPI_Request*) hoc_Emalloc(nranks * 2 * sizeof(MPI_Request));
    hoc_malchk();
    assert(requests != NULL);

    int ngrp;
    int n_requests;
    n_requests = 0;
    for (ngrp = 0; ngrp < (1 << rankp); ngrp++) {
        int target = myrank ^ ngrp;

        if (target >= nranks)
            continue;
        if (recvcnts[target] == 0)
            continue;
        nrn_mpi_assert(MPI_Irecv((static_cast<char*>(recvbuf)) + recv_elsize * rdispls[target],
                                 recvcnts[target],
                                 recvtype,
                                 target,
                                 ALLTOALLV_SPARSE_TAG,
                                 comm,
                                 &requests[n_requests++]));
    }

    nrn_mpi_assert(MPI_Barrier(comm));

    for (ngrp = 0; ngrp < (1 << rankp); ngrp++) {
        int target = myrank ^ ngrp;
        if (target >= nranks)
            continue;
        if (sendcnts[target] == 0)
            continue;
        nrn_mpi_assert(MPI_Isend((static_cast<char*>(sendbuf)) + send_elsize * sdispls[target],
                                 sendcnts[target],
                                 sendtype,
                                 target,
                                 ALLTOALLV_SPARSE_TAG,
                                 comm,
                                 &requests[n_requests++]));
    }

    nrn_mpi_assert(MPI_Waitall(n_requests, requests, MPI_STATUSES_IGNORE));
    free(requests);

    nrn_mpi_assert(MPI_Barrier(comm));

    return MPI_SUCCESS;
}


extern void nrnmpi_dbl_alltoallv_sparse(double* s,
                                        int* scnt,
                                        int* sdispl,
                                        double* r,
                                        int* rcnt,
                                        int* rdispl) {
    MPI_Alltoallv_sparse(s, scnt, sdispl, MPI_DOUBLE, r, rcnt, rdispl, MPI_DOUBLE, nrnmpi_comm);
}
extern void nrnmpi_int_alltoallv_sparse(int* s,
                                        int* scnt,
                                        int* sdispl,
                                        int* r,
                                        int* rcnt,
                                        int* rdispl) {
    MPI_Alltoallv_sparse(s, scnt, sdispl, MPI_INT, r, rcnt, rdispl, MPI_INT, nrnmpi_comm);
}

extern void nrnmpi_long_alltoallv_sparse(int64_t* s,
                                         int* scnt,
                                         int* sdispl,
                                         int64_t* r,
                                         int* rcnt,
                                         int* rdispl) {
    MPI_Alltoallv_sparse(s, scnt, sdispl, MPI_INT64_T, r, rcnt, rdispl, MPI_INT64_T, nrnmpi_comm);
}


extern void nrnmpi_int_gather(int* s, int* r, int cnt, int root) {
    MPI_Gather(s, cnt, MPI_INT, r, cnt, MPI_INT, root, nrnmpi_comm);
}

extern void nrnmpi_int_gatherv(int* s, int scnt, int* r, int* rcnt, int* rdispl, int root) {
    MPI_Gatherv(s, scnt, MPI_INT, r, rcnt, rdispl, MPI_INT, root, nrnmpi_comm);
}

extern void nrnmpi_char_gatherv(char* s, int scnt, char* r, int* rcnt, int* rdispl, int root) {
    MPI_Gatherv(s, scnt, MPI_CHAR, r, rcnt, rdispl, MPI_CHAR, root, nrnmpi_comm);
}

extern void nrnmpi_int_scatter(int* s, int* r, int cnt, int root) {
    MPI_Scatter(s, cnt, MPI_INT, r, cnt, MPI_INT, root, nrnmpi_comm);
}

extern void nrnmpi_char_scatterv(char* s, int* scnt, int* sdispl, char* r, int rcnt, int root) {
    MPI_Scatterv(s, scnt, sdispl, MPI_CHAR, r, rcnt, MPI_CHAR, root, nrnmpi_comm);
}

extern void nrnmpi_int_alltoall(int* s, int* r, int n) {
    MPI_Alltoall(s, n, MPI_INT, r, n, MPI_INT, nrnmpi_comm);
}

extern void nrnmpi_int_alltoallv(int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_INT, r, rcnt, rdispl, MPI_INT, nrnmpi_comm);
}

extern void nrnmpi_long_alltoallv(int64_t* s,
                                  int* scnt,
                                  int* sdispl,
                                  int64_t* r,
                                  int* rcnt,
                                  int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_INT64_T, r, rcnt, rdispl, MPI_INT64_T, nrnmpi_comm);
}

extern void nrnmpi_dbl_alltoallv(double* s,
                                 int* scnt,
                                 int* sdispl,
                                 double* r,
                                 int* rcnt,
                                 int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_DOUBLE, r, rcnt, rdispl, MPI_DOUBLE, nrnmpi_comm);
}

extern void nrnmpi_char_alltoallv(char* s,
                                  int* scnt,
                                  int* sdispl,
                                  char* r,
                                  int* rcnt,
                                  int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_CHAR, r, rcnt, rdispl, MPI_CHAR, nrnmpi_comm);
}

/* following are for the partrans */

void nrnmpi_int_allgather(int* s, int* r, int n) {
    MPI_Allgather(s, n, MPI_INT, r, n, MPI_INT, nrnmpi_comm);
}

void nrnmpi_int_allgather_inplace(int* srcdest, int n) {
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, srcdest, n, MPI_INT, nrnmpi_comm);
}

void nrnmpi_int_allgatherv(int* s, int* r, int* n, int* dspl) {
    MPI_Allgatherv(s, n[nrnmpi_myid], MPI_INT, r, n, dspl, MPI_INT, nrnmpi_comm);
}

void nrnmpi_int_allgatherv_inplace(int* srcdest, int* n, int* dspl) {
    MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, srcdest, n, dspl, MPI_INT, nrnmpi_comm);
}

void nrnmpi_char_allgatherv(char* s, char* r, int* n, int* dspl) {
    MPI_Allgatherv(s, n[nrnmpi_myid], MPI_CHAR, r, n, dspl, MPI_CHAR, nrnmpi_comm);
}

void nrnmpi_long_allgatherv(int64_t* s, int64_t* r, int* n, int* dspl) {
    MPI_Allgatherv(s, n[nrnmpi_myid], MPI_INT64_T, r, n, dspl, MPI_INT64_T, nrnmpi_comm);
}

void nrnmpi_long_allgatherv_inplace(long* srcdest, int* n, int* dspl) {
    MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, srcdest, n, dspl, MPI_LONG, nrnmpi_comm);
}

void nrnmpi_dbl_allgatherv(double* s, double* r, int* n, int* dspl) {
    MPI_Allgatherv(s, n[nrnmpi_myid], MPI_DOUBLE, r, n, dspl, MPI_DOUBLE, nrnmpi_comm);
}

void nrnmpi_dbl_allgatherv_inplace(double* srcdest, int* n, int* dspl) {
    MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, srcdest, n, dspl, MPI_DOUBLE, nrnmpi_comm);
}

void nrnmpi_dbl_broadcast(double* buf, int cnt, int root) {
    MPI_Bcast(buf, cnt, MPI_DOUBLE, root, nrnmpi_comm);
}

void nrnmpi_int_broadcast(int* buf, int cnt, int root) {
    MPI_Bcast(buf, cnt, MPI_INT, root, nrnmpi_comm);
}

void nrnmpi_char_broadcast(char* buf, int cnt, int root) {
    MPI_Bcast(buf, cnt, MPI_CHAR, root, nrnmpi_comm);
}

void nrnmpi_str_broadcast_world(std::string& str, int root) {
    assert(str.size() <= std::numeric_limits<int>::max());
    // broadcast the size from `root` to everyone
    int sz = str.size();
    MPI_Bcast(&sz, 1, MPI_INT, root, nrnmpi_world_comm);
    // resize to the size we received from root
    str.resize(sz);
    if (sz) {
        MPI_Bcast(str.data(), sz, MPI_CHAR, root, nrnmpi_world_comm);
    }
}

int nrnmpi_int_sum_reduce(int in) {
    int result;
    MPI_Allreduce(&in, &result, 1, MPI_INT, MPI_SUM, nrnmpi_comm);
    return result;
}

void nrnmpi_assert_opstep(int opstep, double t) {
    /* all machines in comm should have same opstep and same t. */
    double buf[2];
    if (nrnmpi_numprocs < 2) {
        return;
    }
    buf[0] = (double) opstep;
    buf[1] = t;
    MPI_Bcast(buf, 2, MPI_DOUBLE, 0, nrnmpi_comm);
    if (opstep != (int) buf[0] || t != buf[1]) {
        printf(
            "%d opstep=%d %d  t=%g t-troot=%g\n", nrnmpi_myid, opstep, (int) buf[0], t, t - buf[1]);
        hoc_execerror("nrnmpi_assert_opstep failed", (char*) 0);
    }
}

double nrnmpi_dbl_allmin(double x) {
    double result;
    if (nrnmpi_numprocs < 2) {
        return x;
    }
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, MPI_MIN, nrnmpi_comm);
    return result;
}

static void pgvts_op(double* in, double* inout, int* len, MPI_Datatype* dptr) {
    int i, r = 0;
    assert(*dptr == MPI_DOUBLE);
    assert(*len == 4);
    if (in[0] < inout[0]) {
        /* least time has highest priority */
        r = 1;
    } else if (in[0] == inout[0]) {
        /* when times are equal then */
        if (in[1] < inout[1]) {
            /* NetParEvent done last */
            r = 1;
        } else if (in[1] == inout[1]) {
            /* when times and ops are equal then */
            if (in[2] < inout[2]) {
                /* init done next to last.*/
                r = 1;
            } else if (in[2] == inout[2]) {
                /* when times, ops, and inits are equal then */
                if (in[3] < inout[3]) {
                    /* choose lowest rank */
                    r = 1;
                }
            }
        }
    }
    if (r) {
        for (i = 0; i < 4; ++i) {
            inout[i] = in[i];
        }
    }
}

int nrnmpi_pgvts_least(double* t, int* op, int* init) {
    int i;
    double ibuf[4], obuf[4];
    ibuf[0] = *t;
    ibuf[1] = (double) (*op);
    ibuf[2] = (double) (*init);
    ibuf[3] = (double) nrnmpi_myid;
    for (i = 0; i < 4; ++i) {
        obuf[i] = ibuf[i];
    }
    MPI_Allreduce(ibuf, obuf, 4, MPI_DOUBLE, mpi_pgvts_op, nrnmpi_comm);
    assert(obuf[0] <= *t);
    if (obuf[0] == *t) {
        assert((int) obuf[1] <= *op);
        if ((int) obuf[1] == *op) {
            assert((int) obuf[2] <= *init);
            if ((int) obuf[2] == *init) {
                assert((int) obuf[3] <= nrnmpi_myid);
            }
        }
    }
    *t = obuf[0];
    *op = (int) obuf[1];
    *init = (int) obuf[2];
    if (nrnmpi_myid == (int) obuf[3]) {
        return 1;
    }
    return 0;
}

/* following for splitcell.cpp transfer */
void nrnmpi_send_doubles(double* pd, int cnt, int dest, int tag) {
    MPI_Send(pd, cnt, MPI_DOUBLE, dest, tag, nrnmpi_comm);
}

void nrnmpi_recv_doubles(double* pd, int cnt, int src, int tag) {
    MPI_Status status;
    MPI_Recv(pd, cnt, MPI_DOUBLE, src, tag, nrnmpi_comm, &status);
}

void nrnmpi_postrecv_doubles(double* pd, int cnt, int src, int tag, void** request) {
    MPI_Irecv(pd, cnt, MPI_DOUBLE, src, tag, nrnmpi_comm, (MPI_Request*) request);
}

void nrnmpi_wait(void** request) {
    MPI_Status status;
    MPI_Wait((MPI_Request*) request, &status);
}

void nrnmpi_barrier() {
    if (nrnmpi_numprocs < 2) {
        return;
    }
    MPI_Barrier(nrnmpi_comm);
}

static MPI_Op type2OP(int type) {
    if (type == 1) {
        return MPI_SUM;
    } else if (type == 2) {
        return MPI_MAX;
    } else {
        return MPI_MIN;
    }
}

double nrnmpi_dbl_allreduce(double x, int type) {
    if (nrnmpi_numprocs < 2) {
        return x;
    }
    double result;
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, type2OP(type), nrnmpi_comm);
    return result;
}

extern "C" void nrnmpi_dbl_allreduce_vec(double* src, double* dest, int cnt, int type) {
    assert(src != dest);
    if (nrnmpi_numprocs < 2) {
        for (int i = 0; i < cnt; ++i) {
            dest[i] = src[i];
        }
        return;
    }
    MPI_Allreduce(src, dest, cnt, MPI_DOUBLE, type2OP(type), nrnmpi_comm);
    return;
}

void nrnmpi_longdbl_allreduce_vec(longdbl* src, longdbl* dest, int cnt, int type) {
    assert(src != dest);
    if (nrnmpi_numprocs < 2) {
        for (int i = 0; i < cnt; ++i) {
            dest[i] = src[i];
        }
        return;
    }
    MPI_Allreduce(src, dest, cnt, MPI_LONG_DOUBLE, type2OP(type), nrnmpi_comm);
    return;
}

void nrnmpi_long_allreduce_vec(long* src, long* dest, int cnt, int type) {
    assert(src != dest);
    if (nrnmpi_numprocs < 2) {
        for (int i = 0; i < cnt; ++i) {
            dest[i] = src[i];
        }
        return;
    }
    MPI_Allreduce(src, dest, cnt, MPI_LONG, type2OP(type), nrnmpi_comm);
    return;
}

void nrnmpi_dbl_allgather(double* s, double* r, int n) {
    MPI_Allgather(s, n, MPI_DOUBLE, r, n, MPI_DOUBLE, nrnmpi_comm);
}

static MPI_Comm bgp_comm;

void nrnmpi_multisend_comm() {
    if (!bgp_comm) {
        MPI_Comm_dup(nrnmpi_comm, &bgp_comm);
    }
}

void nrnmpi_multisend_multisend(NRNMPI_Spike* spk, int n, int* hosts) {
    int i;
    MPI_Request r;
    MPI_Status status;
    for (i = 0; i < n; ++i) {
        MPI_Isend(spk, 1, spike_type, hosts[i], 1, bgp_comm, &r);
        MPI_Request_free(&r);
    }
}

int nrnmpi_multisend_single_advance(NRNMPI_Spike* spk) {
    int flag = 0;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, 1, bgp_comm, &flag, &status);
    if (flag) {
        MPI_Recv(spk, 1, spike_type, MPI_ANY_SOURCE, 1, bgp_comm, &status);
    }
    return flag;
}

static int iii;
int nrnmpi_multisend_conserve(int nsend, int nrecv) {
    int tcnts[2];
    tcnts[0] = nsend - nrecv;
    MPI_Allreduce(tcnts, tcnts + 1, 1, MPI_INT, MPI_SUM, bgp_comm);
    return tcnts[1];
}

#endif /*NRNMPI*/
