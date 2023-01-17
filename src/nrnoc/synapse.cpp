#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/synapse.cpp,v 1.2 1997/08/15 13:04:13 hines Exp */
/* modified from fstim.cpp */

/*
fsyn(maxnum)
 allocates space for maxnum synapses. Space for
 previously existing synapses is released. All synapses initialized to
 0 maximum conductance.

fsyn(i, loc, delay, tau, gmax, erev)
 The ith synapse is injected at parameter `loc'
 different synapses do not concatenate but can ride on top of
 each other. delay refers to onset of synapse relative to t=0
 delay and duration are in msec.
 stim in namps.

 a synaptic current defined by
    i = g * (v - erev) 	i(nanoamps), g(microsiemens);
    where
     g = 0 for t < delay and
     g = gmax * (t - delay)/tau * exp(-(t - delay - tau)/tau)
      for t > onset
 this has the property that the maximum value is gmax and occurs at
  t = delay + tau.

fsyni(i)
 returns synaptic current for ith synapse at the value of the
 global time t in units of nanoamps.

fsyng(i)
 returns synaptic conductance for ith synapse at the value of the
 global time t.

*/

#include <stdlib.h>
#include "neuron.h"
#include "section.h"
#include "nrniv_mf.h"
#include <math.h>


#define nt_t nrn_threads->_t

/* impress the stimulus code to do synapses */
typedef struct Stimulus {
    double loc;      /* parameter location (0--1) */
    double delay;    /* value of t in msec for onset */
    double duration; /* turns off at t = delay + duration */
    double mag;      /* conductance in microsiemens */
    double erev;
    double mag_seg; /* value added to rhs, depends on area of seg*/
    double g;       /* holds conductance when current calculated */
    Node* pnd;      /* segment location */
    Section* sec;
} Stimulus;

static int maxstim = 0; /* size of stimulus array */
static Stimulus* pstim; /* pointer to stimulus array */
static void free_syn(void);

static void stim_record(int);

void print_syn(void) {
    int i;

    if (maxstim == 0)
        return;
    /*SUPPRESS 440*/
    Printf("fsyn(%d)\n/* section	fsyn( #, loc, delay(ms), tau(ms), conduct(uS), erev(mV)) */\n",
           maxstim);
    for (i = 0; i < maxstim; i++) {
        Printf("%-15s fsyn(%2d,%4g,%10g,%8g,%14g,%9g)\n",
               secname(pstim[i].sec),
               i,
               pstim[i].loc,
               pstim[i].delay,
               pstim[i].duration,
               pstim[i].mag,
               pstim[i].erev);
    }
}

static double alpha(double x) {
    if (x > 0.0 && x < 10.0) {
        return x * exp(-x + 1.0);
    }
    return 0.0;
}


static double stimulus(int i) {
    double x, g;

    if ((g = pstim[i].mag_seg) == 0.0) {
        pstim[i].g = 0.0;
        return 0.0;
    }
    at_time(nrn_threads, pstim[i].delay);
    x = (nt_t - pstim[i].delay) / pstim[i].duration;
    pstim[i].g = g * alpha(x);
    return pstim[i].g * (NODEV(pstim[i].pnd) - pstim[i].erev);
}

void fsyni(void) {
    int i;
    double cur;

    i = chkarg(1, 0., (double) (maxstim - 1));
    if ((cur = stimulus(i)) != 0.) {
        cur *= pstim[i].mag / pstim[i].mag_seg;
    }
    hoc_retpushx(cur);
}

void fsyng(void) {
    int i;
    double g = 0.0;

    i = chkarg(1, 0., (double) (maxstim - 1));
    IGNORE(stimulus(i));
    g = pstim[i].g;
    if (g != 0.) {
        g *= pstim[i].mag / pstim[i].mag_seg;
    }
    hoc_retpushx(g);
}

void fsyn(void) {
    int i;

    if (nrn_nthread > 1) {
        hoc_execerror("fsyn does not allow threads", "");
    }
    i = chkarg(1, 0., 10000.);
    if (ifarg(2)) {
        if (i >= maxstim) {
            hoc_execerror("index out of range", (char*) 0);
        }
        pstim[i].loc = chkarg(2, 0., 1.);
        pstim[i].delay = chkarg(3, 0., 1e21);
        pstim[i].duration = chkarg(4, 0., 1e21);
        pstim[i].mag = *getarg(5);
        pstim[i].erev = *getarg(6);
        pstim[i].sec = chk_access();
        section_ref(pstim[i].sec);
        stim_record(i);
    } else {
        free_syn();
        maxstim = i;
        if (maxstim) {
            pstim = (Stimulus*) emalloc((unsigned) (maxstim * sizeof(Stimulus)));
        }
        for (i = 0; i < maxstim; i++) {
            pstim[i].loc = 0;
            pstim[i].mag = 0.;
            pstim[i].delay = 1e20;
            pstim[i].duration = 0.;
            pstim[i].erev = 0.;
            pstim[i].sec = 0;
            stim_record(i);
        }
    }
    hoc_retpushx(0.);
}

static void free_syn(void) {
    int i;
    if (maxstim) {
        for (i = 0; i < maxstim; ++i) {
            if (pstim[i].sec) {
                section_unref(pstim[i].sec);
            }
        }
        free((char*) pstim);
        maxstim = 0;
    }
}

static void stim_record(int i) /*fill in the section info*/
{
    double area;
    Section* sec;

    sec = pstim[i].sec;
    if (sec) {
        if (sec->prop) {
            pstim[i].pnd = node_ptr(sec, pstim[i].loc, &area);
            pstim[i].mag_seg = 1.e2 * pstim[i].mag / area;
        } else {
            section_unref(sec);
            pstim[i].sec = 0;
        }
    }
}

void synapse_prepare(void) {
    int i;

    for (i = 0; i < maxstim; i++) {
        stim_record(i);
    }
}

void activsynapse_rhs(void) {
    int i;
    for (i = 0; i < maxstim; i++) {
        if (pstim[i].sec) {
            NODERHS(pstim[i].pnd) -= stimulus(i);
        }
    }
}

void activsynapse_lhs() {
    int i;

    for (i = 0; i < maxstim; i++) {
        if (pstim[i].sec) {
            NODED(pstim[i].pnd) += pstim[i].g;
        }
    }
}
