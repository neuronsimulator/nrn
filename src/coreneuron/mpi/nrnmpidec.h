/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
This file is processed by mkdynam.sh and so it is important that
the prototypes be of the form "type foo(type arg, ...)"
*/

#ifndef nrnmpidec_h
#define nrnmpidec_h


#if NRNMPI
#include <stdlib.h>
namespace coreneuron {
/* from bbsmpipack.c */
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
extern void nrnmpi_bbssend(int dest, int tag, bbsmpibuf* r);
extern int nrnmpi_bbsrecv(int source, bbsmpibuf* r);
extern int nrnmpi_bbssendrecv(int dest, int tag, bbsmpibuf* s, bbsmpibuf* r);

/* from nrnmpi.cpp */
extern void nrnmpi_init(int* pargc, char*** pargv);
extern int nrnmpi_wrap_mpi_init(int* flag);
extern void nrnmpi_finalize(void);
extern void nrnmpi_terminate();
extern void nrnmpi_subworld_size(int n);
extern int nrn_wrap_mpi_init(int* flag);
extern void nrnmpi_check_threading_support();

/* from mpispike.c */
extern void nrnmpi_spike_initialize(void);
extern int nrnmpi_spike_exchange(void);
extern int nrnmpi_spike_exchange_compressed(void);
extern int nrnmpi_int_allmax(int i);
extern void nrnmpi_int_gather(int* s, int* r, int cnt, int root);
extern void nrnmpi_int_gatherv(int* s, int scnt, int* r, int* rcnt, int* rdispl, int root);
extern void nrnmpi_int_allgather(int* s, int* r, int n);
extern void nrnmpi_int_allgatherv(int* s, int* r, int* n, int* dspl);
extern void nrnmpi_int_alltoall(int* s, int* r, int n);
extern void nrnmpi_int_alltoallv(const int* s, const int* scnt, const int* sdispl, int* r, int* rcnt, int* rdispl);
extern void nrnmpi_dbl_allgatherv(double* s, double* r, int* n, int* dspl);
extern void nrnmpi_dbl_alltoallv(double* s,
                                 int* scnt,
                                 int* sdispl,
                                 double* r,
                                 int* rcnt,
                                 int* rdispl);
extern void nrnmpi_char_alltoallv(char* s, int* scnt, int* sdispl, char* r, int* rcnt, int* rdispl);
extern void nrnmpi_dbl_broadcast(double* buf, int cnt, int root);
extern void nrnmpi_int_broadcast(int* buf, int cnt, int root);
extern void nrnmpi_char_broadcast(char* buf, int cnt, int root);
extern int nrnmpi_int_sum_reduce(int in);
extern void nrnmpi_assert_opstep(int opstep, double t);
extern double nrnmpi_dbl_allmin(double x);
extern double nrnmpi_dbl_allmax(double x);
extern int nrnmpi_pgvts_least(double* t, int* op, int* init);
extern void nrnmpi_send_doubles(double* pd, int cnt, int dest, int tag);
extern void nrnmpi_recv_doubles(double* pd, int cnt, int src, int tag);
extern void nrnmpi_postrecv_doubles(double* pd, int cnt, int src, int tag, void** request);
extern void nrnmpi_wait(void** request);
extern void nrnmpi_barrier(void);
extern double nrnmpi_dbl_allreduce(double x, int type);
extern long nrnmpi_long_allreduce(long x, int type);
extern void nrnmpi_dbl_allreduce_vec(double* src, double* dest, int cnt, int type);
extern void nrnmpi_long_allreduce_vec(long* src, long* dest, int cnt, int type);
extern void nrnmpi_dbl_allgather(double* s, double* r, int n);
extern int nrnmpi_initialized();
#if NRN_MULTISEND
extern void nrnmpi_multisend_comm();
extern void nrnmpi_multisend(NRNMPI_Spike* spk, int n, int* hosts);
extern int nrnmpi_multisend_single_advance(NRNMPI_Spike* spk);
extern int nrnmpi_multisend_conserve(int nsend, int nrecv);
#endif

}  // namespace coreneuron
#endif
#endif
