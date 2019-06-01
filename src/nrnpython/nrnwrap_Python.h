#ifndef nrnwrap_Python_h
#define nrnwrap_Python_h
/* avoid "redefined" warnings due to inconsistent configure HAVE_xxx */

#undef HAVE_PUTENV
#undef HAVE_FTIME
#undef HAVE_PROTOTYPES
#if defined(__MINGW32__)
#undef _hypot
#define _hypot hypot
#endif
#include <Python.h>

#endif /* nrnwrap_Python_h */
