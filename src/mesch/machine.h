/*
 * This file has been hacked from the machine.h files supplied from the
 * original meschach distribution.  It's now a generic file that works on
 * all machines.
 * 
 * This file used to define a bunch of HAVE_xyz macros.  This is all handled
 * now in config.h.
 */
/* machine.h.  Generated automatically by configure.  */
/* Any machine specific stuff goes here */
/* Add details necessary for your own installation here! */

/* RCS id: machine.h,v 1.3 1998/08/31 19:47:38 hines Exp */

/* This is for use with "configure" -- if you are not using configure
	then use machine.van for the "vanilla" version of machine.h */

/* Note special macros: ANSI_C (ANSI C syntax)
			SEGMENTED (segmented memory machine e.g. MS-DOS)
			MALLOCDECL (declared if malloc() etc have
					been declared) */

#ifndef _MACHINE_H
#define _MACHINE_H 1

#include <math.h>

#include <../../nrnconf.h>

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#if !defined(HUGE) && defined(HUGE_VAL)
#define HUGE HUGE_VAL
#endif

typedef uint32_t u_int;

/* #undef const */

/* #undef MALLOCDECL */
#define NOT_SEGMENTED 1
#define CHAR0ISDBL0 1
#define HAVE_PROTOTYPES 1
/* #undef HAVE_PROTOTYPES_IN_STRUCT */

/* for inclusion into C++ files */
#ifdef __cplusplus
#define ANSI_C 1
#ifndef HAVE_PROTOTYPES 
#define HAVE_PROTOTYPES 1
#endif
#ifndef HAVE_PROTOTYPES_IN_STRUCT
#define HAVE_PROTOTYPES_IN_STRUCT 1
#endif
#endif /* __cplusplus */

/* example usage: VEC *PROTO(v_get,(int dim)); */
#ifdef HAVE_PROTOTYPES
#define	PROTO(name,args)	name args
#else
#define PROTO(name,args)	name()
#endif /* HAVE_PROTOTYPES */
#ifdef HAVE_PROTOTYPES_IN_STRUCT
/* PROTO_() is to be used instead of PROTO() in struct's and typedef's */
#define	PROTO_(name,args)	name args
#else
#define PROTO_(name,args)	name()
#endif /* HAVE_PROTOTYPES_IN_STRUCT */

/* for basic or larger versions */
#define COMPLEX 1
#define SPARSE 1

/* for loop unrolling */
/* #undef VUNROLL */
/* #undef MUNROLL */

/* for segmented memory */
#ifndef NOT_SEGMENTED
#define	SEGMENTED
#endif

/* An AIX machine had incompatible prototypes between
malloc.h and stdlib.h so prefer stdlib.h if it exists
*/
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
/* if the system has malloc.h */
#ifdef HAVE_MALLOC_H
#define	MALLOCDECL	1
#include	<malloc.h>
#endif
#endif

/* any compiler should have this header */
/* if not, change it */
#include        <stdio.h>


/* Check for ANSI C memmove and memset */
#if defined(STDC_HEADERS) || defined(WIN32)
/* standard copy & zero functions */
#define	MEM_COPY(from,to,size)	memmove((to),(from),(size))
#define	MEM_ZERO(where,size)	memset((where),'\0',(size))

#ifndef ANSI_C
#define ANSI_C 1
#endif

#endif

/* standard headers */
#ifdef ANSI_C
#include	<stdlib.h>
#include	<stddef.h>
#include	<string.h>
#include	<float.h>
#endif


/* if have bcopy & bzero and no alternatives yet known, use them */
#ifdef HAVE_BCOPY
#ifndef MEM_COPY
/* nonstandard copy function */
#define	MEM_COPY(from,to,size)	bcopy((char *)(from),(char *)(to),(int)(size))
#endif
#endif

#ifdef HAVE_BZERO
#ifndef MEM_ZERO
/* nonstandard zero function */
#define	MEM_ZERO(where,size)	bzero((char *)(where),(int)(size))
#endif
#endif

/* if the system has complex.h */
#if 0
#ifdef HAVE_COMPLEX_H
#include	<complex.h>
#endif
/*
 I've commented this out because it causes problems when run through a
 C++ compiler.  complex.h is part of the C++ standard library but does
 something completely different.
*/
#endif

/* If prototypes are available & ANSI_C not yet defined, then define it,
	but don't include any header files as the proper ANSI C headers
        aren't here */
#ifdef HAVE_PROTOTYPES
#ifndef ANSI_C
#define ANSI_C  1
#endif
#endif

/* floating point precision */

/* you can choose single, double or long double (if available) precision */

#define FLOAT 		1
#define DOUBLE 		2
#define LONG_DOUBLE 	3

/* #undef REAL_FLT */
/* #undef REAL_DBL */

/* if nothing is defined, choose double precision */
#ifndef REAL_DBL
#ifndef REAL_FLT
#define REAL_DBL 1
#endif
#endif

/* single precision */
#ifdef REAL_FLT
#define  Real float
#define  LongReal float
#define REAL FLOAT
#define LONGREAL FLOAT
#endif

/* double precision */
#ifdef REAL_DBL
#define Real double
#define LongReal double
#define REAL DOUBLE
#define LONGREAL DOUBLE
#endif


/* machine epsilon or unit roundoff error */
/* This is correct on most IEEE Real precision systems */
#ifdef DBL_EPSILON
#if REAL == DOUBLE
#define	MACHEPS	DBL_EPSILON
#elif REAL == FLOAT
#define	MACHEPS	FLT_EPSILON
#elif REAL == LONGDOUBLE
#define MACHEPS LDBL_EPSILON
#endif
#endif

#define F_MACHEPS 1.19209e-07
#define D_MACHEPS 2.22045e-16

#ifndef MACHEPS
#if REAL == DOUBLE
#define	MACHEPS	D_MACHEPS
#elif REAL == FLOAT  
#define MACHEPS F_MACHEPS
#elif REAL == LONGDOUBLE
#define MACHEPS D_MACHEPS
#endif
#endif

/* #undef M_MACHEPS */

/********************
#ifdef DBL_EPSILON
#define	MACHEPS	DBL_EPSILON
#endif
#ifdef M_MACHEPS
#ifndef MACHEPS
#define MACHEPS	M_MACHEPS
#endif
#endif
********************/

#define	M_MAX_INT 2147483647
#ifdef	M_MAX_INT
#ifndef MAX_RAND
#define	MAX_RAND ((double)(M_MAX_INT))
/* This isn't true on a lot of older unix systems. */
#endif
#endif

/* for non-ANSI systems */
#ifndef HUGE_VAL
#define HUGE_VAL HUGE
#else
#ifndef HUGE
#define HUGE HUGE_VAL
#endif
#endif


#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#endif
