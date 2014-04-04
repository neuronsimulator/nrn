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



#include        <stdio.h>
#include        <ctype.h>
#include        "zmatrix.h"

static char rcsid[] = "zmatio.c,v 1.1 1997/12/04 17:56:11 hines Exp";



/* local variables */
static char line[MAXLINE];

/**************************************************************************
  Input routines
  **************************************************************************/

complex	z_finput(fp)
FILE	*fp;
{
    int		io_code;
    complex	z;

    skipjunk(fp);
    if ( isatty(fileno(fp)) )
    {
	do {
	    fprintf(stderr,"real and imag parts: ");
	    if ( fgets(line,MAXLINE,fp) == NULL )
		error(E_EOF,"z_finput");
#if REAL == DOUBLE
	    io_code = sscanf(line,"%lf%lf",&z.re,&z.im);
#elif REAL == FLOAT
	    io_code = sscanf(line,"%f%f",&z.re,&z.im);
#endif

	} while ( io_code != 2 );
    }
    else
#if REAL == DOUBLE
      if ( (io_code=fscanf(fp," (%lf,%lf)",&z.re,&z.im)) < 2 )
#elif REAL == FLOAT
      if ( (io_code=fscanf(fp," (%f,%f)",&z.re,&z.im)) < 2 )
#endif
	    error((io_code == EOF) ? E_EOF : E_FORMAT,"z_finput");

    return z;
}


ZMAT	*zm_finput(fp,a)
FILE    *fp;
ZMAT	*a;
{
     ZMAT        *izm_finput(),*bzm_finput();
     
     if ( isatty(fileno(fp)) )
	  return izm_finput(fp,a);
     else
	  return bzm_finput(fp,a);
}

/* izm_finput -- interactive input of matrix */
ZMAT     *izm_finput(fp,mat)
FILE    *fp;
ZMAT     *mat;
{
     char       c;
     u_int      i, j, m, n, dynamic;
     /* dynamic set to TRUE if memory allocated here */
     
     /* get matrix size */
     if ( mat != ZMNULL && mat->m<MAXDIM && mat->n<MAXDIM )
     {  m = mat->m;     n = mat->n;     dynamic = FALSE;        }
     else
     {
	  dynamic = TRUE;
	  do
	  {
	       fprintf(stderr,"ComplexMatrix: rows cols:");
	       if ( fgets(line,MAXLINE,fp)==NULL )
		    error(E_INPUT,"izm_finput");
	  } while ( sscanf(line,"%u%u",&m,&n)<2 || m>MAXDIM || n>MAXDIM );
	  mat = zm_get(m,n);
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
			 fprintf(stderr,"old (%14.9g,%14.9g) new: ",
				 mat->me[i][j].re,mat->me[i][j].im);
		    if ( fgets(line,MAXLINE,fp)==NULL )
			 error(E_INPUT,"izm_finput");
		    if ( (*line == 'b' || *line == 'B') && j > 0 )
		    {   j--;    dynamic = FALSE;        goto redo2;     }
		    if ( (*line == 'f' || *line == 'F') && j < n-1 )
		    {   j++;    dynamic = FALSE;        goto redo2;     }
	       } while ( *line=='\0' ||
#if REAL == DOUBLE
			 sscanf(line,"%lf%lf",
#elif REAL == FLOAT
			sscanf(line,"%f%f",
#endif	
				&mat->me[i][j].re,&mat->me[i][j].im)<1 );
	  fprintf(stderr,"Continue: ");
	  if (fscanf(fp,"%c",&c) != 1) { error(E_INPUT, "izm_finput"); }
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

/* bzm_finput -- batch-file input of matrix */
ZMAT     *bzm_finput(fp,mat)
FILE    *fp;
ZMAT     *mat;
{
     u_int      i,j,m,n,dummy;
     int        io_code;
     
     /* get dimension */
     skipjunk(fp);
     if ((io_code=fscanf(fp," ComplexMatrix: %u by %u",&m,&n)) < 2 ||
	 m>MAXDIM || n>MAXDIM )
	  error(io_code==EOF ? E_EOF : E_FORMAT,"bzm_finput");
     
     /* allocate memory if necessary */
     if ( mat==ZMNULL || mat->m<m || mat->n<n )
	  mat = zm_resize(mat,m,n);
     
     /* get entries */
     for ( i=0; i<m; i++ )
     {
	  skipjunk(fp);
	  if ( fscanf(fp," row %u:",&dummy) < 1 )
	       error(E_FORMAT,"bzm_finput");
	  for ( j=0; j<n; j++ )
	  {
	      /* printf("bzm_finput: j = %d\n", j); */
#if REAL == DOUBLE
	      if ((io_code=fscanf(fp," ( %lf , %lf )",
#elif REAL == FLOAT
	      if ((io_code=fscanf(fp," ( %f , %f )",
#endif
				  &mat->me[i][j].re,&mat->me[i][j].im)) < 2 )
		  error(io_code==EOF ? E_EOF : E_FORMAT,"bzm_finput");
	  }
     }
     
     return (mat);
}

ZVEC     *zv_finput(fp,x)
FILE    *fp;
ZVEC     *x;
{
     ZVEC        *izv_finput(),*bzv_finput();
     
     if ( isatty(fileno(fp)) )
	  return izv_finput(fp,x);
     else
	  return bzv_finput(fp,x);
}

/* izv_finput -- interactive input of vector */
ZVEC     *izv_finput(fp,vec)
FILE    *fp;
ZVEC     *vec;
{
     u_int      i,dim,dynamic;  /* dynamic set if memory allocated here */
     
     /* get vector dimension */
     if ( vec != ZVNULL && vec->dim<MAXDIM )
     {  dim = vec->dim; dynamic = FALSE;        }
     else
     {
	  dynamic = TRUE;
	  do
	  {
	       fprintf(stderr,"ComplexVector: dim: ");
	       if ( fgets(line,MAXLINE,fp)==NULL )
		    error(E_INPUT,"izv_finput");
	  } while ( sscanf(line,"%u",&dim)<1 || dim>MAXDIM );
	  vec = zv_get(dim);
     }
     
     /* input elements */
     for ( i=0; i<dim; i++ )
	  do
	  {
	  redo:
	       fprintf(stderr,"entry %u: ",i);
	       if ( !dynamic )
		    fprintf(stderr,"old (%14.9g,%14.9g) new: ",
			    vec->ve[i].re,vec->ve[i].im);
	       if ( fgets(line,MAXLINE,fp)==NULL )
		    error(E_INPUT,"izv_finput");
	       if ( (*line == 'b' || *line == 'B') && i > 0 )
	       {        i--;    dynamic = FALSE;        goto redo;         }
	       if ( (*line == 'f' || *line == 'F') && i < dim-1 )
	       {        i++;    dynamic = FALSE;        goto redo;         }
	  } while ( *line=='\0' ||
#if REAL == DOUBLE
		    sscanf(line,"%lf%lf",
#elif REAL == FLOAT
		    sscanf(line,"%f%f",
#endif  
			   &vec->ve[i].re,&vec->ve[i].im) < 2 );
     
     return (vec);
}

/* bzv_finput -- batch-file input of vector */
ZVEC     *bzv_finput(fp,vec)
FILE    *fp;
ZVEC    *vec;
{
     u_int      i,dim;
     int        io_code;
     
     /* get dimension */
     skipjunk(fp);
     if ((io_code=fscanf(fp," ComplexVector: dim:%u",&dim)) < 1 ||
	  dim>MAXDIM )
	 error(io_code==EOF ? 7 : 6,"bzv_finput");

     
     /* allocate memory if necessary */
     if ( vec==ZVNULL || vec->dim<dim )
	  vec = zv_resize(vec,dim);
     
     /* get entries */
     skipjunk(fp);
     for ( i=0; i<dim; i++ )
#if REAL == DOUBLE
	  if ((io_code=fscanf(fp," (%lf,%lf)",
#elif REAL == FLOAT
          if ((io_code=fscanf(fp," (%f,%f)",
#endif
			      &vec->ve[i].re,&vec->ve[i].im)) < 2 )
	       error(io_code==EOF ? 7 : 6,"bzv_finput");
     
     return (vec);
}

/**************************************************************************
  Output routines
  **************************************************************************/
static char    *zformat = " (%14.9g, %14.9g) ";

char	*setzformat(f_string)
char    *f_string;
{
    char	*old_f_string;
    old_f_string = zformat;
    if ( f_string != (char *)NULL && *f_string != '\0' )
	zformat = f_string;

    return old_f_string;
}

void	z_foutput(fp,z)
FILE	*fp;
complex	z;
{
    fprintf(fp,zformat,z.re,z.im);
    putc('\n',fp);
}

void    zm_foutput(fp,a)
FILE    *fp;
ZMAT     *a;
{
     u_int      i, j, tmp;
     
     if ( a == ZMNULL )
     {  fprintf(fp,"ComplexMatrix: NULL\n");   return;         }
     fprintf(fp,"ComplexMatrix: %d by %d\n",a->m,a->n);
     if ( a->me == (complex **)NULL )
     {  fprintf(fp,"NULL\n");           return;         }
     for ( i=0; i<a->m; i++ )   /* for each row... */
     {
	  fprintf(fp,"row %u: ",i);
	  for ( j=0, tmp=1; j<a->n; j++, tmp++ )
	  {             /* for each col in row... */
	       fprintf(fp,zformat,a->me[i][j].re,a->me[i][j].im);
	       if ( ! (tmp % 2) )       putc('\n',fp);
	  }
	  if ( tmp % 2 != 1 )   putc('\n',fp);
     }
}

void    zv_foutput(fp,x)
FILE    *fp;
ZVEC     *x;
{
     u_int      i, tmp;
     
     if ( x == ZVNULL )
     {  fprintf(fp,"ComplexVector: NULL\n");   return;         }
     fprintf(fp,"ComplexVector: dim: %d\n",x->dim);
     if ( x->ve == (complex *)NULL )
     {  fprintf(fp,"NULL\n");   return;         }
     for ( i=0, tmp=0; i<x->dim; i++, tmp++ )
     {
	  fprintf(fp,zformat,x->ve[i].re,x->ve[i].im);
	  if ( (tmp % 2) == 1 )   putc('\n',fp);
     }
     if ( (tmp % 2) != 0 )        putc('\n',fp);
}


void    zm_dump(fp,a)
FILE    *fp;
ZMAT     *a;
{
	u_int   i, j, tmp;
     
     if ( a == ZMNULL )
     {  fprintf(fp,"ComplexMatrix: NULL\n");   return;         }
     fprintf(fp,"ComplexMatrix: %d by %d @ 0x%p\n",a->m,a->n,a);
     fprintf(fp,"\tmax_m = %d, max_n = %d, max_size = %d\n",
	     a->max_m, a->max_n, a->max_size);
     if ( a->me == (complex **)NULL )
     {  fprintf(fp,"NULL\n");           return;         }
     fprintf(fp,"a->me @ 0x%p\n",(a->me));
     fprintf(fp,"a->base @ 0x%p\n",(a->base));
     for ( i=0; i<a->m; i++ )   /* for each row... */
     {
	  fprintf(fp,"row %u: @ 0x%p ",i,(a->me[i]));
	  for ( j=0, tmp=1; j<a->n; j++, tmp++ )
	  {             /* for each col in row... */
	       fprintf(fp,zformat,a->me[i][j].re,a->me[i][j].im);
	       if ( ! (tmp % 2) )       putc('\n',fp);
	  }
	  if ( tmp % 2 != 1 )   putc('\n',fp);
     }
}



void    zv_dump(fp,x)
FILE    *fp;
ZVEC     *x;
{
     u_int      i, tmp;
     
     if ( ! x )
     {  fprintf(fp,"ComplexVector: NULL\n");   return;         }
     fprintf(fp,"ComplexVector: dim: %d @ 0x%p\n",x->dim,(x));
     if ( ! x->ve )
     {  fprintf(fp,"NULL\n");   return;         }
     fprintf(fp,"x->ve @ 0x%p\n",(x->ve));
     for ( i=0, tmp=0; i<x->dim; i++, tmp++ )
     {
	  fprintf(fp,zformat,x->ve[i].re,x->ve[i].im);
	  if ( tmp % 2 == 1 )   putc('\n',fp);
     }
     if ( tmp % 2 != 0 )        putc('\n',fp);
}

