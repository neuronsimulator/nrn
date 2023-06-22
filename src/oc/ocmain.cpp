#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/ocmain.cpp,v 1.7 1997/07/29 20:23:33 hines Exp */

#include <stdio.h>
#include <stdlib.h>
#include <hocdec.h>

extern const char* neuron_home;

#if defined(WIN32)
void* cvode_pmem;
extern void setneuronhome(const char*);
#endif

static void setnrnhome(const char* arg) {
#ifndef WIN32
    /*
     Gary Holt's first pass at this was:

     Set the NEURONHOME environment variable.  This should override any setting
     in the environment, so someone doesn't accidently use data files from an
     old version of neuron.

     But I have decided to use the environment variable if it exists
    */
    neuron_home = getenv("NEURONHOME");
    if (!neuron_home) {
#if defined(HAVE_SETENV)
        setenv("NEURONHOME", NEURON_DATA_DIR, 1);
        neuron_home = NEURON_DATA_DIR;
#else
#error "I don't know how to set environment variables."
// Maybe in this case the user will have to set it by hand.
#endif
    }

#else  // Not unix:
    setneuronhome(arg);
#endif
}

int main(int argc, const char** argv, const char** envp) {
    int err;
    setnrnhome(argv[0]);
    err = hoc_main1(argc, argv, envp);
    if (!err) {
        hoc_final_exit();
    }
#if EXPRESS
    exit(0);
#else
    return err;
#endif
}


void hoc_single_event_run() { /* for interviews, ivoc make use of own main */
#if INTERVIEWS && 0
    single_event_run();
#endif
    hoc_ret();
    hoc_pushx(0.);
}

int run_til_stdin() {
    return 1;
}
void hoc_notify_value() {}

#ifdef WIN32
void ivcleanup() {}
void ivoc_win32_cleanup() {}
int bad_install_ok;
#endif
