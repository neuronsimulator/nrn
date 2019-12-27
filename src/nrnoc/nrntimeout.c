#include <../../nrnconf.h>
#include <nrnmpi.h>
#if NRNMPI
#if defined(HAVE_SETITIMER) && defined(HAVE_SIGACTION)

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <section.h>

void (*nrntimeout_call)();
static double told;
static struct itimerval value;
#if !defined(BLUEGENE)
static struct sigaction act, oact;
#endif

#define NRNTIMEOUT_DEBUG 0
#if NRNTIMEOUT_DEBUG
extern double nrn_time();
static old_nrn_time;
#endif

static void timed_out(int sig) {
#if NRNTIMEOUT_DEBUG
double z = nrn_time();
printf("timed_out(%d) wall_elapse=%g told=%g t=%g\n", sig, z-old_nrn_time, told, nrn_threads->_t);
old_nrn_time = z;
#endif
	if (nrn_threads->_t == told) { /* nothing has been accomplished since last signal*/
		printf("nrn_timeout t=%g\n", nrn_threads->_t);
		if (nrntimeout_call) {
			(*nrntimeout_call)();
		}
		nrnmpi_abort(0);
	}
	told = nrn_threads->_t;
}

void nrn_timeout(int seconds) {
	if (nrnmpi_myid != 0) { return; }
#if NRNTIMEOUT_DEBUG
printf("nrn_timeout(%d) t=%g\n", seconds, nrn_threads->_t);
old_nrn_time = nrn_time();
#endif
#if BLUEGENE
	if (seconds) {
		told = nrn_threads->_t;
		signal(SIGALRM, timed_out);
	}else{
		signal(SIGALRM, SIG_DFL);
	}
#else
	if (seconds) {
		told = nrn_threads->_t;
		act.sa_handler = timed_out;
		act.sa_flags = SA_RESTART;
		if(sigaction(SIGALRM, &act, &oact)) {
			printf("sigaction failed\n");
			nrnmpi_abort(0);
		}
	}else{
		sigaction(SIGALRM, &oact, (struct sigaction*)0);
	}
#endif
	value.it_interval.tv_sec = seconds;
	value.it_interval.tv_usec = 0;
	value.it_value.tv_sec = seconds;
	value.it_value.tv_usec = 0;
	if(setitimer(ITIMER_REAL, &value, (struct itimerval*)0)) {
		printf("setitimer failed\n");
		nrnmpi_abort(0);
	}
	
}

#else

void nrn_timeout(int seconds) { }

#endif /* not HAVE_SETITIMER */

#endif /*NRNMPI*/
