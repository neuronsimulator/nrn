#include <../../nrnconf.h>
#include "hoc.h"

#if defined(HAVE_GETTIMEOFDAY)

#include <sys/time.h>
static double start_time = 0.;

#else

#include <time.h>
static time_t start_time=0;	/* time_t is a long */
static time_t stop_time=1;

#endif

# define	Ret(a)	hoc_ret(); hoc_pushx(a);

double nrn_time()
{
#if defined(HAVE_GETTIMEOFDAY)
	int ms10;
	struct timeval x;
	gettimeofday(&x, (struct timezone*)0);
	ms10 = x.tv_usec/10000;
	start_time = (100.*(double)(x.tv_sec) + (double)ms10)/100.;
	return (start_time);
#else
	return ((double)time(&start_time));
#endif
}
int hoc_startsw()
{
	Ret(nrn_time());
}

int hoc_stopsw()
{
#if defined(HAVE_GETTIMEOFDAY)
	double y;
	int ms10;
	struct timeval x;
	gettimeofday(&x, (struct timezone*)0);
	ms10 = x.tv_usec/10000;
	y = (double)(x.tv_sec) + (double)ms10/100.;
	Ret(y - start_time);
	start_time = y;
#else
	if (start_time==0) {
		printf("Must use startsw() first.\n");
		Ret(0.);
	} else {
		time(&stop_time);
		Ret((double)(stop_time - start_time));
	}
#endif
	return;
}

double nrn_timeus()
{
#if defined(HAVE_GETTIMEOFDAY)
	struct timeval x;
	gettimeofday(&x, (struct timezone*)0);
	return ((double)x.tv_sec + .000001*((double)x.tv_usec));
#else
	return 0.;
#endif
}

