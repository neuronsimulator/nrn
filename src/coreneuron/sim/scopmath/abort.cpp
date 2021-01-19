/******************************************************************************
 *
 * File: abort.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "coreneuron/utils/nrnoc_aux.hpp"

#ifndef LINT
static char RCSid[] = "abort.c,v 1.2 1997/08/30 14:32:00 hines Exp";
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

#include <setjmp.h>
#include <stdio.h>
#include "errcodes.h"
namespace coreneuron {
int abort_run(int code) {
    switch ((code >= 0) ? code : -code) {
        case EXCEED_ITERS:
            puts("Convergence not achieved in maximum number of iterations");
            break;
        case SINGULAR:
            puts("The matrix in the solution method is singular or ill-conditioned");
            break;
        case PRECISION:
            puts(
                "The increment in the independent variable is less than machine "
                "roundoff error");
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
            puts(
                "Could not open forcing function file\nThe model cannot be run "
                "without the forcing function");
            break;
        case NEG_ARG:
            puts("Cannot compute factorial of negative argument");
            break;
        case RANGE:
            puts(
                "Value of variable is outside the range of the forcing function data "
                "table");
            break;
        default:
            puts("Origin of error is unknown");
    }
    hoc_execerror("scopmath library error", (char*) 0);
    return 0;
}
}  // namespace coreneuron
