#include <../../nrnconf.h>
/* a fake change */
/* /local/src/master/nrn/src/oc/math.c,v 1.6 1999/07/16 13:43:10 hines Exp */

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include "nrnmpiuse.h"
# include	"redef.h"

/*extern int	errno;*/
#define EPS hoc_epsilon
extern double EPS;
#define MAXERRCOUNT 5
int hoc_errno_count;

#if LINT
int	errno;
#endif

#if _CRAY
#define log logl
#define log10 log10l
#define exp expl
#define sqrt sqrtl
#define pow powl
#endif

double	errcheck();

int hoc_atan2() {
	double d, *getarg();
	d = atan2(*getarg(1), *getarg(2));
	hoc_ret();
	pushx(d);
	return 0;
}

double
Log(x)
	double x;
{
	return	errcheck(log(x), "log");
}

double
Log10(x)
	double x;
{
	return errcheck(log10(x), "log10");
}

/* used by nmodl and other c, c++ code */
double
hoc_Exp(x)
	double x;
{
	if (x < -700.) {
		return 0.;
	}else if (x > 700) {
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
double
hoc1_Exp(x)
	double x;
{
	if (x < -700.) {
		return 0.;
	}else if (x > 700) {
		errno = ERANGE;
		return errcheck(exp(700.), "exp");
	}
	return errcheck(exp(x), "exp");
}

double
Sqrt(x)
	double x;
{
	return errcheck(sqrt(x), "sqrt");
}

double
Pow(x, y)
	double x, y;
{
	return errcheck(pow(x, y), "exponentiation");
}

double
integer(x)
	double x;
{
	if (x < 0) {
		return (double)(long)(x - EPS);
	}else{
		return (double)(long)(x + EPS);
	}
}

double
errcheck(d, s)	/* check result of library call */
	double d;
	char *s;
{
	if (errno == EDOM)
	{
		errno = 0;
		execerror(s, "argument out of domain");
	}
	else if (errno == ERANGE)
	{
		errno = 0;
#if 0
		execerror(s, "result out of range");
#else
		if (++hoc_errno_count > MAXERRCOUNT) {
		}else{
			hoc_warning(s, "result out of range");
			if (hoc_errno_count == MAXERRCOUNT) {
fprintf(stderr, "No more errno warnings during this execution\n");
			}
		}
#endif
	}
	return d;
}

int hoc_errno_check() {
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
#if BLUEGENE
                 if (errno == ENOSYS) {
                        errno = 0;
                        return 0;
                 }
#endif
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
#if defined(CYGWIN)
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
fprintf(stderr, "oc: This error occurs often from LINDA and thus will not be further reported.\n"); 
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
