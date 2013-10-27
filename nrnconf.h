#ifndef nrnconf_h
#define nrnconf_h

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>

#define nil NULL
#define Sprintf sprintf

typedef int Datum;
typedef int (*Pfri)();
typedef char Symbol;

#define CACHEVEC 2
#define VEC_A(i) (_nt->_actual_a[(i)]) 
#define VEC_B(i) (_nt->_actual_b[(i)])
#define VEC_D(i) (_nt->_actual_d[(i)])
#define VEC_RHS(i) (_nt->_actual_rhs[(i)])
#define VEC_V(i) (_nt->_actual_v[(i)])
#define VEC_AREA(i) (_nt->_actual_area[(i)])
#define VECTORIZE 1
#define MULTICORE 1

#include <multicore.h>
#include <nrnoc_decl.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern double celsius;
extern double t, dt;
extern int secondorder;
extern int stoprun;
#define tstopbit (1 << 15)
#define tstopset stoprun |= tstopbit
#define tstopunset stoprun &= (~tstopbit)

extern void hoc_execerror(const char*, const char*); /* print and abort */
extern void hoc_warning(const char*, const char*);
extern void* nrn_cacheline_calloc(void** memptr, size_t nmemb, size_t size);
extern double* makevector(size_t size); /* size in bytes */
extern void* emalloc(size_t size);
extern void* ecalloc(size_t n, size_t size);
extern void* erealloc(void* ptr, size_t size);

/* will go away at some point */
typedef struct Point_process {
	int type;
	double* data;
	Datum* pdata;
	void* presyn_; /* for artificial cell net_event */
	void* _vnt; /* NrnThread* */
} Point_process;

extern char* pnt_name(Point_process* pnt);

#if defined(__cplusplus)
}
#endif

#endif
