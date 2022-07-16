#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/parallel.cpp,v 1.5 1997/03/13 14:18:17 hines Exp */
#if MAC
#define OCSMALL 1
#define WIN32   1
#endif

#if !OCSMALL
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(__APPLE__)
#include <crt_externs.h>
#endif
#include "hoc.h"
#include "parse.hpp" /* OBJECTVAR */

static int parallel_seen;

static double* pval;   /* pointer to loop counter value */
static double end_val; /* value to assign loop counter upon completion of loop */

#define NUM_ARGS 256
static char* parallel_argv;
static char* parallel_envp;
static int sargv = 0, senvp = 0;
#endif /*!OCSMALL*/

int parallel_sub = 0;
int parallel_val; /* for use with parallel neuron (see hoc.cpp) */

/*
  stack has final, initial, symbol
  and should contain these on exit in order to execute the following shortfor
*/


void hoc_parallel_begin(void) {
#if !OCSMALL
    Symbol* sym;
    double first, last;
    char* method;
    int i, j;


    last = xpop();
    first = xpop();
    sym = spop();
    pushs(sym);

    method = getenv("NEURON_PARALLEL_METHOD");
    if (!method) {
        pushx(first);
        pushx(last);
        return;
    }
    if (parallel_seen++) {
        hoc_warning("Only one parallel loop per batch run allowed.",
                    "This loop is being executed serially");
        pushx(first);
        pushx(last);
        return;
    }

    if (!parallel_sub) { /* if 0 then master */
        /* the master instance executes the following portion of the loop */
        for (i = ((int) first) + 1; i <= (int) last; i++) {
            // parallel_argv will be null on windows, which triggers an ICE in
            // some cases, see #1840.
            if (parallel_argv) {
                char buf[10], *pnt = parallel_argv;
                /* increment pnt to "00000" */
                for (j = 0; j < 2; j++) {
                    /*EMPTY*/
                    while (*pnt++)
                        ;
                }
                /* replace "00000" with actual value */
                sprintf(buf, "%5d", i);
                strcpy(pnt, buf);
            }
            /* farm-out all but the first instance of the loop */
        }

        /* run in serial */
        pushx(first);
        pushx(last);

        /* assign value of symbol to last+1 as would be upon exiting a serial loop */
        if (!ISARRAY(sym)) {
            if (sym->subtype == USERDOUBLE) {
                pval = sym->u.pval;
            } else {
                pval = OPVAL(sym);
            }
        } else {
            if (sym->subtype == USERDOUBLE) {
                pval = sym->u.pval + araypt(sym, SYMBOL);
            } else {
                pval = OPVAL(sym) + araypt(sym, OBJECTVAR);
            }
        }
        end_val = last + 1;

    } else {
        /* the subsidiary instances do remaining contiguous blocks of the loop */

        /* only do "parallel_val" pass though loop */
        pushx((double) parallel_val);
        pushx((double) parallel_val);
    }
#endif
}

void hoc_parallel_end(void) {
#if !OCSMALL
    /* need to exit after for-loop for all sub-processes */
    if (parallel_sub) {
        hoc_final_exit();
        exit(0);
    } else {
        /* assign loop counter the proper end value */
        *pval = end_val;
    }
#endif
}


int parallel_hoc_main(int i) {
#if !OCSMALL
    /*ARGSUSED*/
    const char **_largv, **_lenvp;
    const char* pnt;
    char *targv, *tenvp;
    int j, _largc;
    _largv = static_cast<const char**>(emalloc(NUM_ARGS * sizeof(char*)));
    _lenvp = static_cast<const char**>(emalloc(NUM_ARGS * sizeof(char*)));
    targv = static_cast<char*>(emalloc(sargv));
    tenvp = static_cast<char*>(emalloc(senvp));

    pnt = targv;
    for (j = 0; *pnt; j++) {
        _largv[j] = pnt;
        /*EMPTY*/
        while (*pnt++)
            ;
    }
    _largc = j;

    pnt = tenvp;
    for (j = 0; *pnt; j++) {
        _lenvp[j] = pnt;
        /*EMPTY*/
        while (*pnt++)
            ;
    }

    /* run is killed at end of parallel for-loop (hoc_parallel_end()) */
    hoc_main1(_largc, _largv, _lenvp);
#endif
    return 0;
}

void save_parallel_argv(int argc, const char** argv) {
    /* first arg is program, save 2 & 3 for -parallel flags */
#if !defined(WIN32)
    const char* pnt;
    int j;

    /* count how long the block of memory should be */
    for (j = 0; j < argc && (strcmp(argv[j], "-") != 0); j++) {
        pnt = argv[j];
        while (*pnt++) {
            sargv++;
        }
        sargv++; /* add room for '\0' */
    }
    sargv += 16; /* add 10 for "-parallel" and 6 for val ("00000") */

    /* need room for extra '\0' at end, each space is of size (char) */
    sargv = (sargv + 1) * sizeof(char);

    /* malloc blocks of memory */
    parallel_argv = static_cast<char*>(emalloc(sargv));

#if 0
    /* place the strings into the memory block separated by '\0' */
    strcpy((pnt = parallel_argv), argv[0]); 
    /*EMPTY*/
    while (*pnt++);
    strcpy(pnt, "-parallel     0"); /* pad val with 00000 (assume max int) */
    pnt += 16;
    for (j = 1; j < argc && (strcmp(argv[j], "-") != 0); j++) {
	if (strcmp(argv[j], "-parallel") == 0) {
	    /* if sub-process then get val, increment past "-parallel" and "val" */
	    parallel_sub = 1; 
	    parallel_val = atoi(argv[++j]);

	} else {
	    strcpy(pnt, argv[j]); 
	    /*EMPTY*/
	    while (*pnt++);
	}
    }
    *pnt = '\0'; /* place extra '\0' at end */
#endif
#endif
}
