#include <../../nrnconf.h>
/* a fake change */
/* /local/src/master/nrn/src/oc/math.cpp,v 1.6 1999/07/16 13:43:10 hines Exp */

#include "hoc.h"
#include "nrnmpiuse.h"
#include "ocfunc.h"
#include <cfenv>
#include <cmath>
#include <errno.h>
#include <stdio.h>


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
    // errno may not be enabled, rely on FE exceptions in that case. See:
    // https://en.cppreference.com/w/cpp/numeric/math/math_errhandling
#ifdef MINGW
    const auto errno_enabled = true;
    const auto check_fe_except = false;
#else
    const auto errno_enabled = math_errhandling & MATH_ERRNO;
    const auto check_fe_except = !errno_enabled && math_errhandling & MATH_ERREXCEPT;
#endif
    if ((errno_enabled && errno == EDOM) || (check_fe_except && std::fetestexcept(FE_INVALID))) {
        if (check_fe_except)
            std::feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        hoc_execerror(s, "argument out of domain");
    } else if ((errno_enabled && errno == ERANGE) ||
               (check_fe_except &&
                (std::fetestexcept(FE_DIVBYZERO) || std::fetestexcept(FE_OVERFLOW) ||
                 std::fetestexcept(FE_UNDERFLOW)))) {
        if (check_fe_except)
            std::feclearexcept(FE_ALL_EXCEPT);
        errno = 0;
        if (++hoc_errno_count > MAXERRCOUNT) {
        } else {
            hoc_warning(s, "result out of range");
            if (hoc_errno_count == MAXERRCOUNT) {
                fprintf(stderr, "No more errno warnings during this execution\n");
            }
        }
    }
    return d;
}

int hoc_errno_check(void) {
    int ierr;
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
