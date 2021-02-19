
#ifndef hocassrt_h
#define hocassrt_h
#include <assert.h>
#undef assert
#undef _assert
# ifndef NDEBUG
# ifndef stderr
# include <stdio.h>
# endif

#if defined(__cplusplus)
extern "C" {
#endif

extern void hoc_execerror(const char*, const char*);

#if defined(__cplusplus)
}
#endif


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
