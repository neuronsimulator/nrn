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
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_init_impl)> nrnmpi_init;
extern "C" void nrnmpi_finalize_impl(void);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_finalize_impl)> nrnmpi_finalize;
extern "C" void nrnmpi_check_threading_support_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_check_threading_support_impl)>
    nrnmpi_check_threading_support;
// Write given buffer to a new file using MPI collective I/O
extern "C" void nrnmpi_write_file_impl(const std::string& filename,
                                       const char* buffer,
                                       size_t length);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_write_file_impl)> nrnmpi_write_file;


/* from mpispike.cpp */
extern "C" int nrnmpi_spike_exchange_impl(int* nin,
                                          NRNMPI_Spike* spikeout,
                                          int icapacity,
                                          NRNMPI_Spike** spikein,
                                          int& ovfl,
                                          int nout,
                                          NRNMPI_Spikebuf* spbufout,
                                          NRNMPI_Spikebuf* spbufin);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_spike_exchange_impl)>
    nrnmpi_spike_exchange;
extern "C" int nrnmpi_spike_exchange_compressed_impl(int,
                                                     unsigned char*&,
                                                     int,
                                                     int*,
                                                     int,
                                                     unsigned char*,
                                                     int,
                                                     unsigned char*,
                                                     int& ovfl);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_spike_exchange_compressed_impl)>
    nrnmpi_spike_exchange_compressed;
extern "C" int nrnmpi_int_allmax_impl(int i);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_allmax_impl)> nrnmpi_int_allmax;
extern "C" void nrnmpi_int_allgather_impl(int* s, int* r, int n);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_allgather_impl)> nrnmpi_int_allgather;
extern "C" void nrnmpi_int_alltoall_impl(int* s, int* r, int n);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_alltoall_impl)> nrnmpi_int_alltoall;
extern "C" void nrnmpi_int_alltoallv_impl(const int* s,
                                          const int* scnt,
                                          const int* sdispl,
                                          int* r,
                                          int* rcnt,
                                          int* rdispl);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_int_alltoallv_impl)> nrnmpi_int_alltoallv;
extern "C" void nrnmpi_dbl_alltoallv_impl(double* s,
                                          int* scnt,
                                          int* sdispl,
                                          double* r,
                                          int* rcnt,
                                          int* rdispl);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_alltoallv_impl)> nrnmpi_dbl_alltoallv;
extern "C" double nrnmpi_dbl_allmin_impl(double x);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allmin_impl)> nrnmpi_dbl_allmin;
extern "C" double nrnmpi_dbl_allmax_impl(double x);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allmax_impl)> nrnmpi_dbl_allmax;
extern "C" void nrnmpi_barrier_impl(void);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_barrier_impl)> nrnmpi_barrier;
extern "C" double nrnmpi_dbl_allreduce_impl(double x, int type);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allreduce_impl)> nrnmpi_dbl_allreduce;
extern "C" void nrnmpi_dbl_allreduce_vec_impl(double* src, double* dest, int cnt, int type);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_dbl_allreduce_vec_impl)>
    nrnmpi_dbl_allreduce_vec;
extern "C" void nrnmpi_long_allreduce_vec_impl(long* src, long* dest, int cnt, int type);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_long_allreduce_vec_impl)>
    nrnmpi_long_allreduce_vec;
extern "C" bool nrnmpi_initialized_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_initialized_impl)> nrnmpi_initialized;
extern "C" void nrnmpi_abort_impl(int);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_abort_impl)> nrnmpi_abort;
extern "C" double nrnmpi_wtime_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_wtime_impl)> nrnmpi_wtime;
extern "C" int nrnmpi_local_rank_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_local_rank_impl)> nrnmpi_local_rank;
extern "C" int nrnmpi_local_size_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_local_size_impl)> nrnmpi_local_size;
#if NRN_MULTISEND
extern "C" void nrnmpi_multisend_comm_impl();
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_comm_impl)>
    nrnmpi_multisend_comm;
extern "C" void nrnmpi_multisend_impl(NRNMPI_Spike* spk, int n, int* hosts);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_impl)> nrnmpi_multisend;
extern "C" int nrnmpi_multisend_single_advance_impl(NRNMPI_Spike* spk);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_single_advance_impl)>
    nrnmpi_multisend_single_advance;
extern "C" int nrnmpi_multisend_conserve_impl(int nsend, int nrecv);
extern mpi_function<cnrn_make_integral_constant_t(nrnmpi_multisend_conserve_impl)>
    nrnmpi_multisend_conserve;
#endif

}  // namespace coreneuron
