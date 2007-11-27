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
		File containing routines for determining Hessenberg
	factorisations.
*/

static	char	rcsid[] = "hessen.c,v 1.1 1997/12/04 17:55:23 hines Exp";

#include	<stdio.h>
#include	"matrix.h"
#include        "matrix2.h"



/* Hfactor -- compute Hessenberg factorisation in compact form.
	-- factorisation performed in situ
	-- for details of the compact form see QRfactor.c and matrix2.doc */
MAT	*Hfactor(A, diag, beta)
MAT	*A;
VEC	*diag, *beta;
{
	static	VEC	*tmp1 = VNULL;
	int	k, limit;

	if ( ! A || ! diag || ! beta )
		error(E_NULL,"Hfactor");
	if ( diag->dim < A->m - 1 || beta->dim < A->m - 1 )
		error(E_SIZES,"Hfactor");
	if ( A->m != A->n )
		error(E_SQUARE,"Hfactor");
	limit = A->m - 1;

	tmp1 = v_resize(tmp1,A->m);
	MEM_STAT_REG(tmp1,TYPE_VEC);

	for ( k = 0; k < limit; k++ )
	{
		get_col(A,(u_int)k,tmp1);
		/* printf("the %d'th column = ");	v_output(tmp1); */
		hhvec(tmp1,k+1,&beta->ve[k],tmp1,&A->me[k+1][k]);
		/* diag->ve[k] = tmp1->ve[k+1]; */
		v_set_val(diag,k,v_entry(tmp1,k+1));
		/* printf("H/h vector = ");	v_output(tmp1); */
		/* printf("from the %d'th entry\n",k+1); */
		/* printf("beta = %g\n",beta->ve[k]); */

		/* hhtrcols(A,k+1,k+1,tmp1,beta->ve[k]); */
		/* hhtrrows(A,0  ,k+1,tmp1,beta->ve[k]); */
		hhtrcols(A,k+1,k+1,tmp1,v_entry(beta,k));
		hhtrrows(A,0  ,k+1,tmp1,v_entry(beta,k));
		/* printf("A = ");		m_output(A); */
	}

	return (A);
}

/* makeHQ -- construct the Hessenberg orthogonalising matrix Q;
	-- i.e. Hess M = Q.M.Q'	*/
MAT	*makeHQ(H, diag, beta, Qout)
MAT	*H, *Qout;
VEC	*diag, *beta;
{
	int	i, j, limit;
	static	VEC	*tmp1 = VNULL, *tmp2 = VNULL;

	if ( H==(MAT *)NULL || diag==(VEC *)NULL || beta==(VEC *)NULL )
		error(E_NULL,"makeHQ");
	limit = H->m - 1;
	if ( diag->dim < limit || beta->dim < limit )
		error(E_SIZES,"makeHQ");
	if ( H->m != H->n )
		error(E_SQUARE,"makeHQ");
	Qout = m_resize(Qout,H->m,H->m);

	tmp1 = v_resize(tmp1,H->m);
	tmp2 = v_resize(tmp2,H->m);
	MEM_STAT_REG(tmp1,TYPE_VEC);
	MEM_STAT_REG(tmp2,TYPE_VEC);

	for ( i = 0; i < H->m; i++ )
	{
		/* tmp1 = i'th basis vector */
		for ( j = 0; j < H->m; j++ )
			/* tmp1->ve[j] = 0.0; */
		    v_set_val(tmp1,j,0.0);
		/* tmp1->ve[i] = 1.0; */
		v_set_val(tmp1,i,1.0);

		/* apply H/h transforms in reverse order */
		for ( j = limit-1; j >= 0; j-- )
		{
			get_col(H,(u_int)j,tmp2);
			/* tmp2->ve[j+1] = diag->ve[j]; */
			v_set_val(tmp2,j+1,v_entry(diag,j));
			hhtrvec(tmp2,beta->ve[j],j+1,tmp1,tmp1);
		}

		/* insert into Qout */
		set_col(Qout,(u_int)i,tmp1);
	}

	return (Qout);
}

/* makeH -- construct actual Hessenberg matrix */
MAT	*makeH(H,Hout)
MAT	*H, *Hout;
{
	int	i, j, limit;

	if ( H==(MAT *)NULL )
		error(E_NULL,"makeH");
	if ( H->m != H->n )
		error(E_SQUARE,"makeH");
	Hout = m_resize(Hout,H->m,H->m);
	Hout = m_copy(H,Hout);

	limit = H->m;
	for ( i = 1; i < limit; i++ )
		for ( j = 0; j < i-1; j++ )
			/* Hout->me[i][j] = 0.0;*/
		    m_set_val(Hout,i,j,0.0);

	return (Hout);
}

