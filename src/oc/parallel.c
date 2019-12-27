#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/parallel.c,v 1.5 1997/03/13 14:18:17 hines Exp */
#if MAC
	#define OCSMALL 1
	#define WIN32 1
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
#include "parse.h" /* OBJECTVAR */

static int parallel_seen;

static double *pval; /* pointer to loop counter value */
static double end_val; /* value to assign loop counter upon completion of loop */

#define NUM_ARGS 256
static char *parallel_argv;
static char *parallel_envp;
static int sargv=0, senvp=0;
#endif /*!OCSMALL*/

int parallel_sub=0;
int parallel_val; /* for use with parallel neuron (see hoc.c) */

/*
  stack has final, initial, symbol
  and should contain these on exit in order to execute the following shortfor
*/

void hoc_parallel_begin(void) {
#if !OCSMALL
		  Symbol *sym;
        double first, last;
        char *method, *getenv();
	int parallel_hoc_main();
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
	    for (i = ((int)first)+1; i <= (int)last; i++) {
		char buf[10], *pnt = parallel_argv;

		/* increment pnt to "00000" */
		for (j = 0; j < 2; j++) {
		    /*EMPTY*/
		    while (*pnt++);
		}
		
		/* replace "00000" with actual value */
		sprintf(buf, "%5d", i);
		strcpy(pnt, buf);

		/* farm-out all but the first instance of the loop */
#if LINDA
		/* place arguments for eval() into tuple space, Linda
		   doesn't seem to want to let the fxn in an eval take 
		   arrays as args */
		__linda_out("parallel sargs", sargv, senvp);
 		__linda_out("parallel args", parallel_argv:sargv, parallel_envp:senvp); 
 		__linda_eval("parallel run", parallel_hoc_main(i), i);
#endif
	    }
	    
#if LINDA
	    /* do first pass though loop on master node (first to first) */
	    pushx(first);
	    pushx(first);
#else
	    /* run in serial if not LINDA */
	    pushx(first);
	    pushx(last);
#endif

	    /* block until all instances of loop have finished */
#if LINDA
	    i = (int)last - (int)first;
	    while (i-- > 0) {
		int err_val, err_num;
		
		__linda_in("parallel run", ?err_val, ?err_num);
		/* could test err_val != 0 but currently will always equal 0 */
	    }
#endif

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
	    pushx((double)parallel_val);
	    pushx((double)parallel_val);

	}
#endif
}

void hoc_parallel_end(void) {
#if !OCSMALL
	/* need to exit after for-loop for all sub-processes */
	if (parallel_sub) {
	    hoc_final_exit();
#if LINDA
	    lexit(0);
#else
	    exit(0);
#endif

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
	_largv = emalloc(NUM_ARGS*sizeof(char*));
	_lenvp = emalloc(NUM_ARGS*sizeof(char*));
#if LINDA
	char name[20];

	gethostname(name, 20);
	Fprintf(stderr, "\nLaunching sub-process on %s.\n\t1\n", name);

 	__linda_in("parallel sargs", ?sargv, ?senvp); 
#endif
	targv = emalloc(sargv);
	tenvp = emalloc(senvp);
	/* pointers need to point to memory that will be filled by __linda_in() */
#if LINDA
  	__linda_in("parallel args", ?targv:, ?tenvp:);  
#endif

	pnt = targv;
	for (j = 0; *pnt; j++) {
 	    _largv[j] = pnt;  
	    /*EMPTY*/
	    while (*pnt++);
	}
	_largc = j;

	pnt = tenvp;
	for (j = 0; *pnt; j++) {
 	    _lenvp[j] = pnt;  
	    /*EMPTY*/
	    while (*pnt++);
	}

	/* run is killed at end of parallel for-loop (hoc_parallel_end()) */
   	hoc_main1(_largc, _largv, _lenvp);   
#endif
	return 0;
}

void save_parallel_argv(int argc, const char** argv) {
    /* first arg is program, save 2 & 3 for -parallel flags */
#if !defined(WIN32)
	const char *pnt;
    int j;

    /* count how long the block of memory should be */
    for (j = 0; j < argc && (strcmp(argv[j], "-") != 0); j++) {
	pnt = argv[j];
	while (*pnt++) { sargv++; }
	sargv++; /* add room for '\0' */
    }
    sargv += 16; /* add 10 for "-parallel" and 6 for val ("00000") */

    /* need room for extra '\0' at end, each space is of size (char) */
    sargv = (sargv + 1) * sizeof(char);

    /* malloc blocks of memory */
    parallel_argv = emalloc(sargv);

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

void save_parallel_envp(void) { 
#if LINDA
#if !defined(__APPLE__)
	extern char** environ;
	char** envp = environ;
#endif
#if defined(__APPLE__)
	char** envp = (*_NSGetEnviron());
#endif
	 char *pnt;
    int j;
	char** envp = environ;

    /* count how long the block of memory should be */
    for (j = 0; envp[j]; j++) {
	pnt = envp[j];
	while (*pnt++) { senvp++; }
	senvp++; /* add room for '\0' */
    }

    /* need room for extra '\0' at end, each space is of size (char) */
    senvp = (senvp + 1) * sizeof(char);

    /* malloc blocks of memory */
    parallel_envp = emalloc(senvp);

    /* place the strings into the memory block separated by '\0' */
    pnt = parallel_envp;
    for (j = 0; envp[j]; j++) {
	strcpy(pnt, envp[j]);
	/*EMPTY*/
	while (*pnt++);
    }
    *pnt = '\0'; /* place extra '\0' at end */
#endif
}
