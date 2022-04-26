#include <../../nrnconf.h>
#include <errno.h>
#include <unistd.h>
#if defined(CYGWIN) || defined(MINGW)

#if !defined(__MINGW32__)
#include "system.cpp"
#define my_off64_t loff_t
#else  // __MINGW32__
#define my_off64_t off64_t
#endif  // __MINGW32__

#include "mswinprt.cpp"

my_off64_t lseek64(int fd, my_off64_t offset, int whence) {
    fprintf(stderr, "called lseek64\n");
    abort();
}

#endif  // CYGWIN or MINGW
