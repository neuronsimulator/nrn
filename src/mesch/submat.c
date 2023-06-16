#include <nrnconf.h>

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


/* 1.2 submat.c 11/25/87 */

#include	<stdio.h>
#include	"matrix.h"

static	char	rcsid[] = "submat.c,v 1.1 1997/12/04 17:55:55 hines Exp";


/* get_col -- gets a specified column of a matrix and retruns it as a vector */
VEC	*get_col(mat,col,vec)
u_int	col;
MAT	*mat;
VEC	*vec;
{
   u_int	i;
   
   if ( mat==(MAT *)NULL )
     error(E_NULL,"get_col");
   if ( col >= mat->n )
     error(E_RANGE,"get_col");
   if ( vec==(VEC *)NULL || vec->dim<mat->m )
     vec = v_resize(vec,mat->m);
   
   for ( i=0; i<mat->m; i++ )
     vec->ve[i] = mat->me[i][col];
   
   return (vec);
}

/* get_row -- gets a specified row of a matrix and retruns it as a vector */
VEC	*get_row(mat,row,vec)
u_int	row;
MAT	*mat;
VEC	*vec;
{
   u_int	i;
   
   if ( mat==(MAT *)NULL )
     error(E_NULL,"get_row");
   if ( row >= mat->m )
     error(E_RANGE,"get_row");
   if ( vec==(VEC *)NULL || vec->dim<mat->n )
     vec = v_resize(vec,mat->n);
   
   for ( i=0; i<mat->n; i++ )
     vec->ve[i] = mat->me[row][i];
   
   return (vec);
}

/* _set_col -- sets column of matrix to values given in vec (in situ) */
MAT	*_set_col(mat,col,vec,i0)
MAT	*mat;
VEC	*vec;
u_int	col,i0;
{
   u_int	i,lim;
   
   if ( mat==(MAT *)NULL || vec==(VEC *)NULL )
     error(E_NULL,"_set_col");
   if ( col >= mat->n )
     error(E_RANGE,"_set_col");
   lim = min(mat->m,vec->dim);
   for ( i=i0; i<lim; i++ )
     mat->me[i][col] = vec->ve[i];
   
   return (mat);
}

/* _set_row -- sets row of matrix to values given in vec (in situ) */
MAT	*_set_row(mat,row,vec,j0)
MAT	*mat;
VEC	*vec;
u_int	row,j0;
{
   u_int	j,lim;
   
   if ( mat==(MAT *)NULL || vec==(VEC *)NULL )
     error(E_NULL,"_set_row");
   if ( row >= mat->m )
     error(E_RANGE,"_set_row");
   lim = min(mat->n,vec->dim);
   for ( j=j0; j<lim; j++ )
     mat->me[row][j] = vec->ve[j];
   
   return (mat);
}

/* sub_mat -- returns sub-matrix of old which is formed by the rectangle
   from (row1,col1) to (row2,col2)
   -- Note: storage is shared so that altering the "new"
   matrix will alter the "old" matrix */
MAT	*sub_mat(old,row1,col1,row2,col2,new)
MAT	*old,*new;
u_int	row1,col1,row2,col2;
{
   u_int	i;
   
   if ( old==(MAT *)NULL )
     error(E_NULL,"sub_mat");
   if ( row1 > row2 || col1 > col2 || row2 >= old->m || col2 >= old->n )
     error(E_RANGE,"sub_mat");
   if ( new==(MAT *)NULL || new->m < row2-row1+1 )
   {
      new = NEW(MAT);
      new->me = NEW_A(row2-row1+1,Real *);
      if ( new==(MAT *)NULL || new->me==(Real **)NULL )
	error(E_MEM,"sub_mat");
      else if (mem_info_is_on()) {
	 mem_bytes(TYPE_MAT,0,sizeof(MAT)+
		      (row2-row1+1)*sizeof(Real *));
      }
      
   }
   new->m = row2-row1+1;
   
   new->n = col2-col1+1;
   
   new->base = (Real *)NULL;
   
   for ( i=0; i < new->m; i++ )
     new->me[i] = (old->me[i+row1]) + col1;
   
   return (new);
}


/* sub_vec -- returns sub-vector which is formed by the elements i1 to i2
   -- as for sub_mat, storage is shared */
VEC	*sub_vec(old,i1,i2,new)
VEC	*old, *new;
int	i1, i2;
{
   if ( old == (VEC *)NULL )
     error(E_NULL,"sub_vec");
   if ( i1 > i2 || old->dim < i2 )
     error(E_RANGE,"sub_vec");
   
   if ( new == (VEC *)NULL )
     new = NEW(VEC);
   if ( new == (VEC *)NULL )
     error(E_MEM,"sub_vec");
   else if (mem_info_is_on()) {
      mem_bytes(TYPE_VEC,0,sizeof(VEC));
   }
   
   
   new->dim = i2 - i1 + 1;
   new->ve = &(old->ve[i1]);
   
   return new;
}
