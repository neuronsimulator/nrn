#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: schedule.c
 *
 * Copyright (c) 1990-1991
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
"schedule.c,v 1.2 1997/08/30 14:32:18 hines Exp";
#endif

/*------------------------------------------------------------------------
 * Abstract: schedule()
 *
 *    Produces a train of Dirac delta functions of amplitudes stored in
 *    a reference data file.  The amplitudes may be of any height and
 *    with any spacing.
 *
 * Calling sequence: schedule(t, filename)
 *
 * Arguments:	t	double	value of variable to test for next
 *				scheduled event
 *		filename char*	file containing scheduled event points
 *				and corresponding amplitudes generated
 *              old_value, *double  saved value from previous call
 *
 * Returns: amplitude if event scheduled, zero otherwise
 *
 * Files accessed: reference data file containing schedule and amplitudes
 *
 *----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "errcodes.h"

typedef struct event
{
    char *file;
    int nevent;
    int count;
    double *sched_t;
    double *ampl;
    struct event *next_event;
}   event_t;

static void init_event();

double
schedule(reset_integ, old_value, t, filename)
double *old_value, t;
int *reset_integ;
char *filename;
{
    static event_t *event_root = (event_t *) 0; /* Root of linked list of schedules */
    event_t *curr_event;
    extern int _ninits;
    static int initialized = 0;

    if (initialized < _ninits)		/* Re-initialize event counters */
    {
        curr_event = event_root;
        while (curr_event != (event_t *) 0)
	{
	    curr_event->count = 0;
	    curr_event = curr_event->next_event;
        }
	initialized = _ninits;
    }
    /* Search for event schedule in linked list */

    curr_event = event_root;
    while (curr_event != (event_t *) 0)
	if (curr_event->file == filename)
	    break;
	else
	    curr_event = curr_event->next_event;
    if (curr_event == (event_t *) 0)
    {
	init_event(&curr_event, filename);	/* Create a new schedule */
	if (event_root == (event_t *) 0)	/* Make it the root */
	    event_root = curr_event;
	else					/* Add new schedule to the list */
	{
	    event_t *search_ptr = event_root;
	    while (search_ptr->next_event != (event_t *) 0)
		search_ptr = search_ptr->next_event;
	    search_ptr->next_event = curr_event;
	}
    }

    /* See if next event is scheduled */

    if (curr_event->count < curr_event->nevent && t >= curr_event->sched_t[curr_event->count])
    {
	*reset_integ = 1;
	return (curr_event->ampl[curr_event->count++]);
    }
    else
	return (0.0);
}

static void init_event(new_event, filename)
event_t **new_event;
char *filename;
{
    FILE *datafile, *fopen();
    int nevent = -6, i;
    char buff[81];
#ifndef MAC
    extern char *fgets();
#endif
    extern double *makevector();
    extern int abort_run();

    if ((datafile = fopen(filename, "r")) == NULL)
	abort_run(NODATA);

    /* Count number of scheduled events */

    while (fgets(buff, 80, datafile) != NULL)
	nevent++;
    rewind(datafile);

    /* Create event data structure */

    if ((*new_event = (event_t *) malloc((unsigned) sizeof(event_t))) ==
				(event_t *) 0)
	abort_run(LOWMEM);
    (*new_event)->file = filename;
    (*new_event)->nevent = nevent;
    (*new_event)->count = 0;
    (*new_event)->sched_t = makevector(nevent);
    (*new_event)->ampl = makevector(nevent);
    (*new_event)->next_event = (event_t *) 0;

    /* Put data into event structure */

    for (i = 0; i < 6; i++)	/* Skip first 6 lines */
	assert(fgets(buff, 80, datafile));
    for (i = 0; i < nevent; i++)
    {
	assert(fgets(buff, 80, datafile));
	sscanf(buff, "%lf %lf", &((*new_event)->sched_t[i]), &((*new_event)->ampl[i]));
    }

    fclose(datafile);
}
