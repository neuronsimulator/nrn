#define int32_t long
#define u_int32_t unsigned long
#define HAVE_LIMITS_H 1
#define HAVE_POSIX_SIGNALS
#define RETSIGTYPE void
#define HAVE_IV 1
#define USEBBS 1
#define HAVE_STL 1
#define yyoverflow printf

#if defined(_MSC_VER)

#define __MWERKS__ 1
#define __WIN32__ 1
#define _Windows 1

#undef near
#define near mynear

#define motif_kit
#define sgi_motif_kit
#define printf myprintf
#define vprintf myvprintf
#define gets mygets
#define puts myputs
#define fprintf myfprintf

#undef DELETE
#undef IGNORE
#define CABLE 1
#define HOC 1
#define OOP 1
#define OC_CLASSES "nrnclass.h"
#define USECVODE 1
#define CVODE 1
#define USEMATRIX 1

#endif

