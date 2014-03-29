#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: adams.c
 *
 * Copyright (c) 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "adams.c,v 1.1.1.1 1994/10/12 17:22:18 hines Exp" ;
#endif

/*--------------------------------------------------------------*/
/*                                                              */
/*  Abstract: adams()                                           */
/*                                                              */
/*    Solves a system of first-order ordinary differential      */
/*    equations using an Adams-Bashforth predictor, an Adams-   */
/*    Moulton modifier, and a corrector iteration.  The names   */
/*    of the state variables must appear in a single block in   */
/*    the variable file.  The names of the time derivatives of  */
/*    the state variables must appear in a single block in the  */
/*    variable file in the same order as the corresponding      */
/*    state variables.  On the first three entries to adams(),	*/
/*    the state variables are extrapolated by runge().  This	*/
/*    generates the history of derivative values needed to	*/
/*    define the interpolating polynomial used by the Adams	*/
/*    predictor and modifier formulas.				*/
/*                                                              */
/*  Returns: int error code (always SUCCESS for adams)		*/
/*                                                              */
/*  Calling sequence:                                           */
/*                                                              */
/*    adams(n, y, d, p, t, h, dy, work)				*/
/*                                                              */
/*  Arguments:                                                  */
/*    Input:    n       number of state variables               */
/*                                                              */
/*              y	pointer to array of addresses of the    */
/*                      state variables                         */
/*                                                              */
/*              d	pointer to array of addresses of the    */
/*                      derivatives of the state variables      */
/*                                                              */
/*              p       array of variable values.               */
/*                                                              */
/*              t       pointer to the global independent       */
/*                      variable (usually, time).               */
/*                                                              */
/*              h       value of the time increment.            */
/*                                                              */
/*              dy      name of the derivative evaluation       */
/*                      function.                               */
/*                                                              */
/*              *work   pointer to storage for function values  */
/*                      at previous time points.                */
/*                                                              */
/*    Output:   p       variables and derivatives are updated   */
/*                                                              */
/*              t       time is incremented by h                */
/*                                                              */
/*              **work  contains history of derivatives for the */
/*                      three most recent time points           */
/*                                                              */
/*  Functions called:  runge                                    */
/*                                                              */
/*--------------------------------------------------------------*/

#include "errcodes.h"

int adams(_ninits, n, y, d, p, t, h, dy, work)
int _ninits, n, (*dy) ();
double p[], *t, h, **work; int y[]; int d[];
#define d_(arg)  p[d[arg]]
#define y_(arg)  p[y[arg]]
{
    static int _reset;
    int i, n2, n3, n4, n5;
    static int count = 0;
    double *scratch;
    extern int runge();
    extern double *makevector();

    n2 = n << 1;
    n3 = n2 + n;
    n4 = n << 2;
    n5 = n4 + n;

    if (*work == (double *) 0)
	*work = makevector(n5);

    if (_reset)
    {
	count = 0;
	_reset = 0;
    }

    switch (count)
    {
	case 0:		/* First time through procedure */

	    /* Store current values of the derivatives */

	    (*dy) (p);
	    for (i = 0; i < n; i++)
		(*work)[i] = d_(i);	/* Storage for first history point */

	    /* Extrapolate to t = t0 + h */

	    scratch = *work + n3;
	    runge(_ninits, n, y, d, p, t, h, dy, &scratch);
	    count++;
	    break;

	case 1:		/* Second time through procedure */

	    /* Store current values of the derivatives */

	    (*dy) (p);
	    for (i = 0; i < n; i++)
		(*work)[n + i] = d_(i);	/* Storage for second history point */

	    /* Extrapolate to t = t0 + 2h */

	    scratch = *work + n3;
	    runge(_ninits, n, y, d, p, t, h, dy, &scratch);
	    count++;
	    break;

	case 2:		/* Third time through procedure */

	    /* Store current values of the derivatives */

	    (*dy) (p);
	    for (i = 0; i < n; i++)
		(*work)[n2 + i] = d_(i);	/* Storage for third history
						 * point */

	    /* Extrapolate to t = t0 + 3h */

	    scratch = *work + n3;
	    runge(_ninits, n, y, d, p, t, h, dy, &scratch);
	    count++;
	    break;

	default:		/* Subsequent passes through procedure */

	    /* Generic step from t to t + h.  Store starting y values */

	    for (i = 0; i < n; i++)
		(*work)[n3 + i] = y_(i);

	    /* Use Adams-Bashforth predictor */

	    (*dy) (p);
	    for (i = 0; i < n; i++)
		y_(i) += (h / 24.) * (55. * (d_(i)) - 59. * ((*work)[n2 + i])
			      + 37. * ((*work)[n + i]) - 9. * ((*work)[i]));
	    *t +=h;

	    /* Shift work array by one frame of width h */

	    for (i = 0; i < n; i++)
	    {
		(*work)[i] = (*work)[n + i];	/* d(t - 2h) */
		(*work)[n + i] = (*work)[n2 + i];	/* d(t - h)  */
		(*work)[n2 + i] = d_(i);	/* d(t)      */
	    }

	    /* Use Adams-Moulton modifier */

	    (*dy) (p);
	    for (i = 0; i < n; i++)
		(*work)[n3 + i] += (h / 24.) * (9. * (d_(i)) + 19. * ((*work)[n2 + i])
				      - 5. * ((*work)[n + i]) + (*work)[i]);

	    /*
	     * Corrector formula: a linear combination of the above two
	     * estimates such that the most significant error terms cancel
	     */

	    for (i = 0; i < n; i++)
		y_(i) = (19. * (y_(i)) + 251. * ((*work)[n3 + i])) / 270.;

	    /* Restore original value of t -- updated in model() */

	    *t -= h;
    }

    return (SUCCESS);
}
