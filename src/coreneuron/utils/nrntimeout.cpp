/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
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
#if ( defined(DISABLE_TIMEOUT) || defined(MINGW) )

void nrn_timeout(int seconds) {
}

#else

void (*nrntimeout_call)();
static double told;
static struct itimerval value;
static struct sigaction act, oact;

static void timed_out(int sig) {
    (void)sig; /* unused */
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
        sigaction(SIGALRM, &oact, (struct sigaction*)0);
    }
    value.it_interval.tv_sec = seconds;
    value.it_interval.tv_usec = 0;
    value.it_value.tv_sec = seconds;
    value.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &value, (struct itimerval*)0)) {
        printf("setitimer failed\n");
        nrn_abort(0);
    }
}

#endif /* DISABLE_TIMEOUT */
}  // namespace coreneuron

#endif /*NRNMPI*/
