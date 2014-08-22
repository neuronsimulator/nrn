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


/* 1.6 matrixio.c 11/25/87 */


#include        <stdio.h>
#include        <ctype.h>
#include        "matrix.h"

static char rcsid[] = "matrixio.c,v 1.1 1997/12/04 17:55:35 hines Exp";


/* local variables */
static char line[MAXLINE];


/**************************************************************************
  Input routines
  **************************************************************************/
/* skipjunk -- skips white spaces and strings of the form #....\n
   Here .... is a comment string */
int     skipjunk(fp)
FILE    *fp;
{
     int        c;
     
     for ( ; ; )        /* forever do... */
     {
	  /* skip blanks */
	  do
	       c = getc(fp);
	  while ( isspace(c) );
	  
	  /* skip comments (if any) */
	  if ( c == '#' )
	       /* yes it is a comment (line) */
	       while ( (c=getc(fp)) != '\n' )
		    ;
	  else
	  {
	       ungetc(c,fp);
	       break;
	  }
     }
     return 0;
}

MAT     *m_finput(fp,a)
FILE    *fp;
MAT     *a;
{
     MAT        *im_finput(),*bm_finput();
     
     if ( isatty(fileno(fp)) )
	  return im_finput(fp,a);
     else
	  return bm_finput(fp,a);
}

/* im_finput -- interactive input of matrix */
MAT     *im_finput(fp,mat)
FILE    *fp;
MAT     *mat;
{
     char       c;
     u_int      i, j, m, n, dynamic;
     /* dynamic set to TRUE if memory allocated here */
     
     /* get matrix size */
     if ( mat != (MAT *)NULL && mat->m<MAXDIM && mat->n<MAXDIM )
     {  m = mat->m;     n = mat->n;     dynamic = FALSE;        }
     else
     {
	  dynamic = TRUE;
	  do
	  {
	       fprintf(stderr,"Matrix: rows cols:");
	       if ( fgets(line,MAXLINE,fp)==NULL )
		    error(E_INPUT,"im_finput");
	  } while ( sscanf(line,"%u%u",&m,&n)<2 || m>MAXDIM || n>MAXDIM );
	  mat = m_get(m,n);
     }
     
     /* input elements */
     for ( i=0; i<m; i++ )
     {
     redo:
	  fprintf(stderr,"row %u:\n",i);
	  for ( j=0; j<n; j++ )
	       do
	       {
	       redo2:
		    fprintf(stderr,"entry (%u,%u): ",i,j);
		    if ( !dynamic )
			 fprintf(stderr,"old %14.9g new: ",
				 mat->me[i][j]);
		    if ( fgets(line,MAXLINE,fp)==NULL )
			 error(E_INPUT,"im_finput");
		    if ( (*line == 'b' || *line == 'B') && j > 0 )
		    {   j--;    dynamic = FALSE;        goto redo2;     }
		    if ( (*line == 'f' || *line == 'F') && j < n-1 )
		    {   j++;    dynamic = FALSE;        goto redo2;     }
#if REAL == DOUBLE
	       } while ( *line=='\0' || sscanf(line,"%lf",&mat->me[i][j])<1 );
#elif REAL == FLOAT
	       } while ( *line=='\0' || sscanf(line,"%f",&mat->me[i][j])<1 );
#endif
	  fprintf(stderr,"Continue: ");
	  if(fscanf(fp,"%c",&c) != 1) {
		error(E_INPUT, "im_finput");
	  }
	  if ( c == 'n' || c == 'N' )
	  {    dynamic = FALSE;                 goto redo;      }
	  if ( (c == 'b' || c == 'B') /* && i > 0 */ )
	  {     if ( i > 0 )
		    i--;
		dynamic = FALSE;        goto redo;
	  }
     }
     
     return (mat);
}

/* bm_finput -- batch-file input of matrix */
MAT     *bm_finput(fp,mat)
FILE    *fp;
MAT     *mat;
{
     u_int      i,j,m,n,dummy;
     int        io_code;
     
     /* get dimension */
     skipjunk(fp);
     if ((io_code=fscanf(fp," Matrix: %u by %u",&m,&n)) < 2 ||
	 m>MAXDIM || n>MAXDIM )
	  error(io_code==EOF ? E_EOF : E_FORMAT,"bm_finput");
     
     /* allocate memory if necessary */
     if ( mat==(MAT *)NULL )
	  mat = m_resize(mat,m,n);
     
     /* get entries */
     for ( i=0; i<m; i++ )
     {
	  skipjunk(fp);
	  if ( fscanf(fp," row %u:",&dummy) < 1 )
	       error(E_FORMAT,"bm_finput");
	  for ( j=0; j<n; j++ )
#if REAL == DOUBLE
	       if ((io_code=fscanf(fp,"%lf",&mat->me[i][j])) < 1 )
#elif REAL == FLOAT
	       if ((io_code=fscanf(fp,"%f",&mat->me[i][j])) < 1 )
#endif
		    error(io_code==EOF ? 7 : 6,"bm_finput");
     }
     
     return (mat);
}

PERM    *px_finput(fp,px)
FILE    *fp;
PERM    *px;
{
     PERM       *ipx_finput(),*bpx_finput();
     
     if ( isatty(fileno(fp)) )
	  return ipx_finput(fp,px);
     else
	  return bpx_finput(fp,px);
}


/* ipx_finput -- interactive input of permutation */
PERM    *ipx_finput(fp,px)
FILE    *fp;
PERM    *px;
{
     u_int      i,j,size,dynamic; /* dynamic set if memory allocated here */
     u_int      entry,ok;
     
     /* get permutation size */
     if ( px!=(PERM *)NULL && px->size<MAXDIM )
     {  size = px->size;        dynamic = FALSE;        }
     else
     {
	  dynamic = TRUE;
	  do
	  {
	       fprintf(stderr,"Permutation: size: ");
	       if ( fgets(line,MAXLINE,fp)==NULL )
		    error(E_INPUT,"ipx_finput");
	  } while ( sscanf(line,"%u",&size)<1 || size>MAXDIM );
	  px = px_get(size);
     }
     
     /* get entries */
     i = 0;
     while ( i<size )
     {
	  /* input entry */
	  do
	  {
	  redo:
	       fprintf(stderr,"entry %u: ",i);
	       if ( !dynamic )
		    fprintf(stderr,"old: %u->%u new: ",
			    i,px->pe[i]);
	       if ( fgets(line,MAXLINE,fp)==NULL )
		    error(E_INPUT,"ipx_finput");
	       if ( (*line == 'b' || *line == 'B') && i > 0 )
	       {        i--;    dynamic = FALSE;        goto redo;      }
	  } while ( *line=='\0' || sscanf(line,"%u",&entry) < 1 );
	  /* check entry */
	  ok = (entry < size);
	  for ( j=0; j<i; j++ )
	       ok &= (entry != px->pe[j]);
	  if ( ok )
	  {
	       px->pe[i] = entry;
	       i++;
	  }
     }
     
     return (px);
}

/* bpx_finput -- batch-file input of permutation */
PERM    *bpx_finput(fp,px)
FILE    *fp;
PERM    *px;
{
     u_int      i,j,size,entry,ok;
     int        io_code;
     
     /* get size of permutation */
     skipjunk(fp);
     if ((io_code=fscanf(fp," Permutation: size:%u",&size)) < 1 ||
	 size>MAXDIM )
	  error(io_code==EOF ? 7 : 6,"bpx_finput");
     
     /* allocate memory if necessary */
     if ( px==(PERM *)NULL || px->size<size )
	  px = px_resize(px,size);
     
     /* get entries */
     skipjunk(fp);
     i = 0;
     while ( i<size )
     {
	  /* input entry */
	  if ((io_code=fscanf(fp,"%*u -> %u",&entry)) < 1 )
	       error(io_code==EOF ? 7 : 6,"bpx_finput");
	  /* check entry */
	  ok = (entry < size);
	  for ( j=0; j<i; j++ )
	       ok &= (entry != px->pe[j]);
	  if ( ok )
	  {
	       px->pe[i] = entry;
	       i++;
	  }
	  else
	       error(E_BOUNDS,"bpx_finput");
     }
     
     return (px);
}


VEC     *v_finput(fp,x)
FILE    *fp;
VEC     *x;
{
     VEC        *ifin_vec(),*bfin_vec();
     
     if ( isatty(fileno(fp)) )
	  return ifin_vec(fp,x);
     else
	  return bfin_vec(fp,x);
}

/* ifin_vec -- interactive input of vector */
VEC     *ifin_vec(fp,vec)
FILE    *fp;
VEC     *vec;
{
     u_int      i,dim,dynamic;  /* dynamic set if memory allocated here */
     
     /* get vector dimension */
     if ( vec != (VEC *)NULL && vec->dim<MAXDIM )
     {  dim = vec->dim; dynamic = FALSE;        }
     else
     {
	  dynamic = TRUE;
	  do
	  {
	       fprintf(stderr,"Vector: dim: ");
	       if ( fgets(line,MAXLINE,fp)==NULL )
		    error(E_INPUT,"ifin_vec");
	  } while ( sscanf(line,"%u",&dim)<1 || dim>MAXDIM );
	  vec = v_get(dim);
     }
     
     /* input elements */
     for ( i=0; i<dim; i++ )
	  do
	  {
	  redo:
	       fprintf(stderr,"entry %u: ",i);
	       if ( !dynamic )
		    fprintf(stderr,"old %14.9g new: ",vec->ve[i]);
	       if ( fgets(line,MAXLINE,fp)==NULL )
		    error(E_INPUT,"ifin_vec");
	       if ( (*line == 'b' || *line == 'B') && i > 0 )
	       {        i--;    dynamic = FALSE;        goto redo;         }
	       if ( (*line == 'f' || *line == 'F') && i < dim-1 )
	       {        i++;    dynamic = FALSE;        goto redo;         }
#if REAL == DOUBLE
	  } while ( *line=='\0' || sscanf(line,"%lf",&vec->ve[i]) < 1 );
#elif REAL == FLOAT
          } while ( *line=='\0' || sscanf(line,"%f",&vec->ve[i]) < 1 );
#endif
     
     return (vec);
}

/* bfin_vec -- batch-file input of vector */
VEC     *bfin_vec(fp,vec)
FILE    *fp;
VEC     *vec;
{
     u_int      i,dim;
     int        io_code;
     
     /* get dimension */
     skipjunk(fp);
     if ((io_code=fscanf(fp," Vector: dim:%u",&dim)) < 1 ||
	 dim>MAXDIM )
	  error(io_code==EOF ? 7 : 6,"bfin_vec");
     
     /* allocate memory if necessary */
     if ( vec==(VEC *)NULL )
	  vec = v_resize(vec,dim);
     
     /* get entries */
     skipjunk(fp);
     for ( i=0; i<dim; i++ )
#if REAL == DOUBLE
	  if ((io_code=fscanf(fp,"%lf",&vec->ve[i])) < 1 )
#elif REAL == FLOAT
	  if ((io_code=fscanf(fp,"%f",&vec->ve[i])) < 1 )
#endif
	       error(io_code==EOF ? 7 : 6,"bfin_vec");
     
     return (vec);
}

/**************************************************************************
  Output routines
  **************************************************************************/
static char    *format = "%14.9g ";

char	*setformat(f_string)
char    *f_string;
{
    char	*old_f_string;
    old_f_string = format;
    if ( f_string != (char *)NULL && *f_string != '\0' )
	format = f_string;

    return old_f_string;
}

void    m_foutput(fp,a)
FILE    *fp;
MAT     *a;
{
     u_int      i, j, tmp;
     
     if ( a == (MAT *)NULL )
     {  fprintf(fp,"Matrix: NULL\n");   return;         }
     fprintf(fp,"Matrix: %d by %d\n",a->m,a->n);
     if ( a->me == (Real **)NULL )
     {  fprintf(fp,"NULL\n");           return;         }
     for ( i=0; i<a->m; i++ )   /* for each row... */
     {
	  fprintf(fp,"row %u: ",i);
	  for ( j=0, tmp=2; j<a->n; j++, tmp++ )
	  {             /* for each col in row... */
	       fprintf(fp,format,a->me[i][j]);
	       if ( ! (tmp % 5) )       putc('\n',fp);
	  }
	  if ( tmp % 5 != 1 )   putc('\n',fp);
     }
}

void    px_foutput(fp,px)
FILE    *fp;
PERM    *px;
{
     u_int      i;
     
     if ( px == (PERM *)NULL )
     {  fprintf(fp,"Permutation: NULL\n");      return;         }
     fprintf(fp,"Permutation: size: %u\n",px->size);
     if ( px->pe == (u_int *)NULL )
     {  fprintf(fp,"NULL\n");   return;         }
     for ( i=0; i<px->size; i++ )
	if ( ! (i % 8) && i != 0 )
	  fprintf(fp,"\n  %u->%u ",i,px->pe[i]);
	else
	  fprintf(fp,"%u->%u ",i,px->pe[i]);
     fprintf(fp,"\n");
}

void    v_foutput(fp,x)
FILE    *fp;
VEC     *x;
{
     u_int      i, tmp;
     
     if ( x == (VEC *)NULL )
     {  fprintf(fp,"Vector: NULL\n");   return;         }
     fprintf(fp,"Vector: dim: %d\n",x->dim);
     if ( x->ve == (Real *)NULL )
     {  fprintf(fp,"NULL\n");   return;         }
     for ( i=0, tmp=0; i<x->dim; i++, tmp++ )
     {
	  fprintf(fp,format,x->ve[i]);
	  if ( tmp % 5 == 4 )   putc('\n',fp);
     }
     if ( tmp % 5 != 0 )        putc('\n',fp);
}


void    m_dump(fp,a)
FILE    *fp;
MAT     *a;
{
	u_int   i, j, tmp;
     
     if ( a == (MAT *)NULL )
     {  fprintf(fp,"Matrix: NULL\n");   return;         }
     fprintf(fp,"Matrix: %d by %d @ 0x%p\n",a->m,a->n,a);
     fprintf(fp,"\tmax_m = %d, max_n = %d, max_size = %d\n",
	     a->max_m, a->max_n, a->max_size);
     if ( a->me == (Real **)NULL )
     {  fprintf(fp,"NULL\n");           return;         }
     fprintf(fp,"a->me @ 0x%p\n",(a->me));
     fprintf(fp,"a->base @ 0x%p\n",(a->base));
     for ( i=0; i<a->m; i++ )   /* for each row... */
     {
	  fprintf(fp,"row %u: @ 0x%p ",i,(a->me[i]));
	  for ( j=0, tmp=2; j<a->n; j++, tmp++ )
	  {             /* for each col in row... */
	       fprintf(fp,format,a->me[i][j]);
	       if ( ! (tmp % 5) )       putc('\n',fp);
	  }
	  if ( tmp % 5 != 1 )   putc('\n',fp);
     }
}

void    px_dump(fp,px)
FILE    *fp;
PERM    *px;
{
     u_int      i;
     
     if ( ! px )
     {  fprintf(fp,"Permutation: NULL\n");      return;         }
     fprintf(fp,"Permutation: size: %u @ 0x%p\n",px->size,(px));
     if ( ! px->pe )
     {  fprintf(fp,"NULL\n");   return;         }
     fprintf(fp,"px->pe @ 0x%p\n",(px->pe));
     for ( i=0; i<px->size; i++ )
	  fprintf(fp,"%u->%u ",i,px->pe[i]);
     fprintf(fp,"\n");
}


void    v_dump(fp,x)
FILE    *fp;
VEC     *x;
{
     u_int      i, tmp;
     
     if ( ! x )
     {  fprintf(fp,"Vector: NULL\n");   return;         }
     fprintf(fp,"Vector: dim: %d @ 0x%p\n",x->dim,(x));
     if ( ! x->ve )
     {  fprintf(fp,"NULL\n");   return;         }
     fprintf(fp,"x->ve @ 0x%p\n",(x->ve));
     for ( i=0, tmp=0; i<x->dim; i++, tmp++ )
     {
	  fprintf(fp,format,x->ve[i]);
	  if ( tmp % 5 == 4 )   putc('\n',fp);
     }
     if ( tmp % 5 != 0 )        putc('\n',fp);
}

