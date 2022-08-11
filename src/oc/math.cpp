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

#ifdef MINGW
static const auto errno_enabled = true;
static const auto check_fe_except = false;
#elif defined(NVHPC_CHECK_FE_EXCEPTIONS)
static constexpr auto errno_enabled = false;
static constexpr auto check_fe_except = true;
static_assert(math_errhandling & MATH_ERREXCEPT);
#else
static const auto errno_enabled = math_errhandling & MATH_ERRNO;
static const auto check_fe_except = !errno_enabled && math_errhandling & MATH_ERREXCEPT;
#endif

static inline void clear_fe_except() {
    if (check_fe_except) {
        std::feclearexcept(FE_ALL_EXCEPT);
    }
}

static double errcheck(double, const char*);

void hoc_atan2(void) {
    double d;
    d = atan2(*hoc_getarg(1), *hoc_getarg(2));
    hoc_ret();
    hoc_pushx(d);
}

double Log(double x) {
    clear_fe_except();
    return errcheck(log(x), "log");
}

double Log10(double x) {
    clear_fe_except();
    return errcheck(log10(x), "log10");
}

/* used by nmodl and other c, c++ code */
double hoc_Exp(double x) {
    if (x < -700.) {
        return 0.;
    } else if (x > 700 && nrn_feenableexcept_ == 0) {
        errno = ERANGE;
        if (check_fe_except) {
            std::feraiseexcept(FE_OVERFLOW);
        }
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
        if (check_fe_except) {
            std::feraiseexcept(FE_OVERFLOW);
        }
        return errcheck(exp(700.), "exp");
    }
    clear_fe_except();
    return errcheck(exp(x), "exp");
}

double Sqrt(double x) {
    clear_fe_except();
    return errcheck(sqrt(x), "sqrt");
}

double Pow(double x, double y) {
    clear_fe_except();
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
    if ((errno_enabled && errno == EDOM) || (check_fe_except && std::fetestexcept(FE_INVALID))) {
        clear_fe_except();
        errno = 0;
        hoc_execerror(s, "argument out of domain");
    } else if ((errno_enabled && errno == ERANGE) ||
               (check_fe_except &&
                (std::fetestexcept(FE_DIVBYZERO) || std::fetestexcept(FE_OVERFLOW) ||
                 std::fetestexcept(FE_UNDERFLOW)))) {
        clear_fe_except();
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
    errno = 0;
    return 0;
}
