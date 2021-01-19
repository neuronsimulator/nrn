/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mpi/nrnmpi.h"

#if NRNMPI

#include <csignal>
#include <sys/time.h>

/* if you are using any sampling based profiling tool,
setitimer will conflict with profiler. In that case,
user can disable setitimer which is just safety for
deadlock situations */
namespace coreneuron {
#if (defined(DISABLE_TIMEOUT) || defined(MINGW))

void nrn_timeout(int seconds) {}

#else

void (*nrntimeout_call)();
static double told;
static struct itimerval value;
static struct sigaction act, oact;

static void timed_out(int sig) {
    (void) sig; /* unused */
#if DEBUG
    printf("timed_out told=%g t=%g\n", told, t);
#endif
    if (nrn_threads->_t == told) { /* nothing has been accomplished since last signal*/
        printf("nrn_timeout t=%g\n", nrn_threads->_t);
        if (nrntimeout_call) {
            (*nrntimeout_call)();
        }
        nrn_abort(0);
    }
    told = nrn_threads->_t;
}

void nrn_timeout(int seconds) {
    if (nrnmpi_myid != 0) {
        return;
    }
#if DEBUG
    printf("nrn_timeout %d\n", seconds);
#endif
    if (seconds) {
        told = nrn_threads->_t;
        act.sa_handler = timed_out;
        act.sa_flags = SA_RESTART;
        if (sigaction(SIGALRM, &act, &oact)) {
            printf("sigaction failed\n");
            nrn_abort(0);
        }
    } else {
        sigaction(SIGALRM, &oact, (struct sigaction*) 0);
    }
    value.it_interval.tv_sec = seconds;
    value.it_interval.tv_usec = 0;
    value.it_value.tv_sec = seconds;
    value.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &value, (struct itimerval*) 0)) {
        printf("setitimer failed\n");
        nrn_abort(0);
    }
}

#endif /* DISABLE_TIMEOUT */
}  // namespace coreneuron

#endif /*NRNMPI*/
