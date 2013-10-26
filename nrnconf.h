#ifndef nrnconf_h
#define nrnconf_h

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

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

#define erealloc realloc /* no hoc_execerror */
#define emalloc malloc

#include <multicore.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern double celsius;
extern double t, dt;
extern int secondorder;
extern int stoprun;

extern void hoc_warning(const char*, const char*);

#if defined(__cplusplus)
}
#endif

#endif
