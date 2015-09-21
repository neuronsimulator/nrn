/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _H_NRNCONF_
#define _H_NRNCONF_

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>

#define NRNBBCORE 1

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
extern void* nrn_cacheline_alloc(void** memptr, size_t size);
extern double* makevector(size_t size); /* size in bytes */
extern void* emalloc(size_t size);
extern void* ecalloc(size_t n, size_t size);
extern void* erealloc(void* ptr, size_t size);
extern double hoc_Exp(double x);

/* will go away at some point */
typedef struct Point_process {
	void* _presyn; /* for artificial cell net_event */
	int _i_instance;
	short _type;
	short _tid; /* NrnThread id */
} Point_process;

extern char* pnt_name(Point_process* pnt);

#if defined(__cplusplus)
}
#endif

#endif
