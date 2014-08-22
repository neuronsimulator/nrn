#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: abort.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "abort.c,v 1.2 1997/08/30 14:32:00 hines Exp" ;
#endif

/*-----------------------------------------------------------------------------
 *
 * ABORT_RUN()
 *
 *    Prints out an error message and returns to the main menu if a solver
 *    routine returns a nonzero error code.
 *
 * Calling sequence: abort_run(code)
 *
 * Argument:	code	int	flag for error
 *
 * Returns:
 *
 * Functions called: abs(), cls(), cursrpos(), puts(), gets()
 *
 * Files accessed:
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <setjmp.h>
#include "errcodes.h"

extern void hoc_execerror(const char*, const char*);

int abort_run(code)
int code;
{
#ifndef MAC
    extern int abs();
#endif
	extern int _modl_cleanup();
#if HOC == 0
    extern jmp_buf ibuf;
#endif
    char tmpstr[4];

#if !HOC
    cls();
    cursrpos(10, 0, 0);
#endif

    switch (abs(code))
    {
	case EXCEED_ITERS:
	    puts("Convergence not achieved in maximum number of iterations");
	    break;
	case SINGULAR:
	    puts("The matrix in the solution method is singular or ill-conditioned");
	    break;
	case PRECISION:
	    puts("The increment in the independent variable is less than machine roundoff error");
	    break;
	case CORR_FAIL:
	    puts("The corrector failed to satisfy the error check");
	    break;
	case DIVERGED:
	    puts("The corrector iteration diverged");
	    break;
	case INCONSISTENT:
            puts("Inconsistent boundary conditions");
	    puts("Convergence not acheived in maximum number of iterations");
	    break;
	case BAD_START:
            puts("Poor starting estimate for initial conditions");
	    puts("The matrix in the solution method is singular or ill-conditioned");
	    break;
	case NODATA:
	    puts("No data found in data file");
	    break;
	case NO_SOLN:
	    puts("No solution was obtained for the coefficients");
	    break;
	case LOWMEM:
	    puts("Insufficient memory to run the model");
	    break;
	case DIVCHECK:
	    puts("Attempt to divide by zero");
	    break;
	case NOFORCE:
	    puts("Could not open forcing function file\nThe model cannot be run without the forcing function");
	    break;
	case NEG_ARG:
	    puts("Cannot compute factorial of negative argument");
	    break;
	case RANGE:
	    puts("Value of variable is outside the range of the forcing function data table");
	    break;
	default:
	    puts("Origin of error is unknown");
    }
#if HOC
    _modl_cleanup();
    hoc_execerror("scopmath library error", (char*)0);
#else
    puts("\nPress <Enter> to abort the run");
    gets(tmpstr);
    longjmp(ibuf, 0);
#endif
    return 0;
}

/* define some routines needed for shared libraries to work */
#if HOC

int prterr(const char* s) {
	hoc_execerror(s, "from prterr");
	return 0;
}


#if 0
_modl_set_dt(newdt) double newdt; { printf("ssimplic.c :: _modl_set_dt can't be called\n");
	exit(1);
}
#endif

#endif
