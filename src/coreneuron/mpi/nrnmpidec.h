/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

/*
This file is processed by mkdynam.sh and so it is important that
the prototypes be of the form "type foo(type arg, ...)"
*/

#pragma once

#include <stdlib.h>

namespace coreneuron {
/* from nrnmpi.cpp */
struct nrnmpi_init_ret_t {
    int numprocs;
    int myid;
};
extern "C" nrnmpi_init_ret_t nrnmpi_init_impl(int* pargc, char*** pargv, bool is_quiet);
declare_mpi_method(nrnmpi_init);
extern "C" void nrnmpi_finalize_impl(void);
declare_mpi_method(nrnmpi_finalize);
extern "C" void nrnmpi_check_threading_support_impl();
declare_mpi_method(nrnmpi_check_threading_support);
// Write given buffer to a new file using MPI collective I/O
extern "C" void nrnmpi_write_file_impl(const std::string& filename,
                                       const char* buffer,
                                       size_t length);
declare_mpi_method(nrnmpi_write_file);


/* from mpispike.cpp */
extern "C" int nrnmpi_spike_exchange_impl(int* nin,
                                          NRNMPI_Spike* spikeout,
                                          int icapacity,
                                          NRNMPI_Spike** spikein,
                                          int& ovfl,
                                          int nout,
                                          NRNMPI_Spikebuf* spbufout,
                                          NRNMPI_Spikebuf* spbufin);
declare_mpi_method(nrnmpi_spike_exchange);
extern "C" int nrnmpi_spike_exchange_compressed_impl(int,
                                                     unsigned char*&,
                                                     int,
                                                     int*,
                                                     int,
                                                     unsigned char*,
                                                     int,
                                                     unsigned char*,
                                                     int& ovfl);
declare_mpi_method(nrnmpi_spike_exchange_compressed);
extern "C" int nrnmpi_int_allmax_impl(int i);
declare_mpi_method(nrnmpi_int_allmax);
extern "C" void nrnmpi_int_allgather_impl(int* s, int* r, int n);
declare_mpi_method(nrnmpi_int_allgather);
extern "C" void nrnmpi_int_alltoall_impl(int* s, int* r, int n);
declare_mpi_method(nrnmpi_int_alltoall);
extern "C" void nrnmpi_int_alltoallv_impl(const int* s,
                                          const int* scnt,
                                          const int* sdispl,
                                          int* r,
                                          int* rcnt,
                                          int* rdispl);
declare_mpi_method(nrnmpi_int_alltoallv);
extern "C" void nrnmpi_dbl_alltoallv_impl(double* s,
                                          int* scnt,
                                          int* sdispl,
                                          double* r,
                                          int* rcnt,
                                          int* rdispl);
declare_mpi_method(nrnmpi_dbl_alltoallv);
extern "C" double nrnmpi_dbl_allmin_impl(double x);
declare_mpi_method(nrnmpi_dbl_allmin);
extern "C" double nrnmpi_dbl_allmax_impl(double x);
declare_mpi_method(nrnmpi_dbl_allmax);
extern "C" void nrnmpi_barrier_impl(void);
declare_mpi_method(nrnmpi_barrier);
extern "C" double nrnmpi_dbl_allreduce_impl(double x, int type);
declare_mpi_method(nrnmpi_dbl_allreduce);
extern "C" void nrnmpi_dbl_allreduce_vec_impl(double* src, double* dest, int cnt, int type);
declare_mpi_method(nrnmpi_dbl_allreduce_vec);
extern "C" void nrnmpi_long_allreduce_vec_impl(long* src, long* dest, int cnt, int type);
declare_mpi_method(nrnmpi_long_allreduce_vec);
extern "C" bool nrnmpi_initialized_impl();
declare_mpi_method(nrnmpi_initialized);
extern "C" void nrnmpi_abort_impl(int);
declare_mpi_method(nrnmpi_abort);
extern "C" double nrnmpi_wtime_impl();
declare_mpi_method(nrnmpi_wtime);
extern "C" int nrnmpi_local_rank_impl();
declare_mpi_method(nrnmpi_local_rank);
extern "C" int nrnmpi_local_size_impl();
declare_mpi_method(nrnmpi_local_size);
#if NRN_MULTISEND
extern "C" void nrnmpi_multisend_comm_impl();
declare_mpi_method(nrnmpi_multisend_comm);
extern "C" void nrnmpi_multisend_impl(NRNMPI_Spike* spk, int n, int* hosts);
declare_mpi_method(nrnmpi_multisend);
extern "C" int nrnmpi_multisend_single_advance_impl(NRNMPI_Spike* spk);
declare_mpi_method(nrnmpi_multisend_single_advance);
extern "C" int nrnmpi_multisend_conserve_impl(int nsend, int nrecv);
declare_mpi_method(nrnmpi_multisend_conserve);
#endif

}  // namespace coreneuron
