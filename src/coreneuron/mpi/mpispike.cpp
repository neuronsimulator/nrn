/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include "coreneuron/nrnconf.h"
/* do not want the redef in the dynamic load case */
#include "coreneuron/mpi/nrnmpiuse.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/nrnmpidec.h"
#include "coreneuron/mpi/nrnmpi_impl.h"
#include "coreneuron/mpi/mpispike.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/utils/nrnoc_aux.hpp"

#if NRNMPI
#include <mpi.h>

#include <cstring>

namespace coreneuron {
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

static void make_spikebuf_type() {
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

void wait_before_spike_exchange() {
    MPI_Barrier(nrnmpi_comm);
}

int nrnmpi_spike_exchange() {
    int n;
    Instrumentor::phase_begin("spike-exchange");

    {
        Instrumentor::phase p("imbalance");
        wait_before_spike_exchange();
    }

    Instrumentor::phase_begin("communication");
    if (!displs) {
        np = nrnmpi_numprocs;
        displs = (int*) emalloc(np * sizeof(int));
        displs[0] = 0;
#if nrn_spikebuf_size > 0
        make_spikebuf_type();
#endif
    }
#if nrn_spikebuf_size == 0
    MPI_Allgather(&nout_, 1, MPI_INT, nin_, 1, MPI_INT, nrnmpi_comm);
    n = nin_[0];
    for (int i = 1; i < np; ++i) {
        displs[i] = n;
        n += nin_[i];
    }
    if (n) {
        if (icapacity_ < n) {
            icapacity_ = n + 10;
            free(spikein_);
            spikein_ = (NRNMPI_Spike*) emalloc(icapacity_ * sizeof(NRNMPI_Spike));
        }
        MPI_Allgatherv(
            spikeout_, nout_, spike_type, spikein_, nin_, displs, spike_type, nrnmpi_comm);
    }
#else
    MPI_Allgather(spbufout_, 1, spikebuf_type, spbufin_, 1, spikebuf_type, nrnmpi_comm);
    int novfl = 0;
    n = spbufin_[0].nspike;
    if (n > nrn_spikebuf_size) {
        nin_[0] = n - nrn_spikebuf_size;
        novfl += nin_[0];
    } else {
        nin_[0] = 0;
    }
    for (int i = 1; i < np; ++i) {
        displs[i] = novfl;
        int n1 = spbufin_[i].nspike;
        n += n1;
        if (n1 > nrn_spikebuf_size) {
            nin_[i] = n1 - nrn_spikebuf_size;
            novfl += nin_[i];
        } else {
            nin_[i] = 0;
        }
    }
    if (novfl) {
        if (icapacity_ < novfl) {
            icapacity_ = novfl + 10;
            free(spikein_);
            spikein_ = (NRNMPI_Spike*) hoc_Emalloc(icapacity_ * sizeof(NRNMPI_Spike));
            hoc_malchk();
        }
        int n1 = (nout_ > nrn_spikebuf_size) ? nout_ - nrn_spikebuf_size : 0;
        MPI_Allgatherv(spikeout_, n1, spike_type, spikein_, nin_, displs, spike_type, nrnmpi_comm);
    }
    ovfl_ = novfl;
#endif
    Instrumentor::phase_end("communication");
    Instrumentor::phase_end("spike-exchange");
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
int nrnmpi_spike_exchange_compressed() {
    if (!displs) {
        np = nrnmpi_numprocs;
        displs = (int*) emalloc(np * sizeof(int));
        displs[0] = 0;
        byteovfl = (int*) emalloc(np * sizeof(int));
    }

    MPI_Allgather(
        spfixout_, ag_send_size_, MPI_BYTE, spfixin_, ag_send_size_, MPI_BYTE, nrnmpi_comm);
    int novfl = 0;
    int ntot = 0;
    int bstot = 0;
    for (int i = 0; i < np; ++i) {
        displs[i] = bstot;
        int idx = i * ag_send_size_;
        int n = spfixin_[idx++] * 256;
        n += spfixin_[idx++];
        ntot += n;
        nin_[i] = n;
        if (n > ag_send_nspike_) {
            int bs = 2 + n * (1 + localgid_size_) - ag_send_size_;
            byteovfl[i] = bs;
            bstot += bs;
            novfl += n - ag_send_nspike_;
        } else {
            byteovfl[i] = 0;
        }
    }
    if (novfl) {
        if (ovfl_capacity_ < novfl) {
            ovfl_capacity_ = novfl + 10;
            free(spfixin_ovfl_);
            spfixin_ovfl_ = (unsigned char*) emalloc(ovfl_capacity_ * (1 + localgid_size_) *
                                                     sizeof(unsigned char));
        }
        int bs = byteovfl[nrnmpi_myid];
        /*
        note that the spfixout_ buffer is one since the overflow
        is contiguous to the first part. But the spfixin_ovfl_ is
        completely separate from the spfixin_ since the latter
        dynamically changes its size during a run.
        */
        MPI_Allgatherv(spfixout_ + ag_send_size_,
                       bs,
                       MPI_BYTE,
                       spfixin_ovfl_,
                       byteovfl,
                       displs,
                       MPI_BYTE,
                       nrnmpi_comm);
    }
    ovfl_ = novfl;
    return ntot;
}

int nrnmpi_int_allmax(int x) {
    int result;
    if (nrnmpi_numprocs < 2) {
        return x;
    }
    MPI_Allreduce(&x, &result, 1, MPI_INT, MPI_MAX, nrnmpi_comm);
    return result;
}

extern void nrnmpi_int_gather(int* s, int* r, int cnt, int root) {
    MPI_Gather(s, cnt, MPI_INT, r, cnt, MPI_INT, root, nrnmpi_comm);
}

extern void nrnmpi_int_gatherv(int* s, int scnt, int* r, int* rcnt, int* rdispl, int root) {
    MPI_Gatherv(s, scnt, MPI_INT, r, rcnt, rdispl, MPI_INT, root, nrnmpi_comm);
}

extern void nrnmpi_int_alltoall(int* s, int* r, int n) {
    MPI_Alltoall(s, n, MPI_INT, r, n, MPI_INT, nrnmpi_comm);
}

extern void nrnmpi_int_alltoallv(const int* s,
                                 const int* scnt,
                                 const int* sdispl,
                                 int* r,
                                 int* rcnt,
                                 int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_INT, r, rcnt, rdispl, MPI_INT, nrnmpi_comm);
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

void nrnmpi_int_allgatherv(int* s, int* r, int* n, int* dspl) {
    MPI_Allgatherv(s, n[nrnmpi_myid], MPI_INT, r, n, dspl, MPI_INT, nrnmpi_comm);
}

void nrnmpi_dbl_allgatherv(double* s, double* r, int* n, int* dspl) {
    MPI_Allgatherv(s, n[nrnmpi_myid], MPI_DOUBLE, r, n, dspl, MPI_DOUBLE, nrnmpi_comm);
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

int nrnmpi_int_sum_reduce(int in) {
    int result;
    MPI_Allreduce(&in, &result, 1, MPI_INT, MPI_SUM, nrnmpi_comm);
    return result;
}

void nrnmpi_assert_opstep(int opstep, double tt) {
    /* all machines in comm should have same opstep and same tt. */
    double buf[2];
    if (nrnmpi_numprocs < 2) {
        return;
    }
    buf[0] = (double) opstep;
    buf[1] = tt;
    MPI_Bcast(buf, 2, MPI_DOUBLE, 0, nrnmpi_comm);
    if (opstep != (int) buf[0] || tt != buf[1]) {
        printf("%d opstep=%d %d  t=%g t-troot=%g\n",
               nrnmpi_myid,
               opstep,
               (int) buf[0],
               tt,
               tt - buf[1]);
        hoc_execerror("nrnmpi_assert_opstep failed", (char*) 0);
    }
}

double nrnmpi_dbl_allmin(double x) {
    double result;
    if (!nrnmpi_use || (nrnmpi_numprocs < 2)) {
        return x;
    }
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, MPI_MIN, nrnmpi_comm);
    return result;
}

double nrnmpi_dbl_allmax(double x) {
    double result;
    if (!nrnmpi_use || (nrnmpi_numprocs < 2)) {
        return x;
    }
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, MPI_MAX, nrnmpi_comm);
    return result;
}

static void pgvts_op(double* in, double* inout, int* len, MPI_Datatype* dptr) {
    int r = 0;
    if (*dptr != MPI_DOUBLE)
        printf("ERROR in mpispike.c! *dptr should be MPI_DOUBLE.");
    if (*len != 4)
        printf("ERROR in mpispike.c! *len should be 4.");
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
        for (int i = 0; i < 4; ++i) {
            inout[i] = in[i];
        }
    }
}

int nrnmpi_pgvts_least(double* tt, int* op, int* init) {
    double ibuf[4], obuf[4];
    ibuf[0] = *tt;
    ibuf[1] = (double) (*op);
    ibuf[2] = (double) (*init);
    ibuf[3] = (double) nrnmpi_myid;
    std::memcpy(obuf, ibuf, 4 * sizeof(double));

    MPI_Allreduce(ibuf, obuf, 4, MPI_DOUBLE, mpi_pgvts_op, nrnmpi_comm);
    assert(obuf[0] <= *tt);
    if (obuf[0] == *tt) {
        assert((int) obuf[1] <= *op);
        if ((int) obuf[1] == *op) {
            assert((int) obuf[2] <= *init);
            if ((int) obuf[2] == *init) {
                assert((int) obuf[3] <= nrnmpi_myid);
            }
        }
    }
    *tt = obuf[0];
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
    if (nrnmpi_numprocs > 1) {
        MPI_Barrier(nrnmpi_comm);
    }
}

double nrnmpi_dbl_allreduce(double x, int type) {
    double result;
    MPI_Op tt;
    if (nrnmpi_numprocs < 2) {
        return x;
    }
    if (type == 1) {
        tt = MPI_SUM;
    } else if (type == 2) {
        tt = MPI_MAX;
    } else {
        tt = MPI_MIN;
    }
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, tt, nrnmpi_comm);
    return result;
}

long nrnmpi_long_allreduce(long x, int type) {
    long result;
    MPI_Op tt;
    if (nrnmpi_numprocs < 2) {
        return x;
    }
    if (type == 1) {
        tt = MPI_SUM;
    } else if (type == 2) {
        tt = MPI_MAX;
    } else {
        tt = MPI_MIN;
    }
    MPI_Allreduce(&x, &result, 1, MPI_LONG, tt, nrnmpi_comm);
    return result;
}

void nrnmpi_dbl_allreduce_vec(double* src, double* dest, int cnt, int type) {
    MPI_Op tt;
    assert(src != dest);
    if (nrnmpi_numprocs < 2) {
        std::memcpy(dest, src, cnt * sizeof(double));
        return;
    }
    if (type == 1) {
        tt = MPI_SUM;
    } else if (type == 2) {
        tt = MPI_MAX;
    } else {
        tt = MPI_MIN;
    }
    MPI_Allreduce(src, dest, cnt, MPI_DOUBLE, tt, nrnmpi_comm);
    return;
}

void nrnmpi_long_allreduce_vec(long* src, long* dest, int cnt, int type) {
    MPI_Op tt;
    assert(src != dest);
    if (nrnmpi_numprocs < 2) {
        std::memcpy(dest, src, cnt * sizeof(long));
        return;
    }
    if (type == 1) {
        tt = MPI_SUM;
    } else if (type == 2) {
        tt = MPI_MAX;
    } else {
        tt = MPI_MIN;
    }
    MPI_Allreduce(src, dest, cnt, MPI_LONG, tt, nrnmpi_comm);
    return;
}

void nrnmpi_dbl_allgather(double* s, double* r, int n) {
    MPI_Allgather(s, n, MPI_DOUBLE, r, n, MPI_DOUBLE, nrnmpi_comm);
}

#if NRN_MULTISEND

static MPI_Comm multisend_comm;

void nrnmpi_multisend_comm() {
    if (!multisend_comm) {
        MPI_Comm_dup(MPI_COMM_WORLD, &multisend_comm);
    }
}

void nrnmpi_multisend(NRNMPI_Spike* spk, int n, int* hosts) {
    MPI_Request r;
    for (int i = 0; i < n; ++i) {
        MPI_Isend(spk, 1, spike_type, hosts[i], 1, multisend_comm, &r);
        MPI_Request_free(&r);
    }
}

int nrnmpi_multisend_single_advance(NRNMPI_Spike* spk) {
    int flag = 0;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, 1, multisend_comm, &flag, &status);
    if (flag) {
        MPI_Recv(spk, 1, spike_type, MPI_ANY_SOURCE, 1, multisend_comm, &status);
    }
    return flag;
}

int nrnmpi_multisend_conserve(int nsend, int nrecv) {
    int tcnts[2];
    tcnts[0] = nsend - nrecv;
    MPI_Allreduce(tcnts, tcnts + 1, 1, MPI_INT, MPI_SUM, multisend_comm);
    return tcnts[1];
}

#endif /*NRN_MULTISEND*/
}  // namespace coreneuron
#endif /*NRNMPI*/
