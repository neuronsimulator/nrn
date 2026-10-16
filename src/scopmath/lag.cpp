#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: lag.c
 *
 * Copyright (c) 1988-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

#include <cstdio>
#include <cstdlib>
/***************************************************************
 *
 *  Abstract: lag()
 *
 *    This function has two independent variables, var and curt,
 *    where var contains its value at curt, and a constant lagt.
 *    It returns a pointer to the value of var at curt-lagt.
 *
 *		lag(var(curt),lagt) = var(curt - lagt)
 *
 *  Returns: Pointer to double for lagged value of var if
 *	     curt >= lagt or pointer to initial value if curt < lagt
 *
 *  Calling sequence: lag(var, curt, lagt, vsize)
 *
 *  Arguments:
 *    Input:	var	*double	pointer to variable whose
 *				value at curt - lagt is desired
 *		curt	double	current value of the monotonically
 *				increasing independent variable
 *				(usually time)
 *		lagt	double	difference between curt and
 *				the previous value of curt for
 *				which we desire the value of var
 *		vsize	int	dimension of var if a vector;
 *				else <= 1
 *
 *  Functions called: makevar, freevars, makenode, freenode
 *
 *  Files accessed: none
 *
 ***************************************************************/

#define CURVAL    (listptr->curptr)
#define LAGVAL    (listptr->lagptr)
#define VSIZE     listptr->dimension
#define INTERPVAL listptr->interpolate

typedef struct node {
    double time;       /* value of curt corresponding to node */
    double* value;     /* Pointer to block of storage for variable
                        * value(s) */
    struct node* next; /* Pointer to next node in data buffer */
} node;

typedef struct varlist {
    double* varptr;               /* Pointer to variable in p array */
    int dimension;                /* = 1 for scalars; = vsize for vectors */
    double offset;                /* lag in the independent variable = lagt */
    struct node *curptr, *lagptr; /* Pointers to locations in data linked list */
    double* interpolate;          /* Pointer to allocated storage for
                                   * interpolated lagged value to be returned */
    struct varlist* next;         /* Pointer to next lagged variable */
} varlist;

static void makevar(varlist** newvar, double* address, int size, double delay);
static void makenode(node** nodeptr, double t, double* dataptr, int datasize);
static void freevars(varlist* list);
static void freenode(node* nodeptr);
static int getinterpval(node* start, node* end, double t, double* interpval, int dimension);

double* lag(double* var, double curt, double lagt, int vsize) {
    static varlist* lagvars = NULL;
    varlist *listptr, *lastvar = NULL;
    node *nodeptr, *nextnode;
    extern int _ninits;
    static int initialized = 0;
    int i, interp = 0;

    if (initialized < _ninits) {
        /* A new run has been started.  Free storage for the linked list */
        freevars(lagvars);
        lagvars = NULL;
        initialized = _ninits;
    }

    /* Search for lag variable in list */
    for (listptr = lagvars; listptr != NULL; listptr = listptr->next)
        if ((listptr->varptr == var) && (listptr->offset == lagt))
            break;
        else
            lastvar = listptr;

    if (listptr == NULL) {
        /* Lag variable not listed; add to list of lagged variables */
        makevar(&listptr, var, vsize, lagt);
        if (lastvar == NULL)
            lagvars = listptr;
        else
            lastvar->next = listptr;
        makenode(&CURVAL, curt, var, VSIZE);
        LAGVAL = CURVAL;
    } else if (curt == CURVAL->time) {
        /*
         * Time point already listed, e.g., for predictor-corrector integrators
         * Update value of variable in current node
         */
        for (i = 0; i < VSIZE; i++)
            *(CURVAL->value + i) = *(var + i);
        interp = (curt - LAGVAL->time >= lagt);
    } else if (curt - LAGVAL->time >= lagt) {
        /* Lag period exceeded */
        makenode(&(CURVAL->next), curt, var, VSIZE);
        CURVAL = CURVAL->next;
        for (nodeptr = LAGVAL; curt - (nodeptr->next)->time >= lagt; nodeptr = nextnode) {
            nextnode = nodeptr->next;
            freenode(nodeptr);
        }
        LAGVAL = nodeptr;
        interp = getinterpval(LAGVAL, LAGVAL->next, curt - lagt, INTERPVAL, VSIZE);
    } else if (curt > CURVAL->time) {
        /* Haven't exceeded lag period yet */
        makenode(&(CURVAL->next), curt, var, VSIZE);
        CURVAL = CURVAL->next;
    }
    if (!interp)
        return (LAGVAL->value);
    else
        return (INTERPVAL);
}

static void makevar(varlist** newvar, double* address, int size, double delay) {
    /* Allocate storage for new element of varlist */
    *newvar = (varlist*) malloc((unsigned) sizeof(varlist));

    /* Store information about new lagged variable */
    if (size < 1)
        size = 1;
    (*newvar)->varptr = address;
    (*newvar)->dimension = size;
    (*newvar)->offset = delay;
    (*newvar)->interpolate = (double*) malloc((unsigned) (size * sizeof(double)));
    (*newvar)->next = NULL;
}

static void freevars(varlist* list) {
    varlist *listptr, *nextptr;
    node *nodeptr, *nextnode;

    for (listptr = list; listptr != NULL; listptr = nextptr) {
        /* Free linked list of saved data */
        for (nodeptr = LAGVAL; nodeptr != NULL; nodeptr = nextnode) {
            nextnode = nodeptr->next;
            freenode(nodeptr);
        }

        nextptr = listptr->next;
        free((char*) INTERPVAL);
        free((char*) listptr);
    }
}

static void makenode(node** nodeptr, double t, double* dataptr, int datasize) {
    int i;

    /* Allocate storage for node and data */
    *nodeptr = (node*) malloc((unsigned) sizeof(node));
    (*nodeptr)->value = (double*) malloc((unsigned) (datasize * sizeof(double)));

    /* Store data for later retrieval */
    (*nodeptr)->time = t;
    for (i = 0; i < datasize; i++)
        *((*nodeptr)->value + i) = *(dataptr + i);

    (*nodeptr)->next = NULL;
}

static void freenode(node* nodeptr) {
    free((char*) nodeptr->value);
    free((char*) nodeptr);
}

static int getinterpval(node* start, node* end, double t, double* interpval, int dimension) {
    int i;
    double t_ratio;

    /*
     * Perform linear interpolation for lagged value over interval
     * Return 1 if interpolation is required, else return 0
     */
    if (t > start->time) {
        t_ratio = (t - start->time) / (end->time - start->time);
        for (i = 0; i < dimension; i++)
            *(interpval + i) = *(start->value + i) +
                               t_ratio * (*(end->value + i) - *(start->value + i));
        return (1);
    } else
        return (0);
}
