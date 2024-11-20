#include "oc_ansi.h"
#ifndef __INTEL_LLVM_COMPILER
#pragma STDC FENV_ACCESS ON
#endif

#include <../../nrnconf.h>
/* a fake change */
/* /local/src/master/nrn/src/oc/math.cpp,v 1.6 1999/07/16 13:43:10 hines Exp */

#include "nrnmpiuse.h"
#include "ocfunc.h"
#include "nrnassrt.h"

#include <cfenv>
#include <cmath>
#include <cerrno>
#include <cstdio>

#if NRN_ENABLE_ARCH_INDEP_EXP_POW
#include <mpfr.h>
#endif

#define EPS         hoc_epsilon
#define MAXERRCOUNT 5

int hoc_errno_count;

#ifdef MINGW
static const auto errno_enabled = true;
static const auto check_fe_except = false;
#elif defined(NRN_CHECK_FE_EXCEPTIONS)
static constexpr auto errno_enabled = false;
static constexpr auto check_fe_except = true;
#ifdef math_errhandling
// LLVM-based Intel compiles don't define math_errhandling when -fp-model=fast
static_assert(math_errhandling & MATH_ERREXCEPT);
#endif
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

double hoc_Log(double x) {
    clear_fe_except();
    return errcheck(log(x), "log");
}

double hoc_Log10(double x) {
    clear_fe_except();
    return errcheck(log10(x), "log10");
}

#if NRN_ENABLE_ARCH_INDEP_EXP_POW

static double accuracy32(double val) {
    int ex;
    double mant = frexp(val, &ex);
    // round to about 32 bits after .
    double prec = 4294967296.0;
    double result = mant * prec;
    result = round(result);
    result /= prec;
    result = ldexp(result, ex);
    return result;
}

static double pow_arch_indep(double x, double y) {
    mpfr_prec_t prec = 53;
    mpfr_rnd_t rnd = MPFR_RNDN;

    mpfr_t x_, y_;
    mpfr_init2(x_, prec);
    mpfr_init2(y_, prec);

    mpfr_set_d(x_, x, rnd);
    mpfr_set_d(y_, y, rnd);

    mpfr_pow(x_, x_, y_, rnd);

    double r = mpfr_get_d(x_, rnd);

    mpfr_clear(x_);
    mpfr_clear(y_);

    return r;
}

static double exp_arch_indep(double x) {
    mpfr_prec_t prec = 53;
    mpfr_rnd_t rnd = MPFR_RNDN;

    mpfr_t x_;
    mpfr_init2(x_, prec);
    mpfr_set_d(x_, x, rnd);
    mpfr_exp(x_, x_, rnd);
    double r = mpfr_get_d(x_, rnd);
    mpfr_clear(x_);
    return r;
}

static double pow_precision32(double x, double y) {
    return accuracy32(pow(x, y));
}

static double exp_precision32(double x) {
    return accuracy32(exp(x));
}

static double (*pow_ptr)(double x, double y) = pow;
static double (*pow_ieee_ptr)(double x,
                              double y) = pow;  // avoid error! Maybe because of c++ overloading.
static double (*exp_ptr)(double x) = exp;

int nrn_use_exp_pow_precision(int style) {
    if (style == 0) {  // default IEEE
        pow_ptr = pow;
        exp_ptr = exp;
    } else if (style == 1) {  // 53 bit mpfr
        pow_ptr = pow_arch_indep;
        exp_ptr = exp_arch_indep;
    } else if (style == 2) {  // 32 bit truncation
        pow_ptr = pow_precision32;
        exp_ptr = exp_precision32;
    }
    return style;
}

#endif  // NRN_ENABLE_ARCH_INDEP_EXP_POW

void hoc_use_exp_pow_precision() {
    int style = 0;
    if (ifarg(1)) {
        style = chkarg(1, 0.0, 2.0);
    }
#if NRN_ENABLE_ARCH_INDEP_EXP_POW
    style = nrn_use_exp_pow_precision(style);
#else
    style = 0;
#endif  // NRN_ENABLE_ARCH_INDEP_EXP_POW
    hoc_ret();
    hoc_pushx(double(style));
}

// Try to overcome difference between linux and windows.
// by rounding mantissa to 32 bit accuracy or using
// hopefully arch independent mpfr for exp and pow.

double hoc_pow(double x, double y) {
#if NRN_ENABLE_ARCH_INDEP_EXP_POW
    return (*pow_ptr)(x, y);
#else
    return pow(x, y);
#endif
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

#if NRN_ENABLE_ARCH_INDEP_EXP_POW
    return (*exp_ptr)(x);
#else
    return exp(x);
#endif
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

double hoc_Sqrt(double x) {
    clear_fe_except();
    return errcheck(sqrt(x), "sqrt");
}

double hoc_Pow(double x, double y) {
    clear_fe_except();
    return errcheck(hoc_pow(x, y), "exponentiation");
}

double hoc_integer(double x) {
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
