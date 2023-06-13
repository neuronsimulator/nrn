#pragma once

#include "./nrnmpi.h"
// bbsmpipack.cpp
struct bbsmpibuf {
    char* buf;
    int size;
    int pkposition;
    int upkpos;
    int keypos;
    int refcount;
};

extern "C" bbsmpibuf* nrnmpi_newbuf_impl(int size);
declare_mpi_method(nrnmpi_newbuf);
extern "C" void nrnmpi_copy_impl(bbsmpibuf* dest, bbsmpibuf* src);
declare_mpi_method(nrnmpi_copy);
extern "C" void nrnmpi_ref_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_ref);
extern "C" void nrnmpi_unref_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_unref);
extern "C" void nrnmpi_upkbegin_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_upkbegin);
extern "C" char* nrnmpi_getkey_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_getkey);
extern "C" int nrnmpi_getid_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_getid);
extern "C" int nrnmpi_upkint_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_upkint);
extern "C" double nrnmpi_upkdouble_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_upkdouble);
extern "C" void nrnmpi_upkvec_impl(int n, double* x, bbsmpibuf* buf);
declare_mpi_method(nrnmpi_upkvec);
extern "C" char* nrnmpi_upkstr_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_upkstr);
extern "C" char* nrnmpi_upkpickle_impl(size_t* size, bbsmpibuf* buf);
declare_mpi_method(nrnmpi_upkpickle);
extern "C" void nrnmpi_pkbegin_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_pkbegin);
extern "C" void nrnmpi_enddata_impl(bbsmpibuf* buf);
declare_mpi_method(nrnmpi_enddata);
extern "C" void nrnmpi_pkint_impl(int i, bbsmpibuf* buf);
declare_mpi_method(nrnmpi_pkint);
extern "C" void nrnmpi_pkdouble_impl(double x, bbsmpibuf* buf);
declare_mpi_method(nrnmpi_pkdouble);
extern "C" void nrnmpi_pkvec_impl(int n, double* x, bbsmpibuf* buf);
declare_mpi_method(nrnmpi_pkvec);
extern "C" void nrnmpi_pkstr_impl(const char* s, bbsmpibuf* buf);
declare_mpi_method(nrnmpi_pkstr);
extern "C" void nrnmpi_pkpickle_impl(const char* s, size_t size, bbsmpibuf* buf);
declare_mpi_method(nrnmpi_pkpickle);
extern "C" int nrnmpi_iprobe_impl(int* size, int* tag, int* source);
declare_mpi_method(nrnmpi_iprobe);
extern "C" void nrnmpi_probe_impl(int* size, int* tag, int* source);
declare_mpi_method(nrnmpi_probe);
extern "C" void nrnmpi_bbssend_impl(int dest, int tag, bbsmpibuf* r);
declare_mpi_method(nrnmpi_bbssend);
extern "C" int nrnmpi_bbsrecv_impl(int source, bbsmpibuf* r);
declare_mpi_method(nrnmpi_bbsrecv);
extern "C" int nrnmpi_bbssendrecv_impl(int dest, int tag, bbsmpibuf* s, bbsmpibuf* r);
declare_mpi_method(nrnmpi_bbssendrecv);
// nrnmpi.cpp
extern "C" void nrnmpi_init_impl(int nrnmpi_under_nrncontrol, int* pargc, char*** pargv);
declare_mpi_method(nrnmpi_init);
extern "C" int nrnmpi_wrap_mpi_init_impl(int* flag);
declare_mpi_method(nrnmpi_wrap_mpi_init);
extern "C" double nrnmpi_wtime_impl();
declare_mpi_method(nrnmpi_wtime);
extern "C" void nrnmpi_terminate_impl();
declare_mpi_method(nrnmpi_terminate);
extern "C" void nrnmpi_abort_impl(int errcode);
declare_mpi_method(nrnmpi_abort);
extern "C" void nrnmpi_subworld_size_impl(int n);
declare_mpi_method(nrnmpi_subworld_size);
// mpispike.cpp
extern "C" void nrnmpi_spike_initialize_impl();
declare_mpi_method(nrnmpi_spike_initialize);
extern "C" int nrnmpi_spike_exchange_impl(int* nin,
                                          int* nout,
                                          int* icapacity,
                                          NRNMPI_Spike** spikein,
                                          NRNMPI_Spike** spikeout);
declare_mpi_method(nrnmpi_spike_exchange);
extern "C" int nrnmpi_spike_exchange_compressed_impl(int* nin,
                                                     unsigned char* spfixin_ovfl,
                                                     unsigned char* spikeout_fixed,
                                                     unsigned char* spikein_fixed,
                                                     int ag_send_size,
                                                     int ag_send_nspike,
                                                     int localgid_size,
                                                     int* ovfl_capacity,
                                                     int* ovfl);
declare_mpi_method(nrnmpi_spike_exchange_compressed);
extern "C" double nrnmpi_mindelay_impl(double maxdel);
declare_mpi_method(nrnmpi_mindelay);
extern "C" int nrnmpi_int_allmax_impl(int i);
declare_mpi_method(nrnmpi_int_allmax);
extern "C" void nrnmpi_int_gather_impl(int* s, int* r, int cnt, int root);
declare_mpi_method(nrnmpi_int_gather);
extern "C" void nrnmpi_int_gatherv_impl(int* s, int scnt, int* r, int* rcnt, int* rdispl, int root);
declare_mpi_method(nrnmpi_int_gatherv);
extern "C" void nrnmpi_char_gatherv_impl(char* s,
                                         int scnt,
                                         char* r,
                                         int* rcnt,
                                         int* rdispl,
                                         int root);
declare_mpi_method(nrnmpi_char_gatherv);
extern "C" void nrnmpi_int_scatter_impl(int* s, int* r, int cnt, int root);
declare_mpi_method(nrnmpi_int_scatter);
extern "C" void nrnmpi_char_scatterv_impl(char* s,
                                          int* scnt,
                                          int* sdispl,
                                          char* r,
                                          int rcnt,
                                          int root);
declare_mpi_method(nrnmpi_char_scatterv);
extern "C" void nrnmpi_int_allgather_impl(int* s, int* r, int n);
declare_mpi_method(nrnmpi_int_allgather);
extern "C" void nrnmpi_int_allgather_inplace_impl(int* srcdest, int n);
declare_mpi_method(nrnmpi_int_allgather_inplace);
extern "C" void nrnmpi_int_allgatherv_inplace_impl(int* srcdest, int* n, int* dspl);
declare_mpi_method(nrnmpi_int_allgatherv_inplace);
extern "C" void nrnmpi_int_allgatherv_impl(int* s, int* r, int* n, int* dspl);
declare_mpi_method(nrnmpi_int_allgatherv);
extern "C" void nrnmpi_char_allgatherv_impl(char* s, char* r, int* n, int* dspl);
declare_mpi_method(nrnmpi_char_allgatherv);
extern "C" void nrnmpi_int_alltoall_impl(int* s, int* r, int n);
declare_mpi_method(nrnmpi_int_alltoall);
extern "C" void nrnmpi_int_alltoallv_impl(int* s,
                                          int* scnt,
                                          int* sdispl,
                                          int* r,
                                          int* rcnt,
                                          int* rdispl);
declare_mpi_method(nrnmpi_int_alltoallv);
extern "C" void nrnmpi_int_alltoallv_sparse_impl(int* s,
                                                 int* scnt,
                                                 int* sdispl,
                                                 int* r,
                                                 int* rcnt,
                                                 int* rdispl);
declare_mpi_method(nrnmpi_int_alltoallv_sparse);
extern "C" void nrnmpi_long_allgatherv_impl(int64_t* s, int64_t* r, int* n, int* dspl);
declare_mpi_method(nrnmpi_long_allgatherv);
extern "C" void nrnmpi_long_allgatherv_inplace_impl(long* srcdest, int* n, int* dspl);
declare_mpi_method(nrnmpi_long_allgatherv_inplace);
extern "C" void nrnmpi_long_alltoallv_impl(int64_t* s,
                                           int* scnt,
                                           int* sdispl,
                                           int64_t* r,
                                           int* rcnt,
                                           int* rdispl);
declare_mpi_method(nrnmpi_long_alltoallv);
extern "C" void nrnmpi_long_alltoallv_sparse_impl(int64_t* s,
                                                  int* scnt,
                                                  int* sdispl,
                                                  int64_t* r,
                                                  int* rcnt,
                                                  int* rdispl);
declare_mpi_method(nrnmpi_long_alltoallv_sparse);
extern "C" void nrnmpi_dbl_allgatherv_impl(double* s, double* r, int* n, int* dspl);
declare_mpi_method(nrnmpi_dbl_allgatherv);
extern "C" void nrnmpi_dbl_allgatherv_inplace_impl(double* srcdest, int* n, int* dspl);
declare_mpi_method(nrnmpi_dbl_allgatherv_inplace);
extern "C" void nrnmpi_dbl_alltoallv_impl(double* s,
                                          int* scnt,
                                          int* sdispl,
                                          double* r,
                                          int* rcnt,
                                          int* rdispl);
declare_mpi_method(nrnmpi_dbl_alltoallv);
extern "C" void nrnmpi_dbl_alltoallv_sparse_impl(double* s,
                                                 int* scnt,
                                                 int* sdispl,
                                                 double* r,
                                                 int* rcnt,
                                                 int* rdispl);
declare_mpi_method(nrnmpi_dbl_alltoallv_sparse);
extern "C" void nrnmpi_char_alltoallv_impl(char* s,
                                           int* scnt,
                                           int* sdispl,
                                           char* r,
                                           int* rcnt,
                                           int* rdispl);
declare_mpi_method(nrnmpi_char_alltoallv);
extern "C" void nrnmpi_dbl_broadcast_impl(double* buf, int cnt, int root);
declare_mpi_method(nrnmpi_dbl_broadcast);
extern "C" void nrnmpi_int_broadcast_impl(int* buf, int cnt, int root);
declare_mpi_method(nrnmpi_int_broadcast);
extern "C" void nrnmpi_char_broadcast_impl(char* buf, int cnt, int root);
declare_mpi_method(nrnmpi_char_broadcast);
extern "C" void nrnmpi_str_broadcast_world_impl(std::string& str, int root);
declare_mpi_method(nrnmpi_str_broadcast_world);
extern "C" int nrnmpi_int_sum_reduce_impl(int in);
declare_mpi_method(nrnmpi_int_sum_reduce);
extern "C" void nrnmpi_assert_opstep_impl(int opstep, double t);
declare_mpi_method(nrnmpi_assert_opstep);
extern "C" double nrnmpi_dbl_allmin_impl(double x);
declare_mpi_method(nrnmpi_dbl_allmin);
extern "C" int nrnmpi_pgvts_least_impl(double* t, int* op, int* init);
declare_mpi_method(nrnmpi_pgvts_least);
extern "C" void nrnmpi_send_doubles_impl(double* pd, int cnt, int dest, int tag);
declare_mpi_method(nrnmpi_send_doubles);
extern "C" void nrnmpi_recv_doubles_impl(double* pd, int cnt, int src, int tag);
declare_mpi_method(nrnmpi_recv_doubles);
extern "C" void nrnmpi_postrecv_doubles_impl(double* pd, int cnt, int src, int tag, void** request);
declare_mpi_method(nrnmpi_postrecv_doubles);
extern "C" void nrnmpi_wait_impl(void** request);
declare_mpi_method(nrnmpi_wait);
extern "C" void nrnmpi_barrier_impl();
declare_mpi_method(nrnmpi_barrier);
extern "C" double nrnmpi_dbl_allreduce_impl(double x, int type);
declare_mpi_method(nrnmpi_dbl_allreduce);
extern "C" void nrnmpi_dbl_allreduce_vec_impl(double* src, double* dest, int cnt, int type);
declare_mpi_method(nrnmpi_dbl_allreduce_vec);
extern "C" void nrnmpi_longdbl_allreduce_vec_impl(long double* src,
                                                  long double* dest,
                                                  int cnt,
                                                  int type);
declare_mpi_method(nrnmpi_longdbl_allreduce_vec);
extern "C" void nrnmpi_long_allreduce_vec_impl(long* src, long* dest, int cnt, int type);
declare_mpi_method(nrnmpi_long_allreduce_vec);
extern "C" void nrnmpi_dbl_allgather_impl(double* s, double* r, int n);
declare_mpi_method(nrnmpi_dbl_allgather);
extern "C" void nrnmpi_multisend_comm_impl();
declare_mpi_method(nrnmpi_multisend_comm);
extern "C" void nrnmpi_multisend_multisend_impl(NRNMPI_Spike* spk, int n, int* hosts);
declare_mpi_method(nrnmpi_multisend_multisend);
extern "C" int nrnmpi_multisend_single_advance_impl(NRNMPI_Spike* spk);
declare_mpi_method(nrnmpi_multisend_single_advance);
extern "C" int nrnmpi_multisend_conserve_impl(int nsend, int nrecv);
declare_mpi_method(nrnmpi_multisend_conserve);
