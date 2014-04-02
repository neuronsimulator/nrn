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


/* ivecop.c  */

#include	<stdio.h>
#include 	"matrix.h"

static	char	rcsid[] = "ivecop.c,v 1.1 1997/12/04 17:55:30 hines Exp";

static char    line[MAXLINE];



/* iv_get -- get integer vector -- see also memory.c */
IVEC	*iv_get(dim)
int	dim;
{
   IVEC	*iv;
   /* u_int	i; */
   
   if (dim < 0)
     error(E_NEG,"iv_get");

   if ((iv=NEW(IVEC)) == IVNULL )
     error(E_MEM,"iv_get");
   else if (mem_info_is_on()) {
      mem_bytes(TYPE_IVEC,0,sizeof(IVEC));
      mem_numvar(TYPE_IVEC,1);
   }
   
   iv->dim = iv->max_dim = dim;
   if ((iv->ive = NEW_A(dim,int)) == (int *)NULL )
     error(E_MEM,"iv_get");
   else if (mem_info_is_on()) {
      mem_bytes(TYPE_IVEC,0,dim*sizeof(int));
   }
   
   return (iv);
}

/* iv_free -- returns iv & asoociated memory back to memory heap */
int	iv_free(iv)
IVEC	*iv;
{
   if ( iv==IVNULL || iv->dim > MAXDIM )
     /* don't trust it */
     return (-1);
   
   if ( iv->ive == (int *)NULL ) {
      if (mem_info_is_on()) {
	 mem_bytes(TYPE_IVEC,sizeof(IVEC),0);
	 mem_numvar(TYPE_IVEC,-1);
      }
      free((char *)iv);
   }
   else
   {
      if (mem_info_is_on()) {
	 mem_bytes(TYPE_IVEC,sizeof(IVEC)+iv->max_dim*sizeof(int),0);
	 mem_numvar(TYPE_IVEC,-1);
      }	
      free((char *)iv->ive);
      free((char *)iv);
   }
   
   return (0);
}

/* iv_resize -- returns the IVEC with dimension new_dim
   -- iv is set to the zero vector */
IVEC	*iv_resize(iv,new_dim)
IVEC	*iv;
int	new_dim;
{
   int	i;
   
   if (new_dim < 0)
     error(E_NEG,"iv_resize");

   if ( ! iv )
     return iv_get(new_dim);
   
   if (new_dim == iv->dim)
     return iv;

   if ( new_dim > iv->max_dim )
   {
      if (mem_info_is_on()) {
	 mem_bytes(TYPE_IVEC,iv->max_dim*sizeof(int),
		      new_dim*sizeof(int));
      }
      iv->ive = RENEW(iv->ive,new_dim,int);
      if ( ! iv->ive )
	error(E_MEM,"iv_resize");
      iv->max_dim = new_dim;
   }
   if ( iv->dim <= new_dim )
     for ( i = iv->dim; i < new_dim; i++ )
       iv->ive[i] = 0;
   iv->dim = new_dim;
   
   return iv;
}

/* iv_copy -- copy integer vector in to out
   -- out created/resized if necessary */
IVEC	*iv_copy(in,out)
IVEC	*in, *out;
{
   int		i;
   
   if ( ! in )
     error(E_NULL,"iv_copy");
   out = iv_resize(out,in->dim);
   for ( i = 0; i < in->dim; i++ )
     out->ive[i] = in->ive[i];
   
   return out;
}

/* iv_move -- move selected pieces of an IVEC
	-- moves the length dim0 subvector with initial index i0
	   to the corresponding subvector of out with initial index i1
	-- out is resized if necessary */
IVEC	*iv_move(in,i0,dim0,out,i1)
IVEC	*in, *out;
int	i0, dim0, i1;
{
    if ( ! in )
	error(E_NULL,"iv_move");
    if ( i0 < 0 || dim0 < 0 || i1 < 0 ||
	 i0+dim0 > in->dim )
	error(E_BOUNDS,"iv_move");

    if ( (! out) || i1+dim0 > out->dim )
	out = iv_resize(out,i1+dim0);

    MEM_COPY(&(in->ive[i0]),&(out->ive[i1]),dim0*sizeof(int));

    return out;
}

/* iv_add -- integer vector addition -- may be in-situ */
IVEC	*iv_add(iv1,iv2,out)
IVEC	*iv1,*iv2,*out;
{
   u_int	i;
   int	*out_ive, *iv1_ive, *iv2_ive;
   
   if ( iv1==IVNULL || iv2==IVNULL )
     error(E_NULL,"iv_add");
   if ( iv1->dim != iv2->dim )
     error(E_SIZES,"iv_add");
   if ( out==IVNULL || out->dim != iv1->dim )
     out = iv_resize(out,iv1->dim);
   
   out_ive = out->ive;
   iv1_ive = iv1->ive;
   iv2_ive = iv2->ive;
   
   for ( i = 0; i < iv1->dim; i++ )
     out_ive[i] = iv1_ive[i] + iv2_ive[i];
   
   return (out);
}



/* iv_sub -- integer vector addition -- may be in-situ */
IVEC	*iv_sub(iv1,iv2,out)
IVEC	*iv1,*iv2,*out;
{
   u_int	i;
   int	*out_ive, *iv1_ive, *iv2_ive;
   
   if ( iv1==IVNULL || iv2==IVNULL )
     error(E_NULL,"iv_sub");
   if ( iv1->dim != iv2->dim )
     error(E_SIZES,"iv_sub");
   if ( out==IVNULL || out->dim != iv1->dim )
     out = iv_resize(out,iv1->dim);
   
   out_ive = out->ive;
   iv1_ive = iv1->ive;
   iv2_ive = iv2->ive;
   
   for ( i = 0; i < iv1->dim; i++ )
     out_ive[i] = iv1_ive[i] - iv2_ive[i];
   
   return (out);
}

/* iv_foutput -- print a representation of iv on stream fp */
void	iv_foutput(fp,iv)
FILE	*fp;
IVEC	*iv;
{
   int	i;
   
   fprintf(fp,"IntVector: ");
   if ( iv == IVNULL )
   {
      fprintf(fp,"**** NULL ****\n");
      return;
   }
   fprintf(fp,"dim: %d\n",iv->dim);
   for ( i = 0; i < iv->dim; i++ )
   {
      if ( (i+1) % 8 )
	fprintf(fp,"%8d ",iv->ive[i]);
      else
	fprintf(fp,"%8d\n",iv->ive[i]);
   }
   if ( i % 8 )
     fprintf(fp,"\n");
}


/* iv_finput -- input integer vector from stream fp */
IVEC	*iv_finput(fp,x)
FILE	*fp;
IVEC	*x;
{
   IVEC	*iiv_finput(),*biv_finput();
   
   if ( isatty(fileno(fp)) )
     return iiv_finput(fp,x);
   else
     return biv_finput(fp,x);
}

/* iiv_finput -- interactive input of IVEC iv */
IVEC	*iiv_finput(fp,iv)
FILE	*fp;
IVEC	*iv;
{
   u_int	i,dim,dynamic;	/* dynamic set if memory allocated here */
   
   /* get dimension */
   if ( iv != (IVEC *)NULL && iv->dim<MAXDIM )
   {	dim = iv->dim;	dynamic = FALSE;	}
   else
   {
      dynamic = TRUE;
      do
      {
	 fprintf(stderr,"IntVector: dim: ");
	 if ( fgets(line,MAXLINE,fp)==NULL )
	   error(E_INPUT,"iiv_finput");
      } while ( sscanf(line,"%u",&dim)<1 || dim>MAXDIM );
      iv = iv_get(dim);
   }
   
   /* input elements */
   for ( i=0; i<dim; i++ )
     do
     {
      redo:
	fprintf(stderr,"entry %u: ",i);
	if ( !dynamic )
	  fprintf(stderr,"old: %-9d  new: ",iv->ive[i]);
	if ( fgets(line,MAXLINE,fp)==NULL )
	  error(E_INPUT,"iiv_finput");
	if ( (*line == 'b' || *line == 'B') && i > 0 )
	{	i--;	dynamic = FALSE;	goto redo;	   }
	if ( (*line == 'f' || *line == 'F') && i < dim-1 )
	{	i++;	dynamic = FALSE;	goto redo;	   }
     } while ( *line=='\0' || sscanf(line,"%d",&iv->ive[i]) < 1 );
   
   return (iv);
}

/* biv_finput -- batch-file input of IVEC iv */
IVEC	*biv_finput(fp,iv)
FILE	*fp;
IVEC	*iv;
{
   u_int	i,dim;
   int	io_code;
   
   /* get dimension */
   skipjunk(fp);
   if ((io_code=fscanf(fp," IntVector: dim:%u",&dim)) < 1 ||
       dim>MAXDIM )
     error(io_code==EOF ? 7 : 6,"biv_finput");
   
   /* allocate memory if necessary */
   if ( iv==(IVEC *)NULL || iv->dim<dim )
     iv = iv_resize(iv,dim);
   
   /* get entries */
   skipjunk(fp);
   for ( i=0; i<dim; i++ )
     if ((io_code=fscanf(fp,"%d",&iv->ive[i])) < 1 )
       error(io_code==EOF ? 7 : 6,"biv_finput");
   
   return (iv);
}

/* iv_dump -- dumps all the contents of IVEC iv onto stream fp */
void	iv_dump(fp,iv)
FILE*fp;
IVEC*iv;
{
   int		i;
   
   fprintf(fp,"IntVector: ");
   if ( ! iv )
   {
      fprintf(fp,"**** NULL ****\n");
      return;
   }
   fprintf(fp,"dim: %d, max_dim: %d\n",iv->dim,iv->max_dim);
   fprintf(fp,"ive @ 0x%p\n", iv->ive);
   for ( i = 0; i < iv->max_dim; i++ )
   {
      if ( (i+1) % 8 )
	fprintf(fp,"%8d ",iv->ive[i]);
      else
	fprintf(fp,"%8d\n",iv->ive[i]);
   }
   if ( i % 8 )
     fprintf(fp,"\n");
}	

#define	MAX_STACK	60


/* iv_sort -- sorts vector x, and generates permutation that gives the order
   of the components; x = [1.3, 3.7, 0.5] -> [0.5, 1.3, 3.7] and
   the permutation is order = [2, 0, 1].
   -- if order is NULL on entry then it is ignored
   -- the sorted vector x is returned */
IVEC	*iv_sort(x, order)
IVEC	*x;
PERM	*order;
{
   int		*x_ive, tmp, v;
   /* int		*order_pe; */
   int		dim, i, j, l, r, tmp_i;
   int		stack[MAX_STACK], sp;
   
   if ( ! x )
     error(E_NULL,"v_sort");
   if ( order != PNULL && order->size != x->dim )
     order = px_resize(order, x->dim);
   
   x_ive = x->ive;
   dim = x->dim;
   if ( order != PNULL )
     px_ident(order);
   
   if ( dim <= 1 )
     return x;
   
   /* using quicksort algorithm in Sedgewick,
      "Algorithms in C", Ch. 9, pp. 118--122 (1990) */
   sp = 0;
   l = 0;	r = dim-1;	v = x_ive[0];
   for ( ; ; )
   {
      while ( r > l )
      {
	 /* "i = partition(x_ive,l,r);" */
	 v = x_ive[r];
	 i = l-1;
	 j = r;
	 for ( ; ; )
	 {
	    while ( x_ive[++i] < v )
	      ;
	    --j;
	    while ( x_ive[j] > v && j != 0 )
	        --j;
	    if ( i >= j )	break;
	    
	    tmp = x_ive[i];
	    x_ive[i] = x_ive[j];
	    x_ive[j] = tmp;
	    if ( order != PNULL )
	    {
	       tmp_i = order->pe[i];
	       order->pe[i] = order->pe[j];
	       order->pe[j] = tmp_i;
	    }
	 }
	 tmp = x_ive[i];
	 x_ive[i] = x_ive[r];
	 x_ive[r] = tmp;
	 if ( order != PNULL )
	 {
	    tmp_i = order->pe[i];
	    order->pe[i] = order->pe[r];
	    order->pe[r] = tmp_i;
	 }
	 
	 if ( i-l > r-i )
	 {   stack[sp++] = l;   stack[sp++] = i-1;   l = i+1;   }
	 else
	 {   stack[sp++] = i+1;   stack[sp++] = r;   r = i-1;   }
      }
      
      /* recursion elimination */
      if ( sp == 0 )
	break;
      r = stack[--sp];
      l = stack[--sp];
   }
   
   return x;
}
