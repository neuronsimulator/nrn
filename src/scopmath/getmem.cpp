#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: getmem.c
 *
 * Copyright (c) 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "getmem.c,v 1.2 1997/08/30 14:32:09 hines Exp" ;
#endif

/****************************************************************/
/*								*/
/*  This file contains routines to allocate and free memory	*/
/*  from the heap for vectors and matrices used by SCoP numeri-	*/
/*  cal methods routines.					*/
/*								*/
/****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "errcodes.h"
#include "newton_struct.h"

double* makevector(int nrows)
{
    double *vector;

    vector = (double *) malloc((unsigned) (nrows * sizeof(double)));
    if (vector == NULL)
	abort_run(LOWMEM);
    else
	return (vector);
    return (double*)0;
}

int freevector(double* vector)
{
    if (vector != NULL)
	free((char *) vector);
    return 0;
}

double** makematrix(int nrows, int ncols)
{
    int i;
    double **matrix;

    matrix = (double **) malloc((unsigned) (nrows * sizeof(double *)));
    if (matrix == NULL)
	abort_run(LOWMEM);

    *matrix = (double *) malloc((unsigned) (nrows * ncols * sizeof(double)));
    if (*matrix == NULL)
	abort_run(LOWMEM);

    for (i = 1; i < nrows; i++)
	matrix[i] = matrix[i - 1] + ncols;
    return (matrix);
}

int freematrix(double** matrix)
{
    if (matrix != NULL)
    {
	free((char *) *matrix);
	free((char *) matrix);
    }
    return 0;
}

/**********************************************************
 *
 *  Routines for setting "malloced" array elements to 0.0
 *
 *********************************************************/

int zero_matrix(double** matrix, int rows, int cols)
{
    int i, j;

    for (i = 0; i < rows; i++)
	for (j = 0; j < cols; j++)
	    matrix[i][j] = 0.0;
    return 0;
}

int zero_vector(double* vector, int n)
{
    int i;

    for (i = 0; i < n; i++)
	vector[i] = 0.0;
    return 0;
}

int zero_ptrvector(double** ptrvector, int n)
{
    int i;

    for (i = 0; i < n; i++)
	*ptrvector[i] = 0.0;
    return 0;
}
