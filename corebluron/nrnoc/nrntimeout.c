/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrnmpi/nrnmpi.h"

#if NRNMPI
#if 1 || (defined(HAVE_SETITIMER) && defined(HAVE_SIGACTION))

#include <signal.h>
#include <sys/time.h>

void (*nrntimeout_call)();
static double told;
static struct itimerval value;
#if !defined(BLUEGENE)
static struct sigaction act, oact;
#endif

static void timed_out(int sig) {
	(void)sig; /* unused */
#if 0
printf("timed_out told=%g t=%g\n", told, t);
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
#if 0
printf("nrn_timeout %d\n", seconds);
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
