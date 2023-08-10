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

// olupton 2022-07-06: dynamic MPI needs to dlopen some of these (slightly
// redefined) symbol names, so keep C linkage for simplicity
extern "C" {
// clang-format off
extern bbsmpibuf* nrnmpi_newbuf(int size);
extern void nrnmpi_copy(bbsmpibuf* dest, bbsmpibuf* src);
extern void nrnmpi_ref(bbsmpibuf* buf);
extern void nrnmpi_unref(bbsmpibuf* buf);

extern void nrnmpi_upkbegin(bbsmpibuf* buf);
extern char* nrnmpi_getkey(bbsmpibuf* buf);
extern int nrnmpi_getid(bbsmpibuf* buf);
extern int nrnmpi_upkint(bbsmpibuf* buf);
extern double nrnmpi_upkdouble(bbsmpibuf* buf);
extern void nrnmpi_upkvec(int n, double* x, bbsmpibuf* buf);
extern char* nrnmpi_upkstr(bbsmpibuf* buf);
extern char* nrnmpi_upkpickle(size_t* size, bbsmpibuf* buf);

extern void nrnmpi_pkbegin(bbsmpibuf* buf);
extern void nrnmpi_enddata(bbsmpibuf* buf);
extern void nrnmpi_pkint(int i, bbsmpibuf* buf);
extern void nrnmpi_pkdouble(double x, bbsmpibuf* buf);
extern void nrnmpi_pkvec(int n, double* x, bbsmpibuf* buf);
extern void nrnmpi_pkstr(const char* s, bbsmpibuf* buf);
extern void nrnmpi_pkpickle(const char* s, size_t size, bbsmpibuf* buf);

extern int nrnmpi_iprobe(int* size, int* tag, int* source);
extern void nrnmpi_probe(int* size, int* tag, int* source);
extern void nrnmpi_bbssend(int dest, int tag, bbsmpibuf* r);
extern int nrnmpi_bbsrecv(int source, bbsmpibuf* r);
extern int nrnmpi_bbssendrecv(int dest, int tag, bbsmpibuf* s, bbsmpibuf* r);

/* from nrnmpi.cpp */
extern void nrnmpi_init(int nrnmpi_under_nrncontrol, int* pargc, char*** pargv);
extern int nrnmpi_wrap_mpi_init(int* flag);
extern double nrnmpi_wtime();
extern void nrnmpi_terminate();
extern void nrnmpi_abort(int errcode);
extern void nrnmpi_subworld_size(int n);
extern void nrnmpi_get_subworld_info(int* cnt, int* index, int* rank, int* numprocs, int* numprocs_world);


/* from mpispike.cpp */
extern void nrnmpi_spike_initialize();
extern int nrnmpi_spike_exchange(int* ovfl, int* nout, int* nin, NRNMPI_Spike* spikeout, NRNMPI_Spike** spikein, int* icapacity_);
extern int nrnmpi_spike_exchange_compressed(int localgid_size, int ag_send_size, int ag_send_nspike, int* ovfl_capacity, int* ovfl, unsigned char* spfixout, unsigned char* spfixin, unsigned char** spfixin_ovfl, int* nin_);
extern double nrnmpi_mindelay(double maxdel);
extern int nrnmpi_int_allmax(int i);
extern void nrnmpi_int_gather(int* s, int* r, int cnt, int root);
extern void nrnmpi_int_gatherv(int* s, int scnt, int* r, int* rcnt, int* rdispl, int root);
extern void nrnmpi_char_gatherv(char* s, int scnt, char* r, int* rcnt, int* rdispl, int root);
extern void nrnmpi_int_scatter(int* s, int* r, int cnt, int root);
extern void nrnmpi_char_scatterv(char* s, int* scnt, int* sdispl, char* r, int rcnt, int root);
extern void nrnmpi_int_allgather(int* s, int* r, int n);
extern void nrnmpi_int_allgather_inplace(int* srcdest, int n);
extern void nrnmpi_int_allgatherv_inplace(int* srcdest, int* n, int* dspl);
extern void nrnmpi_int_allgatherv(int* s, int* r, int* n, int* dspl);
extern void nrnmpi_char_allgatherv(char* s, char* r, int* n, int* dspl);
extern void nrnmpi_int_alltoall(int* s, int* r, int n);
extern void nrnmpi_int_alltoallv(int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl);
extern void nrnmpi_int_alltoallv_sparse(int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl);
extern void nrnmpi_long_allgatherv(int64_t* s, int64_t* r, int* n, int* dspl);
extern void nrnmpi_long_allgatherv_inplace(long* srcdest, int* n, int* dspl);
extern void nrnmpi_long_alltoallv(int64_t* s, int* scnt, int* sdispl, int64_t* r, int* rcnt, int* rdispl);
extern void nrnmpi_long_alltoallv_sparse(int64_t* s, int* scnt, int* sdispl, int64_t* r, int* rcnt, int* rdispl);
extern void nrnmpi_dbl_allgatherv(double* s, double* r, int* n, int* dspl);
extern void nrnmpi_dbl_allgatherv_inplace(double* srcdest, int* n, int* dspl);
extern void nrnmpi_dbl_alltoallv(double* s, int* scnt, int* sdispl, double* r, int* rcnt, int* rdispl);
extern void nrnmpi_dbl_alltoallv_sparse(double* s, int* scnt, int* sdispl, double* r, int* rcnt, int* rdispl);
extern void nrnmpi_char_alltoallv(char* s, int* scnt, int* sdispl, char* r, int* rcnt, int* rdispl);
extern void nrnmpi_dbl_broadcast(double* buf, int cnt, int root);
extern void nrnmpi_int_broadcast(int* buf, int cnt, int root);
extern void nrnmpi_char_broadcast(char* buf, int cnt, int root);
extern void nrnmpi_str_broadcast_world(std::string& str, int root);
extern int nrnmpi_int_sum_reduce(int in);
extern void nrnmpi_assert_opstep(int opstep, double t);
extern double nrnmpi_dbl_allmin(double x);
extern int nrnmpi_pgvts_least(double* t, int* op, int* init);
extern void nrnmpi_send_doubles(double* pd, int cnt, int dest, int tag);
extern void nrnmpi_recv_doubles(double* pd, int cnt, int src, int tag);
extern void nrnmpi_postrecv_doubles(double* pd, int cnt, int src, int tag, void** request);
extern void nrnmpi_wait(void** request);
extern void nrnmpi_barrier();
extern double nrnmpi_dbl_allreduce(double x, int type);

extern void nrnmpi_dbl_allreduce_vec(double* src, double* dest, int cnt, int type);
extern void nrnmpi_longdbl_allreduce_vec(longdbl* src, longdbl* dest, int cnt, int type);
extern void nrnmpi_long_allreduce_vec(long* src, long* dest, int cnt, int type);

extern void nrnmpi_dbl_allgather(double* s, double* r, int n);
#if NRNMPI
extern void nrnmpi_multisend_comm();
extern void nrnmpi_multisend_multisend(NRNMPI_Spike* spk, int n, int* hosts);
extern int nrnmpi_multisend_single_advance(NRNMPI_Spike* spk);
extern int nrnmpi_multisend_conserve(int nsend, int nrecv);
#endif
// clang-format on
}
>>>>>>> origin/master

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
