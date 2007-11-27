/* /local/src/master/nrn/src/oc/hocassrt.h,v 1.1.1.1 1994/10/12 17:22:03 hines Exp */
/*
hocassrt.h,v
 * Revision 1.1.1.1  1994/10/12  17:22:03  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.63  1993/11/04  15:55:48  hines
 * port to solaris2 (no more warnings)
 *
 * Revision 2.37  1993/03/15  10:04:27  hines
 * assert needs to be #undef before #define
 *
 * Revision 1.2  92/08/07  16:13:24  hines
 * sections as objects. sections now live in nmodl style list
 * 
 * Revision 1.1  91/10/11  11:12:28  hines
 * Initial revision
 * 
 * Revision 2.0  89/07/07  11:31:26  mlh
 * Preparation for newcable
 * 
 * Revision 1.1  89/07/07  11:16:15  mlh
 * Initial revision
 * 
*/

#ifndef hocassrt_h
#define hocassrt_h
#undef assert
#undef _assert
# ifndef NDEBUG
# ifndef stderr
# include <stdio.h>
# endif
#if defined(__STDC__)
# define assert(ex) {if (!(ex)){fprintf(stderr,"Assertion failed: file %s, line %d\n", __FILE__,__LINE__);hoc_execerror(#ex, (char *)0);}}
#else
# define assert(ex) {if (!(ex)){fprintf(stderr,"Assertion failed: file %s, line %d\n", __FILE__,__LINE__);hoc_execerror("ex", (char *)0);}}
#endif
# else
# define _assert(ex) ;
# define assert(ex) ;
# endif
#endif
