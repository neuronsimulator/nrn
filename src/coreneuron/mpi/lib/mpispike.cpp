/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include "coreneuron/nrnconf.h"
/* do not want the redef in the dynamic load case */
#include "coreneuron/mpi/nrnmpiuse.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/nrnmpidec.h"
#include "nrnmpi.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/utils/nrn_assert.h"

#include <mpi.h>

#include <cstring>

namespace coreneuron {
extern MPI_Comm nrnmpi_comm;

static int np;
static int* displs{nullptr};
static int* byteovfl{nullptr}; /* for the compressed transfer method */
static MPI_Datatype spike_type;

static void* emalloc(size_t size) {
    void* memptr = malloc(size);
    assert(memptr);
    return memptr;
}

// Register type NRNMPI_Spike
void nrnmpi_spike_initialize() {
    NRNMPI_Spike s;
    int block_lengths[2] = {1, 1};
    MPI_Aint addresses[3];

    MPI_Get_address(&s, &addresses[0]);
    MPI_Get_address(&(s.gid), &addresses[1]);
    MPI_Get_address(&(s.spiketime), &addresses[2]);

    MPI_Aint displacements[2] = {addresses[1] - addresses[0], addresses[2] - addresses[0]};

    MPI_Datatype typelist[2] = {MPI_INT, MPI_DOUBLE};
    MPI_Type_create_struct(2, block_lengths, displacements, typelist, &spike_type);
    MPI_Type_commit(&spike_type);
}

#if nrn_spikebuf_size > 0

static MPI_Datatype spikebuf_type;

// Register type NRNMPI_Spikebuf
static void make_spikebuf_type() {
    NRNMPI_Spikebuf s;
    int block_lengths[3] = {1, nrn_spikebuf_size, nrn_spikebuf_size};
    MPI_Datatype typelist[3] = {MPI_INT, MPI_INT, MPI_DOUBLE};

    MPI_Aint addresses[4];
    MPI_Get_address(&s, &addresses[0]);
    MPI_Get_address(&(s.nspike), &addresses[1]);
    MPI_Get_address(&(s.gid[0]), &addresses[2]);
    MPI_Get_address(&(s.spiketime[0]), &addresses[3]);

    MPI_Aint displacements[3] = {addresses[1] - addresses[0],
                                 addresses[2] - addresses[0],
                                 addresses[3] - addresses[0]};

    MPI_Type_create_struct(3, block_lengths, displacements, typelist, &spikebuf_type);
    MPI_Type_commit(&spikebuf_type);
}
#endif

void wait_before_spike_exchange() {
    MPI_Barrier(nrnmpi_comm);
}

int nrnmpi_spike_exchange_impl(int* nin,
                               NRNMPI_Spike* spikeout,
                               int icapacity,
                               NRNMPI_Spike** spikein,
                               int& ovfl,
                               int nout,
                               NRNMPI_Spikebuf* spbufout,
                               NRNMPI_Spikebuf* spbufin) {
    nrn_assert(spikein);
    Instrumentor::phase_begin("spike-exchange");

    {
        Instrumentor::phase p("imbalance");
        wait_before_spike_exchange();
    }

    Instrumentor::phase_begin("communication");
    if (!displs) {
        np = nrnmpi_numprocs_;
        displs = (int*) emalloc(np * sizeof(int));
        displs[0] = 0;
#if nrn_spikebuf_size > 0
        make_spikebuf_type();
#endif
    }
#if nrn_spikebuf_size == 0
    MPI_Allgather(&nout, 1, MPI_INT, nin, 1, MPI_INT, nrnmpi_comm);
    int n = nin[0];
    for (int i = 1; i < np; ++i) {
        displs[i] = n;
        n += nin[i];
    }
    if (n) {
        if (icapacity < n) {
            icapacity = n + 10;
            free(*spikein);
            *spikein = (NRNMPI_Spike*) emalloc(icapacity * sizeof(NRNMPI_Spike));
        }
        MPI_Allgatherv(spikeout, nout, spike_type, *spikein, nin, displs, spike_type, nrnmpi_comm);
    }
#else
    MPI_Allgather(spbufout, 1, spikebuf_type, spbufin, 1, spikebuf_type, nrnmpi_comm);
    int novfl = 0;
    int n = spbufin[0].nspike;
    if (n > nrn_spikebuf_size) {
        nin[0] = n - nrn_spikebuf_size;
        novfl += nin[0];
    } else {
        nin[0] = 0;
    }
    for (int i = 1; i < np; ++i) {
        displs[i] = novfl;
        int n1 = spbufin[i].nspike;
        n += n1;
        if (n1 > nrn_spikebuf_size) {
            nin[i] = n1 - nrn_spikebuf_size;
            novfl += nin[i];
        } else {
            nin[i] = 0;
        }
    }
    if (novfl) {
        if (icapacity < novfl) {
            icapacity = novfl + 10;
            free(*spikein);
            *spikein = (NRNMPI_Spike*) emalloc(icapacity * sizeof(NRNMPI_Spike));
        }
        int n1 = (nout > nrn_spikebuf_size) ? nout - nrn_spikebuf_size : 0;
        MPI_Allgatherv(spikeout, n1, spike_type, *spikein, nin, displs, spike_type, nrnmpi_comm);
    }
    ovfl = novfl;
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
int nrnmpi_spike_exchange_compressed_impl(int localgid_size,
                                          unsigned char*& spfixin_ovfl,
                                          int send_nspike,
                                          int* nin,
                                          int ovfl_capacity,
                                          unsigned char* spikeout_fixed,
                                          int ag_send_size,
                                          unsigned char* spikein_fixed,
                                          int& ovfl) {
    if (!displs) {
        np = nrnmpi_numprocs_;
        displs = (int*) emalloc(np * sizeof(int));
        displs[0] = 0;
    }
    if (!byteovfl) {
        byteovfl = (int*) emalloc(np * sizeof(int));
    }
    MPI_Allgather(
        spikeout_fixed, ag_send_size, MPI_BYTE, spikein_fixed, ag_send_size, MPI_BYTE, nrnmpi_comm);
    int novfl = 0;
    int ntot = 0;
    int bstot = 0;
    for (int i = 0; i < np; ++i) {
        displs[i] = bstot;
        int idx = i * ag_send_size;
        int n = spikein_fixed[idx++] * 256;
        n += spikein_fixed[idx++];
        ntot += n;
        nin[i] = n;
        if (n > send_nspike) {
            int bs = 2 + n * (1 + localgid_size) - ag_send_size;
            byteovfl[i] = bs;
            bstot += bs;
            novfl += n - send_nspike;
        } else {
            byteovfl[i] = 0;
        }
    }
    if (novfl) {
        if (ovfl_capacity < novfl) {
            ovfl_capacity = novfl + 10;
            free(spfixin_ovfl);
            spfixin_ovfl = (unsigned char*) emalloc(ovfl_capacity * (1 + localgid_size) *
                                                    sizeof(unsigned char));
        }
        int bs = byteovfl[nrnmpi_myid_];
        /*
        note that the spikeout_fixed buffer is one since the overflow
        is contiguous to the first part. But the spfixin_ovfl is
        completely separate from the spikein_fixed since the latter
        dynamically changes its size during a run.
        */
        MPI_Allgatherv(spikeout_fixed + ag_send_size,
                       bs,
                       MPI_BYTE,
                       spfixin_ovfl,
                       byteovfl,
                       displs,
                       MPI_BYTE,
                       nrnmpi_comm);
    }
    ovfl = novfl;
    return ntot;
}

int nrnmpi_int_allmax_impl(int x) {
    int result;
    MPI_Allreduce(&x, &result, 1, MPI_INT, MPI_MAX, nrnmpi_comm);
    return result;
}

extern void nrnmpi_int_alltoall_impl(int* s, int* r, int n) {
    MPI_Alltoall(s, n, MPI_INT, r, n, MPI_INT, nrnmpi_comm);
}

extern void nrnmpi_int_alltoallv_impl(const int* s,
                                      const int* scnt,
                                      const int* sdispl,
                                      int* r,
                                      int* rcnt,
                                      int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_INT, r, rcnt, rdispl, MPI_INT, nrnmpi_comm);
}

extern void nrnmpi_dbl_alltoallv_impl(double* s,
                                      int* scnt,
                                      int* sdispl,
                                      double* r,
                                      int* rcnt,
                                      int* rdispl) {
    MPI_Alltoallv(s, scnt, sdispl, MPI_DOUBLE, r, rcnt, rdispl, MPI_DOUBLE, nrnmpi_comm);
}

/* following are for the partrans */

void nrnmpi_int_allgather_impl(int* s, int* r, int n) {
    MPI_Allgather(s, n, MPI_INT, r, n, MPI_INT, nrnmpi_comm);
}

double nrnmpi_dbl_allmin_impl(double x) {
    double result;
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, MPI_MIN, nrnmpi_comm);
    return result;
}

double nrnmpi_dbl_allmax_impl(double x) {
    double result;
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, MPI_MAX, nrnmpi_comm);
    return result;
}

void nrnmpi_barrier_impl() {
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

double nrnmpi_dbl_allreduce_impl(double x, int type) {
    double result;
    MPI_Allreduce(&x, &result, 1, MPI_DOUBLE, type2OP(type), nrnmpi_comm);
    return result;
}

void nrnmpi_dbl_allreduce_vec_impl(double* src, double* dest, int cnt, int type) {
    assert(src != dest);
    MPI_Allreduce(src, dest, cnt, MPI_DOUBLE, type2OP(type), nrnmpi_comm);
}

void nrnmpi_long_allreduce_vec_impl(long* src, long* dest, int cnt, int type) {
    assert(src != dest);
    MPI_Allreduce(src, dest, cnt, MPI_LONG, type2OP(type), nrnmpi_comm);
}

#if NRN_MULTISEND

static MPI_Comm multisend_comm;

void nrnmpi_multisend_comm_impl() {
    if (!multisend_comm) {
        MPI_Comm_dup(MPI_COMM_WORLD, &multisend_comm);
    }
}

void nrnmpi_multisend_impl(NRNMPI_Spike* spk, int n, int* hosts) {
    MPI_Request r;
    for (int i = 0; i < n; ++i) {
        MPI_Isend(spk, 1, spike_type, hosts[i], 1, multisend_comm, &r);
        MPI_Request_free(&r);
    }
}

int nrnmpi_multisend_single_advance_impl(NRNMPI_Spike* spk) {
    int flag = 0;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, 1, multisend_comm, &flag, &status);
    if (flag) {
        MPI_Recv(spk, 1, spike_type, MPI_ANY_SOURCE, 1, multisend_comm, &status);
    }
    return flag;
}

int nrnmpi_multisend_conserve_impl(int nsend, int nrecv) {
    int tcnts[2];
    tcnts[0] = nsend - nrecv;
    MPI_Allreduce(tcnts, tcnts + 1, 1, MPI_INT, MPI_SUM, multisend_comm);
    return tcnts[1];
}

#endif /*NRN_MULTISEND*/
}  // namespace coreneuron
