#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/clamp.cpp,v 1.2 1997/08/15 13:04:10 hines Exp */

/* modified from fstim.cpp */
/* 4/9/2002 modified to conform to new treeset.cpp */

/*
fclamp(maxlevel, loc)
 allocates space for maxlevel level changes all at location loc
 previously existing clamp is released.
 All clamp levels are initialized to clamp_resist series resistance.
 and 0 duration.  After the final level change the clamp is off.

fclamp(i, duration, vc)
 the maxlevel level changes concatenate. This function specifies
 the magnitude and duration of the ith level.

fclampv()
 the value of the clamp potential at the global time t

fclampi()
 the clamp current at the global time t

*/

#include <stdlib.h>
#include "neuron.h"
#include "section.h"
#include "nrniv_mf.h"


static double loc;
static Node* pnd;
static Section* sec;
static double gtemp;
static int maxlevel = 0;                /* size of clamp array */
static double *duration, *vc, *tswitch; /* maxlevel of them */
static int oldsw = 0;

static double clampval();

extern void clamp_prepare();

#define nt_t nrn_threads->_t

void print_clamp() {
    int i;

    if (maxlevel == 0)
        return;
#ifndef _OMPSS_
    Printf(
        "%s fclamp(%d, %g) /* Second arg is location */\n\
/* fclamp( #, duration(ms), magnitude(mV)) ; clamp_resist = %g */\n",
        secname(sec),
        maxlevel,
        loc,
        clamp_resist);
#else
    Printf(
        "%s fclamp(%d, %g) /* Second arg is location */\nfclamp( #, duration(ms), magnitude(mV)) ; "
        "clamp_resist = %g */\n",
        secname(sec),
        maxlevel,
        loc,
        clamp_resist);
#endif
    for (i = 0; i < maxlevel; i++) {
        Printf("   fclamp(%2d,%13g,%14g)\n", i, duration[i], vc[i]);
    }
}

void fclampv(void) {
    if (maxlevel) {
        hoc_retpushx(clampval());
    } else {
        hoc_retpushx(0.);
    }
}

void fclampi(void) {
    double v;

    if (maxlevel) {
        v = clampval();
        if (gtemp) {
            hoc_retpushx(-(NODEV(pnd) - v) / clamp_resist);
        } else {
            hoc_retpushx(0.);
        }
    } else {
        hoc_retpushx(0.);
    }
}

static void free_clamp(void);

void fclamp(void) {
    int i;

    if (nrn_nthread > 1) {
        hoc_execerror("fsyn does not allow threads", "");
    }
    /* two args are maxlevel, loc */
    /* three args are level, duration, vc */
    i = chkarg(1, 0., 10000.);
    if (ifarg(3)) {
        int num;
        if (i >= maxlevel) {
            hoc_execerror("level index out of range", (char*) 0);
        }
        duration[i] = chkarg(2, 0., 1e21);
        vc[i] = *getarg(3);

        tswitch[0] = -1e-9;
        for (num = 0; num < maxlevel; num++) {
            tswitch[num + 1] = tswitch[num] + duration[num];
        }
        oldsw = 0;
        hoc_retpushx(tswitch[maxlevel - 1]);
    } else {
        free_clamp();
        maxlevel = i;
        if (maxlevel) {
            duration = (double*) emalloc((unsigned) (maxlevel * sizeof(double)));
            vc = (double*) emalloc((unsigned) (maxlevel * sizeof(double)));
            tswitch = (double*) emalloc((unsigned) ((maxlevel + 1) * sizeof(double)));
            for (i = 0; i < maxlevel; i++) {
                duration[i] = 0.;
                vc[i] = 0.;
                tswitch[i] = -1e-9;
            }
            tswitch[maxlevel] = -1e-9;
            loc = chkarg(2, 0., 1.);
            sec = chk_access();
            section_ref(sec);
            clamp_prepare();
        }
    }
    hoc_retpushx(0.);
}

static void free_clamp(void) {
    if (maxlevel) {
        free((char*) duration);
        free((char*) vc);
        free((char*) tswitch);
        maxlevel = 0;
        section_unref(sec);
        sec = 0;
    }
}

void clamp_prepare(void) /*fill in the section info*/
{
    double area;

    if (!maxlevel) {
        return;
    }
    if (sec->prop) {
        pnd = node_ptr(sec, loc, &area);
    } else {
        free_clamp();
        return;
    }
    if (clamp_resist <= 0) {
        hoc_execerror("clamp_resist must be > 0 in megohms", (char*) 0);
    }
}

void activclamp_rhs(void) {
    double v;
    if (!maxlevel) {
        return;
    }
    v = clampval();
#if EXTRACELLULAR
    if (pnd->extnode) {
        NODERHS(pnd) += gtemp * (v - NODEV(pnd) - pnd->extnode->v[0]);
    } else {
        NODERHS(pnd) += gtemp * (v - NODEV(pnd));
    }

#else
    NODERHS(pnd) += gtemp * (v - NODEV(pnd));
#endif
}

void activclamp_lhs(void) {
    double v;
    if (!maxlevel) {
        return;
    }
    NODED(pnd) += gtemp;
}

static double clampval(void) {
    gtemp = 1.e2 / clamp_resist / NODEAREA(pnd);
    for (;;) {
        at_time(nrn_threads, tswitch[oldsw]);
        if (nt_t < tswitch[oldsw]) {
            if (oldsw == 0) {
                break;
            }
            /* for cvode the other was inefficient since t is non-monotonic */
            --oldsw;
        } else {
            if (nt_t < tswitch[oldsw + 1]) {
                break;
            } else {
                if (++oldsw == maxlevel) {
                    --oldsw;
                    gtemp = 0.;
                    break;
                }
            }
        }
    }
    return vc[oldsw];
}
