#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: force.c
 *
 * Copyright (c) 1987-1990
 *   Duke University
 *
 * This file contains the following routines, plus supporting routines:
 *   force	Reads file of data points and does spline interpolation
 *   stepforce  Like force, except it does not interpolate and returns
 *			the y value for the next lowest x.
 *
 ******************************************************************************/
#include "errcodes.hpp"
#include "newton_struct.h"
#include "../oc/nrnassrt.h"
#include "scoplib.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace neuron::scopmath;  // for errcodes.hpp
typedef struct Spline {
    struct Spline* next; /* link to next spline in list */
    char* name;          /* filename where spline came from */
    int npts;            /* number of base points in spline */
    double* x;           /* x-values of base points */
    double* y;           /* y-values of base points */
    double* width;       /* interval widths */
    double* der;         /* 2nd derivatives at base points */
} Spline;

#define SP0 (Spline*) 0

static Spline* splist = SP0;     /* list of splines */
static Spline* steplist = SP0;   /* list of steps */
static Spline* lastspline = SP0; /* the spline used on previous call */
static Spline* laststep = SP0;   /* the step list used on previous call */

extern int DEFLT;

/**************************************************************************
 *  Abstract force()
 *
 *	Reads a file containing the coordinates of a forcing function in
 *	reference data file format; calls derivs() to evaluate the spacings
 *	of the base points and the second derivatives at the interior base
 *	points; calls spline to perform the interpolation.
 *
 *  Returns: double precision value of interpolated spline function
 *
 *  Calling sequence: force(t, filename)
 *
 *  Arguments:
 *    t, double, value of test variable at which interpoated value
                of forcing function is desired
 *    filename, string, name of file containing coordinates of
 *				forcing function
 *
 *  Functions called: init_force(), getspline(), derivs() , spline()
 *
 *  Files accessed:
 *
 **************************************************************************/
static int init_force(char*);
double force(double t, char* filename) {
    Spline* sp;
    /* Check if Spline structure already made */

    for (sp = splist; sp; sp = sp->next) {
        if (strcmp(filename, sp->name) == 0)
            break;
    }
    if (sp == NULL)
        /* Read file and create Spline structure */
        init_force(filename);
    else
        lastspline = sp;

    /* Check if interpolation point is outside range of base points */

    if (t < lastspline->x[0]) /* Below 1st base point ?*/
    {
        if ((lastspline->x[0] - t) > (0.1 * lastspline->width[0]))
            /* Low by more than 10% of the 1st interval? */
            abort_run(RANGE); /* Yes, abort with error message. */
        else                  /* No, do linear extrapolation */
        {
            return (lastspline->y[0] - (lastspline->x[0] - t) *
                                           (lastspline->y[1] - lastspline->y[0]) /
                                           lastspline->width[0]);
        }
    } else if (t > lastspline->x[lastspline->npts - 1]) /* Above last base point? */
    {
        if ((t - lastspline->x[lastspline->npts - 1]) >
            (0.1 * lastspline->width[lastspline->npts - 2]))
            /* High by more than 10% of the last interval? */
            abort_run(RANGE); /* Yes, abort with error message. */
        else {
            int n; /* No, do linear interpolation. */
            n = lastspline->npts;
            return (lastspline->y[n - 1] + (t - lastspline->x[n - 1]) *
                                               (lastspline->y[n - 1] - lastspline->y[n - 2]) /
                                               lastspline->width[n - 2]);
        }
    }

    /* Within range; perform spline interpolation */

    return (spline(
        lastspline->npts, lastspline->x, lastspline->y, lastspline->width, lastspline->der, t));
}

/**************************************************************************
 *  Abstract stepforce()
 *
 *	Reads a file containing the coordinates of a forcing function in
 *	reference data file format
 *
 *  Returns: double precision value of function, the data value for
 *	x <= t (without interpolation).
 *
 *  Calling sequence: stepforce(t, filename, reset_integ, old_value)
 *
 *  Arguments:
 *    reset_integ, int *, flag output - set to 1 when a discontinuity
 *				occurs
 *    old_value, double *, storage for the last value produced by this
 *				function; used for detecting discontinuities
 *    t, double, value of test variable at which interpoated value
 *				of forcing function is desired
 *    filename, string, name of file containing coordinates of
 *				forcing function
 *
 *  Functions called: init_force(), getspline(), lookup()
 *
 *  Files accessed:
 *
 **************************************************************************/
double stepforce(int* reset_integ, double* old_value, double t, char* filename) {
    double return_val;
    Spline* sp;
    static int initialized = 0;
    extern int _ninits;

    /* Check if Spline structure already made */
    /* Note: this function uses the Spline structure for storing */
    /*       data points, even though there is no interpolation. */

    for (sp = splist; sp; sp = sp->next) {
        if (strcmp(filename, sp->name) == 0)
            break;
    }
    if (sp == NULL)
        /* Read file and create Spline structure */
        init_force(filename);
    else
        lastspline = sp;

    /* Check if starting a new run */

    if (initialized < _ninits) {
        initialized = _ninits;
        *old_value = 0.;
    }

    /* Check if interpolation point is outside range of base points */

    if (t < (lastspline->x[0] - 0.1 * lastspline->width[0]))
        /* Below lower limit: Print message and return to main menu */
        abort_run(RANGE);
    else if (t <=
             (lastspline->x[lastspline->npts - 1] + 0.1 * lastspline->width[lastspline->npts - 2]))
    /* Within tolerance; use point at or below current t value */
    {
        int i;
        for (i = 0; i <= (lastspline->npts - 2); ++i)
            if (t < lastspline->x[i + 1])
                break;
        return_val = lastspline->y[i];
        if (return_val != *old_value)
            *reset_integ = 1;
        *old_value = return_val;
        return (return_val);
    } else
        /* Above upper limit; Print message and return to main menu */
        abort_run(RANGE);
    return 0;
}

/*---------------------------------------------------------------------------
 * init_force()
 *
 *    Reads data file and fills Spline structure
 *
 * Calling sequence: init_force(filename)
 *
 * Argument:	filename    char*	name of file of forcing function values
 *
 * Returns:
 *
 * Functions called: fopen(), rewind(), fclose(), fgets(), gets(), puts(),
 *		     derivs(), getspline(), abort_run()
 *
 * Files accessed: filename (input)
 *-------------------------------------------------------------------------*/
static Spline* getspline(char* filename, int ipt);
static int init_force(char* filename) {
    FILE* pfile;
    extern char* fgets(char*, int, FILE*);
    extern char* gets();
    char tmpstr[81];
    int n, i, j = 0, jmove, error;
    float tempx, tempy;
    Spline* newspline;

    if ((pfile = fopen(filename, "r")) != NULL) {
        /* Count number of base points in data file */

        for (n = -6; fgets(tmpstr, 81, pfile) != NULL; ++n)
            ;
        rewind(pfile);

        /* Create new Spline structure and link to existing ones */

        newspline = getspline(filename, n);
        if (splist == NULL)
            splist = newspline;
        else
            lastspline->next = newspline;

        /* Make new spline the default spline */

        lastspline = newspline;

        /* Skip first 6 lines of file */

        for (i = 0; i < 6; ++i)
            nrn_assert(fgets(tmpstr, 81, pfile));

        /* Read coordinates of forcing function */

        while (fscanf(pfile, "%e %e", &tempx, &tempy) != EOF) {
            lastspline->x[j] = tempx;
            lastspline->y[j++] = tempy;
        }

        /* Sort base points in ascending order of x values */

        for (j = 1; j < n; j++) {
            jmove = j;
            for (i = j - 1; i >= 0; i--)
                if (lastspline->x[i] > lastspline->x[j])
                    jmove = i;
            if (jmove == j)
                continue;

            tempx = lastspline->x[j];
            tempy = lastspline->y[j];

            /* Ripple base points with larger x values one location higher */

            for (i = j; i > jmove; i--) {
                lastspline->x[i] = lastspline->x[i - 1];
                lastspline->y[i] = lastspline->y[i - 1];
            }

            /* Move current base point to proper location */

            lastspline->x[i] = tempx;
            lastspline->y[i] = tempy;
        }

        /* Calculate spacings of base points and second derivatives */

        fclose(pfile);
        if ((error = derivs(lastspline->npts,
                            lastspline->x,
                            lastspline->y,
                            lastspline->width,
                            lastspline->der)) != SUCCESS) {
            abort_run(error);
        }
    } else /* Forcing function file not found */
        abort_run(NOFORCE);
    return 0;
}

/*---------------------------------------------------------------------------
 * getspline()
 *
 *    Allocates memory for Spline structure and loads in data
 *
 * Calling sequence: getspline(filename, ipt)
 *
 * Arguments:
 *	filename	char*	name of forcing function file
 *	ipt		int	number of data points in forcing function
 *
 * Returns: pointer to allocated Spline structure
 *
 * Functions called: malloc(), gets(), makevector(), strcpy(), abort_run()
 *
 * Files accessed:
 *-------------------------------------------------------------------------*/
static Spline* getspline(char* filename, int ipt) {
    Spline* newspline;

    /* Allocate memory for Spline structure */

    if ((newspline = (Spline*) malloc((unsigned) sizeof(Spline))) == NULL)
        abort_run(LOWMEM);

    newspline->next = SP0;
    newspline->name = (char*) malloc((unsigned) strlen(filename) + 1);
    strcpy(newspline->name, filename);
    newspline->npts = ipt;
    newspline->x = makevector(ipt);
    newspline->y = makevector(ipt);
    newspline->width = makevector(ipt);
    newspline->der = makevector(ipt);

    return (newspline);
}
