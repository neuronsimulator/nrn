/*
This file is processed by mkdynam.sh and so it is important that
the prototypes be of the form "type foo(type arg, ...)"
*/

#ifndef nrnmpidec_h
#define nrnmpidec_h
#include <nrnmpiuse.h>
#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif
typedef long double longdbl;
#if NRNMPI
#include <stdlib.h>
#if defined(__cplusplus)
extern "C" {
#endif

/* from bbsmpipack.cpp */
typedef struct bbsmpibuf {
	char* buf;
	int size;
	int pkposition;
	int upkpos;
	int keypos;
	int refcount;
} bbsmpibuf;

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


/* from mpispike.cpp */
extern void nrnmpi_spike_initialize();
extern int nrnmpi_spike_exchange();
extern int nrnmpi_spike_exchange_compressed();
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
extern void nrnmpi_char_broadcast_world(char** pstr, int root);
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
#if BGPDMA
extern void nrnmpi_bgp_comm();
extern void nrnmpi_bgp_multisend(NRNMPI_Spike* spk, int n, int* hosts);
extern int nrnmpi_bgp_single_advance(NRNMPI_Spike* spk);
extern int nrnmpi_bgp_conserve(int nsend, int nrecv);
#endif


#if defined(__cplusplus)
}
#endif

#endif
#endif
