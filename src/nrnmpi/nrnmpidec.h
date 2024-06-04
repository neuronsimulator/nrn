/*
This file is processed by mkdynam.sh and so it is important that the prototypes
be of the form "type foo(type arg, ...)". Moreover, the * needs to be attached
to the type, e.g. `T*` is valid, but `T *` isn't.
*/

#pragma once
#include <nrnmpiuse.h>
#include <cstdint>
using longdbl = long double;
#if NRNMPI
#include <stdlib.h>
#include <string>

/* from bbsmpipack.cpp */
typedef struct bbsmpibuf {
    char* buf;
    int size;
    int pkposition;
    int upkpos;
    int keypos;
    int refcount;
} bbsmpibuf;

struct NRNMPI_Spike;

namespace neuron::container {
struct MemoryStats;
struct MemoryUsage;
}  // namespace neuron::container

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

/* from memory_usage.cpp */
extern void nrnmpi_memory_stats(neuron::container::MemoryStats& stats, neuron::container::MemoryUsage const& usage);
extern void nrnmpi_print_memory_stats(neuron::container::MemoryStats const& stats);

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
extern void nrnmpi_int_alltoallv(const int* s, const int* scnt, const int* sdispl, int* r, int* rcnt, int* rdispl);
extern void nrnmpi_int_alltoallv_sparse(int* s, int* scnt, int* sdispl, int* r, int* rcnt, int* rdispl);
extern void nrnmpi_long_allgatherv(int64_t* s, int64_t* r, int* n, int* dspl);
extern void nrnmpi_long_allgatherv_inplace(long* srcdest, int* n, int* dspl);
extern void nrnmpi_long_alltoallv(int64_t* s, int* scnt, int* sdispl, int64_t* r, int* rcnt, int* rdispl);
extern void nrnmpi_long_alltoallv_sparse(int64_t* s, int* scnt, int* sdispl, int64_t* r, int* rcnt, int* rdispl);
extern void nrnmpi_dbl_allgatherv(double* s, double* r, int* n, int* dspl);
extern void nrnmpi_dbl_allgatherv_inplace(double* srcdest, int* n, int* dspl);
extern void nrnmpi_dbl_alltoallv(const double* s, const int* scnt, const int* sdispl, double* r, int* rcnt, int* rdispl);
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
#endif
