#include <../../nrnconf.h>
/* a fake change */
/* /local/src/master/nrn/src/oc/math.cpp,v 1.6 1999/07/16 13:43:10 hines Exp */

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include "nrnmpiuse.h"
#include "ocfunc.h"
#include "hoc.h"


#define EPS         hoc_epsilon
#define MAXERRCOUNT 5

int hoc_errno_count;

#if _CRAY
#define log   logl
#define log10 log10l
#define exp   expl
#define sqrt  sqrtl
#define pow   powl
#endif

static double errcheck(double, const char*);

void hoc_atan2(void) {
    double d;
    d = atan2(*hoc_getarg(1), *hoc_getarg(2));
    hoc_ret();
    hoc_pushx(d);
}

double Log(double x) {
    return errcheck(log(x), "log");
}

double Log10(double x) {
    return errcheck(log10(x), "log10");
}

/* used by nmodl and other c, c++ code */
double hoc_Exp(double x) {
    if (x < -700.) {
        return 0.;
    } else if (x > 700 && nrn_feenableexcept_ == 0) {
        errno = ERANGE;
        if (++hoc_errno_count < MAXERRCOUNT) {
            fprintf(stderr, "exp(%g) out of range, returning exp(700)\n", x);
        }
        if (hoc_errno_count == MAXERRCOUNT) {
            fprintf(stderr, "No more errno warnings during this execution\n");
        }
        return exp(700.);
    }
    return exp(x);
}

/* used by interpreter */
double hoc1_Exp(double x) {
    if (x < -700.) {
        return 0.;
    } else if (x > 700) {
        errno = ERANGE;
        return errcheck(exp(700.), "exp");
    }
    return errcheck(exp(x), "exp");
}

double Sqrt(double x) {
    return errcheck(sqrt(x), "sqrt");
}

double Pow(double x, double y) {
    return errcheck(pow(x, y), "exponentiation");
}

double integer(double x) {
    if (x < 0) {
        return (double) (long) (x - EPS);
    } else {
        return (double) (long) (x + EPS);
    }
}

double errcheck(double d, const char* s) /* check result of library call */
{
    if (errno == EDOM) {
        errno = 0;
        hoc_execerror(s, "argument out of domain");
    } else if (errno == ERANGE) {
        errno = 0;
#if 0
        hoc_execerror(s, "result out of range");
#else
        if (++hoc_errno_count > MAXERRCOUNT) {
        } else {
            hoc_warning(s, "result out of range");
            if (hoc_errno_count == MAXERRCOUNT) {
                fprintf(stderr, "No more errno warnings during this execution\n");
            }
        }
#endif
    }
    return d;
}

int hoc_errno_check(void) {
    int ierr;
#if LINDA
    static parallel_eagain = 0;
#endif

#if 1
    errno = 0;
    return 0;
#else
    if (errno) {
        if (errno == EAGAIN) {
            /* Ubiquitous on many systems and it seems not to matter */
            errno = 0;
            return 0;
        }
#if !defined(MAC) || defined(DARWIN)
        if (errno == ENOENT) {
            errno = 0;
            return 0;
        }
#endif
        if (++hoc_errno_count > MAXERRCOUNT) {
            errno = 0;
            return 0;
        }
#ifdef MINGW
        if (errno == EBUSY) {
            errno = 0;
            return 0;
        }
#endif
        switch (errno) {
        case EDOM:
            fprintf(stderr, "A math function was called with argument out of domain\n");
            break;
        case ERANGE:
            fprintf(stderr, "A math function was called that returned an out of range value\n");
            break;
#if LINDA
            /* regularly set by eval() and perhaps other linda commands */
        case EAGAIN:
            if (parallel_eagain++ == 0) {
                perror("oc");
                fprintf(stderr,
                        "oc: This error occurs often from LINDA and thus will not be further "
                        "reported.\n");
            }
            break;
#endif
        default:
            perror("oc");
            break;
        }
        if (hoc_errno_count == MAXERRCOUNT) {
            fprintf(stderr, "No more errno warnings during this execution\n");
        }
    }
    ierr = errno;
    errno = 0;
    return ierr;
#endif
}
