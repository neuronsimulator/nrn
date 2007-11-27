#include <../../nrnconf.h>

/**************************************************************************
**
** Copyright (C) 1993 David E. Steward & Zbigniew Leyk, all rights reserved.
**
**			     Meschach Library
** 
** This Meschach Library is provided "as is" without any express 
** or implied warranty of any kind with respect to this software. 
** In particular the authors shall not be liable for any direct, 
** indirect, special, incidental or consequential damages arising 
** in any way from use of the software.
** 
** Everyone is granted permission to copy, modify and redistribute this
** Meschach Library, provided:
**  1.  All copies contain this copyright notice.
**  2.  All modified copies shall carry a notice stating who
**      made the last modification and the date of such modification.
**  3.  No charge is made for this software or works derived from it.  
**      This clause shall not be construed as constraining other software
**      distributed on the same medium as this software, nor is a
**      distribution fee considered a charge.
**
***************************************************************************/


/*
		Files for matrix computations

	Householder transformation file. Contains routines for calculating
	householder transformations, applying them to vectors and matrices
	by both row & column.
*/

/* hsehldr.c 1.3 10/8/87 */
static	char	rcsid[] = "hsehldr.c,v 1.1 1997/12/04 17:55:24 hines Exp";

#include	<stdio.h>
#include	"matrix.h"
#include        "matrix2.h"
#include	<math.h>


/* hhvec -- calulates Householder vector to eliminate all entries after the
	i0 entry of the vector vec. It is returned as out. May be in-situ */
VEC	*hhvec(vec,i0,beta,out,newval)
VEC	*vec,*out;
u_int	i0;
Real	*beta,*newval;
{
	Real	norm;

	out = _v_copy(vec,out,i0);
	norm = sqrt(_in_prod(out,out,i0));
	if ( norm <= 0.0 )
	{
		*beta = 0.0;
		return (out);
	}
	*beta = 1.0/(norm * (norm+fabs(out->ve[i0])));
	if ( out->ve[i0] > 0.0 )
		*newval = -norm;
	else
		*newval = norm;
	out->ve[i0] -= *newval;

	return (out);
}

/* hhtrvec -- apply Householder transformation to vector -- may be in-situ */
VEC	*hhtrvec(hh,beta,i0,in,out)
VEC	*hh,*in,*out;	/* hh = Householder vector */
u_int	i0;
double	beta;
{
	Real	scale;
	/* u_int	i; */

	if ( hh==(VEC *)NULL || in==(VEC *)NULL )
		error(E_NULL,"hhtrvec");
	if ( in->dim != hh->dim )
		error(E_SIZES,"hhtrvec");
	if ( i0 > in->dim )
		error(E_BOUNDS,"hhtrvec");

	scale = beta*_in_prod(hh,in,i0);
	out = v_copy(in,out);
	__mltadd__(&(out->ve[i0]),&(hh->ve[i0]),-scale,(int)(in->dim-i0));
	/************************************************************
	for ( i=i0; i<in->dim; i++ )
		out->ve[i] = in->ve[i] - scale*hh->ve[i];
	************************************************************/

	return (out);
}

/* hhtrrows -- transform a matrix by a Householder vector by rows
	starting at row i0 from column j0 -- in-situ */
MAT	*hhtrrows(M,i0,j0,hh,beta)
MAT	*M;
u_int	i0, j0;
VEC	*hh;
double	beta;
{
	Real	ip, scale;
	int	i /*, j */;

	if ( M==(MAT *)NULL || hh==(VEC *)NULL )
		error(E_NULL,"hhtrrows");
	if ( M->n != hh->dim )
		error(E_RANGE,"hhtrrows");
	if ( i0 > M->m || j0 > M->n )
		error(E_BOUNDS,"hhtrrows");

	if ( beta == 0.0 )	return (M);

	/* for each row ... */
	for ( i = i0; i < M->m; i++ )
	{	/* compute inner product */
		ip = __ip__(&(M->me[i][j0]),&(hh->ve[j0]),(int)(M->n-j0));
		/**************************************************
		ip = 0.0;
		for ( j = j0; j < M->n; j++ )
			ip += M->me[i][j]*hh->ve[j];
		**************************************************/
		scale = beta*ip;
		if ( scale == 0.0 )
		    continue;

		/* do operation */
		__mltadd__(&(M->me[i][j0]),&(hh->ve[j0]),-scale,
							(int)(M->n-j0));
		/**************************************************
		for ( j = j0; j < M->n; j++ )
			M->me[i][j] -= scale*hh->ve[j];
		**************************************************/
	}

	return (M);
}


/* hhtrcols -- transform a matrix by a Householder vector by columns
	starting at row i0 from column j0 -- in-situ */
MAT	*hhtrcols(M,i0,j0,hh,beta)
MAT	*M;
u_int	i0, j0;
VEC	*hh;
double	beta;
{
	/* Real	ip, scale; */
	int	i /*, k */;
	static	VEC	*w = VNULL;

	if ( M==(MAT *)NULL || hh==(VEC *)NULL )
		error(E_NULL,"hhtrcols");
	if ( M->m != hh->dim )
		error(E_SIZES,"hhtrcols");
	if ( i0 > M->m || j0 > M->n )
		error(E_BOUNDS,"hhtrcols");

	if ( beta == 0.0 )	return (M);

	w = v_resize(w,M->n);
	MEM_STAT_REG(w,TYPE_VEC);
	v_zero(w);

	for ( i = i0; i < M->m; i++ )
	    if ( hh->ve[i] != 0.0 )
		__mltadd__(&(w->ve[j0]),&(M->me[i][j0]),hh->ve[i],
							(int)(M->n-j0));
	for ( i = i0; i < M->m; i++ )
	    if ( hh->ve[i] != 0.0 )
		__mltadd__(&(M->me[i][j0]),&(w->ve[j0]),-beta*hh->ve[i],
							(int)(M->n-j0));
	return (M);
}

